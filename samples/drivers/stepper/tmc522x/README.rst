.. zephyr:code-sample:: tmc522x
   :name: TMC522x stepper
   :relevant-api: stepper_interface

   Rotate a TMC522x stepper motor in ping-pong position mode.

Description
***********

This sample application demonstrates the TMC522x stepper motor driver
using position mode (move_by) with the split stepper_driver and
stepper_ctrl APIs. The motor continuously moves between positive and
negative target positions, reporting the actual position on each
completion event.

References
**********

.. note::
   TMC522x datasheets are not yet publicly available.

Wiring
******

This sample supports both SPI and I2C variants of the TMC522x family.
The board's devicetree overlay must define ``stepper`` and ``stepper-drv``
aliases for the motion controller and stepper driver nodes respectively.

Building and Running
********************

This sample requires a TMC522x stepper motor controller. It should work
with any platform featuring a SPI or I2C peripheral interface. It does
not work on QEMU.

Build for SPI (TMC5221)
=======================

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/stepper/tmc522x
   :board: nucleo_f413zh
   :gen-args: -DDTC_OVERLAY_FILE=boards/nucleo_f413zh_spi.overlay
   :goals: build flash

Build for I2C (TMC5222)
=======================

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/stepper/tmc522x
   :board: nucleo_f413zh
   :gen-args: -DDTC_OVERLAY_FILE=boards/nucleo_f413zh_i2c.overlay
   :goals: build flash

Sample Output
=============

.. code-block:: console

   [00:00:00.001,000] <inf> tmc522x_sample: Starting TMC522x stepper sample
   [00:00:00.001,000] <inf> tmc522x_sample: Microstep resolution: 256
   [00:00:00.325,000] <inf> tmc522x_sample: Actual position: 3200
   [00:00:00.649,000] <inf> tmc522x_sample: Actual position: -3200
   [00:00:00.973,000] <inf> tmc522x_sample: Actual position: 3200
   All tests complete
