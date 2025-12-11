.. zephyr:board:: p2d

Overview
********

Pebble 2 Duo is a smart watch based on the nRF52840 series chip SoC.

More information about the watch can be found at the `RePebble website`_.

Hardware
********

Pebble 2 Duo provides the following hardware components:

- Nordic nRF52840
- nPM1300 PMIC for power supply and battery charging
- GD25LE255E 256 Mb QSPI NOR
- Sharp LS013B7DH05 Memory-in-Pixel (MiP) monochrome display
- PWM backlight control using TPS22916 driver
- OPT3001 ambient light sensor
- 4 physical buttons
- LSM6DSOW IMU (accelerometer/gyroscope)
- MMC5603NJ magnetometer
- MSM261DDB020 PDM microphone
- Speaker driven by DA7212 audio codec
- LRA driven by TI DRV2604 haptic driver
- Programming connector

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

References
**********

.. target-notes::

.. _RePebble website:
   https://repebble.com/
