# vcb2verilog

`vcb2verilog` converts Virtual Circuit Board (VCB) projects into Verilog HDL.
It reads `.vcb` project files and writes a Verilog module that can be simulated
or passed to an FPGA synthesis tool. It does not assign board pins or program
FPGA hardware.

The reader handles the Godot-style and JSON VCB project formats. Circuit layers
are compressed with Zstandard.

## Features

- Combinational gates: AND, OR, NOT, NAND, NOR, XOR, and XNOR.
- READ and WRITE connections, LEDs, traces, and bundles.
- BUS and Mesh merging for compatible components of the same logic type. BUS
  is a component-merging mechanism, not a signal wire. Mesh joins matching
  components across the design.
- Tunnel links to the first Tunnel found along a straight row or column.
- Cross preserves separate paths through a crossing.
- Clocked elements and VCB virtual memory, with behavior controlled by the
  clock options below.

## Build

Requirements: a C++17 compiler, CMake 3.14 or newer, Zstandard development
files, and GLM headers. OpenMP is optional.

On Debian or Ubuntu:

```bash
sudo apt update
sudo apt install build-essential cmake libzstd-dev libglm-dev
# Optional: OpenMP support
sudo apt install libomp-dev
```

Build from the repository root:

```bash
cmake -S . -B build
cmake --build build -j
```

The executable is `build/vcb2verilog`.

## Usage

```bash
./build/vcb2verilog [OPTIONS] <input.vcb> [output.v]
```

For example, convert the included counter project:

```bash
./build/vcb2verilog examples/counter_2bit.vcb -o counter_2bit.v
```

If no output path is given, the input path is used with its extension replaced
by `.v`. Use `-o -` to write Verilog to stdout; status messages go to stderr.
The default module name is the sanitized input filename stem.

### Options

| Option | Description and default |
|---|---|
| `-i, --input <file>` | Input `.vcb` file. Defaults to the first positional argument; required if none is given. |
| `-o, --output <file>` | Output Verilog file. Defaults to the input path with `.v` extension. `-` writes to stdout. |
| `-m, --module <name>` | Top-level module name. Defaults to the sanitized input filename stem. |
| `-e, --expose-inputs` | Expose eligible undriven READ/Trace/Bundle groups as input ports. Disabled by default. |
| `--no-expose-inputs` | Tie eligible undriven READ/Trace/Bundle groups to `1'b0`. This is the default. |
| `--clock <mode>` | Clock mode: `auto` (default), `on`, or `off`. See [Clock behavior](#clock-behavior). |
| `-c, --clock-period <N>` | Divider period for VCB Clock elements, in input `clk` cycles. Defaults to `2`; values below `2` are clamped to `2`. |
| `--no-opt` | Disable Yosys optimization. Disabled by default. |
| `-O, --opt-level <level>` | Yosys optimization level: `opt` (default) or `full`. Ignored when `--no-opt` is used. |
| `-M, --mem <file>` | Override memory with a `.vcbmem` file. If omitted, memory data embedded in the input `.vcb` is used. |
| `-v, --verbose` | Print translation stages and circuit statistics. Disabled by default. |
| `--debug` | Print detailed topology diagnostics. Disabled by default. |
| `-h, --help` | Show command-line help. Not shown by default. |
| `--version` | Show the program version. Not printed by default. |

Run `./build/vcb2verilog --help` for the full help text.

### Examples

```bash
# Choose an output file and module name
./build/vcb2verilog circuit.vcb -o output.v --module my_circuit

# Expose undriven traces as inputs
./build/vcb2verilog circuit.vcb --expose-inputs

# Force clock and reset generation
./build/vcb2verilog circuit.vcb --clock on

# Replace embedded virtual-memory contents
./build/vcb2verilog memory_project.vcb --mem initial.vcbmem

# Send only Verilog to stdout
./build/vcb2verilog circuit.vcb -o - > output.v

# Show translation progress and statistics
./build/vcb2verilog circuit.vcb --verbose
```

## Signal and component behavior

By default, eligible undriven READ, Trace, and Bundle groups that feed logic are
assigned `1'b0`. `--expose-inputs` promotes them to top-level inputs named
`input_X_Y`. WRITE and LED groups become output ports named `output_X_Y` and
`led_X_Y`. Free Latch controls can appear as `sw_X_Y` inputs. Sequential
circuits may also have `clk` and `rst` ports.

An ordinary Bundle carries routed signal connections. BUS and Mesh merge
compatible components of the same logic type into one logical group. A Tunnel
links matching groups through the first Tunnel encountered in a straight
direction. Cross keeps intersecting paths separate.

## Clock behavior

- `auto` (default) adds `clk` and `rst` when the circuit graph contains
  recognized sequential elements. VCB tick timing can make the intended timing
  unclear for some circuits.
- `on` always adds `clk` and `rst` and registers all gates. Use it only when
  the circuit is designed for synchronous gate behavior.
- `off` requests combinational output. Stateful elements may not work correctly
  in this mode. Virtual memory still uses a clock.

In `auto` and `off` modes, Buffer elements are treated as combinational logic,
not as one-cycle delays. In `on` mode, Buffers are clocked. `--clock-period`
controls the divider period for VCB Clock elements; it is measured in cycles
of the input `clk`.

## Virtual memory

For projects with VCB virtual memory enabled, the converter emits a Verilog
memory initialized with `$readmemh` and writes a hex `.mem` initialization file
next to the output Verilog file (or in the working directory for stdout
output). The `.vcb` project's embedded memory data is used by default.
`--mem <file.vcbmem>` (or `-M`) replaces it with an external binary memory file.
The project must contain virtual memory to use this option.

## Yosys optimization

Yosys is optional. When it is available, the converter runs the generated file
through Yosys using `opt` by default. `-O full` selects a more extensive
optimization pass. `--no-opt` skips optimization. Optimization is also skipped
when output is sent to stdout. If Yosys is unavailable, the converter keeps the
generated Verilog without optimization.

## Tests

Build the executable, then run the regression suite:

```bash
cmake --build build -j
python3 tests/run_tests.py
```

The test suite uses Yosys and Icarus Verilog (`iverilog`). It also needs either
the `zstd` command-line tool or Python's `zstandard` package to generate test
projects.

## Project layout

- `src/main.cpp` — command-line interface and conversion flow.
- `src/openVCBReader.cpp` — VCB project loading.
- `src/openVCBPreprocessing.cpp` — pixel connectivity and component grouping.
- `src/verilog_emitter.cpp` — Verilog generation.
- `examples/` — sample VCB projects.
- `tests/run_tests.py` — regression tests.
- `gorder/` — bundled graph-ordering code.

## License and acknowledgements

The project is licensed under GPL-3.0-or-later; see [LICENSE.md](LICENSE.md).
It builds on the openVCB file-format parser by [Jerry](https://github.com/kittybupu/openVCB).
Virtual Circuit Board is available on [Steam](https://store.steampowered.com/app/1885690/Virtual_Circuit_Board/).
