#!/usr/bin/env python3
import subprocess, sys, os

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(REPO, "build", "vcb2verilog")

def run(cmd):
    return subprocess.run(cmd, capture_output=True, text=True)

# Basic smoke test: counter example
r = run([BIN, os.path.join(REPO, "examples", "counter_2bit.vcb"), "-o", "/tmp/scratchpad/ct.v"])
if r.returncode != 0:
    print("[FAIL] counter build")
    sys.exit(1)
print("[PASS] counter build")
sys.exit(0)
