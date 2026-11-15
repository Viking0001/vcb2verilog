#!/usr/bin/env python3
import base64
import json
import os
import re
import shutil
import struct
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(REPO, "build", "vcb2verilog")
TMP_DIR = "/tmp/vcb2verilog_tests"

COLORS = {
    ".": None,
    "Trace": 0x5A5845,
    "Trace1": 0x9FA8AE,
    "Trace2": 0xA1555E,
    "Trace3": 0xA16C56,
    "Trace4": 0x6CA156,
    "Trace5": 0x5693A1,
    "R": 0x2E475D,
    "W": 0x4D383E,
    "Cross": 0x66788E,
    "Buffer": 0x3E4D3E,
    "OR": 0x384B56,
    "NAND": 0x4D392F,
    "NOT": 0x4D3744,
    "NOR": 0x1A3C56,
    "AND": 0x4D453E,
    "XOR": 0x433D56,
    "NXOR": 0x3B2854,
    "XNOR": 0x3B2854,
    "Clock": 0x4D243C,
    "Latch": 0x384D47,
    "LED": 0x323841,
    "BUS": 0x6C9799,
    "BUS0": 0x7A2F24,
    "BUS1": 0x3E7A24,
    "BUS2": 0x24417A,
    "BUS3": 0x25627A,
    "BUS4": 0x7A2D66,
    "BUS5": 0x7A7024,
    "Tunnel": 0x535572,
    "Mesh": 0x646A57,
}


def compress_zstd(raw: bytes) -> bytes:
    try:
        import zstandard as zstd  # type: ignore
        return zstd.ZstdCompressor().compress(raw)
    except ImportError:
        p = subprocess.run(
            ["zstd", "-c", "-q"],
            input=raw,
            capture_output=True,
            check=True,
        )
        return p.stdout


def build_vcb(grid_rows: list[list[str]], out_path: str) -> None:
    height = len(grid_rows)
    width = len(grid_rows[0])
    for r in grid_rows:
        assert len(r) == width, f"Nekonzistentní šířka řádku: {len(r)} != {width}"

    buf = bytearray(width * height * 4)
    for y, row in enumerate(grid_rows):
        for x, token in enumerate(row):
            if token not in COLORS:
                raise ValueError(f"Neznámý prvek '{token}' na ({x},{y})")
            rgb = COLORS[token]
            idx = (y * width + x) * 4
            if rgb is None:
                buf[idx : idx + 4] = b"\x00\x00\x00\x00"
            else:
                r = (rgb >> 16) & 0xFF
                g = (rgb >> 8) & 0xFF
                b = rgb & 0xFF
                buf[idx : idx + 4] = struct.pack("BBBB", r, g, b, 255)

    comp = compress_zstd(bytes(buf))
    header = struct.pack("<6i", 2, height, 2, width, 2, width * height * 4)
    layer0 = base64.b64encode(comp + header).decode("ascii")

    doc = {
        "file_type": "RF_VCB_PROJECT",
        "file_version": 2,
        "title": "",
        "creation_date": 0,
        "last_edit_date": 0,
        "time_worked_on": 0,
        "layers": [layer0],
        "assembly": "",
        "is_vmem_enabled": False,
        "vmem_settings": [0] * 16,
        "camera_position": [0.0, 0.0],
        "camera_zoom": 1.0,
        "led_palette": ["0x323841", "0xffffff"],
        "docking_sizes": [0, 0, 0, 0],
        "docking_collapseness": [False] * 6,
        "docking_sidepanels": ["", "", "", ""],
        "assembly_is_external": False,
        "vmem_is_external": False,
        "notes": "",
        "clock_interval": 1,
        "timer_interval": 500,
        "random_seed": 1,
        "random_is_time_seed": True,
        "vdisplay_is_enabled": False,
        "vdisplay_is_visible": True,
        "vdisplay_settings": [0] * 8,
        "vdisplay_color_depth": 1,
        "vdisplay_direction": 0,
        "vdisplay_palette": ["", ""],
        "vinput_is_enabled": False,
        "vinput_settings": [0] * 7,
        "vinput_mode": 0,
        "vinput_bindings": "",
        "vmem_data": "",
        "decoration_palette": [""] * 32,
    }

    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(doc, f)


def parse_ports(verilog_path: str):
    with open(verilog_path, "r", encoding="utf-8") as f:
        code = f.read()

    mod_m = re.search(r"\bmodule\s+([A-Za-z0-9_]+)\s*\(", code)
    assert mod_m, "Nepodařilo se najít název modulu ve Verilogu"
    mod_name = mod_m.group(1)

    def coords(name: str):
        m = re.search(r"_(\d+)_(\d+)$", name)
        return (int(m.group(1)), int(m.group(2))) if m else (0, 0)

    inputs = sorted(set(re.findall(r"\binput\s+(?:wire\s+)?([A-Za-z0-9_]+)", code)), key=coords)
    outputs = sorted(set(re.findall(r"\boutput\s+(?:wire\s+)?([A-Za-z0-9_]+)", code)), key=coords)
    return mod_name, inputs, outputs, code


def run_yosys_check(verilog_path: str) -> None:
    cmd = [
        "yosys",
        "-Q",
        "-p",
        f"read_verilog {verilog_path}; hierarchy -auto-top; proc; check; opt",
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"Yosys selhal:\n{res.stdout}\n{res.stderr}")


def simulate_comb(verilog_path: str, test_name: str, expected_table: list[tuple[tuple[int, ...], tuple[int, ...]]]) -> None:
    mod_name, inputs, outputs, _ = parse_ports(verilog_path)
    sw_inputs = [p for p in inputs if p.startswith("sw_")]
    other_inputs = [p for p in inputs if not p.startswith("sw_")]
    if other_inputs:
        raise AssertionError(f"[{test_name}] Neočekávané extra vstupy v modulu: {other_inputs}")

    tb_path = os.path.join(TMP_DIR, f"{test_name}_tb.v")
    vvp_path = os.path.join(TMP_DIR, f"{test_name}.vvp")

    lines = [
        "`timescale 1ns/1ps",
        "module tb;",
    ]
    for inp in sw_inputs:
        lines.append(f"  reg {inp} = 0;")
    for out in outputs:
        lines.append(f"  wire {out};")

    port_conns = ", ".join(f".{p}({p})" for p in sw_inputs + outputs)
    lines.append(f"  {mod_name} dut ({port_conns});")
    lines.append("  initial begin")

    for idx, (in_vals, out_vals) in enumerate(expected_table):
        assert len(in_vals) == len(sw_inputs), f"Počet vstupů nesedí: {len(in_vals)} vs {sw_inputs}"
        assert len(out_vals) == len(outputs), f"Počet výstupů nesedí: {len(out_vals)} vs {outputs}"
        for name, val in zip(sw_inputs, in_vals):
            lines.append(f"    {name} = 1'b{val};")
        lines.append("    #10;")
        for name, exp in zip(outputs, out_vals):
            lines.append(
                f'    if ({name} !== 1\'b{exp}) begin '
                f'$display("FAIL_CASE {idx} {name}=%b expected={exp}", {name}); $finish_and_return(1); end'
            )
    lines.append('    $display("SIM_OK");')
    lines.append("    $finish;")
    lines.append("  end")
    lines.append("endmodule")

    with open(tb_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")

    c = subprocess.run(["iverilog", "-o", vvp_path, verilog_path, tb_path], capture_output=True, text=True)
    if c.returncode != 0:
        raise RuntimeError(f"iverilog selhal:\n{c.stderr}")

    s = subprocess.run(["vvp", vvp_path], capture_output=True, text=True)
    if s.returncode != 0 or "SIM_OK" not in s.stdout:
        raise RuntimeError(f"Simulace selhala:\n{s.stdout}\n{s.stderr}")


def simulate_seq_buffer(verilog_path: str, test_name: str) -> None:
    mod_name, inputs, outputs, _ = parse_ports(verilog_path)
    assert "clk" in inputs, f"[{test_name}] Chybí port 'clk', nalezené vstupy: {inputs}"
    sw_inputs = [p for p in inputs if p.startswith("sw_")]
    assert len(sw_inputs) == 1 and len(outputs) == 1
    sw = sw_inputs[0]
    led = outputs[0]
    has_rst = "rst" in inputs

    tb_path = os.path.join(TMP_DIR, f"{test_name}_tb.v")
    vvp_path = os.path.join(TMP_DIR, f"{test_name}.vvp")

    conns = [f".clk(clk)", f".{sw}({sw})", f".{led}({led})"]
    if has_rst:
        conns.append(".rst(rst)")

    tb = rf"""`timescale 1ns/1ps
module tb;
  reg clk = 0;
  {"reg rst = 1;" if has_rst else ""}
  reg {sw} = 0;
  wire {led};
  {mod_name} dut ({", ".join(conns)});
  always #5 clk = ~clk;
  initial begin
    {"#12 rst = 0;" if has_rst else "#12;"}
    if ({led} !== 1'b0) begin $display("FAIL init %b", {led}); $finish_and_return(1); end
    {sw} = 1'b1;
    #2;
    if ({led} !== 1'b0) begin $display("FAIL before posedge %b", {led}); $finish_and_return(1); end
    @(posedge clk); @(posedge clk); @(posedge clk); #2;
    if ({led} !== 1'b1) begin $display("FAIL after posedge %b", {led}); $finish_and_return(1); end
    $display("SIM_OK");
    $finish;
  end
endmodule
"""
    with open(tb_path, "w", encoding="utf-8") as f:
        f.write(tb)

    c = subprocess.run(["iverilog", "-o", vvp_path, verilog_path, tb_path], capture_output=True, text=True)
    if c.returncode != 0:
        raise RuntimeError(f"iverilog selhal:\n{c.stderr}")
    s = subprocess.run(["vvp", vvp_path], capture_output=True, text=True)
    if s.returncode != 0 or "SIM_OK" not in s.stdout:
        raise RuntimeError(f"Sekvenční simulace selhala:\n{s.stdout}\n{s.stderr}")


def compile_vcb(vcb_path: str, out_v: str) -> None:
    res = subprocess.run([BIN, vcb_path, "-o", out_v], capture_output=True, text=True)
    if res.returncode != 0:
        raise RuntimeError(f"vcb2verilog selhal:\n{res.stdout}\n{res.stderr}")


def main() -> int:
    if not os.path.exists(BIN):
        print(f"[FAIL] Binárka nenalezena: {BIN}")
        return 1

    if os.path.exists(TMP_DIR):
        shutil.rmtree(TMP_DIR)
    os.makedirs(TMP_DIR, exist_ok=True)

    tests = []

    def test_smoke_counter():
        out_v = os.path.join(TMP_DIR, "counter_2bit.v")
        compile_vcb(os.path.join(REPO, "examples", "counter_2bit.vcb"), out_v)
        run_yosys_check(out_v)

    tests.append(("1_smoke_counter_2bit", test_smoke_counter))

    def test_all_logic_gates():
        w = 26
        grid = [["."] * w for _ in range(9)]
        grid[0][0] = "Latch"
        grid[1][0] = "W"
        for x in range(w):
            grid[2][x] = "Trace"
        for x in range(w):
            grid[6][x] = "Trace1"
        grid[7][0] = "W"
        grid[8][0] = "Latch"

        gates = ["AND", "OR", "XOR", "NAND", "NOR", "NXOR"]
        for i, g in enumerate(gates):
            gx = 2 + i * 4
            grid[3][gx] = "R"
            grid[4][gx] = g
            grid[5][gx] = "R"
            grid[4][gx + 1] = "W"
            grid[4][gx + 2] = "R"
            grid[4][gx + 3] = "LED"

        vcb = os.path.join(TMP_DIR, "all_gates.vcb")
        out_v = os.path.join(TMP_DIR, "all_gates.v")
        build_vcb(grid, vcb)
        compile_vcb(vcb, out_v)
        run_yosys_check(out_v)
        table = [
            ((0, 0), (0, 0, 0, 0, 1, 1)),
            ((0, 1), (0, 1, 1, 1, 0, 0)),
            ((1, 0), (0, 1, 1, 1, 0, 0)),
            ((1, 1), (1, 1, 0, 1, 0, 1)),
        ]
        simulate_comb(out_v, "all_gates", table)

    tests.append(("2_all_two_input_gates", test_all_logic_gates))

    def test_not_chain():
        grid = [
            ["Latch", "W", "Trace", "R", "NOT", "W", "Trace1", "R", "LED", "."],
            [".",     ".", ".",     ".", ".",   ".", "R",      ".", ".",   "."],
            [".",     ".", ".",     ".", ".",   ".", "NOT",    "W", "R",   "LED"],
        ]
        vcb = os.path.join(TMP_DIR, "not_chain.vcb")
        out_v = os.path.join(TMP_DIR, "not_chain.v")
        build_vcb(grid, vcb)
        compile_vcb(vcb, out_v)
        run_yosys_check(out_v)
        table = [
            ((0,), (1, 0)),
            ((1,), (0, 1)),
        ]
        simulate_comb(out_v, "not_chain", table)

    tests.append(("3_not_and_double_negation", test_not_chain))

    def test_no_spurious_inputs():
        grid = [
            ["Latch", "W", "Trace", "R", "LED"],
            [".",     ".", ".",     ".", "."],
            ["R",     "Trace1", ".", ".", "."],
            ["BUS2",  "R", ".",     ".", "."],
        ]
        vcb = os.path.join(TMP_DIR, "floating_r.vcb")
        out_v = os.path.join(TMP_DIR, "floating_r.v")
        build_vcb(grid, vcb)
        compile_vcb(vcb, out_v)
        run_yosys_check(out_v)
        _, inputs, outputs, _ = parse_ports(out_v)
        assert len(inputs) == 1 and inputs[0].startswith("sw_"), f"Falešné vstupy: {inputs}"
        assert len(outputs) == 1
        simulate_comb(out_v, "floating_r", [((0,), (0,)), ((1,), (1,))])

    tests.append(("4_no_spurious_input_ports", test_no_spurious_inputs))

    def test_bus_channel_isolation():
        grid = [
            ["Latch", "W", "BUS2", ".",    ".",   ".", "."],
            [".",     ".", "BUS2", "R",    "AND", "W", "LED"],
            ["Latch", "W", "R",    "BUS2", ".",   ".", "."],
            [".",     ".", "BUS2", "W",    "R",   "LED", "."],
        ]
        vcb = os.path.join(TMP_DIR, "bus_iso.vcb")
        out_v = os.path.join(TMP_DIR, "bus_iso.v")
        build_vcb(grid, vcb)
        compile_vcb(vcb, out_v)
        run_yosys_check(out_v)
        _, inputs, outputs, _ = parse_ports(out_v)
        assert "clk" not in inputs and "rst" not in inputs, f"Nechtěné clk/rst: {inputs}"
        assert len(outputs) == 2

    tests.append(("5_bus_channel_isolation", test_bus_channel_isolation))

    def test_3x22_half_adder():
        grid = [
            ["LED",   ".",    "LED"],
            ["R",     ".",    "R"],
            ["W",     ".",    "W"],
            ["BUS2",  "R",    "XOR"],
            ["BUS2",  ".",    "."],
            ["BUS2",  ".",    "AND"],
            ["BUS2",  ".",    "R"],
            ["BUS2",  "R",    "."],
            ["BUS2",  "AND",  "R"],
            ["BUS2",  "AND",  "Buffer"],
            ["BUS2",  "W",    "AND"],
            ["BUS2",  "BUS2", "R"],
            ["BUS2",  "W",    "AND"],
            ["BUS2",  "BUS2", "."],
            ["BUS2",  "W",    "AND"],
            ["BUS2",  "BUS2", "R"],
            ["BUS2",  ".",    "W"],
            ["W",     "BUS2", "XOR"],
            ["AND",   "BUS2", "AND"],
            ["R",     "XOR",  "R"],
            ["W",     ".",    "W"],
            ["Latch", ".",    "Latch"],
        ]
        vcb = os.path.join(TMP_DIR, "test2_3x22.vcb")
        out_v = os.path.join(TMP_DIR, "test2_3x22.v")
        build_vcb(grid, vcb)
        compile_vcb(vcb, out_v)
        run_yosys_check(out_v)
        _, inputs, _, _ = parse_ports(out_v)
        assert "clk" not in inputs and "rst" not in inputs, f"Nechtěné clk/rst: {inputs}"
        table = [
            ((0, 0), (0, 0)),
            ((0, 1), (1, 1)),
            ((1, 0), (1, 1)),
            ((1, 1), (1, 0)),
        ]
        simulate_comb(out_v, "test2_3x22", table)

    tests.append(("6_3x22_bus2_xor_or_regression", test_3x22_half_adder))

    def test_buffer_dff():
        grid = [
            ["Latch", "W", "Trace", "R", "Buffer", "W", "Trace1", "R", "LED"],
        ]
        vcb = os.path.join(TMP_DIR, "buffer_dff.vcb")
        out_v = os.path.join(TMP_DIR, "buffer_dff.v")
        build_vcb(grid, vcb)
        subprocess.run([BIN, vcb, "-o", out_v, "--clock", "on"], check=True, capture_output=True)
        run_yosys_check(out_v)
        simulate_seq_buffer(out_v, "buffer_dff")

    tests.append(("7_sequential_buffer_dff", test_buffer_dff))

    def test_wired_or():
        grid = [
            ["Latch", "W", "Trace", "R", "LED"],
            [".",     ".", "Trace", ".", "."],
            ["Latch", "W", "Trace", ".", "."],
        ]
        vcb = os.path.join(TMP_DIR, "wired_or.vcb")
        out_v = os.path.join(TMP_DIR, "wired_or.v")
        build_vcb(grid, vcb)
        compile_vcb(vcb, out_v)
        run_yosys_check(out_v)
        table = [
            ((0, 0), (0,)),
            ((0, 1), (1,)),
            ((1, 0), (1,)),
            ((1, 1), (1,)),
        ]
        simulate_comb(out_v, "wired_or", table)

    tests.append(("8_wired_or_multi_write", test_wired_or))

    passed = 0
    failed = 0
    for name, fn in tests:
        try:
            fn()
            print(f"[PASS] {name}")
            passed += 1
        except Exception as e:
            print(f"[FAIL] {name}: {e}")
            failed += 1

    print(f"\nSouhrn: {passed}/{len(tests)} testů prošlo.")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
