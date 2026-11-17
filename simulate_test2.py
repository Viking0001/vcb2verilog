import subprocess
import sys
import re

TB = """`timescale 1ns/1ps
module tb;
  reg a3, b3, a2, b2, a1, b1, a0, b0, cin;
  wire cout, s3, s2, s1, s0, cin_out;
  test2 dut (
    .sw_1483_983(a3), .sw_1485_983(b3),
    .sw_1487_983(a2), .sw_1489_983(b2),
    .sw_1491_983(a1), .sw_1493_983(b1),
    .sw_1495_983(a0), .sw_1497_983(b0),
    .sw_1499_983(cin),
    .led_1483_962(cout), .led_1485_962(s3),
    .led_1488_962(s2), .led_1493_962(s1),
    .led_1497_962(s0), .led_1499_962(cin_out)
  );
  integer i;
  reg [4:0] exp_sum;
  integer err_cout = 0, err_s3 = 0, err_s2 = 0, err_s1 = 0, err_s0 = 0, err_cin = 0;
  initial begin
    for (i = 0; i < 512; i = i + 1) begin
      {a3, a2, a1, a0, b3, b2, b1, b0, cin} = i[8:0];
      #5;
      exp_sum = {1'b0, a3, a2, a1, a0} + {1'b0, b3, b2, b1, b0} + cin;
      if (cout !== exp_sum[4]) err_cout = err_cout + 1;
      if (s3 !== exp_sum[3]) err_s3 = err_s3 + 1;
      if (s2 !== exp_sum[2]) err_s2 = err_s2 + 1;
      if (s1 !== exp_sum[1]) err_s1 = err_s1 + 1;
      if (s0 !== exp_sum[0]) err_s0 = err_s0 + 1;
      if (cin_out !== cin) err_cin = err_cin + 1;
    end
    $display("ERRORS: Cout=%0d, S3=%0d, S2=%0d, S1=%0d, S0=%0d, Cin=%0d (out of 512)",
             err_cout, err_s3, err_s2, err_s1, err_s0, err_cin);
    if (err_cout + err_s3 + err_s2 + err_s1 + err_s0 + err_cin == 0) begin
      $display("ALL 512 VECTORS PASSED!");
    end
    $finish;
  end
endmodule
"""

def main():
    res = subprocess.run(["cmake", "--build", "build", "-j"])
    if res.returncode != 0:
        sys.exit(1)

    print("Running python3 tests/run_tests.py...")
    res = subprocess.run(["python3", "tests/run_tests.py"])
    if res.returncode != 0:
        print("Tests failed!")
        sys.exit(1)

    print("Compiling test2.vcb...")
    res = subprocess.run(["./build/vcb2verilog", "-O", "full", "./test2.vcb", "-o", "./build/test2.v"])
    if res.returncode != 0:
        print("vcb2verilog failed!")
        sys.exit(1)

    with open("./build/test2.v", "r", encoding="utf-8") as f:
        verilog = f.read()
    logic_verilog = re.sub(r"/\*[\s\S]*?\*/|//[^\n]*", "", verilog)
    if re.search(r"\b(?:reg|always|posedge|negedge)\b", logic_verilog):
        print("test2.v is not purely combinational")
        sys.exit(1)
    if re.search(r"\b(?:clk|rst)\b", logic_verilog, re.IGNORECASE):
        print("test2.v contains an unexpected clock or reset signal")
        sys.exit(1)

    yosys = subprocess.run(
        ["yosys", "-Q", "-p", "read_verilog ./build/test2.v; hierarchy -auto-top; proc; opt; check; scc"],
        capture_output=True,
        text=True,
    )
    if yosys.returncode != 0 or "Found 0 SCCs" not in yosys.stdout:
        print("Yosys found a combinational loop or failed to check test2.v")
        print(yosys.stdout)
        print(yosys.stderr)
        sys.exit(1)
    print("Yosys: 0 combinational SCCs; Verilog is combinational.")

    with open("/tmp/tb_test2.v", "w") as f:
        f.write(TB)

    res = subprocess.run(["iverilog", "-o", "/tmp/tb_test2.vvp", "./build/test2.v", "/tmp/tb_test2.v"])
    if res.returncode != 0:
        print("iverilog failed!")
        sys.exit(1)

    res = subprocess.run(["vvp", "/tmp/tb_test2.vvp"], capture_output=True, text=True)
    print(res.stdout, end="")
    print(res.stderr, end="", file=sys.stderr)
    if res.returncode != 0:
        print("vvp failed!")
        sys.exit(1)
    if "ALL 512 VECTORS PASSED!" not in res.stdout:
        print("test2.v failed one or more of the 512 adder vectors")
        sys.exit(1)

if __name__ == "__main__":
    main()
