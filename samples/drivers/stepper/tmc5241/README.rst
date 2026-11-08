.. zephyr:code-sample:: tmc5241
   :name: TMC5241 stepper
   :relevant-api: stepper_interface

   Exercise the TMC5241 stepper controller+driver driver end-to-end, doubling as the
   Level 2 functional-test harness (see stp.md L2-001..L2-016).

Description
***********

This sample enables the driver, round-trips microstep resolution, configures and arms
StallGuard, then ping-pongs the motor back and forth using the ramp generator's
position-mode moves, while logging every ``stepper_ctrl_event`` that fires. It also
briefly exercises velocity-mode ``run()``/``stop()`` and the Trinamic-specific runtime
``set_max_velocity()`` API. Each block in ``src/main.c`` is commented with the stp.md
test ID(s) (``L2-001``..``L2-016``) it exercises.

For the TMC2241 (driver-only) variant, only the ``stepper.h`` portion applies - build
against an ``adi,tmc2241`` node instead and skip the ``stepper_ctrl_*`` calls (no ramp
generator hardware exists on that chip).

References
**********

 - TMC5241: https://www.analog.com/media/en/technical-documentation/data-sheets/TMC5241_datasheet_rev1.10.pdf
 - TMC2241: https://www.analog.com/media/en/technical-documentation/data-sheets/TMC2241_datasheet_rev1.09.pdf

Wiring
******

The provided ``boards/max32655fthr_max32655_m4.overlay`` wires the TMC5241 onto the
MAX32655FTHR's Feather SPI header (``feather_spi``/``spi1``, CS0). ``boards/nucleo_g071rb.overlay``
is also provided as a generic reference. Both are placeholders for the stallguard2-threshold/ramp
values - adapt those to the actual motor. The board's Devicetree must define ``stepper-driver`` and
``stepper-ctrl`` aliases.

Building and Running
*********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/stepper/tmc5241
   :board: max32655fthr/max32655/m4
   :goals: build flash

Sample Output
=============

.. code-block:: console

   Starting tmc5241 stepper sample
   Microstep resolution is 8
   set_micro_step_res(3) [invalid] returned -22 (expect -EINVAL)
   set_microstep_interval() returned -88 (expect -ENOSYS)
   event: steps completed
   is_moving: 0
   Actual position: 200

   <repeats endlessly>
