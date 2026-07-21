.. zephyr:board:: framework_ledmatrix

Overview
********

The Framework Laptop 16 compatible LED Matrix module is based on an RP2040 MCU and has an array of
9x36 white PWM LEDs controlled by a Lumissil IS31FL3741A.

Hardware
********

* Dual core Arm Cortex-M0+ processor running up to 133MHz
* 264KB on-chip SRAM
* 1MB on-board QSPI flash with XIP capabilities
* Built-in 9x36 white LED matrix
* 2 way DIP Switch - one for user control, one to stay in bootloader
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

Here is an example of building the sample for enabling USB console.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/console
   :board: framework_ledmatrix
   :goals: build
   :compact:

You must flash the LED Matrix with an UF2 file. One option is to use West (Zephyr’s meta-tool). To
enter the UF2 flashing mode, remove the module flip the dip switch 2 and assemble it again. It will
appear on the host as a mass storage device. At this point you can flash the image file by running:

.. code-block:: bash

  west flash

After flashing, switch the DIP switch back again to run the firmware.

Alternatively, you can locate the generated :file:`build/zephyr/zephyr.uf2` file and simply
drag-and-drop to the device after entering the UF2 flashing mode.

References
**********

* `Official Product Page`_
* `Official Developer Documentation`_

.. _Official Product Page: https://frame.work/products/16-led-matrix
.. _Official Developer Documentation: https://github.com/FrameworkComputer/InputModules
