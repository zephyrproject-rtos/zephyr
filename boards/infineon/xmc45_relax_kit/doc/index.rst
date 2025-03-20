.. zephyr:board:: xmc45_relax_kit

Overview
********

The XMC4500 Relax Kit is designed to evaluate the capabilities of the XMC4500
Microcontroller. It is based on High performance ARM Cortex-M4F which can run
up to 120MHz.

Features:
=========

* ARM Cortex-M4F XMC4500
* 32 Mbit Quad-SPI Flash
* 4 x SPI-Master, 3x I2C, 3 x I2S, 3 x UART, 2 x CAN, 17 x ADC
* 2 pin header x1 and x2 with 80 pins
* Two buttons and two LEDs for user interaction
* Detachable on-board debugger (second XMC4500) with Segger J-Link

Details on the Relax Kit development board can be found in the `Relax Kit User Manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

The on-board 12-MHz crystal allows the device to run at its maximum operating speed of 120MHz.

Build hello world sample
************************
Here is an example for building the :zephyr:code-sample:`hello_world` sample application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xmc45_relax_kit
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
            west build -b xmc45_relax_kit -p always samples/hello_world

            west flash
            west debug

      .. group-tab:: Linux

         .. code-block:: shell

            # Do a pristine build
            west build -b xmc45_relax_kit -p always samples/hello_world

            west flash
            west debug

Once the gdb console starts after executing the west debug command, you may now set breakpoints and perform other standard GDB debugging.

References
**********

.. target-notes::

.. _Relax Kit User Manual:
   https://www.infineon.com/dgdl/Board_Users_Manual_XMC4500_Relax_Kit-V1_R1.2_released.pdf?fileId=db3a30433acf32c9013adf6b97b112f9

.. _XMC4500 TRM:
   https://www.infineon.com/dgdl/Infineon-xmc4500_rm_v1.6_2016-UM-v01_06-EN.pdf?fileId=db3a30433580b3710135a5f8b7bc6d13
