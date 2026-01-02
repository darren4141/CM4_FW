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
    
    clib._i2s_init()
    clib._i2s_start_recording()
        
    _cbuf = (c_uint8 * (1024 * 2))()
        
    try:
        with open("test_record.pcm", "wb") as f:
            while True:
                n = clib._i2s_rb_pop(_cbuf, len(_cbuf))
                if n > 0:
                    f.write(bytes(_cbuf[:n]))
                else:
                    time.sleep(0.001)
    except KeyboardInterrupt:
        pass
    finally:
        clib._i2s_deinit()

    print("Wrote:")

def finish(): 
    # finish sequence goes here
    clib._i2s_deinit()
    return

if __name__ == "__main__":
    main()