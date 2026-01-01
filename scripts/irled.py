import clib
import time
from ctypes import c_int, byref
import signal

def avg_list(list_in: list):
    sum = 0
    for item in list_in:
        sum += item
    return (sum / 5) - 40

def main():
    counter = -0.075

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
    prev_bpm = 0
    try:
        while True:
            if clib._irled_get_hb_state() == 1:
                clib._blinky_toggle(4)
                clib._irled_clear_hb_state()
            time.sleep(0.05)
    except KeyboardInterrupt:
        pass
    finally:
        finish()

def finish():
    clib._irled_stop_reading()
    clib._irled_deinit()
    clib._i2c_deinit(2)

if __name__ == "__main__":
    main()