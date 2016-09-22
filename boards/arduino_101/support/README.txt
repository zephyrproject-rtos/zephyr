Restoring Original Bootloader RPM
=================================

If you have installed a custom bootloader on the Arduino 101 device for Zephyr
development you can restore the original bootloader using the flashpack utility
available from here:

https://downloadcenter.intel.com/downloads/eula/25470/Arduino-101-software-package?httpDown=https%3A%2F%2Fdownloadmirror.intel.com%2F25470%2Feng%2Farduino101-factory_recovery-flashpack.tar.bz2

Follow the README files in this package for instructions how to restore the
original factory settings.

The Arduino 101 board in Zephyr does support the original bootloader only now.
The advantages of the original bootloader are the following:

- Supports flashing using USB DFU
- Supports flashing the Bluetooth firmware
- Does not require JTAG
