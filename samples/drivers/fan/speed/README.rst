.. zephyr:code-sample:: fan-speed
   :name: Fan Speed
   :relevant-api: fan_interface

   Cycle a fan through a range of speeds.

Overview
********

This application steps the fan device referenced by the ``fan0`` :ref:`devicetree <dt-guide>`
alias through a range of speeds from stopped to full speed, pausing at each step.

The sample exercises the high-level :ref:`fan API <fan_api>` through :c:func:`fan_set_speed`,
which drives the backend:

* :dtcompatible:`fan-pwm` for fans whose speed is controlled by a PWM channel.

Requirements
************

The target board must expose a fan device through the ``fan0`` devicetree alias. Boards that do
not have a fan wired in their DTS can provide an :file:`<board>.overlay` file under
:file:`boards/`.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/fan/speed
   :goals: build flash
