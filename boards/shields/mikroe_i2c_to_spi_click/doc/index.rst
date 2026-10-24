.. _mikroe_i2c_to_spi_click_shield:

MikroElektronika I2C to SPI Click
=================================

Overview
********

`I2C to SPI Click`_ is a mikroBUS™ add-on that converts the host I2C bus into
an SPI controller using the NXP SC18IS602B bridge.

The host talks I2C on the mikroBUS™ socket. SPI is provided on the Click extra
header (not the mikroBUS™ SPI pins).

Requirements
************

This shield can only be used with a board that provides a mikroBUS™ socket and
defines ``mikrobus_i2c`` and ``mikrobus_header`` node labels. See
:ref:`shields` for more details.

The default I2C address is ``0x28``. Confirm the address jumpers on the Click.

RST is mikroBUS pin 1 and INT is mikroBUS pin 7. SPI is on the Click extra
header, not the mikroBUS SPI pins.

Programming
***********

Set ``--shield mikroe_i2c_to_spi_click`` when you invoke ``west build``.
For example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/spi/sc18is60x
   :board: <board>
   :shield: mikroe_i2c_to_spi_click
   :goals: build

To confirm the bridge is on the bus, build the shell sample with I2C and SPI
shell support and run ``i2c scan`` (address ``0x28``) then ``device list``.

References
**********

.. _I2C to SPI Click: https://www.mikroe.com/i2c-to-spi-click
