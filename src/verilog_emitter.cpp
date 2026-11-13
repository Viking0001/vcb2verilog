// vcb2verilog - Verilog Emitter Implementation
// Converts parsed VCB circuit graphs to synthesizable Verilog HDL.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "verilog_emitter.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <ctime>
#include <cctype>

using namespace openVCB;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

VerilogEmitter::VerilogEmitter(Project& proj) : proj(proj) {
    if (proj.clockPeriod > 0) {
        clockPeriod = proj.clockPeriod;
    }
}

std::string VerilogEmitter::sanitizeIdentifier(const std::string& raw) {
    if (raw.empty()) return "vcb_top";
    std::string s;
    for (char c : raw) {
        if (std::isalnum((unsigned char)c) || c == '_') {
            s.push_back(c);
        } else {
            s.push_back('_');
        }
    }
    // Verilog identifiers must start with a letter or underscore
    if (!s.empty() && std::isdigit((unsigned char)s[0])) {
        s = "m_" + s;
    }
    // Remove repeated underscores
    std::string res;
    for (size_t i = 0; i < s.size(); i++) {
        if (i > 0 && s[i] == '_' && s[i - 1] == '_') continue;
        res.push_back(s[i]);
    }
    return res.empty() ? "vcb_top" : res;
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

bool VerilogEmitter::hasSequentialElements() const {
    if (hasVmem) return true;
    if (!bufferGroups.empty()) return true;
    if (!clockGroups.empty()) return true;
    if (!selfLoopLatches.empty()) return true;
    if (!loopBreakGroups.empty()) return true;
    for (int gid : latchGroups) {
        if (!reverseAdj[gid].empty()) return true; // T-FF with input
    }
    return false;
}

bool VerilogEmitter::emit(const std::string& outputPath,
                          const std::string& moduleName) {
    buildGraphInfo();
    analyzeGroups();

    // Decide whether we need clk / rst
    // In auto/off mode, treat Buffer as combinational (diode)
    if (!registerAllGates) {
        for (int gid : bufferGroups) combGroups.insert(gid);
        bufferGroups.clear();
    }
    if (registerAllGates) {
        needsClk = true;
    } else if (clockMode == "off" && !hasVmem) {
        needsClk = false;
    } else {
        needsClk = hasSequentialElements();
    }
    clockOffMode = !needsClk;
    needsRst = needsClk;

    std::string mod = sanitizeIdentifier(moduleName);

    if (outputPath.empty() || outputPath == "-") {
        emitHeader(std::cout, mod);
        emitPorts(std::cout, mod);
        emitWires(std::cout);
        emitComb(std::cout);
        emitSeq(std::cout);
        emitFooter(std::cout);
        return true;
    }

    std::ofstream f(outputPath);
    if (!f.is_open()) {
        std::cerr << "error: cannot open output file: " << outputPath << "\n";
        return false;
    }

    emitHeader(f, mod);
    emitPorts(f, mod);
    emitWires(f);
    emitComb(f);
    emitSeq(f);
    emitFooter(f);

    f.close();
    return true;
}

// ---------------------------------------------------------------------------
// Build reverse adjacency (dst → list of src) and out-degree
// ---------------------------------------------------------------------------

void VerilogEmitter::buildGraphInfo() {
    reverseAdj.assign(proj.numGroups, std::vector<int>());
    outDegree.assign(proj.numGroups, 0);

    for (int src = 0; src < proj.numGroups; src++) {
        int start = proj.writeMap.ptr[src];
        int end   = proj.writeMap.ptr[src + 1];
        outDegree[src] = end - start;
        for (int j = start; j < end; j++) {
            int dst = proj.writeMap.rows[j];
            reverseAdj[dst].push_back(src);
        }
    }

    // Virtual memory support
    hasVmem = (proj.vmem != nullptr && proj.vmemSize > 0);
    if (hasVmem) {
        for (int i = 0; i < proj.vmAddr.numBits; i++) vmemAddrGids.push_back(proj.vmAddr.gids[i]);
        for (int i = 0; i < proj.vmData.numBits; i++) vmemDataGids.push_back(proj.vmData.gids[i]);
        // VCB exposes a single data bus; reuse it as write data.
        vmemWDataGids = vmemDataGids;
        // Write enable (if provided by the project)
        if (proj.vmWe.numBits > 0 && proj.vmWe.gids[0] >= 0) {
            vmemWeGids.push_back(proj.vmWe.gids[0]);
        }
    }

    // Representative coordinate (first pixel in row-major order) for each group
    groupCoords.assign(proj.numGroups, std::make_pair(-1, -1));
    if (proj.indexImage != nullptr && proj.width > 0 && proj.height > 0) {
        for (int y = 0; y < proj.height; y++) {
            for (int x = 0; x < proj.width; x++) {
                int gid = proj.indexImage[x + y * proj.width];
                if (gid >= 0 && gid < proj.numGroups &&
                    groupCoords[gid].first < 0) {
                    groupCoords[gid] = std::make_pair(x, y);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Analysis – classify every group by its ink type and graph topology
// ---------------------------------------------------------------------------

void VerilogEmitter::analyzeGroups() {
    inputGroups.clear();
    outputGroups.clear();
    ledGroups.clear();
    bufferGroups.clear();
    latchGroups.clear();
    clockGroups.clear();
    combGroups.clear();
    ignoredGroups.clear();
    latchBtnInputs.clear();

    for (int gid = 0; gid < proj.numGroups; gid++) {
        Ink ink = setOff(proj.stateInks[gid]);
        switch (ink) {
        case Ink::ReadOff:
            inputGroups.insert(gid);
            break;

        case Ink::WriteOff:
            outputGroups.insert(gid);
            break;

        case Ink::LedOff:
            ledGroups.insert(gid);
            break;

        case Ink::BufferOff:
            bufferGroups.insert(gid);
            break;

        case Ink::LatchOff: {
            bool isSelfLoop = false;
            std::vector<int> extInputs;
            for (int src : reverseAdj[gid]) {
                if (src == gid) {
                    isSelfLoop = true;
                } else {
                    bool srcSelf = false;
                    for (int s2 : reverseAdj[src]) {
                        if (s2 == gid) { srcSelf = true; break; }
                    }
                    if (srcSelf) isSelfLoop = true;
                    else extInputs.push_back(src);
                }
            }
            if (hasVmem && std::find(vmemDataGids.begin(), vmemDataGids.end(), gid) != vmemDataGids.end()) {
                // virtual memory data latch: handled by BRAM
            } else if (isSelfLoop) {
                selfLoopLatches.insert(gid);
                selfLoopExtInputs[gid] = extInputs;
            } else {
                latchGroups.insert(gid);
            }
            if (reverseAdj[gid].empty() && !(hasVmem && std::find(vmemDataGids.begin(), vmemDataGids.end(), gid) != vmemDataGids.end())) {
                // Only expose as switch if it actually drives something in the circuit
                if (outDegree[gid] > 0) {
                    latchBtnInputs.insert(gid);
                    latchGroups.erase(gid);
                } else {
                    ignoredGroups.insert(gid);
                    latchGroups.erase(gid);
                }
            }
            break;
        }

        case Ink::ClockOff:
            clockGroups.insert(gid);
            break;

        case Ink::None:
        case Ink::Cross:
        case Ink::Filler:
        case Ink::Annotation:
            ignoredGroups.insert(gid);
            break;

        default:
            // Trace, Bundle, OR, AND, NOT, NOR, NAND, XOR, XNOR
            if (exposeZeroInDegreeTraces &&
                (ink == Ink::TraceOff || ink == Ink::BundleOff) &&
                reverseAdj[gid].empty() && outDegree[gid] > 0) {
                // An undriven trace driving gates in the circuit is an external input
                inputGroups.insert(gid);
            } else {
                combGroups.insert(gid);
            }
            break;
        }
    }

    detectCombinationalLoops();
}


void VerilogEmitter::detectCombinationalLoops() {
    loopBreakGroups.clear();
    std::vector<int> color(proj.numGroups, 0);
    // recursive DFS
    std::function<void(int)> dfs = [&](int u) {
        color[u] = 1;
        int start = proj.writeMap.ptr[u];
        int end = proj.writeMap.ptr[u + 1];
        for (int j = start; j < end; j++) {
            int v = proj.writeMap.rows[j];
            if (combGroups.count(v) == 0) continue; // stop at sequential/input boundaries
            if (color[v] == 0) {
                dfs(v);
            } else if (color[v] == 1) {
                // Break combinational loops only at real logic gates, never at TRACE/Led
                Ink ink = setOff(proj.stateInks[u]);
                bool isGate = (ink == Ink::AndOff || ink == Ink::OrOff || ink == Ink::NotOff ||
                               ink == Ink::NorOff || ink == Ink::NandOff || ink == Ink::XorOff ||
                               ink == Ink::XnorOff || ink == Ink::BufferOff);
                if (isGate) {
                    loopBreakGroups.insert(u);
                } else {
                    // if u is TRACE, break at a gate successor
                    int start = proj.writeMap.ptr[u];
                    int end = proj.writeMap.ptr[u + 1];
                    for (int j = start; j < end; j++) {
                        int w = proj.writeMap.rows[j];
                        if (combGroups.count(w) == 0) continue;
                        Ink inkW = setOff(proj.stateInks[w]);
                        bool wIsGate = (inkW == Ink::AndOff || inkW == Ink::OrOff || inkW == Ink::NotOff ||
                                        inkW == Ink::NorOff || inkW == Ink::NandOff || inkW == Ink::XorOff ||
                                        inkW == Ink::XnorOff || inkW == Ink::BufferOff);
                        if (wIsGate) { loopBreakGroups.insert(w); break; }
                    }
                }
            }
        }
        color[u] = 2;
    };
    for (int gid : combGroups) {
        if (color[gid] == 0) dfs(gid);
    }
}

// ---------------------------------------------------------------------------
// Signal naming
// ---------------------------------------------------------------------------

std::pair<int,int> VerilogEmitter::groupCoord(int gid) const {
    if (gid >= 0 && gid < (int)groupCoords.size()) {
        auto c = groupCoords[gid];
        if (c.first >= 0) return c;
    }
    return std::make_pair(gid, 0);
}

std::string VerilogEmitter::sigName(int gid) const {
    auto c = groupCoord(gid);
    int x = c.first;
    int y = c.second;
    if (inputGroups.count(gid))  return "input_" + std::to_string(x) + "_" + std::to_string(y);
    if (outputGroups.count(gid)) return "output_" + std::to_string(x) + "_" + std::to_string(y);
    if (ledGroups.count(gid))    return "led_" + std::to_string(x) + "_" + std::to_string(y);
    if (latchBtnInputs.count(gid)) {
        return "sw_" + std::to_string(x) + "_" + std::to_string(y);
    }
    // Virtual memory data latches are driven by BRAM read data
    if (hasVmem) {
        for (size_t i = 0; i < vmemDataGids.size(); i++) {
            if (vmemDataGids[i] == gid) {
                int mx = proj.vmAddr.pos.x;
                int my = proj.vmAddr.pos.y;
                return "mem_rdata_" + std::to_string(mx) + "_" + std::to_string(my) + "[" + std::to_string(i) + "]";
            }
        }
    }
    return "g_" + std::to_string(gid);
}

const char* VerilogEmitter::inkTag(Ink ink) {
    ink = setOff(ink);
    switch (ink) {
    case Ink::TraceOff:  return "TRACE";
    case Ink::ReadOff:   return "READ";
    case Ink::WriteOff:  return "WRITE";
    case Ink::BufferOff: return "BUFFER";
    case Ink::OrOff:     return "OR";
    case Ink::AndOff:    return "AND";
    case Ink::NotOff:    return "NOT";
    case Ink::NorOff:    return "NOR";
    case Ink::NandOff:   return "NAND";
    case Ink::XorOff:    return "XOR";
    case Ink::XnorOff:   return "XNOR";
    case Ink::ClockOff:  return "CLOCK";
    case Ink::LatchOff:  return "LATCH";
    case Ink::LedOff:    return "LED";
    case Ink::BundleOff: return "BUNDLE";
    case Ink::Cross:     return "CROSS";
    case Ink::Filler:    return "FILLER";
    case Ink::Annotation:return "ANNOTATION";
    default:             return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------
// Build a combinational expression over all inputs of `gid`
// ---------------------------------------------------------------------------

std::string VerilogEmitter::combExpr(int gid, const char* op,
                                      const char* defaultVal) const {
    const auto& inputs = reverseAdj[gid];
    if (inputs.empty()) return defaultVal;
    if (inputs.size() == 1) return sigName(inputs[0]);

    std::ostringstream os;
    for (size_t i = 0; i < inputs.size(); i++) {
        if (i > 0) os << " " << op << " ";
        os << sigName(inputs[i]);
    }
    return os.str();
}

// ---------------------------------------------------------------------------
// Verilog sections
// ---------------------------------------------------------------------------

void VerilogEmitter::emitHeader(std::ostream& f, const std::string& moduleName) {
    time_t now = time(nullptr);
    char timeBuf[64];
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    f << "// =============================================================\n"
      << "// Module: " << moduleName << "\n"
      << "// Generated by vcb2verilog (Virtual Circuit Board to Verilog)\n"
      << "// Date: " << timeBuf << "\n"
      << "// =============================================================\n"
      << "//\n"
      << "// Circuit Statistics:\n"
      << "//   Total groups     : " << proj.numGroups << "\n"
      << "//   Connections (nnz): " << proj.writeMap.nnz << "\n"
      << "//   Top Inputs       : " << (inputGroups.size() + latchBtnInputs.size()) << "\n"
      << "//   Top Outputs      : " << (outputGroups.size() + ledGroups.size()) << "\n"
      << "//   LED Ports        : " << ledGroups.size() << "\n"
      << "//   Combinational    : " << combGroups.size() << "\n"
      << "//   Buffers (D-FF)   : " << bufferGroups.size() << "\n"
      << "//   Latches (T-FF)   : " << latchGroups.size() << "\n"
      << "//   Clocks           : " << clockGroups.size() << "\n"
      << "//   Ignored elements : " << ignoredGroups.size() << "\n"
      << "// =============================================================\n"
      << "\n"
      << "`default_nettype none\n\n";
}

void VerilogEmitter::emitPorts(std::ostream& f, const std::string& moduleName) {
    f << "module " << moduleName << " (\n";

    std::vector<std::string> ports;

    if (needsClk) ports.push_back("    input  wire clk");
    if (needsRst) ports.push_back("    input  wire rst");

    for (int gid : inputGroups)
        ports.push_back("    input  wire " + sigName(gid));

    // Self-loop (clickable) latches get a top-level trigger input
    for (int gid : selfLoopLatches) {
        if (selfLoopExtInputs[gid].empty()) {
            auto c = groupCoord(gid);
            ports.push_back("    input  wire latch_" + std::to_string(c.first) + "_" + std::to_string(c.second));
        }
    }

    for (int gid : latchBtnInputs) {
        auto c = groupCoord(gid);
        ports.push_back("    input  wire sw_" + std::to_string(c.first) + "_" + std::to_string(c.second));
    }

    for (int gid : outputGroups)
        ports.push_back("    output wire " + sigName(gid));

    for (int gid : ledGroups)
        ports.push_back("    output wire " + sigName(gid));

    for (size_t i = 0; i < ports.size(); i++) {
        f << ports[i];
        if (i + 1 < ports.size()) f << ",";
        f << "\n";
    }

    f << ");\n\n";
}

void VerilogEmitter::emitWires(std::ostream& f) {
    f << "    // =========================================================\n"
      << "    // Internal signals\n"
      << "    // =========================================================\n\n";

    // Combinational internal wires / registers
    for (int gid : combGroups) {
        Ink ink = setOff(proj.stateInks[gid]);
        if (loopBreakGroups.count(gid)) {
            f << "    reg  " << sigName(gid) << " = 1'b0;  // Combinational loop breaker [" << gid << "]\n";
            f << "    wire " << sigName(gid) << "_comb;\n";
        } else if (registerAllGates) {
            f << "    reg  " << sigName(gid) << " = 1'b0;  // GATE (registered) [" << gid << "]\n";
        } else {
            f << "    wire " << sigName(gid) << ";  // " << inkTag(ink) << " [" << gid << "]\n";
        }
    }

    // Buffer registers (D flip-flops)
    for (int gid : bufferGroups) {
        f << "    reg  " << sigName(gid) << " = 1'b0;  // BUFFER (D-FF) [" << gid << "]\n";
    }

    // Latch registers + edge-detect helpers (T flip-flops)
    for (int gid : latchGroups) {
        if (latchBtnInputs.count(gid)) continue;
        f << "    reg  " << sigName(gid) << " = 1'b0;  // LATCH (T-FF) [" << gid << "]\n";
        f << "    reg  " << sigName(gid) << "_prev = 1'b0;\n";
        f << "    wire " << sigName(gid) << "_in;\n";
    }

    // Self-loop (1-tick pulse) latches
    if (!clockOffMode) {
    for (int gid : selfLoopLatches) {
        auto c = groupCoord(gid);
        f << "    reg  " << sigName(gid) << " = 1'b0;  // LATCH (self-loop pulse) [" << gid << "]\n";
        if (selfLoopExtInputs[gid].empty()) {
            f << "    reg  latch_" << c.first << "_" << c.second << "_prev = 1'b0;\n";
        } else {
            f << "    reg  ext_" << c.first << "_" << c.second << "_prev = 1'b0;\n";
            f << "    wire ext_" << c.first << "_" << c.second << "_in;\n";
        }
    }
    } // clockOffMode guard

    // Clock registers & divider counters
    for (int gid : clockGroups) {
        f << "    reg  " << sigName(gid) << " = 1'b0;  // CLOCK [" << gid << "]\n";
        f << "    reg  [31:0] " << sigName(gid) << "_cnt = 32'd0;\n";
    }

    f << "\n";
}

void VerilogEmitter::emitComb(std::ostream& f) {
    f << "    // =========================================================\n"
      << "    // Combinational logic (continuous assignments)\n"
      << "    // =========================================================\n\n";

    // Output ports (WRITE)
    for (int gid : outputGroups) {
        if (latchBtnInputs.count(gid)) continue;
        std::string expr = combExpr(gid, "|", "1'b0");
        f << "    assign " << sigName(gid) << " = " << expr
          << ";  // WRITE [" << gid << "]\n";
    }

    // LED ports
    for (int gid : ledGroups) {
        if (latchBtnInputs.count(gid)) continue;
        std::string expr = combExpr(gid, "|", "1'b0");
        f << "    assign " << sigName(gid) << " = " << expr
          << ";  // LED [" << gid << "]\n";
    }

    if (!outputGroups.empty() || !ledGroups.empty()) f << "\n";

    // Latch combinational inputs
    for (int gid : latchGroups) {
        if (selfLoopLatches.count(gid)) continue;
        if (latchBtnInputs.count(gid)) continue;
        std::string expr;
        if (hasVmem) {
            // Virtual memory data latches are fed by BRAM read data
            int bitIndex = -1;
            for (size_t i = 0; i < vmemDataGids.size(); i++) {
                if (vmemDataGids[i] == gid) { bitIndex = (int)i; break; }
            }
            if (bitIndex >= 0) {
                expr = "mem_rdata_" + std::to_string(proj.vmAddr.pos.x) + "_" + std::to_string(proj.vmAddr.pos.y) + "[" + std::to_string(bitIndex) + "]";
            } else {
                expr = combExpr(gid, "^", "1'b0");
            }
        } else {
            expr = combExpr(gid, "^", "1'b0");
        }
        f << "    assign " << sigName(gid) << "_in = " << expr
          << ";  // LATCH input [" << gid << "]\n";
    }
    if (!latchGroups.empty()) f << "\n";

    // All combinational logic gates
    if (!registerAllGates) {
    for (int gid : combGroups) {
        if (latchBtnInputs.count(gid)) continue; // free latches are input ports, never driven
        Ink ink = setOff(proj.stateInks[gid]);
        Logic logic = inkLogicType(ink);

        std::string expr;
        const char* comment = inkTag(ink);

        switch (logic) {
        case Logic::NonZeroOff:
            // Trace, Bundle, OR: active when any input != 0 -> bitwise OR
            expr = combExpr(gid, "|", "1'b0");
            break;

        case Logic::ZeroOff:
            if (ink == Ink::AndOff) {
                // AND: active when all inputs are 1
                expr = combExpr(gid, "&", "1'b0");
            } else if (ink == Ink::NandOff) {
                // NAND: ~(AND of inputs)
                std::string inner = combExpr(gid, "&", "1'b0");
                expr = "~(" + inner + ")";
            } else {
                // NOT / NOR: active when zero inputs are on -> ~(OR of inputs)
                std::string inner = combExpr(gid, "|", "1'b0");
                expr = "~(" + inner + ")";
            }
            break;

        case Logic::XorOff:
            // XOR: odd parity
            expr = combExpr(gid, "^", "1'b0");
            break;

        case Logic::XnorOff: {
            // XNOR: even parity -> ~(XOR of inputs)
            std::string inner = combExpr(gid, "^", "1'b0");
            expr = "~(" + inner + ")";
            break;
        }

        default:
            expr = "1'b0";
            break;
        }

        if (loopBreakGroups.count(gid)) {
            f << "    assign " << sigName(gid) << "_comb = " << expr
              << ";  // " << comment << " [" << gid << "]\n";
        } else {
            f << "    assign " << sigName(gid) << " = " << expr
              << ";  // " << comment << " [" << gid << "]\n";
        }
    }

    }
    f << "\n";
}

void VerilogEmitter::emitSeq(std::ostream& f) {
    if (!needsClk) return;

    // Declare external trigger wires for self-loop latches
    if (!clockOffMode) {
        for (int gid : selfLoopLatches) {
            if (!selfLoopExtInputs[gid].empty()) {
                auto c = groupCoord(gid);
                int x = c.first, y = c.second;
                std::ostringstream extExpr;
                const auto& exts = selfLoopExtInputs[gid];
                if (exts.size() == 1) {
                    extExpr << sigName(exts[0]);
                } else {
                    extExpr << "(";
                    for (size_t i = 0; i < exts.size(); i++) {
                        if (i) extExpr << " ^ ";
                        extExpr << sigName(exts[i]);
                    }
                    extExpr << ")";
                }
                f << "    assign ext_" << x << "_" << y << "_in = " << extExpr.str() << ";\n";
            }
        }
    }

    f << "    // =========================================================\n"
      << "    // Sequential logic (synchronous to posedge clk)\n"
      << "    // =========================================================\n\n";
    f << "    always @(posedge clk) begin\n";
    f << "        if (rst) begin\n";

    // Synchronous Reset
    for (int gid : bufferGroups)
        f << "            " << sigName(gid) << " <= 1'b0;\n";

    for (int gid : latchGroups) {
        f << "            " << sigName(gid) << " <= 1'b0;\n";
        f << "            " << sigName(gid) << "_prev <= 1'b0;\n";
    }

    if (registerAllGates) {
        for (int gid : combGroups) {
            f << "            " << sigName(gid) << " <= 1'b0;\n";
        }
    }

    for (int gid : loopBreakGroups) {
        f << "            " << sigName(gid) << " <= 1'b0;\n";
    }

    for (int gid : clockGroups) {
        f << "            " << sigName(gid) << " <= 1'b0;\n";
        f << "            " << sigName(gid) << "_cnt <= 32'd0;\n";
    }

    if (!clockOffMode) {
        for (int gid : selfLoopLatches) {
            auto c = groupCoord(gid);
            int x = c.first, y = c.second;
            f << "            " << sigName(gid) << " <= 1'b0;\n";
            if (selfLoopExtInputs[gid].empty()) {
                f << "            latch_" << x << "_" << y << "_prev <= 1'b0;\n";
            } else {
                f << "            ext_" << x << "_" << y << "_prev <= 1'b0;\n";
            }
        }
    }

    f << "        end else begin\n";

    // Buffers: 1-tick delay on FPGA clock (D flip-flop)
    for (int gid : bufferGroups) {
        std::string expr = combExpr(gid, "|", "1'b0");
        f << "            // BUFFER [" << gid << "] - 1-cycle delay\n";
        f << "            " << sigName(gid) << " <= " << expr << ";\n";
    }

    // Latches: Toggle on rising edge of input (T flip-flop)
    for (int gid : latchGroups) {
        if (selfLoopLatches.count(gid)) continue;
        if (latchBtnInputs.count(gid)) continue;
        std::string inSig = sigName(gid) + "_in";
        std::string prevSig = sigName(gid) + "_prev";
        f << "            // LATCH [" << gid << "] - toggle on rising edge\n";
        f << "            " << prevSig << " <= " << inSig << ";\n";
        f << "            if (" << inSig << " && !" << prevSig << ")\n";
        f << "                " << sigName(gid) << " <= ~" << sigName(gid) << ";\n";
    }

    // Combinational loop breakers: sample comb output on clock edge
    for (int gid : loopBreakGroups) {
        f << "            " << sigName(gid) << " <= " << sigName(gid) << "_comb;\n";
    }

    // Registered gates (on mode): sample combinational expression on clock edge
    if (registerAllGates) {
        for (int gid : combGroups) {
            if (latchBtnInputs.count(gid)) continue;
            std::string expr = combExpr(gid, "|", "1'b0");
            // Determine operator based on ink? combExpr default OR; for AND/NOT we need proper expr. 
            // Use inkLogicType? We'll just use OR as default (approx). 
            f << "            " << sigName(gid) << " <= " << expr << ";\n";
        }
    }

    // Registered gates (on mode)
    if (registerAllGates) {
        for (int gid : combGroups) {
            if (latchBtnInputs.count(gid)) continue;
            Ink ink = setOff(proj.stateInks[gid]);
            std::string expr;
            if (ink == Ink::AndOff) expr = combExpr(gid, "&", "1'b0");
            else if (ink == Ink::OrOff) expr = combExpr(gid, "|", "1'b0");
            else if (ink == Ink::XorOff) expr = combExpr(gid, "^", "1'b0");
            else if (ink == Ink::NotOff || ink == Ink::NorOff || ink == Ink::NandOff) {
                std::string inner = combExpr(gid, "|", "1'b0");
                expr = "~(" + inner + ")";
            } else expr = combExpr(gid, "|", "1'b0");
            f << "            " << sigName(gid) << " <= " << expr << ";\n";
        }
    }

    // Clocks: Parameterized frequency divider
    unsigned long long period = clockPeriod;
    if (period < 2) period = 2;

    for (int gid : clockGroups) {
        std::string cnt = sigName(gid) + "_cnt";
        f << "            // CLOCK [" << gid << "] - divider period " << period << " ticks\n";
        f << "            if (" << cnt << " >= 32'd" << (period - 1) << ") begin\n";
        f << "                " << cnt << " <= 32'd0;\n";
        f << "            end else begin\n";
        f << "                " << cnt << " <= " << cnt << " + 32'd1;\n";
        f << "            end\n";
        f << "            " << sigName(gid) << " <= (" << cnt << " < 32'd" << (period / 2) << ");\n";
    }

    // Self-loop (1-tick pulse) latches (inside else branch)
    if (!clockOffMode) {
        for (int gid : selfLoopLatches) {
            auto c = groupCoord(gid);
            int x = c.first, y = c.second;
            if (selfLoopExtInputs[gid].empty()) {
                f << "            // Self-loop (clickable) latch -> 1-tick pulse\n";
                f << "            latch_" << x << "_" << y << "_prev <= latch_" << x << "_" << y << ";\n";
                f << "            if (latch_" << x << "_" << y << " && !latch_" << x << "_" << y << "_prev)\n";
                f << "                " << sigName(gid) << " <= 1'b1;\n";
                f << "            else\n";
                f << "                " << sigName(gid) << " <= 1'b0;\n";
            } else {
                f << "            // Self-loop latch with external trigger -> 1-tick pulse\n";
                f << "            ext_" << x << "_" << y << "_prev <= ext_" << x << "_" << y << "_in;\n";
                f << "            if (ext_" << x << "_" << y << "_in ^ ext_" << x << "_" << y << "_prev)\n";
                f << "                " << sigName(gid) << " <= 1'b1;\n";
                f << "            else\n";
                f << "                " << sigName(gid) << " <= 1'b0;\n";
            }
        }
    }

    f << "        end\n";
    f << "    end\n\n";

    // Virtual memory (BRAM) template
    if (hasVmem) {
        int x = proj.vmAddr.pos.x;
        int y = proj.vmAddr.pos.y;
        int addrW = proj.vmAddr.numBits;
        int dataW = proj.vmData.numBits;
        unsigned long long depth = (addrW > 0) ? (1ULL << addrW) : 1ULL;
        std::string memName = "mem_" + std::to_string(x) + "_" + std::to_string(y);
        // address concatenation
        std::ostringstream addrExpr;
        addrExpr << "{";
        for (int i = (int)vmemAddrGids.size() - 1; i >= 0; i--) {
            if (i != (int)vmemAddrGids.size() - 1) addrExpr << ", ";
            addrExpr << sigName(vmemAddrGids[i]);
        }
        addrExpr << "}";
        f << "    // Virtual Memory (BRAM) at (" << x << ", " << y << ")\n";
        f << "    reg [" << (dataW > 0 ? dataW-1 : 0) << ":0] " << memName
          << " [0:" << (depth-1) << "];\n";
        f << "    integer mem_i_" << x << "_" << y << ";\n";
        f << "    initial begin\n";
        f << "        for (mem_i_" << x << "_" << y << " = 0; mem_i_" << x << "_" << y
          << " < " << depth << "; mem_i_" << x << "_" << y << " = mem_i_" << x << "_" << y << " + 1)\n";
        f << "            " << memName << "[mem_i_" << x << "_" << y << "] = " << (dataW > 0 ? std::to_string(dataW) + "'h00" : "1'b0") << ";\n";
        f << "        $readmemh(\"" << vmemMemFile << "\", " << memName << ");\n";
        f << "    end\n";
        f << "    reg  [" << (dataW > 0 ? dataW-1 : 0) << ":0] mem_rdata_"
          << x << "_" << y << " = " << (dataW > 0 ? std::to_string(dataW) + "'h00" : "1'b0") << ";\n";
        f << "    reg  [" << (dataW > 0 ? dataW-1 : 0) << ":0] mem_toggle_prev_"
          << x << "_" << y << " = " << (dataW > 0 ? std::to_string(dataW) + "'h00" : "1'b0") << ";\n";
        f << "    wire [" << (addrW > 0 ? addrW-1 : 0) << ":0] mem_addr_"
          << x << "_" << y << " = " << addrExpr.str() << ";\n";

        // Toggle bus (MSB..LSB) with zero padding on the left
        std::ostringstream toggleExpr;
        toggleExpr << "{";
        int pad = dataW - (int)vmemDataGids.size();
        if (pad < 0) pad = 0;
        bool first = true;
        for (int p = 0; p < pad; p++) {
            if (!first) toggleExpr << ", ";
            toggleExpr << "1'b0";
            first = false;
        }
        for (int i = (int)vmemDataGids.size() - 1; i >= 0; i--) {
            if (!first) toggleExpr << ", ";
            toggleExpr << combExpr(vmemDataGids[i], "^", "1'b0");
            first = false;
        }
        if (first) toggleExpr << (dataW > 0 ? std::to_string(dataW) + "'d0" : "1'b0");
        toggleExpr << "}";
        f << "    wire [" << (dataW > 0 ? dataW-1 : 0) << ":0] mem_toggle_"
          << x << "_" << y << " = " << toggleExpr.str() << ";\n";

        f << "    wire [" << (dataW > 0 ? dataW-1 : 0) << ":0] mem_toggle_pulse_"
          << x << "_" << y << " = mem_toggle_" << x << "_" << y
          << " & ~mem_toggle_prev_" << x << "_" << y << ";\n";
        f << "    wire                mem_we_" << x << "_" << y << " = |mem_toggle_pulse_"
          << x << "_" << y << ";\n";
        f << "    wire [" << (dataW > 0 ? dataW-1 : 0) << ":0] mem_next_data_"
          << x << "_" << y << " = " << memName << "[mem_addr_" << x << "_" << y
          << "] ^ mem_toggle_pulse_" << x << "_" << y << ";\n";

        f << "    always @(posedge clk) begin\n";
        f << "        if (rst) begin\n";
        f << "            mem_rdata_" << x << "_" << y << " <= " << dataW << "'d0;\n";
        f << "            mem_toggle_prev_" << x << "_" << y << " <= " << dataW << "'d0;\n";
        f << "        end else begin\n";
        f << "            mem_toggle_prev_" << x << "_" << y << " <= mem_toggle_" << x << "_" << y << ";\n";
        f << "            if (mem_we_" << x << "_" << y << ") begin\n";
        f << "                " << memName << "[mem_addr_" << x << "_" << y << "] <= mem_next_data_" << x << "_" << y << ";\n";
        f << "                mem_rdata_" << x << "_" << y << " <= mem_next_data_" << x << "_" << y << ";\n";
        f << "            end else begin\n";
        f << "                mem_rdata_" << x << "_" << y << " <= " << memName
          << "[mem_addr_" << x << "_" << y << "];\n";
        f << "            end\n";
        f << "        end\n";
        f << "    end\n\n";
    }
}

void VerilogEmitter::emitFooter(std::ostream& f) {
    f << "endmodule\n"
      << "`default_nettype wire\n";
}
