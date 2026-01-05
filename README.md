https://docs.google.com/spreadsheets/d/1XDxm2utaFQ-OXD1oPfhDBG_OyDTtbuzdnD-7CK9p-SM/edit?usp=sharing

Raspberry Pi setup instructions:

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
