.. zephyr:code-sample:: tmc50xx
   :name: TMC50XX stepper
   :relevant-api: stepper_interface


   Rotate a TMC50XX stepper motor and change velocity at runtime.

Description
***********

This sample application periodically spins the stepper clockwise and counterclockwise depending on
the :kconfig:option:`CONFIG_PING_PONG_N_REV` configuration.

References
**********

 - TMC5041: https://www.analog.com/media/en/technical-documentation/data-sheets/TMC5041_datasheet_rev1.16.pdf
 - TMC5072: https://www.analog.com/media/en/technical-documentation/data-sheets/TMC5072_datasheet_rev1.26.pdf

Wiring
*******

This sample uses the TMC5072 BOB controlled using the SPI interface. The board's Devicetree must define
a ``stepper`` alias for the stepper motor node.

Building and Running
********************

This project spins the stepper and outputs the events to the console. It requires an TMC50XX stepper
driver. It should work with any platform featuring a SPI peripheral interface.
It does not work on QEMU.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/stepper/tmc50xx
   :board: nucleo_g071rb
   :goals: build flash

Sample Output
=============

.. code-block:: console

   Starting tmc50xx stepper sample
   stepper is 0x8007240, name is tmc_stepper@0
   stepper_callback steps completed changing direction
   stepper_callback steps completed changing direction
   stepper_callback steps completed changing direction

<repeats endlessly>
