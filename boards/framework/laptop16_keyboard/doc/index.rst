.. zephyr:board:: framework_laptop16_keyboard

Overview
********

The Framework Laptop 16 compatible Keyboard modules, based on RP2040 MCU, come in different sizes
and with per-key RGB or white backlight:

* RGB backlight

  * ANSI Keyboard
  * Macropad

* White backlight

  * ANSI Keyboard
  * ISO Keyboard
  * Numpad

Hardware
********
* Dual core Arm Cortex-M0+ processor running up to 133MHz
* 264KB on-chip SRAM
* 1MB on-board QSPI flash with XIP capabilities
* Backlight

  * 1 PWM channel controlling the backlight
  * Per-key RGB backlight

* Numpad/Macropad: 8x4 keyboard matrix, with 21/24 keys
* Keyboard: 16x8 keyboard matrix
* 1 Analog input for key scanning
* 1 SGM48751 Mux for connecting any of the 8 KSI to the ADC input
* USB 1.1 controller (host/device)
* 8 Programmable I/O (PIO) for custom peripherals
* 1 Watchdog timer peripheral

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Using UF2
---------

Here is an example of building the sample for driving the built-in led.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky_pwm
   :board: rp2040_plus
   :goals: build
   :compact:

You must flash the keyboard with an UF2 file. One option is to use West (Zephyr’s meta-tool). To
enter the UF2 flashing mode, remove the module and hold down the both ALT keys (or ``1`` and ``6``
on the numpad) while assembling (powering) it again. It will appear on the host as a mass storage
device. At this point you can flash the image file by running:

.. code-block:: bash

  west flash

Alternatively, you can locate the generated :file:`build/zephyr/zephyr.uf2` file and simply
drag-and-drop to the device after entering the UF2 flashing mode.

References
**********

* Official Product Pages

  * `Keyboard`_
  * `Numpad`_
  * `Macropad`_

* `Official Developer Documentation`_

.. _Keyboard: https://frame.work/products/keyboard-module
.. _Numpad: https://frame.work/products/16-numpad
.. _Macropad: https://frame.work/products/16-rgb-macropad
.. _Official Developer Documentation: https://github.com/FrameworkComputer/InputModules
