# CM4_FW repository:
## Custom hardware - firmware interface for Raspberry Pi CM4

### Key features:
#### Build system: 

This project uses a Makefile to compile C libraries and projects into a binary .so (shared object) file. The binary file is loaded through python via CDLL and C projects are run through python scripts.

#### Libraries:
- GPIO
    - Uses memory mapped IO for reading/writing
    - Support for configuring interrupts using /dev/gpiochip kernel driver
- I2C drivers
    - Support for read/write/read-then-write functions
    - Uses /dev/i2c kernel driver
- I2S audio drivers
    - Multithreaded architecture
    - Microphone input passes data to a circular buffer
    - Speaker output, supports input from .pcm file or raw bytes
    - Using Advanced Linux Sound Architecture (ALSA) kernel driver
    - Hardware specs:
        - Speaker: MAX98357A amplifier
        - Mic: SPH0645LM4H
- PWM
    - Using the hardware PCA9685PW PWM driver
    - Communication through I2C
    - Support for 16-channel PWM output

#### Projects:
- Current sense
    - INA219 through I2C
- Servo control
    - Run off PWM driver
    - Features raw control as well as timed smooth move commands
- irled heartbeat sensor
    - MAX30105 via I2C
    - Multithreaded processing
    - Thread for data input, store to circular buffer
    - Calculation thread, performs EMA, BPF, smoothing, and thresholding to extract heartbeat data
    - [Smoothing calculations](https://docs.google.com/spreadsheets/d/1XDxm2utaFQ-OXD1oPfhDBG_OyDTtbuzdnD-7CK9p-SM/edit?usp=sharing)
    - Sleep & active modes to save CPU usage
- Unimplemented projects:
    - MPU6050 Gyroscope driver
    - MIPI-CSI 2 Video driver for camera
    - Battery state of charge estimation using current sense + voltage reading


#### Scripts
- Test scripts using argument parser to display functionality
- boot.py is run on boot


# Raspberry Pi setup instructions:

Install alsa asoundlib:

`sudo apt install libasound2-dev`

Modify config.txt file:

`sudo nano /boot/firmware/config.txt`

Add/uncomment the lines:

`dtoverlay=i2c-gpio,bus=3,i2c_gpio_sda=4,i2c_gpio_scl=5`
`dtparam=i2c_arm=on`
`dtparam=i2s=on`

Create a directory called dtoverlays in home/ted

`mkdir dtoverlays`

Copy dts file into directory (could be through ssh)

`scp dts_overlays\sph0645-max98357a-overlay.dts ted@tedberry:/home/ted/dtoverlays`

Compile dtbo file:

`dtc -@ -I dts -O dtb -o sph0645-max98357a.dtbo dtoverlays/sph0645-max98357a-overlay.dts`

Copy to overlays folder:

`sudo cp sph0645-max98357a.dtbo /boot/firmware/overlays/`

Add overlay to config.txt:

`dtoverlay=cm4-sph0645-max98357a`
