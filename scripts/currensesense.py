import clib
import time
from ctypes import c_int, c_float, byref

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
        
    ret = clib._currentsense_init()
    if ret != 0:
        print("_currentsense_init() failed")
    else:
        print("_currentsense_init() success")
    
    while(True):
        reading = c_float()
        clib._currentsense_read(byref(reading))
        print(f"Current reading: {reading.value}")
        time.sleep(1)

if __name__ == "__main__":
    main()