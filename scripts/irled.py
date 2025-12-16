import clib
import time
from ctypes import c_int, byref
import signal

def main():
    
    ret = clib._gpio_regs_init()
    if ret != 0:
        print("_gpio_init() failed")
    else:
        print("_gpio_init() success")
        
    
    i2c_addr = c_int(2)
    ret = clib._i2c_init(i2c_addr)
    if ret != 0:
        print("_i2c_init() failed")
    else:
        print("_i2c_init() success")
        
    ret = clib._irled_init()
    if ret != 0:
        print("_irled_init() failed")
    else:
        print("_irled_init() success")
        
    ret = clib._irled_start_reading()
    if ret != 0:
        print("_irled_start_reading() failed")
    else:
        print("_irled_start_reading() success")

    sample = clib.Max30102Sample()
    try:
        while True:
            ret = 0
            while ret == 0:
                ret = clib._irled_pop_sample(byref(sample))
                print(f"ir: {sample.ir} red: {sample.red}")
            print("batch popped")
            time.sleep(0.075)
    except KeyboardInterrupt:
        pass
    finally:
        clib._irled_stop_reading()
        clib._irled_deinit()

if __name__ == "__main__":
    main()