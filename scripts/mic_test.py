import clib
import time
from ctypes import c_double

def main():
    # init goes here
    time.sleep(0.5)
    
    path = "./audio/good_morning.pcm"
    
    clib._pcm_record(path.encode("utf-8"), c_double(5.0))
    
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