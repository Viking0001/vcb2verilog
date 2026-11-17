// vcb2verilog - Virtual Circuit Board (.vcb) to Verilog HDL Transpiler
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "openVCB.h"
#include "verilog_emitter.h"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

static const char* VERSION = "1.0.0";

void printBanner() {
    std::cout << "=====================================================\n"
              << " vcb2verilog v" << VERSION << " - Virtual Circuit Board to Verilog\n"
              << " Target: Synthesizable Verilog HDL for FPGA\n"
              << "=====================================================\n";
}

void printHelp() {
    std::cout << "vcb2verilog - Convert Virtual Circuit Board projects to Verilog HDL\n\n"
              << "Usage:\n"
              << "  vcb2verilog [OPTIONS] <input.vcb> [output.v]\n"
              << "\n"
              << "Description:\n"
              << "  Convert a VCB circuit to synthesizable Verilog for simulation or FPGA use.\n"
              << "  Input and output files may also be given with --input and --output.\n\n"
              << "Options:\n"
              << "  Input / Output:\n"
              << "    -i, --input <file>       Input .vcb file (default: first positional arg)\n"
              << "    -o, --output <file>      Output file (default: input path with .v suffix)\n"
              << "                             Use '-' to write Verilog to stdout.\n"
              << "    -m, --module <name>      Module (default: sanitized input file stem)\n\n"
              << "  Verilog Generation:\n"
              << "    -e, --expose-inputs      Expose undriven traces feeding logic as input ports\n"
              << "                             (disabled by default).\n"
              << "        --no-expose-inputs   Tie undriven traces to 1'b0 (default behavior).\n"
              << "    -M, --mem <file>         Override from .vcbmem (default: use embedded .vcb\n"
              << "                             memory; no external file is set).\n\n"
              << "  Clock Configuration:\n"
              << "        --clock <mode>       Clock mode: auto, on, or off (default: auto).\n"
              << "                             auto: infer sequential elements from topology; VCB\n"
              << "                               ticks can make intended timing ambiguous.\n"
              << "                             on: force clk/rst and register all gates.\n"
              << "                               Use when auto misses required clocking.\n"
              << "                             off: request combinational output; sequential\n"
              << "                               elements may not work correctly in this mode.\n"
              << "                               Virtual memory still uses a clock.\n"
              << "    -c, --clock-period <N>   Divider period for VCB Clock elements, in input clk\n"
              << "                             cycles (default: 2; minimum: 2).\n\n"
              << "  Optimization:\n"
              << "        --no-opt             Disable Yosys optimization (disabled by default);\n"
              << "                             default: use Yosys for file output.\n"
              << "    -O, --opt-level <level>  Yosys level: opt (default) or full.\n"
              << "                             --no-opt disables optimization at either level.\n\n"
              << "  General:\n"
              << "    -v, --verbose            Show translation stages and circuit statistics\n"
              << "                             (disabled by default).\n"
              << "        --debug              Emit topology logs (disabled by default).\n"
              << "    -h, --help               Show this help and exit (not shown by default).\n"
              << "        --version            Show version and exit (not shown by default).\n\n"
              << "Dependencies:\n"
              << "  Yosys is optional. If unavailable, the raw Verilog is kept. Optimization is\n"
              << "  skipped when writing Verilog to stdout.\n\n"
              << "Examples:\n"
              << "  vcb2verilog circuit.vcb\n"
              << "  vcb2verilog circuit.vcb -o output.v\n"
              << "  vcb2verilog circuit.vcb --module adder_top\n"
              << "  vcb2verilog circuit.vcb --expose-inputs\n"
              << "  vcb2verilog circuit.vcb --clock on\n"
              << "  vcb2verilog circuit.vcb --mem initial.vcbmem\n"
              << "  vcb2verilog circuit.vcb -o -\n"
              << "  vcb2verilog circuit.vcb --verbose\n\n"
              << "Exit Codes:\n"
              << "  0  Success, including --help and --version.\n"
              << "  1  Input, option, or conversion error.\n";
}

int main(int argc, char** argv) {
    std::string inputFile;
    std::string outputFile;
    std::string moduleName;
    unsigned long long clockPeriod = 2;
    bool exposeInputs = false; // Undriven traces are tied low unless explicitly exposed.
    bool verbose = false;
    bool optimize = true;
    std::string optLevel = "opt";
    std::string memFile;
    std::string clockMode = "auto";

    // 1. Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        } else if (arg == "--version") {
            std::cout << "vcb2verilog version " << VERSION << "\n";
            return 0;
        } else if (arg == "--no-opt") {
            optimize = false;
        } else if (arg == "--opt-level" || arg == "-O") {
            if (++i >= argc) {
                std::cerr << "Error: Option " << arg << " requires an optimization level (opt or full).\n";
                return 1;
            }
            optLevel = argv[i];
            if (optLevel != "opt" && optLevel != "full") {
                std::cerr << "Error: Unknown optimization level: \"" << optLevel << "\". Use 'opt' or 'full'.\n";
                return 1;
            }
        } else if (arg == "--mem" || arg == "-M") {
            if (++i >= argc) {
                std::cerr << "Error: Option " << arg << " requires a .vcbmem file path.\n";
                return 1;
            }
            memFile = argv[i];
        } else if (arg == "--clock") {
            if (++i >= argc) {
                std::cerr << "Error: Option " << arg << " requires a mode (auto, on, or off).\n";
                return 1;
            }
            clockMode = argv[i];
            if (clockMode != "auto" && clockMode != "on" && clockMode != "off") {
                std::cerr << "Error: Unknown clock mode: \"" << clockMode << "\". Use 'auto', 'on', or 'off'.\n";
                return 1;
            }
        } else if (arg == "--debug") {
            openVCB::g_debug = true;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-e" || arg == "--expose-inputs") {
            exposeInputs = true;
        } else if (arg == "--no-expose-inputs") {
            exposeInputs = false;
        } else if (arg == "-i" || arg == "--input") {
            if (++i >= argc) {
                std::cerr << "Error: Option " << arg << " requires an input file path.\n";
                return 1;
            }
            inputFile = argv[i];
        } else if (arg == "-o" || arg == "--output") {
            if (++i >= argc) {
                std::cerr << "Error: Option " << arg << " requires an output file path.\n";
                return 1;
            }
            outputFile = argv[i];
        } else if (arg == "-m" || arg == "--module") {
            if (++i >= argc) {
                std::cerr << "Error: Option " << arg << " requires a module name.\n";
                return 1;
            }
            moduleName = argv[i];
        } else if (arg == "-c" || arg == "--clock-period") {
            if (++i >= argc) {
                std::cerr << "Error: Option " << arg << " requires a positive integer (clock period).\n";
                return 1;
            }
            clockPeriod = std::strtoull(argv[i], nullptr, 10);
            if (clockPeriod < 2) clockPeriod = 2;
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Error: Unknown option: \"" << arg << "\"\n";
            std::cerr << "For usage help, run: " << argv[0] << " --help\n";
            return 1;
        } else {
            // Positional arguments
            if (inputFile.empty()) {
                inputFile = arg;
            } else if (outputFile.empty()) {
                outputFile = arg;
            } else {
                std::cerr << "Error: Too many positional arguments: \"" << arg << "\"\n";
                std::cerr << "For usage help, run: " << argv[0] << " --help\n";
                return 1;
            }
        }
    }

    // 2. Verify input file was provided
    if (inputFile.empty()) {
        std::cerr << "Error: No input .vcb file specified!\n\n";
        printHelp();
        return 1;
    }

    // 3. File existence and accessibility checks
    if (!fs::exists(inputFile)) {
        std::cerr << "Error: Input file does not exist: \"" << inputFile << "\"\n";
        return 1;
    }

    if (fs::is_directory(inputFile)) {
        std::cerr << "Error: Specified path is a directory, not a file: \"" << inputFile << "\"\n";
        return 1;
    }

    // Verify file can be opened and is not empty
    {
        std::ifstream testStream(inputFile, std::ios::binary);
        if (!testStream.is_open()) {
            std::cerr << "Error: Cannot open file for reading (check permissions): \"" << inputFile << "\"\n";
            return 1;
        }
        testStream.seekg(0, std::ios::end);
        if (testStream.tellg() == 0) {
            std::cerr << "Error: Input file is empty (0 bytes): \"" << inputFile << "\"\n";
            return 1;
        }
    }

    // Auto-generate output filename from input stem + .v
    if (outputFile.empty()) {
        fs::path p(inputFile);
        outputFile = p.replace_extension(".v").string();
    }
    std::ostream& statusOutput = outputFile == "-" ? std::cerr : std::cout;

    // Check that target directory for output file exists
    if (outputFile != "-") {
        fs::path outPath(outputFile);
        if (outPath.has_parent_path() && !fs::exists(outPath.parent_path())) {
            std::cerr << "Error: Destination directory for output file does not exist: \""
                      << outPath.parent_path().string() << "\"\n";
            return 1;
        }
    }

    // Auto-generate Verilog module name from input file stem
    if (moduleName.empty()) {
        fs::path p(inputFile);
        moduleName = VerilogEmitter::sanitizeIdentifier(p.stem().string());
    }

    if (outputFile != "-") {
        printBanner();
        statusOutput << "Input file   : " << inputFile << "\n"
                  << "Output file  : " << outputFile << "\n"
                  << "Module name  : " << moduleName << "\n"
                  << "Clock period : " << clockPeriod << " cycles\n"
                  << "-----------------------------------------------------\n";
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    // 4. Load and decompress VCB file
    openVCB::Project proj;
    proj.clockPeriod = clockPeriod;

    if (verbose) statusOutput << "[1/3] Loading and decompressing VCB circuit...\n";
    if (!proj.readFromVCB(inputFile)) {
        std::cerr << "Error: Failed to parse or decompress circuit from: \"" << inputFile << "\"\n";
        return 1;
    }

    if (proj.width <= 0 || proj.height <= 0) {
        std::cerr << "Error: File does not contain a valid circuit grid.\n";
        return 1;
    }

    // 5. Topology analysis and netlist generation
    if (verbose) {
        statusOutput << "[2/3] Analyzing circuit topology ("
                  << proj.width << "x" << proj.height << " grid)...\n";
    }
    proj.preprocess(false);

    if (verbose) {
        statusOutput << "      Discovered " << proj.numGroups << " logic groups with "
                  << proj.writeMap.nnz << " wire connections.\n";
    }

    // Assemble virtual memory (if present)
    proj.assembleVmem();

    // Override virtual memory contents from external .vcbmem file if requested
    if (!memFile.empty()) {
        if (!fs::exists(memFile)) {
            std::cerr << "Error: Memory file does not exist: \"" << memFile << "\"\n";
            return 1;
        }
        if (!proj.vmem || proj.vmemSize == 0) {
            std::cerr << "Error: Project does not contain virtual memory, cannot apply --mem.\n";
            return 1;
        }
        std::ifstream mf(memFile, std::ios::binary);
        if (!mf.is_open()) {
            std::cerr << "Error: Cannot open memory file: \"" << memFile << "\"\n";
            return 1;
        }
        mf.seekg(0, std::ios::end);
        size_t fsize = (size_t)mf.tellg();
        mf.seekg(0, std::ios::beg);
        std::vector<unsigned char> buf(fsize);
        mf.read((char*)buf.data(), fsize);
        if ((size_t)mf.gcount() != fsize) {
            std::cerr << "Error: Failed to read memory file: \"" << memFile << "\"\n";
            return 1;
        }
        size_t expected = (size_t)proj.vmemSize * 4;
        if (fsize != expected) {
            std::cerr << "Warning: Memory file size (" << fsize << " bytes) differs from expected ("
                      << expected << " bytes).\n";
        }
        for (size_t i = 0; i < proj.vmemSize; i++) {
            unsigned int val = 0;
            for (int b = 0; b < 4; b++) {
                size_t idx = i*4 + b;
                if (idx < buf.size()) val |= ((unsigned int)buf[idx]) << (8*b);
            }
            proj.vmem[i] = (int)val;
        }
        statusOutput << "Loaded virtual memory contents from: " << memFile << "\n";
    }

    // Export initial memory contents to .mem file (hex) if virtual memory present
    std::string memFilePath;
    if (proj.vmem && proj.vmemSize > 0) {
        std::string outDir = ".";
        if (outputFile != "-") {
            fs::path op(outputFile);
            if (op.has_parent_path()) outDir = op.parent_path().string();
        }
        std::string memName = VerilogEmitter::sanitizeIdentifier(moduleName) + "_mem_" +
                              std::to_string(proj.vmAddr.pos.x) + "_" +
                              std::to_string(proj.vmAddr.pos.y) + ".mem";
        memFilePath = outDir + "/" + memName;
        std::ofstream memOut(memFilePath);
        if (memOut.is_open()) {
            for (size_t i = 0; i < proj.vmemSize; i++) {
                memOut << std::hex << std::setw(8) << std::setfill('0') << proj.vmem[i] << std::dec << "\n";
            }
            memOut.close();
            statusOutput << "Exported virtual memory init file: " << memFilePath << "\n";
        } else {
            std::cerr << "Warning: Could not write memory init file: " << memFilePath << "\n";
        }
    }

    // 6. Emit synthesizable Verilog HDL
    if (verbose) statusOutput << "[3/3] Emitting synthesizable Verilog...\n";

    VerilogEmitter emitter(proj);
    emitter.exposeZeroInDegreeTraces = exposeInputs;
    emitter.clockPeriod = clockPeriod;
    emitter.clockMode = clockMode;
    emitter.registerAllGates = (clockMode == "on");
    emitter.vmemMemFile = memFilePath.empty() ? std::string() : memFilePath;

    if (clockMode == "off" && emitter.hasSequentialElements()) {
        statusOutput << "WARNING: Circuit contains sequential elements (e.g. T-flip-flops, memory), but clock mode is 'off'. These may not work correctly without 'clk'.\n";
    }
    if (clockMode == "off" && emitter.getNumLoopBreaks() > 0) {
        statusOutput << "WARNING: Circuit contains combinational loop(s), but clock mode is 'off'. Raw feedback may cause 'x' states in simulation or synthesis errors.\n";
    }

    if (!emitter.emit(outputFile, moduleName)) {
        std::cerr << "Error: Failed to write Verilog output to: \"" << outputFile << "\"\n";
        return 1;
    }

    // Optional Yosys optimization of the generated Verilog
    if (optimize && outputFile != "-") {
        // Check whether yosys is available in PATH
        if (std::system("command -v yosys > /dev/null 2>&1") != 0) {
            statusOutput << "Warning: 'yosys' not found in PATH. Skipping optimization; raw .v file kept.\n";
        } else {
            std::string script;
            if (optLevel == "full") {
                script = "proc; opt -full; abc -g AND,OR,XOR,NAND,NOR,XNOR; opt_clean -purge";
            } else {
                script = "proc; opt; opt_clean -purge";
            }

            std::string tmpFile = outputFile + ".yosys_tmp.v";
            // Build shell command with quoted paths
            std::string cmd = "yosys -q -p \"read_verilog " + outputFile +
                              "; " + script + "; write_verilog -noattr " + tmpFile + "\" > /dev/null 2>&1";

            statusOutput << "Optimizing output with Yosys (level: " << optLevel << ")...\n";
            int rc = std::system(cmd.c_str());
            if (rc == 0) {
                std::error_code ec;
                if (std::filesystem::exists(tmpFile, ec)) {
                    std::filesystem::remove(outputFile, ec);
                    std::filesystem::rename(tmpFile, outputFile, ec);
                    if (ec) {
                        statusOutput << "Warning: Yosys succeeded but could not replace output file: " << ec.message() << "\n";
                        std::filesystem::remove(tmpFile, ec);
                    } else {
                        statusOutput << "Yosys optimization completed successfully.\n";
                    }
                } else {
                    statusOutput << "Warning: Yosys finished but produced no output. Raw .v file kept.\n";
                }
            } else {
                statusOutput << "Warning: Yosys optimization failed (exit code " << rc << "). Raw .v file kept.\n";
                std::error_code ec;
                std::filesystem::remove(tmpFile, ec);
            }
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (outputFile != "-") {
        statusOutput << "-----------------------------------------------------\n"
                  << "Translation completed successfully in " << ms << " ms!\n"
                  << "  Total groups/gates : " << proj.numGroups << "\n"
                  << "  Wire connections   : " << proj.writeMap.nnz << "\n"
                  << "  Input ports        : " << emitter.getNumInputs() << "\n"
                  << "  Output ports       : " << (emitter.getNumOutputs() + emitter.getNumLeds()) << "\n"
                  << "  LED outputs        : " << emitter.getNumLeds() << "\n"
                  << "  Combinational gates: " << emitter.getNumComb() << "\n"
                  << "  Registers (D-FF)   : " << emitter.getNumBuffers() << "\n"
                  << "  Latches (T-FF)     : " << emitter.getNumLatches() << "\n"
                  << "  Clock dividers     : " << emitter.getNumClocks() << "\n"
                  << "Generated file: " << outputFile << "\n"
                  << "=====================================================\n";
    }

    return 0;
}
