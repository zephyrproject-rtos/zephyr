.. _mikroe_weather_click:

MikroElektronika Silent Step 2 Click
####################################

Overview
********

The MikroElektronika `Silent Step 2 Click`_ features the `ADI TMC2130`_ stepper driver with step-dir
control via gpio and configuration via spi. A `TI TCA9538`_ gpio expander accessed via i2c offers
access to additional pins of th stepper driver.
The TMC2130 uses by default the work-queue timing source, but that can be changed.



Requirements
************

This shield can only be used with a board that provides a mikroBUS
socket and defines a ``mikrobus_i2c`` node label for the mikroBUS I2C
interface, a ``mikrobus_spi`` node label for the mikroBUS SPI
interface and a ``mikrobus_header`` node label (see :ref:`shields` for more details).

Programming
***********

.. zephyr-app-commands::
   :zephyr-app: <app>
   :board: <board>
   :shield: mikroe_silent_step_2_click
   :goals: build

.. _Silent Step 2 Click:
   https://www.mikroe.com/silent-step-2-click

.. _ADI TMC2130:
   https://www.analog.com/en/products/tmc2130.html

.. _TI TCA9538:
   https://www.ti.com/product/TCA9538
