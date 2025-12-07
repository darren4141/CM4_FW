import ctypes
import os
import time
from ctypes import c_int

lib = ctypes.CDLL(os.path.join("build", "lib.so"))

_blinky_step = lib.blinky_step

_blinky_step.argtypes = []
_blinky_step.restype = c_int

if __name__ == "__main__":
    
    for i in range(10):
        state = _blinky_step()
        print(f"[Python] step {i}: LED state = {state}")
        time.sleep(0.5)
        