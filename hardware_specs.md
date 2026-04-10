# Hardware Specifications

## MCU: Raspberry Pi Compute Module 4

- **Processor**: ARM Cortex-A72 (BCM2835)
- **Documentation**:
  - [CM4 Datasheet](https://datasheets.raspberrypi.com/cm4/cm4-datasheet.pdf)
  - [BCM2835 ARM Datasheet](https://datasheets.broadcom.com/datasheets/28301065-8612-4ba8-9b5a-0727dc57c0c2.pdf)
  - [Detailed Pinout](https://datasheets.raspberrypi.com/cm4/cm4-pinout.pdf)
- **System Load**:
  - Peak current: 5V, 1.2A
  - Standard current: 5V, 500mA
- **Available Interfaces**:
  - I2C channels: 5
  - I2S audio bus: 1
  - Camera interface: MIPI-CSI 2
  - GPIO pins: Full access via memory-mapped I/O

---

## Power Management

### Overview
- **Total system max current draw**: 6A from battery
- **Power rails**: 5V main bus, 3.3V secondary bus
- **Architecture**: Two independent buck converters + LDO for redundancy

### Buck Converter #1: MP2338
- **Input**: Battery (7.2V nominal, 8.4V max)
- **Output**: 5V @ 3A max
- **Purpose**: Pi power and camera supply
- **Total load**: ~2.5A (Pi + camera)
- **Enable pin**: Requires ≥1.3V (max 6V)
- **Enable jumper**: Emergency; provides stable 5V bypass if Buck #1 fails

### Buck Converter #2: MPM3650
- **Input**: Battery (7.2V nominal)
- **Output**: 5V @ 5A max
- **Recommended minimum input**: 5.7V (limits duty cycle to 0.88)
- **Purpose**: Servo motors, speaker, peripherals
- **Total load (nominal)**: Up to 3.5A
- **Max theoretical load**: 4.1A (if LDO maxed out)

### LDO: AP2112K-3.3
- **Input**: 5V from buck converters
- **Output**: 3.3V @ 600mA max
- **Current load and headroom**: 165.8mA total draw with 434mA available
- **Purpose**: PWM controller, heartbeat sensors, microphone, MPU6050

---

## Battery System

### Batteries: 18650 Rechargeable

- **Cell Specification**: 2500mAh per cell
- **Peak supply current**: 8A
- **Continuous supply current**: 5A
- **Battery voltage**: 3.7V nominal per cell
- **Configuration**: 2S (series) = 7.4V nominal, 8.4V max
- **Total capacity**: 5000mAh (18.5Wh)

### Battery Protection Circuit
- **Topology**: AO4421 P-channel FET + diode + resistor protection
- **Function**: Protects against reverse polarity and over-discharge
- **Parasitic loss**: Minimal

### Battery Monitor (Comparator)
- **IC**: TLV7031DBV analog comparator
- **Input**:  Battery voltage divider
- **Output**: BATMON GPIO pin
- **Behavior**: Pulls GPIO low when battery voltage falls below threshold
- **Power supply**: 5V
- **Current draw**: ~0A (negligible)

### Battery Recharger
- **IC**: TP4056 lithium charging module
- **Input**: USB Type-C, 5V power-only (no data lines)
- **Charging current**: Configured via external resistor
- **Protection**: Built-in overcharge and over-current protection

---

## Current Sense

### Measurement Points
- **Location**: Main battery line
- **Shunt resistor**: 0805 0.01Ω 1W (0.36W power dissipation)
- **Measurement range**: 0-6A
- **Resolution**: 183μA per LSB (6A / 32768 steps)

### INA219 Module
- **Part**: INA219 current/power monitor
- **I2C address**: 0x45 (configurable, hardwired to this value)
- **Connection**: I2C2 bus
- **Maximum shunt voltage**: 320mV (at 32A for margin)
- **Current draw**: 1.5mA
- **Measurement registers**:
  - Shunt voltage (raw voltage across 0.01Ω resistor)
  - Bus voltage (battery line voltage)
  - Current (calculated from shunt voltage)
  - Power (bus voltage × current)
- **Calibration**: Configured for accurate 6A range readings

---

## System Peripheral Map

### I2C Bus Assignments

| Component | I2C Address | Bus | Access Type | Notes |
|-----------|-------------|-----|-------------|-------|
| Camera (OmniVision sensor) | - | CAM_I2C | Polling | 22-pin FPC connector |
| Heartbeat sensor 1 | 0x57 | I2C1 | Interrupt | MAX30105 optical |
| Heartbeat sensor 2 | 0x57 | I2C2 | Interrupt | MAX30105 optical |
| MPU6050 Gyro | 0x68 or 0x69 | I2C1 | Polling | Configurable via AD0 pin |
| PWM Driver | 0x47 | I2C2 | Write-only | PCA9685PW servo controller |
| Current Sense | 0x45 | I2C2 | Polling | INA219 on battery line |

### Bus Architecture
- **I2C1**: Heartbeat sensor 1, MPU6050
- **I2C2**: Heartbeat sensor 2, PWM driver, current sense
- **CAM_I2C**: Camera (dedicated CSI-2 camera interface)

---

## Servo Motors (4-channel)

### Servo Driver: PCA9685PW
- **Part**: 16-channel PWM LED driver
- **Output frequency**: 50Hz (20ms period, standard servo)
- **PWM resolution**: 4096 steps per channel
- **I2C address**: 0x47
- **I2C frequency**: 100kHz
- **Address configuration**: A0-A5 pins allow up to 64 unique addresses
- **Power supply**: 5V
- **Current draw**:
  - Quiescent: 10mA
  - Per active channel: 25mA nominal
  - Total @ 4 channels: ~110-135mA
- **Output channels used**: 4 (any of the 16 available)

### SG90 Servo Motors (x4)
- **Operating voltage**: 5V
- **Angular range**: 0-180° (standard hobby servo)
- **Maximum torque**: 1.8 kgf/cm
- **Speed**: 0.1s / 60° (at rated voltage)
- **Current per servo**:
  - Operating: 750mA
  - Combined (4x): 3A total
- **Control signal**: PWM 1-2ms pulse at 50Hz

---

## Camera: Raspberry Pi 4 Camera Module

- **Connector**: 22-pin FPC (Flexible Printed Circuit)
- **Resolution**: 8MP (3280×2464)
- **I2C interface**: Uses dedicated CAM_I2C bus (polling mode)
- **Power supply**: 3.3V
- **Current draw**: 250mA during operation
- **Sensor**: OmniVision OV5647
- **CSI-2 differential pairs**: 50Ω impedance
- **PCB stackup**: JLC04161H-7628 (standard Raspberry Pi spec for camera signals)
- **Frame rate**: Up to 30fps @ full resolution

---

## Heartbeat Sensors (MAX30105 Optical)

### Specifications
- **Part number**: MAX30105 (Note: initial design considered MAX30102, upgraded to MAX30105)
- **Sensor type**: Integrated IR LED + photodetector
- **I2C address**: 0x57 (fixed, non-configurable)
- **I2C bus assignment**:
  - Sensor 1: I2C1 with interrupt-driven reading
  - Sensor 2: I2C2 with interrupt-driven reading
- **Operating voltage**: 3.3V
- **Current draw**: 600μA per sensor (1.2mA combined)
- **Interrupt pin**: Pulled high, asserted low on data ready
- **FIFO buffer**: Sample averaging configurable (×4, ×8 samples)
- **Rollover mode**: Circular buffer operation

### Signal Processing Pipeline
- **EMA filtering**: Exponential moving average for noise reduction
- **Band-pass filter (BPF)**: Isolates heartbeat frequency (typically 0.5-4 Hz)
- **Smoothing**: Temporal filtering to reduce motion artifacts
- **Thresholding**: Peak detection to extract beat events
- **Output**: Calculated BPM and beat detection timestamps

---

## Inertial Measurement Unit (IMU): MPU6050

- **Part**: MPU6050 6-axis IMU
- **Sensor suite**:
  - 3-axis accelerometer (±2g, ±4g, ±8g, ±16g selectable)
  - 3-axis gyroscope (±250°/s, ±500°/s, ±1000°/s, ±2000°/s selectable)
- **I2C address**: 0x68 or 0x69 (selectable via AD0 pin)
- **I2C bus**: I2C1
- **Access mode**: Polling
- **Operating voltage**: 3.3V
- **Current draw**: ~4mA
- **Output**: 16-bit resolution for all axes
- **Reliability**: Provides reliable pitch and roll measurements

---

## Audio Subsystem

### Microphone: SPH0645LM4H
- **Type**: MEMS microphone (dual-channel array)
- **Digital output**: I2S interface
- **Channel selection** (via SEL pin):
  - SEL to GND: Outputs left channel
  - SEL to 3.3V (VDD): Outputs right channel
- **Operating voltage**: 3.3V
- **Current draw**: 600μA
- **Output impedance**: ~2.7kΩ
- **Sensitivity**: -26 dBFS (A-weighted)
- **Frequency response**: 100Hz - 20kHz

### Speaker Amplifier: MAX98357A
- **Type**: Class-D audio amplifier (mono output)
- **Operating voltage**: 5V
- **Current draw**: 3mA idle
- **Output power**: Up to 3.125W @ 5V into 4Ω load
- **Gain selection** (via GAIN pin):
  - GAIN to GND: +3dB
  - GAIN to NC (not connected): +9dB
  - GAIN to VDD (5V): +15dB
- **Shutdown control** (SD pin):
  - SD to GND: Low power mode / off
  - SD to VDD (5V): Power on (active)
- **Digital input**: I2S interface

### I2S Audio Bus
- **GPIO pins used**: 18, 19, 20, 21 (BCM2835 standard I2S pins)
- **Signal lines**:
  - GPIO18: BCLK (bit clock, generated by CM4)
  - GPIO19: LRCLK (left/right clock, generated by CM4)
  - GPIO20: DOUT (data to speaker)
  - GPIO21: DIN (data from microphone)
- **Operating voltage**: 3.3V (GPIO output), 5V (power for MAX98357A)
- **Data format**: I2S standard (MSB-first, LSB justified)

---

## Connectors and Peripherals

### Connector Standard
- **Type**: Hirose DF13 line (1.25mm pitch)
- **Sources**:
  - [Socket connectors (Digi-Key)](https://www.digikey.com/en/products/detail/hirose-electric-co-ltd/DF13-4P-1.25DSA/530253)
  - [Plug connectors (Digi-Key)](https://www.digikey.com/en/products/detail/hirose-electric-co-ltd/DF13-4P-1.25H/530266)

### ESD Protection
- **Devices**: All plug-in peripherals
- **Protection**: Bottom-layer ESD zener diodes (ESD9B5.0ST5G)
- **Configuration**: Clamp to 3.3V rail for 3.3V signals, isolated for 5V signals

---

## Current Draw Breakdown

| Component | Operating Voltage | Max Current | Bus | Notes |
|-----------|-------------------|-------------|-----|-------|
| **Compute Module + Camera** | 5V | 3A | 5V main | Pi + OV5647 sensor |
| **Servos ×4** | 5V | 750mA each, 3A total | 5V main | SG90 servo motors |
| **Speaker** | 5V | 500mA | 5V main | MAX98357A amplifier |
| **3.3V LDO** | 5V in / 3.3V out | 600mA | 5V main | Supply for 3.3V rail |
| **Current sense** | 5V | 1.5mA | 5V main | INA219 module |
| **PWM driver** | 3.3V | 160mA | 3.3V | PCA9685PW |
| **Heartbeat sensors ×2** | 3.3V | 600μA each, 1.2mA | 3.3V | MAX30105 ×2 |
| **Microphone** | 3.3V | 600μA | 3.3V | SPH0645LM4H |
| **MPU6050** | 3.3V | 4mA | 3.3V | Gyroscope |
| **Total system (operational)** | - | ~6A peak | Mixed | All systems active |

### Power Rail Summary
- **5V Rail**: Supports Pi, camera, servos, speaker, LDO input (~3.5-4A max)
- **3.3V Rail**: Supports PWM, sensors, microphone (~6.5mA available headroom)

---

## Hardware Revision History

### Version #2 Changes
- **GPIO voltage reference**: Tied GPIO_VREF to GPIO_3.3V_OUT for stability
- **Test points**: Added test points for all connector pins (debugging)
- **IRLED interrupt**: Added pull-up resistor on interrupt pin for reliable edge detection
- **IRLED sensors**: Reduced from 2 to 1 sensor (cost/complexity optimization)
- **PCB layout**: Consolidated everything to single layer (4-layer PCB stack)
- **ESD protection**: Upgraded diodes to solderable ESD9B5.0ST5G package
- **I2C bus assignment**: Swapped IRLED sensor to built-in I2C bus (cleaner integration)

---

## Power Budget Analysis

### Worst-case scenario (all systems active):
- CM4 + Camera: 3.0A @ 5V
- 4× Servos: 3.0A @ 5V (all moving)
- Speaker: 0.5A @ 5V
- LDO (3.3V rail): 0.6A input (35.2mA output load @ 3.3V)
- **5V bus total**: ~7.1A (exceeds 6A battery limit)

### Practical continuous operation:
- Limited to ~6A battery continuous supply
- Servo duty cycle: 50-75% reduces average
- Audio operation: Intermittent use
- **Recommended**: Throttle servo movement speed or limit parallel operation

### Battery life estimate (conservative):
- **Capacity**: 5000mAh @ 7.4V = 37Wh
- **Average draw**: 4-5A @ 7.4V ≈ 30-37W
- **Runtime**: ~1 hour with moderate use

