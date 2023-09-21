.. _is31fl3216a:

is31fl3216a: 16 channels PWD LEDs controller
############################################

Overview
********

This sample controls up to 16 LEDs connected to a is31fl3216a driver.

Each LED is gradually pulsed until it reach 100% of luminosity and gradually
turned off again.

Once each LED was pulsed, multiple LEDs are pulse simultaneously using the
``write_channels`` LED API.

Test pattern
============

For each LED:
- Increase the luminosity until 100% is reached
- Decrease the luminosity until completely turned off

- Increase the luminosity of LEDs 2 to 4 until 100% is reached
- Decrease the luminosity of LEDs 2 to 4 until completely turned off

Building and Running
********************

This sample can be built and executed when the devicetree has an I2C device node
with compatible :dtcompatible:`issi,is31fl3216a` enabled, along with the relevant
bus controller node also being enabled.

As an example this sample provides a DTS overlay for the :ref:`lpcxpresso55s28`
board (:file:`boards/lpcxpresso55s28.overlay`). It assumes that a I2C
_is31fl3216a LED driver (with 16 LEDs wired) is connected to the I2C bus at
address 0x74.

References
**********

- is31fl3216a: https://lumissil.com/assets/pdf/core/IS31FL3216A_DS.pdf
