import clib
import time
from ctypes import c_int, c_float, byref

def main():
    # init goes here
    
    clib._gpio_regs_init()
    clib._gpio_set_mode(16, 1)
    clib._gpio_write(16, 1)
    time.sleep(0.5)
    
    path = "./audio/good_morning.pcm"
    
    clib._pcm_play(path.encode("utf-8"))
    
    try:
        while(True):
            time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        finish()

def finish():
    # finish sequence goes here
    clib._gpio_write(16, 0)
    return

if __name__ == "__main__":
    main()