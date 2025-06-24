﻿.. zephyr:board:: numaker_m55m1

Overview
********

The NuMaker M55M1 is an Internet of Things (IoT) application focused platform
specially developed by Nuvoton. The NuMaker-M55M1 is based on the NuMicro® M55M1
series MCU with ARM® -Cortex®-M55 core.

Features
========
- 32-bit Arm Cortex®-M55 M55M1H2LJAE MCU
- Core clock up to 220 MHz
- 2 MB embedded Dual Bank Flash and 1344 KB SRAM
- 128 KB DTCM and 64 KB ITCM
- USB 2.0 Full-Speed OTG / Device
- USB 1.1 Host
- Arduino UNO compatible interface
- One push-button is for reset
- Two LEDs: one is for power indication and the other is for user-defined
- On-board NU-Link2 ICE debugger/programmer with SWD connector

More information about the board can be found at the `NuMaker M55M1 User Manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

The on-board 12-MHz crystal allows the device to run at its maximum operating speed of 220 MHz.

More details about the supported peripherals are available in `M55M1 TRM`_

Building and Flashing
*********************
Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

On board debugger Nu-link2 can emulate UART0 as a virtual COM port over usb,
To enable this, set ISW1 DIP switch 1-3 (TXD RXD VOM) to ON.
Connect the NuMaker-M55M1 to your host computer using the USB port, then
run a serial host program to connect with your board. For example:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: numaker_m55m1
   :goals: flash

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: numaker_m55m1
   :goals: debug

Step through the application in your debugger.

References
**********

.. target-notes::

.. _NuMaker M55M1 User Manual:
   https://www.nuvoton.com/products/microcontrollers/arm-cortex-m55-mcus/m55m1-series/
.. _M55M1 TRM:
   https://www.nuvoton.com/products/microcontrollers/arm-cortex-m55-mcus/m55m1-series/
