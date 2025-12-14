import clib
import time
from ctypes import c_int, c_int32, byref, Structure

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
    
    while(True):
        reading = clib.Max30102Sample()
        clib._irled_pop_sample(byref(reading))
        print(f"ir: {reading.ir} led: {reading.red}")
        time.sleep(0.1)

if __name__ == "__main__":
    main()