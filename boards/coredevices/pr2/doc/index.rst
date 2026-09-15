.. zephyr:board:: pr2

Overview
********

Pebble Round 2 is a smart watch based on the SF32LB52x series chip SoC.

More information about the watch can be found at the `RePebble website`_.

Hardware
********

Pebble Round 2 provides the following hardware components:

- SiFli SF32LB52JUD6
- nPM1300 PMIC for power supply and battery charging
- GD25Q256E 256 Mb QSPI NOR
- JDI LS013B7DD02 Memory-in-Pixel (MiP) 64-color round display
- CST816T capacitive touch display driver
- RGB backlight driven by AW9346E
- W1160 ambient light sensor
- 4 physical buttons
- LIS2DW12 low-power accelerometer
- MMC5603NJ magnetometer
- Dual PDM microphone
- LRA driven by AW86225CSR
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
