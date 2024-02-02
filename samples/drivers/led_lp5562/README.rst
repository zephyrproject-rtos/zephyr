.. zephyr:code-sample:: lp5562
   :name: LP5562 RGB LED
   :relevant-api: led_interface

   Control 4 RGB LEDs connected to an LP5562 driver chip.

Overview
********

This sample controls 4 LEDs connected to a TI LP5562 driver, using the
following pattern:

 1. turn on LEDs to be red
 2. turn on LEDs to be green
 3. turn on LEDs to be blue
 4. turn on LEDs to be white
 5. turn on LEDs to be yellow
 6. turn on LEDs to be purple
 7. turn on LEDs to be cyan
 8. turn on LEDs to be orange
 9. turn off LEDs
 10. blink the LEDs in white
 11. turn off LEDs
 12. blink the LEDs in purple
 13. turn off LEDs

Refer to the `LP5562 Manual`_ for the RGB LED connections and color channel
mappings used by this sample.

Building and Running
********************

Build the application for the :ref:`nrf52840dk_nrf52840` board, and connect
a LP5562 LED driver on the bus I2C0 at the address 0x30.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led_lp5562
   :board: nrf52840dk/nrf52840
   :goals: build
   :compact:

For flashing the application, refer to the Flashing section of the
:ref:`nrf52840dk_nrf52840` board documentation.

.. _LP5562 Manual: http://www.ti.com/lit/ds/symlink/lp5562.pdf
