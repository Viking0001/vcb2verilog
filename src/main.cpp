// vcb2verilog - Virtual Circuit Board (.vcb) to Verilog HDL Transpiler
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "openVCB.h"
#include "verilog_emitter.h"

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

void printHelp(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS] <input.vcb> [-o <output.v>]\n"
              << "       " << progName << " [OPTIONS] -i <input.vcb> [-o <output.v>]\n\n"
              << "Description:\n"
              << "  Transpiles digital logic circuits from Virtual Circuit Board (.vcb)\n"
              << "  into clean, synthesizable Verilog HDL (.v) for FPGA silicon.\n\n"
              << "Options:\n"
              << "  -i, --input <file>         Path to input .vcb file\n"
              << "                             (can also be passed directly as 1st positional argument)\n"
              << "  -o, --output <file>        Path to output .v file\n"
              << "                             (default: input stem + .v, or '-' for stdout)\n"
              << "  -m, --module <name>        Top-level Verilog module name\n"
              << "                             (default: derived from input file stem)\n"
              << "  -c, --clock-period <N>     Clock divider period in FPGA cycles (default: 2)\n"
              << "  -e, --expose-inputs        Expose undriven traces as top-level input ports (default)\n"
              << "      --no-expose-inputs     Tie undriven traces to ground (1'b0)\n"
              << "  -v, --verbose              Display detailed translation stages & circuit statistics\n"
              << "  -h, --help                 Show this help message and exit\n"
              << "      --version              Show version information and exit\n\n"
              << "Examples:\n"
              << "  " << progName << " circuit.vcb\n"
              << "  " << progName << " circuit.vcb -o output.v\n"
              << "  " << progName << " -i circuit.vcb -o output.v -m my_circuit -v\n";
}

int main(int argc, char** argv) {
    std::string inputFile;
    std::string outputFile;
    std::string moduleName;
    unsigned long long clockPeriod = 2;
    bool exposeInputs = true;
    bool verbose = false;

    // 1. Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printBanner();
            printHelp(argv[0]);
            return 0;
        } else if (arg == "--version") {
            std::cout << "vcb2verilog version " << VERSION << "\n";
            return 0;
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
        printBanner();
        std::cerr << "Error: No input .vcb file specified!\n\n";
        printHelp(argv[0]);
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
        std::cout << "Input file   : " << inputFile << "\n"
                  << "Output file  : " << outputFile << "\n"
                  << "Module name  : " << moduleName << "\n"
                  << "Clock period : " << clockPeriod << " cycles\n"
                  << "-----------------------------------------------------\n";
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    // 4. Load and decompress VCB file
    openVCB::Project proj;
    proj.clockPeriod = clockPeriod;

    if (verbose) std::cout << "[1/3] Loading and decompressing VCB circuit...\n";
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
        std::cout << "[2/3] Analyzing circuit topology ("
                  << proj.width << "x" << proj.height << " grid)...\n";
    }
    proj.preprocess(false);

    if (verbose) {
        std::cout << "      Discovered " << proj.numGroups << " logic groups with "
                  << proj.writeMap.nnz << " wire connections.\n";
    }

    // 6. Emit synthesizable Verilog HDL
    if (verbose) std::cout << "[3/3] Emitting synthesizable Verilog...\n";

    VerilogEmitter emitter(proj);
    emitter.exposeZeroInDegreeTraces = exposeInputs;
    emitter.clockPeriod = clockPeriod;

    if (!emitter.emit(outputFile, moduleName)) {
        std::cerr << "Error: Failed to write Verilog output to: \"" << outputFile << "\"\n";
        return 1;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (outputFile != "-") {
        std::cout << "-----------------------------------------------------\n"
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
