.. zephyr:code-sample:: lp5569
   :name: LP5569 9-channel LED controller
   :relevant-api: led_interface

   Control 9 LEDs connected to an LP5569 driver chip.

Overview
********

This sample controls 9 LEDs connected to an LP5569 driver. The sample turns
all LEDs on and switches all LEDs off again within a one second interval.

Building and Running
********************

Build the application for the :ref:`nrf52840dk_nrf52840` board, and connect
a LP5569 LED controller on the bus I2C0 at the address 0x32.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/lp5569
   :board: nrf52840dk/nrf52840
   :goals: build
   :compact:

For flashing the application, refer to the Flashing section of the
:ref:`nrf52840dk_nrf52840` board documentation.

.. code-block:: none

  *** Booting Zephyr OS build zephyr-v3.3.0 ***
  [00:00:00.361,694] <inf> app: Found LED device lp5569@32
  [00:00:00.361,694] <inf> app: Testing 9 LEDs ..

References
**********

- LP5569 Datasheet: https://www.ti.com/product/de-de/LP5569
