import clib
import time
from ctypes import c_int, c_int32, c_float, POINTER

def main():
    servo_angles = [-90, -30, 30, 90]
    
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
        
    clib._servo_init()
    
    while(True):   
        print("servo set angle")
        for i in range (0, 4):
            clib._servo_set_angle(i, servo_angles[i])
            servo_angles[i] = servo_angles[(i+1)%4]
        time.sleep(1)

if __name__ == "__main__":
    main()