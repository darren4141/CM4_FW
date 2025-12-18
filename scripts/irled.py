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
        idx = 0
        ampl_list = [90, 90, 90, 90, 90]
        running_avg = []
        
        
        while True:
            ret = 0
            val_list = []
            min_val = 10000000
            ret = clib._irled_pop_sample(byref(sample))
            if ret == 0:
                counter += 1
            
            while ret == 0:
                val_list.append(sample.ir)
                if sample.ir < min_val:
                    min_val = sample.ir
                ret = clib._irled_pop_sample(byref(sample))
                        
            # print("batch popped")
            hb = False
            val_num = 0
            for val in val_list:
                val_num += 1
                if min_val > 10000 and val - min_val >= 100 and val - min_val < 200:
                    if counter < 7:
                        continue
                    hb = True
                    counter += (val_num/10)
                    bpm = 60 / (counter * 0.075)
                    
                    running_avg.append(bpm)
                    if len(running_avg) > 5:
                        running_avg.pop(0)
                    bpm_avg = 0
                    for item in running_avg:
                        bpm_avg += item
                    
                    bpm_avg /= len(running_avg)
                    print("beat", val-min_val, ampl_list, val, counter, bpm, bpm_avg)
                    counter = 0
                    ampl_list[idx] = val - min_val
                    idx += 1
                    idx %= 5
                    
                    continue
            if hb == False:
                print("...")
            time.sleep(0.075)
    except KeyboardInterrupt:
        pass
    finally:
        clib._irled_stop_reading()
        clib._irled_deinit()

if __name__ == "__main__":
    main()