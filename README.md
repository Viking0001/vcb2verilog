# vcb2verilog

**vcb2verilog** is an open-source command-line tool that transpiles digital circuit blueprints designed in the sandbox game [Virtual Circuit Board (VCB)](https://store.steampowered.com/app/1885690/Virtual_Circuit_Board/) into clean, synthesizable **Verilog HDL** for real FPGA hardware (e.g., Lattice iCE40, ECP5, Xilinx Artix/Spartan, Gowin) and Verilog simulators.

---

> [!WARNING]
> ### 🤖 AI Development Disclaimer
> This project was developed and refactored with the assistance of artificial intelligence (LLM coding agents). While the generated code has been compiled, analyzed, and verified with test projects, **AI can make mistakes**. Circuit designs intended for safety-critical systems or production hardware should always be independently reviewed, simulated (e.g., via Icarus Verilog or Verilator), and validated before deployment on physical silicon.

---

## 🚀 Key Features

- **Direct VCB to Verilog Transpilation**: Converts `.vcb` files (both VCB v1 Godot format and VCB v2 JSON base64 format) into standard, synthesizable Verilog HDL.
- **Continuous Combinational Logic**: Translates logic gates (`AND`, `OR`, `NOT`, `NAND`, `NOR`, `XOR`, `XNOR`, `TRACE`, `BUNDLE`) into asynchronous continuous assignments (`assign`), propagating signals in nanoseconds on FPGA LUTs without waiting for software game ticks.
- **Hardware-Accurate Latches (T-Flip-Flop)**: Maps VCB Latch memory elements into clocked T-Flip-Flops with rising-edge detection (`always @(posedge clk)`). Latches without internal driving inputs are automatically exposed as external pushbutton (`btn_N`) inputs.
- **Pipeline Delay Buffers (D-Flip-Flop)**: Translates 1-tick delay VCB Buffers into synchronous D-type flip-flop registers.
- **Top-Level Ports & LEDs**: Automatically maps boundary READ/WRITE pins and game LEDs to top-level module ports (`input wire in_N`, `output wire led_N`).
- **Open-Source EDA Toolchain Integration**: Designed for seamless synthesis with **Yosys**, place-and-route with **nextpnr**, and simulation with **Icarus Verilog** (`iverilog`).
- **Blazing Fast**: Parses, processes 2D grid connectivity, and emits Verilog for 130,000+ gate microarchitectures in under **200 milliseconds**.

---

## 📦 Requirements & Dependencies

- **C++17** compatible compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- **CMake** 3.14 or newer
- **libzstd-dev** (Zstandard compression library)
- **libglm-dev** (OpenGL Mathematics header library)
- **libomp-dev** (OpenMP for multithreaded grid parsing, optional)

### Installing Dependencies

**Ubuntu / Debian / Raspberry Pi OS:**
```bash
sudo apt update
sudo apt install -y build-essential cmake libzstd-dev libglm-dev libomp-dev
```

**Fedora / RHEL:**
```bash
sudo dnf install -y gcc-c++ cmake zstd-devel glm-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake zstd glm openmp
```

---

## 🛠️ Build & Installation

### Build with CMake

```bash
git clone https://github.com/Viking0001/openVCB.git vcb2verilog
cd vcb2verilog
mkdir build && cd build
cmake ..
make -j$(nproc)
```

The compiled executable `vcb2verilog` will be created inside the `build/` directory.

### Direct Build with g++ (without CMake)

```bash
g++ -std=c++17 -O3 -Wall -Wno-switch -Wno-unused-parameter \
    src/main.cpp \
    src/verilog_emitter.cpp \
    src/openVCB.cpp \
    src/openVCBReader.cpp \
    src/openVCBBlueprint.cpp \
    src/openVCBPreprocessing.cpp \
    src/openVCBAssembler.cpp \
    src/openVCBExpr.cpp \
    gorder/Graph.cpp \
    gorder/Util.cpp \
    gorder/UnitHeap.cpp \
    -Isrc -I. -lzstd -fopenmp -o vcb2verilog
```

---

## 💡 Quick Start & CLI Usage

```
vcb2verilog [OPTIONS] <input.vcb> [-o <output.v>]
vcb2verilog -i <input.vcb> [-o <output.v>]
```

### Options

| Flag | Argument | Description | Default |
|---|---|---|---|
| `-i, --input` | `<file>` | Path to the input `.vcb` file | *1st positional argument* |
| `-o, --output` | `<file>` | Output `.v` file path (`-` for stdout) | `<input_stem>.v` |
| `-m, --module` | `<name>` | Name of the generated Verilog top module | Derived from input |
| `-c, --clock-period` | `<N>` | Clock divider period (FPGA cycles per VCB tick) | `2` |
| `-e, --expose-inputs` | *None* | Expose undriven traces as top-level `input wire` | Enabled |
| `--no-expose-inputs` | *None* | Tie undriven traces to constant `1'b0` | Disabled |
| `-v, --verbose` | *None* | Print detailed translation steps & netlist stats | Disabled |
| `-h, --help` | *None* | Display help message and exit | |
| `--version` | *None* | Display version information and exit | |

### Conversion Example

Convert the included 2-bit counter example:
```bash
./build/vcb2verilog examples/counter_2bit.vcb -o counter_2bit.v
```

Output:
```
=====================================================
 vcb2verilog v1.0.0 - Virtual Circuit Board to Verilog
 Cílová platforma: Syntetizovatelný Verilog pro FPGA
=====================================================
Vstupní soubor : examples/counter_2bit.vcb
Výstupní soubor: counter_2bit.v
Název modulu   : counter_2bit
Perioda hodin  : 2 cyklů
-----------------------------------------------------
Překlad úspěšně dokončen za 34.7 ms!
  Počet skupin/hradel: 7
  Počet spojů        : 6
  Vstupní porty      : 2
  Výstupní porty     : 1
  LED výstupy        : 1
  Kombinační hradla  : 4
  Registry (D-FF)    : 0
  Paměti (Latch/T-FF): 2
  Děličky hodin      : 0
Výsledný soubor: counter_2bit.v
=====================================================
```

---

## 🔬 Hardware Synthesis & Simulation Verification

### 1. Verification with Icarus Verilog Simulation

You can verify the syntax and simulate the generated Verilog using `iverilog`:

```bash
# Check syntax & compile simulation
iverilog -Wall -g2012 counter_2bit.v -o sim.out
```

### 2. FPGA Synthesis with Yosys (e.g. Lattice iCE40)

Run synthesis to gate-level netlist:

```bash
# Synthesize to iCE40 primitives with Yosys
yosys -p "synth_ice40 -top counter_2bit -json counter_2bit.json" counter_2bit.v
```

### 3. Place & Route and Bitstream Generation (nextpnr + IceStorm)

```bash
# Place and Route targeting an iCE40-HX1K chip
nextpnr-ice40 --hx1k --package tq144 --json counter_2bit.json --pcf pins.pcf --asc counter_2bit.asc

# Pack into FPGA bitstream
icepack counter_2bit.asc counter_2bit.bin

# Flash bitstream to FPGA board (e.g., IceBreaker, iCEstick)
iceprog counter_2bit.bin
```

---

## 🗺️ VCB Element Mapping Reference

| VCB Ink Element | Internal Logic | Verilog Representation | Hardware Implementation |
|---|---|---|---|
| **READ** | Abstract Input Port | `input wire in_GID` | External FPGA pin / button |
| **WRITE** | Abstract Output Port | `output wire out_GID` | External FPGA pin / pad |
| **LED** | Visual Indicator | `output wire led_GID` | On-board FPGA LED |
| **TRACE / BUNDLE** | Wire / 16-color Bus | `assign g_N = in1 \| in2;` | Combinational routing network |
| **CROSS** | Isolated Wire Crossing | Independent Net Labels | Physical 3D routing on FPGA metal layers |
| **BUFFER** | 1-tick Game Delay | `always @(posedge clk) g_N <= in;` | 1-cycle D Flip-Flop register |
| **LATCH** | Toggle on Input Pulse | `if (in && !prev) q <= ~q;` | Clocked T Flip-Flop with edge detector |
| **CLOCK** | Periodic Game Clock | Synchronous 32-bit Counter | Parameterized frequency divider on `clk` |
| **AND / NAND** | All Inputs Active | `assign g_N = in1 & in2;` | FPGA Look-Up Table (LUT) |
| **OR / NOR** | Any Input Active | `assign g_N = in1 \| in2;` | FPGA Look-Up Table (LUT) |
| **XOR / XNOR** | Parity Checker | `assign g_N = in1 ^ in2;` | FPGA Look-Up Table (LUT) |
| **BREAKPOINT** | Simulation Pause | *Ignored* | Omitted from synthesizable hardware |

---

## 📜 License & Credits

- Licensed under the **GNU General Public License v3.0 (GPL-3.0-or-later)**. See [LICENSE.md](LICENSE.md).
- Based on the original open-source [openVCB](https://github.com/kittybupu/openVCB) parser by Jerry.
- Virtual Circuit Board is developed by Corbinian. Available on [Steam](https://store.steampowered.com/app/1885690/Virtual_Circuit_Board/).
