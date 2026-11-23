.. zephyr:board:: numaker_m031ki

Overview
********

The NuMaker M031KI is a microcontroller platform with comprehensive peripheral integration
specially developed by Nuvoton. The NuMaker-M031KI is based on the NuMicro® M031
series MCU with ARM® -Cortex®-M0 core.

Features:
=========
- 32-bit Arm Cortex®-M0 M031KIAAE MCU
- Core clock up to 48 MHz
- 512 KB embedded Dual Bank Flash and 96 KB SRAM
- Arduino UNO compatible interface
- One push-button is for reset
- Two LEDs: one is for power indication and the other is for user-defined
- On-board NU-Link2 ICE debugger/programmer with SWD connector

More information about the board can be found at the `NuMaker M031KI User Manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

The on-board 12-MHz crystal allows the device to run at its maximum operating speed of 48MHz.

More details about the supported peripherals are available in `M031 TRM`_

Building and Flashing
*********************

.. zephyr:board-supported-runners::

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

On board debugger Nu-link2 can emulate UART0 as a virtual COM port over usb,
To enable this, set ISW1 DIP switch 1-3 (TXD RXD VOM) to ON.
Connect the NuMaker-M3351KI to your host computer using the USB port, then
run a serial host program to connect with your board. For example:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: numaker_m031ki
   :goals: flash

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: numaker_m031ki
   :goals: debug

Step through the application in your debugger.

VS Code Support
===============

Here is to go through VS Code instead of command line.

Please install Nuvoton NuMicro Cortex-M Pack and follow getting start guide of this pack.
This pack is a complete development toolkit for Nuvoton’s NuMicro Cortex-M microcontrollers
in Visual Studio Code.
URL of this pack is
https://marketplace.visualstudio.com/items?itemName=Nuvoton.nuvoton-numicro-cortex-m-pack

References
**********

.. target-notes::

.. _NuMaker M031KI User Manual:
   https://www.nuvoton.com/products/microcontrollers/arm-cortex-m0-mcus/m031-series/?group=Document&tab=2
.. _M031 TRM:
   https://www.nuvoton.com/products/microcontrollers/arm-cortex-m0-mcus/m031-series/?group=Document&tab=2
