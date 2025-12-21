import ctypes
import os
from ctypes import c_int, c_int32, c_float, c_double, c_char_p, POINTER
import platform

_here = os.path.dirname(os.path.abspath(__file__))

if platform.system() == "Windows":
    print("Resolving ../build/sim/lib.so")
    _lib_path = os.path.join(_here, "..", "build", "sim", "lib.so")
elif platform.system() == "Linux":
    print("Resolving ../build/rpi/lib.so")
    _lib_path = os.path.join(_here, "..", "build", "rpi", "lib.so")
else:
    print("Unknown OS")

print("Loading .so at:", _lib_path)

lib = ctypes.CDLL(_lib_path)

_gpio_regs_init = lib.gpio_regs_init
_gpio_regs_init.argtypes = []
_gpio_regs_init.restype = c_int

_gpio_set_mode = lib.gpio_set_mode
_gpio_set_mode.argtypes = [c_int, c_int]
_gpio_set_mode.restype = c_int

_gpio_write = lib.gpio_write
_gpio_write.argtypes = [c_int, c_int]
_gpio_write.restype = c_int

_i2c_init = lib.i2c_init
_i2c_init.argtypes = [c_int]
_i2c_init.restype = c_int

_i2c_deinit = lib.i2c_deinit
_i2c_deinit.argtypes = [c_int]
_i2c_deinit.restype = c_int

_pwm_controller_init = lib.pwm_controller_init
_pwm_controller_init.argtypes = [c_int32]
_pwm_controller_init.restype = c_int

_pwm_controller_deinit = lib.pwm_controller_deinit
_pwm_controller_deinit.argtypes = []
_pwm_controller_deinit.restype = c_int

_blinky_init = lib.blinky_init
_blinky_init.argtypes = []
_blinky_init.restype = c_int

_blinky_set = lib.blinky_set
_blinky_set.argtypes = [c_int, c_int]
_blinky_set.restype = c_int

_blinky_toggle = lib.blinky_toggle
_blinky_toggle.argtypes = [c_int]
_blinky_toggle.restype = c_int

_blinky_set_pwm = lib.blinky_set_pwm
_blinky_set_pwm.argtypes = [c_int, c_float]
_blinky_set_pwm.restype = c_int

_servo_init = lib.servo_init
_servo_init.argtypes = []
_servo_init.restype = c_int

_servo_deinit = lib.servo_deinit
_servo_deinit.argtypes = []
_servo_deinit.restype = c_int

_servo_get = lib.servo_get
_servo_get.argtypes = [c_int, POINTER(c_float)]
_servo_get.restype = c_int

_servo_set_angle = lib.servo_set_angle
_servo_set_angle.argtypes = [c_int, c_float]
_servo_set_angle.restype = c_int

_servo_move_smooth = lib.servo_move_smooth
_servo_move_smooth.argtypes = [c_int, c_float, c_float]
_servo_move_smooth.restype = c_int

_irled_init = lib.irled_init
_irled_init.argtypes = []
_irled_init.restype = c_int

_irled_deinit = lib.irled_deinit
_irled_deinit.argtypes = []
_irled_deinit.restype = c_int

_irled_start_reading = lib.irled_start_reading
_irled_start_reading.argtypes = []
_irled_start_reading.restype = c_int

_irled_stop_reading = lib.irled_stop_reading
_irled_stop_reading.argtypes = []
_irled_stop_reading.restype = c_int

class Max30102Sample(ctypes.Structure):
    _fields_ = [
        ("ir", ctypes.c_int32),
        ("red", ctypes.c_int32)
    ]

_irled_pop_sample = lib.irled_pop_sample
_irled_pop_sample.argtypes = [POINTER(Max30102Sample)]
_irled_pop_sample.restype = c_int

_currentsense_init = lib.currentsense_init
_currentsense_init.argtypes = []
_currentsense_init.restype = c_int

_currentsense_read = lib.currentsense_read
_currentsense_read.argtypes = [POINTER(c_float)]
_currentsense_read.restype = c_int


_pcm_record = lib.pcm_record
_pcm_record.argtypes = [c_char_p, c_double]
_pcm_record.restype = c_int

_pcm_play = lib.pcm_play
_pcm_play.argtypes = [c_char_p]
_pcm_play.restype = c_int