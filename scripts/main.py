import clib
import time
from ctypes import c_int, c_int32, c_float, POINTER

def main():
    clib._gpio_init()
    
    i2c_addr = c_int(2)
    i2c_freq = c_int32(100 * 1000)
    clib._i2c_init(i2c_addr, i2c_freq)
    
    clib._pwm_controller_init()
    clib._blinky_init()
    clib._blinky_set(4, 1)
    
    while(True):   
        clib._blinky_toggle(4)
        clib._blinky_toggle(5)
        time.sleep(0.5)

if __name__ == "__main__":
    main()