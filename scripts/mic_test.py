import clib
import time
import os
from ctypes import c_double, c_uint8

def main():
    # init goes here
    time.sleep(0.5)
    
    _here = os.path.dirname(os.path.abspath(__file__))

    _aud_path = os.path.join(_here, "recordings")
        
    if not os.path.isdir(_aud_path):
        raise FileNotFoundError(_aud_path)
    
    clib._pcm_init()
    clib._pcm_start_recording()
        
    _cbuf = (c_uint8 * (1024 * 2))()
        
    try:
        while(True):
            n = clib._pcm_rb_pop(_cbuf, len(_cbuf))
            if n > 0:
                print("CHUNK")
                print(_cbuf[:n])
            time.sleep(0.01)
    except KeyboardInterrupt:
        pass
    finally:
        finish()

def finish():
    # finish sequence goes here
    clib._pcm_deinit()
    return

if __name__ == "__main__":
    main()