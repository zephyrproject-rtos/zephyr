.. zephyr:code-sample:: tmc50xx
   :name: TMC50XX Dual controller/driver for up to two 2-phase bipolar stepper motors.
          StealthChop™ no-noise stepper operation.
          Integrated motion controller with SPI interface
   :relevant-api: stepper_interface

Description
***********

This sample application periodically spins the stepper CW and CCW depending on the
:kconfig:option:`CONFIG_PING_PONG_N_REV` configuration.

References
**********

 - TMC5041: https://www.analog.com/media/en/technical-documentation/data-sheets/TMC5041_datasheet_rev1.16.pdf
 - TMC5072: https://www.analog.com/media/en/technical-documentation/data-sheets/TMC5072_datasheet_rev1.26.pdf

Wiring
*******

This sample uses the TMC5072 BOB controlled using the spi interface.


Building and Running
********************

This project spins the stepper and outputs the events to the console. It requires an TMC50XX stepper
driver. It should work with any platform featuring a SPI peripheral interface.
It does not work on QEMU.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/stepper/basic
   :board: nucleo_g071rb
   :goals: build flash

Sample Output
=============

.. code-block:: console

   Starting basic stepper sample
   stepper is 0x8007240, name is tmc_stepper@0
   stepper_callback steps completed changing direction
   stepper_callback steps completed changing direction
   stepper_callback steps completed changing direction

<repeats endlessly>
