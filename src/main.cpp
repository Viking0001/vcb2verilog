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
              << " Cílová platforma: Syntetizovatelný Verilog pro FPGA\n"
              << "=====================================================\n";
}

void printHelp(const char* progName) {
    std::cout << "Použití: " << progName << " [PŘEPÍNAČE] <vstup.vcb> [-o <vystup.v>]\n"
              << "         " << progName << " [PŘEPÍNAČE] -i <vstup.vcb> [-o <vystup.v>]\n\n"
              << "Popis:\n"
              << "  Převádí logické obvody ze hry Virtual Circuit Board (.vcb)\n"
              << "  do syntetizovatelného kódu ve Verilogu (.v) pro FPGA čipy.\n\n"
              << "Přepínače:\n"
              << "  -i, --input <soubor>       Cesta ke vstupnímu .vcb souboru\n"
              << "                             (lze zadat i přímo jako 1. poziční argument)\n"
              << "  -o, --output <soubor>      Cesta k výstupnímu .v souboru\n"
              << "                             (výchozí: název vstupu s příponou .v, nebo '-' pro stdout)\n"
              << "  -m, --module <název>       Název vygenerovaného Verilog top modulu\n"
              << "                             (výchozí: odvozeno z názvu vstupního souboru)\n"
              << "  -c, --clock-period <N>     Perioda herních hodin v FPGA cyklech (výchozí: 2)\n"
              << "  -e, --expose-inputs        Vystavit nepřipojené vodiče jako top-level vstupní porty (výchozí)\n"
              << "      --no-expose-inputs     Nepřipojené vodiče uzemnit (přiřadit 1'b0)\n"
              << "  -v, --verbose              Podrobný výpis průběhu a statistiky zapojení\n"
              << "  -h, --help                 Zobrazit tuto nápovědu a ukončit program\n"
              << "      --version              Zobrazit verzi programu a ukončit program\n\n"
              << "Příklady použití:\n"
              << "  " << progName << " obvod.vcb\n"
              << "  " << progName << " obvod.vcb -o vystup.v\n"
              << "  " << progName << " -i obvod.vcb -o vystup.v -m muj_obvod -v\n";
}

int main(int argc, char** argv) {
    std::string inputFile;
    std::string outputFile;
    std::string moduleName;
    unsigned long long clockPeriod = 2;
    bool exposeInputs = true;
    bool verbose = false;

    // 1. Zpracování argumentů příkazové řádky
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printBanner();
            printHelp(argv[0]);
            return 0;
        } else if (arg == "--version") {
            std::cout << "vcb2verilog verze " << VERSION << "\n";
            return 0;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-e" || arg == "--expose-inputs") {
            exposeInputs = true;
        } else if (arg == "--no-expose-inputs") {
            exposeInputs = false;
        } else if (arg == "-i" || arg == "--input") {
            if (++i >= argc) {
                std::cerr << "Chyba: Přepínač " << arg << " vyžaduje cestu ke vstupnímu souboru.\n";
                return 1;
            }
            inputFile = argv[i];
        } else if (arg == "-o" || arg == "--output") {
            if (++i >= argc) {
                std::cerr << "Chyba: Přepínač " << arg << " vyžaduje cestu k výstupnímu souboru.\n";
                return 1;
            }
            outputFile = argv[i];
        } else if (arg == "-m" || arg == "--module") {
            if (++i >= argc) {
                std::cerr << "Chyba: Přepínač " << arg << " vyžaduje název Verilog modulu.\n";
                return 1;
            }
            moduleName = argv[i];
        } else if (arg == "-c" || arg == "--clock-period") {
            if (++i >= argc) {
                std::cerr << "Chyba: Přepínač " << arg << " vyžaduje celé číslo (periodu hodin).\n";
                return 1;
            }
            clockPeriod = std::strtoull(argv[i], nullptr, 10);
            if (clockPeriod < 2) clockPeriod = 2;
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Chyba: Neznámý parametr: \"" << arg << "\"\n";
            std::cerr << "Pro nápovědu použijte: " << argv[0] << " --help\n";
            return 1;
        } else {
            // Poziční argument
            if (inputFile.empty()) {
                inputFile = arg;
            } else if (outputFile.empty()) {
                outputFile = arg;
            } else {
                std::cerr << "Chyba: Příliš mnoho pozičních argumentů: \"" << arg << "\"\n";
                std::cerr << "Pro nápovědu použijte: " << argv[0] << " --help\n";
                return 1;
            }
        }
    }

    // 2. Kontrola, zda byl zadán vstupní soubor
    if (inputFile.empty()) {
        printBanner();
        std::cerr << "Chyba: Nebyl zadán žádný vstupní soubor (.vcb)!\n\n";
        printHelp(argv[0]);
        return 1;
    }

    // 3. Ošetření chyb přístupu k souborům
    if (!fs::exists(inputFile)) {
        std::cerr << "Chyba: Vstupní soubor neexistuje: \"" << inputFile << "\"\n";
        return 1;
    }

    if (fs::is_directory(inputFile)) {
        std::cerr << "Chyba: Zadaná cesta je adresář, nikoli soubor: \"" << inputFile << "\"\n";
        return 1;
    }

    // Ověření, že soubor lze otevřít a není prázdný
    {
        std::ifstream testStream(inputFile, std::ios::binary);
        if (!testStream.is_open()) {
            std::cerr << "Chyba: Soubor nelze otevřít pro čtení (zkontrolujte oprávnění): \"" << inputFile << "\"\n";
            return 1;
        }
        testStream.seekg(0, std::ios::end);
        if (testStream.tellg() == 0) {
            std::cerr << "Chyba: Vstupní soubor je prázdný (0 bajtů): \"" << inputFile << "\"\n";
            return 1;
        }
    }

    // Automatický název výstupního souboru podle vstupu s příponou .v
    if (outputFile.empty()) {
        fs::path p(inputFile);
        outputFile = p.replace_extension(".v").string();
    }

    // Ověření existence nadřazeného adresáře pro výstupní soubor
    if (outputFile != "-") {
        fs::path outPath(outputFile);
        if (outPath.has_parent_path() && !fs::exists(outPath.parent_path())) {
            std::cerr << "Chyba: Cílový adresář pro výstupní soubor neexistuje: \""
                      << outPath.parent_path().string() << "\"\n";
            return 1;
        }
    }

    // Automatický název Verilog modulu z názvu souboru
    if (moduleName.empty()) {
        fs::path p(inputFile);
        moduleName = VerilogEmitter::sanitizeIdentifier(p.stem().string());
    }

    if (outputFile != "-") {
        printBanner();
        std::cout << "Vstupní soubor : " << inputFile << "\n"
                  << "Výstupní soubor: " << outputFile << "\n"
                  << "Název modulu   : " << moduleName << "\n"
                  << "Perioda hodin  : " << clockPeriod << " cyklů\n"
                  << "-----------------------------------------------------\n";
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    // 4. Načtení a dekomprese VCB souboru
    openVCB::Project proj;
    proj.clockPeriod = clockPeriod;

    if (verbose) std::cout << "[1/3] Načítám a dekomprimuji VCB obvod...\n";
    if (!proj.readFromVCB(inputFile)) {
        std::cerr << "Chyba: Nepodařilo se zpracovat obvod ze souboru: \"" << inputFile << "\"\n";
        return 1;
    }

    if (proj.width <= 0 || proj.height <= 0) {
        std::cerr << "Chyba: Soubor neobsahuje platnou mřížku obvodu.\n";
        return 1;
    }

    // 5. Analýza topologie a sestavení grafu propojení
    if (verbose) {
        std::cout << "[2/3] Analyzuji topologii obvodu (mřížka "
                  << proj.width << "x" << proj.height << ")...\n";
    }
    proj.preprocess(false);

    if (verbose) {
        std::cout << "      Nalezeno " << proj.numGroups << " logických skupin a "
                  << proj.writeMap.nnz << " propojení vodičů.\n";
    }

    // 6. Generování syntetizovatelného Verilogu
    if (verbose) std::cout << "[3/3] Generuji syntetizovatelný Verilog...\n";

    VerilogEmitter emitter(proj);
    emitter.exposeZeroInDegreeTraces = exposeInputs;
    emitter.clockPeriod = clockPeriod;

    if (!emitter.emit(outputFile, moduleName)) {
        std::cerr << "Chyba: Nepodařilo se zapsat Verilog do: \"" << outputFile << "\"\n";
        return 1;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (outputFile != "-") {
        std::cout << "-----------------------------------------------------\n"
                  << "Překlad úspěšně dokončen za " << ms << " ms!\n"
                  << "  Počet skupin/hradel: " << proj.numGroups << "\n"
                  << "  Počet spojů        : " << proj.writeMap.nnz << "\n"
                  << "  Vstupní porty      : " << emitter.getNumInputs() << "\n"
                  << "  Výstupní porty     : " << (emitter.getNumOutputs() + emitter.getNumLeds()) << "\n"
                  << "  LED výstupy        : " << emitter.getNumLeds() << "\n"
                  << "  Kombinační hradla  : " << emitter.getNumComb() << "\n"
                  << "  Registry (D-FF)    : " << emitter.getNumBuffers() << "\n"
                  << "  Paměti (Latch/T-FF): " << emitter.getNumLatches() << "\n"
                  << "  Děličky hodin      : " << emitter.getNumClocks() << "\n"
                  << "Výsledný soubor: " << outputFile << "\n"
                  << "=====================================================\n";
    }

    return 0;
}
