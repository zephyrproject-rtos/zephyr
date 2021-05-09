.. _stm32-pm-blinky-sample:

Blinky
######

Overview
********

Blinky is a simple application which blinks an LED forever using the :ref:`GPIO
API <gpio_api>` in low power context.

.. _stm32-pm-blinky-sample-requirements:

Requirements
************

You will see this error if you try to build Blinky for an unsupported board:

.. code-block:: none

   Unsupported board: led0 devicetree alias is not defined

The board must have an LED connected via a GPIO pin. These are called "User
LEDs" on many of Zephyr's :ref:`boards`. The LED must be configured using the
``led0`` :ref:`devicetree <dt-guide>` alias. This is usually done in the
:ref:`BOARD.dts file <devicetree-in-out-files>` or a :ref:`devicetree overlay
<set-devicetree-overlays>`.

The board should support enabling PM.

Building and Running
********************

Build and flash Blinky as follows, changing ``nucleo_l476rg`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_l476rg
   :goals: build flash
   :compact:

After flashing, the LED starts to blink. Blinky does not print to the console.
