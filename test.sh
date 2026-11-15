python3 - <<'PY'
with open("src/openVCBPreprocessing.cpp", "r", encoding="utf-8") as f:
    src = f.read()

# Nahradime omezujici podminku v Kroku 3 tak, aby brala vsechna cista hradla (včetně OR)
old_step3 = """        // 3) Powered gate -> nedrivovany BUS (jen kdyz na BUS nezapisuje zadne W)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Ink curInk = image[x + y * width].getInk();
                int curGid = indexImage[x + y * width];
                if (curGid < 0 || !isLogicGate(curInk) || setOff(curInk) == Ink::BufferOff) continue;
                if (inDeg[curGid] < 2) continue;"""

new_step3 = """        // 3) Powered gate -> nedrivovany BUS (včetně OR/AND/XOR)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Ink curInk = image[x + y * width].getInk();
                int curGid = indexImage[x + y * width];
                if (curGid < 0 || !isLogicGate(curInk) || setOff(curInk) == Ink::BufferOff) continue;"""

assert old_step3 in src, "Nenalezen krok 3"
src = src.replace(old_step3, new_step3, 1)

with open("src/openVCBPreprocessing.cpp", "w", encoding="utf-8") as f:
    f.write(src)
print("Opraveno v openVCBPreprocessing.cpp")
PY
cmake --build build -j$(nproc) && python3 tests/run_tests.py && ./build/vcb2verilog -O full ~/Documents/VirtualCircuitBoard/projects/test2.vcb -o ./build/test.v && python3 - <<'PY'
import subprocess

tb = """`timescale 1ns/1ps
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
  initial begin
    for (i = 0; i < 512; i = i + 1) begin
      {a3, a2, a1, a0, b3, b2, b1, b0, cin} = i[8:0];
      #5;
      exp_sum = {1'b0, a3, a2, a1, a0} + {1'b0, b3, b2, b1, b0} + cin;
      if ({cout, s3, s2, s1, s0} !== exp_sum || cin_out !== cin) begin
        $display("FAIL i=%0d A=%b B=%b Cin=%b -> got {Cout,S}=%b%b%b%b%b (cin_out=%b), exp=%b",
                 i, {a3,a2,a1,a0}, {b3,b2,b1,b0}, cin, cout, s3, s2, s1, s0, cin_out, exp_sum);
        $finish_and_return(1);
      end
    end
    $display("PASS: Vsech 512 kombinaci 4-bitove CLA scitacky sedi na 100%%!");
    $finish;
  end
endmodule
"""
with open("/tmp/test2_tb.v", "w") as f:
    f.write(tb)
subprocess.run(["iverilog", "-o", "/tmp/test2.vvp", "./build/test.v", "/tmp/test2_tb.v"], check=True)
subprocess.run(["vvp", "/tmp/test2.vvp"], check=True)
PY
