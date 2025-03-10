.. zephyr:board:: xmc47_relax_kit

Overview
********

The XMC4700 Relax Kit is designed to evaluate the capabilities of the XMC4700
Microcontroller. It is based on High performance ARM Cortex-M4F which can run
up to 144MHz.

Features:
=========

* ARM Cortex-M4F XMC4700
* On-board Debug Probe with USB interface supporting SWD + SWO
* Virtual COM Port via Debug Probe
* USB (Micro USB Plug)
* 32 Mbit Quad-SPI Flash
* Ethernet PHY and RJ45 Jack
* 32.768 kHz RTC Crystal
* microSD Card Slot
* CAN Transceiver
* 2 pin header x1 and x2 with 80 pins
* Two buttons and two LEDs for user interaction

Details on the Relax Kit development board can be found in the `Relax Kit User Manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

More details about the supported peripherals are available in `XMC4700 TRM`_

Build hello world sample
************************
Here is an example for building the :zephyr:code-sample:`hello_world` sample application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xmc47_relax_kit
   :goals: build

Programming and Debugging
*************************
West Commands
=============
Here is an example for the :zephyr:code-sample:`hello_world` application.

   .. tabs::
      .. group-tab:: Windows

         .. code-block:: shell

            # Do a pristine build
            west build -b xmc47_relax_kit -p always samples/hello_world

            west flash
            west debug

      .. group-tab:: Linux

         .. code-block:: shell

            # Do a pristine build
            west build -b xmc47_relax_kit -p always samples/hello_world

            west flash
            west debug

Once the gdb console starts after executing the west debug command, you may now set breakpoints and perform other standard GDB debugging.

References
**********

.. target-notes::

.. _Relax Kit User Manual:
   https://www.infineon.com/dgdl/Infineon-Board_User_Manual_XMC4700_XMC4800_Relax_Kit_Series-UserManual-v01_04-EN.pdf?fileId=5546d46250cc1fdf01513f8e052d07fc

.. _XMC4700 TRM:
   https://www.infineon.com/dgdl/Infineon-ReferenceManual_XMC4700_XMC4800-UM-v01_03-EN.pdf?fileId=5546d462518ffd850151904eb90c0044
