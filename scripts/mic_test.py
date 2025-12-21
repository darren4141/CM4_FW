import clib
import time
import os
from ctypes import c_double

def main():
    # init goes here
    time.sleep(0.5)
    
    _here = os.path.dirname(os.path.abspath(__file__))

    _aud_path = os.path.join(_here, "recordings")
        
    if not os.path.isdir(_aud_path):
        raise FileNotFoundError(_aud_path)
    
    clib._pcm_record(os.fsencode(_aud_path), c_double(5.0))
    
    print("done recording")
    
    try:
        while(True):
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        finish()

def finish():
    # finish sequence goes here
    return

if __name__ == "__main__":
    main()