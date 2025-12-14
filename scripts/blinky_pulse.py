import clib
import time
from ctypes import c_int, c_int32, c_float, POINTER

def main():
    led4_freq = 1.0
    led5_freq = 0
    led4_direction = 1
    led5_direction = 0
    
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
        
    
    pwm_freq = c_int(50)
    ret = clib._pwm_controller_init(pwm_freq)
    if ret != 0:
        print("_pwm_controller_init() failed")
    else:
        print("_pwm_controller_init() success")
        
    clib._blinky_init()
    clib._blinky_set(4, 1)
    
    while(True):   
        print("blinky pulse")
        clib._blinky_set_pwm(4, c_float(led4_freq))
        clib._blinky_set_pwm(5, c_float(led5_freq))
        
        if led4_freq >= 1:
            led4_direction = 1
        elif led4_freq <= 0:
            led4_direction = 0
            
        if led4_direction == 1:
            led4_freq -= 0.05
        elif led4_direction == 0:
            led4_freq += 0.05
            
            
        if led4_freq >= 1:
            led5_direction = 1
        elif led4_freq <= 0:
            led5_direction = 0
            
        if led5_direction == 1:
            led4_freq -= 0.05
        elif led5_direction == 0:
            led4_freq += 0.05
        time.sleep(0.1)

if __name__ == "__main__":
    main()