import clib
import time
import os
from ctypes import c_int, c_float, byref

def main():
    # init goes here
    
    clib._gpio_regs_init()
    clib._gpio_set_mode(16, 1)
    clib._gpio_write(16, 1)
    time.sleep(0.5)
    
    _here = os.path.dirname(os.path.abspath(__file__))

    _aud_path = os.path.join(_here, "audio", "kick-drum.pcm")
        
    if not os.path.isfile(_aud_path):
        raise FileNotFoundError(_aud_path)
    
    with open(_aud_path, "rb") as f:
        audio = bytearray(f.read())
    
    clib._i2s_init()
    clib.i2s_play_raw(audio)
        
    try:
        while(True):
            time.sleep(0.1)
    except KeyboardInterrupt:
        pass
    finally:
        finish()

def finish():
    # finish sequence goes here
    clib._i2s_deinit()
    clib._gpio_write(16, 0)
    return

if __name__ == "__main__":
    main()