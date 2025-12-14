import clib
import time
from ctypes import c_int, c_int32, c_float, POINTER

def main():
    
    ret = clib._gpio_regs_init()
    if ret != 0:
        print("_gpio_init() failed")
    else:
        print("_gpio_init() success")
        
    
    i2c_addr = c_int(2)
    i2c_freq = c_int32(100 * 1000)
    clib._i2c_init(i2c_addr, i2c_freq)
    if ret != 0:
        print("_i2c_init() failed")
    else:
        print("_i2c_init() success")
        
    
    pwm_freq = c_int(50)
    clib._pwm_controller_init(pwm_freq)
    if ret != 0:
        print("_pwm_controller_init() failed")
    else:
        print("_pwm_controller_init() success")
        
    clib._blinky_init()
    clib._blinky_set(4, 1)
    
    while(True):   
        clib._blinky_toggle(4)
        clib._blinky_toggle(5)
        time.sleep(0.5)

if __name__ == "__main__":
    main()