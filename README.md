# CM4_FW repository:
## Custom hardware - firmware interface for Raspberry Pi CM4

### Key features:

A complete C library and application framework for interfacing with custom hardware on the Raspberry Pi CM4. This project demonstrates professional firmware development practices including hardware abstraction, multithreaded real-time processing, and kernel driver integration.

#### Build System

This project uses a **Makefile-based build system** with dual backend support:

- **Compiler**: GCC with flags: `-Wall -Wextra -O2 -fPIC`
- **Output**: Shared object library (`.so`) compiled for either simulation or Raspberry Pi
- **Backend Selection**: 
  - `make sim` - Builds simulator backend for testing on Linux/Windows
  - `make rpi` - Builds real hardware backend for Raspberry Pi CM4
  - `make clean` - Clean build artifacts
- **Linking**: Dynamic linking with ALSA audio library (-lasound)
- **Python Interface**: Compiled C library loaded via Python's ctypes.CDLL for easy scripting

#### Libraries

**GPIO (General Purpose Input/Output)**
- Memory-mapped I/O for direct hardware register access (base: 0x7E200000)
- Register-level control: GPFSEL (mode), GPSET/GPCLR (output), GPLEV (input)
- 8 GPIO modes: INPUT, OUTPUT, ALT0-ALT5
- Edge detection (interrupt support): none/rising/falling/both
- Kernel /dev/gpiochip driver integration for configurable interrupts
- Dual implementations: `gpio.c` (real hardware) and `gpio_sim.c` (simulation)

**I2C (Inter-Integrated Circuit Bus)**
- Supports I2C Bus 1 and Bus 2
- Hardware controller base addresses: BSC1 (0x7E804000), BSC2 (0x7E205600)
- Register operations: control (I2C_C), status (I2C_S), data length (I2C_DLEN), FIFO
- Operations: read, write, read-then-write with error handling
- Configurable I2C clock frequency
- Status codes for transfer active (I2C_TA), done, error, and timeout detection
- Dual implementations: `i2c.c` (real hardware) and `i2c_sim.c` (simulation)

**I2S Audio (Integrated Interchip Sound)**
- Multithreaded architecture with dedicated record and playback threads
- **Recording**: Microphone thread captures continuously to 1MB circular buffer (2^20 bytes, ~10.9s stereo)
- **Playback**: Speaker thread outputs from .pcm file or raw byte stream
- **Audio Format**: S16_LE (16-bit signed little-endian), 48kHz sample rate
  - Playback: Mono (1 channel)
  - Recording: Stereo (2 channels)
- **Hardware**:
  - Speaker: MAX98357A Class-D amplifier
  - Microphone: SPH0645LM4H dual-channel MEMS mic
  - Device nodes: plughw:0,0 (mic) / plughw:0,1 (speaker)
- Driver: Advanced Linux Sound Architecture (ALSA)
- Functions: `i2s_init()`, `i2s_deinit()`, `i2s_start_recording()`, `i2s_record_to_file()`, `i2s_play_file()`, `i2s_play_raw()`, `i2s_rb_pop()`

**PWM Controller (PCA9685PW)**
- 16-channel PWM output via I2C (address: 0x47, frequency: 100kHz)
- PWM resolution: 4096 steps per channel
- Base clock: 25MHz (configurable via prescaler)
- Register architecture: 
  - MODE1/MODE2: Control (sleep, reset, inversion, etc.)
  - LED0-LED15: Individual ON/OFF time registers (4 bytes each)
  - ALL_LED: Broadcast control for all 16 channels
  - PRESCALER: Frequency configuration
- Status checks and reinitialization support

#### Projects

**LED Blinky (PWM Control)**
- Dual-LED controller on PCA9685 channels 4-5
- Modes: OFF, ON, PWM (brightness 0.0-1.0)
- Functions: `blinky_init()`, `blinky_set()`, `blinky_set_pwm()`
- Uses PCA9685 PWM driver with I2C interface

**Servo Motor Control (4-channel)**
- 4 independent servo channels (0-3) operating at 50Hz (20ms period)
- Angular range: -90° to +90°
- Pulse widths: 1.0ms to 2.0ms (standard hobby servo)
- Motion features:
  - Raw angle setting: `servo_set_angle(channel, angle)`
  - Smooth timed moves with configurable speed (max 360°/s default)
  - Control thread at 1kHz (1ms resolution)
  - Per-servo state: current angle, target angle, motion step
  - Thread-safe operation with background motion control
- Implementation wraps the PCA9685 PWM controller

**Current Sense (INA219)**
- I2C address: 0x41
- Real-time current and power monitoring
- Configuration:
  - Shunt resistor: 10mΩ
  - Max current: 6A (LSB ≈ 183μA at 32768 ADC steps)
  - Calibration register for accurate measurements
- Measurement registers: shunt voltage, bus voltage, power, current
- Functions: `currentsense_init()`, `currentsense_read()`
- Application: System power and load monitoring

**IR LED Heartbeat Sensor (MAX30105)**
- I2C address: 0x57
- Optical sensor with integrated IR LED and photodetector
- **Multithreaded Architecture**:
  - **Input thread**: Continuously reads sensor FIFO to circular buffer
  - **Processing thread**: Real-time signal processing and detection
  - Sleep/active modes for CPU efficiency
- **Sampling**: Configurable FIFO averaging (×4, ×8 samples)
- **Signal Processing**:
  - Exponential Moving Average (EMA) filtering
  - Band-pass filter (BPF) for heartbeat frequency isolation
  - Smoothing and noise reduction algorithms ([calculation details](https://docs.google.com/spreadsheets/d/1XDxm2utaFQ-OXD1oPfhDBG_OyDTtbuzdnD-7CK9p-SM/edit?usp=sharing))
  - Thresholding for beat peak detection
- Output: Extracted BPM and beat detection events

**Unimplemented Projects (Hardware Prepared)**
- MPU6050: 6-axis accelerometer/gyroscope (I2C)
- MIPI-CSI 2: Video interface for Raspberry Pi camera module
- Battery management: State-of-charge estimation from INA219 + voltage

#### Scripts

Test and utility Python scripts using ctypes bindings to C library:

- **blinky_toggle.py**: Simple LED on/off toggle
- **blinky_pulse.py**: PWM fade effect with continuous brightness modulation
- **servo_test_setpoint.py**: Servo control demo cycling through 4 preset angles (-90°, -30°, +30°, +90°)
- **currentsense.py**: Continuous current monitoring with logging
- **irled.py**: Heartbeat sensor basic data collection
- **irled_py_processing.py**: Advanced heartbeat signal processing in Python
- **mic_test.py**: Microphone input capture and diagnostics
- **speaker_test_file.py**: Audio file (.pcm) playback
- **speaker_test_raw.py**: Direct raw audio data playback
- **audio_interface.py**: Abstraction layer for audio operations
- **boot.py**: Startup script (auto-runs on system boot)
- **template.py**: Base template for new test scripts
- **clib.py**: Core ctypes wrapper providing bindings for all C library functions

#### Additional Features

- **Code Formatting**: Uncrustify configuration for consistent style
- **Device Tree Overlays**: Custom .dts overlay for audio hardware configuration
- **Error Handling**: Comprehensive StatusCode enum for error reporting
- **Verbosity Levels**: Debug output control (NONE, LEVEL_1, LEVEL_2, LEVEL_3)
- **Ring Buffers**: Circular buffers for I2S audio and IR sensor real-time streaming


# Building and Running

### Build System Targets

```bash
make sim           # Build simulator backend (Linux/Windows testing)
make rpi           # Build real hardware backend for Raspberry Pi CM4
make clean         # Remove all build artifacts
```

The Makefile automatically selects object files based on backend:
- **sim**: GPIO simulated ops, I2C simulated, no I2S
- **rpi**: Hardware GPIO via memory-mapped I/O, kernel I2C driver, ALSA audio

Output: `build/{backend}/lib.so`

### Running Scripts

All Python test scripts require:
1. C library compiled: `make sim` or `make rpi`
2. Python 3 with ctypes (standard library)
3. For RPi: Kernel modules for I2C and GPIO loaded

Example:
```bash
python3 scripts/blinky_pulse.py
python3 scripts/servo_test_setpoint.py
python3 scripts/currentsense.py
```

# Raspberry Pi CM4 Setup Instructions

### System Dependencies

Install required development libraries:

```bash
sudo apt install libasound2-dev
```

This installs ALSA (Advanced Linux Sound Architecture) headers and libraries needed for audio I/O.

### Kernel Configuration

Modify `/boot/firmware/config.txt` to enable hardware interfaces:

```bash
sudo nano /boot/firmware/config.txt
```

Add or uncomment these lines:

```
dtoverlay=i2c-gpio,bus=3,i2c_gpio_sda=4,i2c_gpio_scl=5
dtparam=i2c_arm=on
dtparam=i2s=on
```

This enables:
- I2C communication (GPIO bitbanged on pins 4/5 as bus 3, I2C hardware buses)
- I2S audio interface for microphone and speaker

### Audio Hardware Overlay Compilation

Create and compile the custom device tree overlay for audio peripherals:

Create overlay directory:

```bash
mkdir ~/dtoverlays
```

Copy or download the overlay definition:

```bash
scp dts_overlays/sph0645-max98357a-overlay.dts user@raspberrypi:/home/user/dtoverlays
# or download directly on RPi
```

Compile device tree binary blob:

```bash
dtc -@ -I dts -O dtb -o sph0645-max98357a.dtbo dtoverlays/sph0645-max98357a-overlay.dts
```

Install to system overlays:

```bash
sudo cp sph0645-max98357a.dtbo /boot/firmware/overlays/
```

Enable the overlay in config.txt:

```bash
sudo nano /boot/firmware/config.txt
# Add line:
dtoverlay=cm4-sph0645-max98357a
```

Reboot for kernel changes to take effect:

```bash
sudo reboot
```

### Building for Raspberry Pi

On the Raspberry Pi or cross-compile environment:

```bash
make rpi
```

This produces `build/rpi/lib.so` with hardware drivers enabled.

### Pin Configuration Reference

- **I2C Bus 3**: GPIO 4 (SDA), GPIO 5 (SCL) - Software bitbanged I2C
- **I2C Hardware**: Buses 1 & 2 - Native I2C controllers
- **I2S Audio**: Standard I2S pins for MAX98357A speaker and SPH0645LM4H microphone
- **GPIO**: All GPIO pins available via memory-mapped register access

# Hardware Interface Reference

## I2C Device Addresses

| Device | Address | Function | Protocol |
|--------|---------|----------|----------|
| PCA9685 | 0x47 | 16-channel PWM controller | I2C @ 100kHz |
| INA219 | 0x41 | Current sense monitor | I2C |
| MAX30105 | 0x57 | IR LED heartbeat sensor | I2C |
| MPU6050 | TBD | Accelerometer/Gyroscope | I2C (unimplemented) |

## GPIO Register Memory Map

All GPIO registers are accessed via memory-mapped I/O at base address **0x7E200000** (physical address).

Key register offsets:
- **GPFSEL** (0x00-0x05): GPIO function select (mode configuration)
- **GPSET** (0x1C, 0x20): GPIO set output high
- **GPCLR** (0x28, 0x2C): GPIO clear output low
- **GPLEV** (0x34, 0x38): GPIO level read (input status)
- **GPEDS** (0x40, 0x44): GPIO event detect status
- **GPREN** (0x4C, 0x50): GPIO rising edge detect enable
- **GPFEN** (0x58, 0x5C): GPIO falling edge detect enable

## I2C Controllers

| I2C Bus | Base Address | Clock Source | Speed |
|---------|--------------|--------------|-------|
| Bus 1 (BSC1) | 0x7E804000 | 150MHz (typical) | Configurable |
| Bus 2 (BSC2) | 0x7E205600 | 150MHz (typical) | Configurable |
| Bus 3 (GPIO Bit-bang) | GPIO 4/5 | Software | 100kHz default |

Key BSC register offsets:
- **BSC_C** (0x00): Control register
- **BSC_S** (0x04): Status register
- **BSC_DLEN** (0x08): Data length
- **BSC_A** (0x0C): Slave address
- **BSC_FIFO** (0x10): Data FIFO
- **BSC_DIV** (0x14): Clock divider
- **BSC_DEL** (0x18): Delay settings
- **BSC_CLKT** (0x1C): Clock timeout

## Digital Audio Interface (I2S/ALSA)

- **Sample Format**: S16_LE (16-bit signed, little-endian)
- **Sample Rate**: 48kHz
- **Playback**: Mono (1 channel)
- **Recording**: Stereo (2 channels)
- **Ring Buffer Size**: 1MB (2^20 bytes) = ~10.9 seconds stereo @ 48kHz
- **Read/Wait Period**: 100ms (monitoring interval)

# Architecture and Design

## Dual Backend Architecture

The project supports two build configurations:

### Simulator Backend (`make sim`)
- For development and testing on Windows/Linux
- Uses simulated GPIO, I2C, and audio operations
- No actual hardware required
- Useful for testing logic without RPi

### Hardware Backend (`make rpi`)
- Full hardware driver implementations
- Memory-mapped GPIO access
- Kernel I2C driver integration (/dev/i2c)
- ALSA audio subsystem for microphone/speaker I/O

## Threading Model

- **I2S Audio**: Independent record and playback threads reading/writing circular buffers
- **Servo Control**: 1kHz control thread for smooth motion interpolation
- **IR Sensor**: Separate input and processing threads with circular buffer
- **Lock-free Design**: Ring buffers used for inter-thread communication (minimal synchronization)

## Status Code Reporting

All library functions return `StatusCode` enum for error handling:

```c
typedef enum {
  STATUS_CODE_OK                  = 0,
  STATUS_CODE_INVALID_ARGS        = -1,
  STATUS_CODE_NOT_INITIALIZED     = -2,
  STATUS_CODE_ALREADY_INITIALIZED = -3,
  STATUS_CODE_TIMEOUT             = -4,
  STATUS_CODE_UNIMPLEMENTED       = -5,
  STATUS_CODE_THREAD_FAILURE      = -6,
  STATUS_CODE_MEM_ACCESS_FAILURE  = -7,
  STATUS_CODE_OUT_OF_MEMORY       = -8,
  STATUS_CODE_FAILED              = -9,
} StatusCode;
```

# Testing the System

## Quick Verification

After successful build and setup:

1. **Verify Compilation**:
   ```bash
   make rpi          # Should produce build/rpi/lib.so without errors
   ```

2. **Test Infrastructure**:
   ```bash
   python3 scripts/clib.py    # Verify C library loads
   ```

3. **Test Individual Modules**:
   ```bash
   python3 scripts/blinky_toggle.py         # GPIO & PWM test
   python3 scripts/servo_test_setpoint.py   # I2C & servo control test
   python3 scripts/currentsense.py          # INA219 current monitoring
   ```

4. **Audio Test** (requires hardware):
   ```bash
   python3 scripts/mic_test.py              # Microphone capture
   python3 scripts/speaker_test_file.py     # Speaker playback
   ```

## Signal Processing Tests

The heartbeat sensor scripts include signal processing validation:

```bash
python3 scripts/irled.py                    # Raw sensor data
python3 scripts/irled_py_processing.py      # Processed heartbeat detection
```
