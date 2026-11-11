// vcb2verilog - Verilog Emitter
// Converts parsed VCB circuit graphs to synthesizable Verilog HDL.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "openVCB.h"
#include <string>
#include <fstream>
#include <vector>
#include <set>
#include <utility>

class VerilogEmitter {
public:
    explicit VerilogEmitter(openVCB::Project& proj);

    /// If true, traces with in-degree 0 that drive internal gates are exposed as input ports
    bool exposeZeroInDegreeTraces = true;
    /// Custom clock period (number of ticks per pulse)
    unsigned long long clockPeriod = 2;

    /// Emit Verilog to the given file (or stdout if outputPath is empty or "-").
    /// Returns true on success.
    bool emit(const std::string& outputPath, const std::string& moduleName);

    // Statistics getters
    int getNumInputs() const { return (int)(inputGroups.size() + latchBtnInputs.size()); }
    int getNumOutputs() const { return (int)outputGroups.size(); }
    int getNumLeds() const { return (int)ledGroups.size(); }
    int getNumBuffers() const { return (int)bufferGroups.size(); }
    int getNumLatches() const { return (int)latchGroups.size(); }
    int getNumClocks() const { return (int)clockGroups.size(); }
    int getNumComb() const { return (int)combGroups.size(); }

    /// Sanitize arbitrary string into a valid Verilog identifier
    static std::string sanitizeIdentifier(const std::string& raw);

private:
    openVCB::Project& proj;

    // Representative grid coordinate (first pixel) of each group
    std::vector<std::pair<int,int>> groupCoords;

    // ---------- analysis results ----------
    std::set<int> inputGroups;     // READ / external traces → top-level input ports
    std::set<int> outputGroups;    // WRITE → top-level output ports
    std::set<int> ledGroups;       // LED   → top-level output ports
    std::set<int> bufferGroups;    // BUFFER → D-FF (1-tick delay)
    std::set<int> latchGroups;     // LATCH  → T-FF (toggle on rising edge)
    std::set<int> clockGroups;     // CLOCK  → frequency divider
    std::set<int> combGroups;      // everything else that is combinational
    std::set<int> ignoredGroups;   // Filler, Annotation, Cross, None
    std::set<int> latchBtnInputs;  // Latches with no internal inputs -> external btn_GID

    bool needsClk = false;   // any sequential element present?
    bool needsRst = false;

    // reverse adjacency: reverseAdj[dst] = { src0, src1, … }
    std::vector<std::vector<int>> reverseAdj;
    // out-degree: outDegree[src]
    std::vector<int> outDegree;

    // ---------- pipeline ----------
    void analyzeGroups();
    void buildGraphInfo();

    void emitHeader  (std::ostream& f, const std::string& moduleName);
    void emitPorts   (std::ostream& f, const std::string& moduleName);
    void emitWires   (std::ostream& f);
    void emitComb    (std::ostream& f);
    void emitSeq     (std::ostream& f);
    void emitFooter  (std::ostream& f);

    // ---------- helpers ----------
    std::string sigName(int gid) const;
    std::pair<int,int> groupCoord(int gid) const;
    static const char* inkTag(openVCB::Ink ink);
    std::string combExpr(int gid, const char* op, const char* defaultVal) const;
};
