.. zephyr:board:: pt2

Overview
********

Pebble Time 2 is a smart watch based on the SF32LB52x series chip SoC.

More information about the watch can be found at the `RePebble website`_.

Hardware
********

Pebble Time 2 provides the following hardware components:

- SiFli SF32LB52JUD6
- nPM1300 PMIC for power supply and battery charging
- GD25Q256E 256 Mb QSPI NOR
- JDI LPM015M135A Memory-in-Pixel (MiP) 64-color display
- CST816D capacitive touch display driver
- RGB backlight driven by AW2016
- W1160 ambient light sensor
- 4 physical buttons
- LSM6DSOW IMU (accelerometer/gyroscope)
- LIS2DW12 low-power accelerometer
- MMC5603NJ magnetometer
- Dual PDM microphone
- Speaker driven by AW8155BFCR amplifier
- LRA driven by AW86225CSR
- GH3026 heart-rate monitor sensor
- Programming connector

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Refer to `sftool website`_ for more information.

References
**********

.. target-notes::

.. _RePebble website:
   https://repebble.com/

.. _sftool website:
   https://github.com/OpenSiFli/sftool
