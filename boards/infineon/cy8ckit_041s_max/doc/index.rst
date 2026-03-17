.. zephyr:board:: cy8ckit_041s_max

Overview
********

The PSOC™ 4100S Max Pioneer Kit (CY8CKIT-041S-MAX) enables you to evaluate and develop applications using the PSOC™ 4100S Max microcontroller, part of Infineon's PSOC™ 4 family.
The device integrates an Arm Cortex-M0+ CPU running up to 48 MHz, combining programmable analog and digital subsystems to support flexible mixed-signal designs. It features up to 384 KB Flash and up to 32 KB SRAM, and includes a wide range of configurable peripherals such as SAR ADC, comparators, opamps (CTBm), CapSense™ capacitive touch sensing, and TCPWM for timer/counter/PWM functionality.

32-bit MCU subsystem
- 48-MHz Arm® Cortex®-M0+ CPU with single-cycle multiply
- Up to 384 KB of flash with read accelerator
- Up to 32 KB of SRAM
- Direct memory access (DMA)
- Low-power 1.71 V to 5.5 V operation
- Deep sleep mode with low-power touch sensing
- Active touch detection and tracking with low power consumption
- Real Time clock support
- Power supply: 3.3 V or 5 V operation

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

Here is an example for building the :zephyr:code-sample:`hello_world` sample application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: cy8ckit_041s_max
   :goals: build

Flashing
========

The CY8CKIT-041S-MAX includes an onboard programmer/debugger (`KitProg3`_) to provide debugging, flash programming, and serial communication over USB. Flash and debug commands use OpenOCD and require a custom Infineon OpenOCD version, that supports KitProg3, to be installed.

Infineon OpenOCD Installation
=============================

Both the full `ModusToolbox`_ and the `ModusToolbox Programming Tools`_ packages include Infineon OpenOCD.
Installing either of these packages will also install Infineon OpenOCD.

If neither package is installed, a minimal installation can be done by downloading the `Infineon OpenOCD`_ release for your system and manually extract the files to a location of your choice.

.. note:: Linux requires device access rights to be set up for KitProg3. This is handled automatically by the ModusToolbox and ModusToolbox Programming Tools installations. When doing a minimal installation, this can be done manually by executing the script ``openocd/udev_rules/install_rules.sh``.

West Commands
=============

The path to the installed Infineon OpenOCD executable must be available to the ``west`` tool commands. There are multiple ways of doing this. The example below uses a permanent CMake argument to set the CMake variable ``OPENOCD``.

Run ``west config`` once to set permanent CMake argument:

   .. tabs::
      .. group-tab:: Windows

         .. code-block:: shell

            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd.exe

      .. group-tab:: Linux

         .. code-block:: shell

            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

Once configured, you can build and flash applications:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: cy8ckit_041s_max
   :goals: build flash debug

Once the gdb console starts after executing the west debug command, you may now set breakpoints and perform other standard GDB debugging.

References
**********

.. target-notes::

.. _cy8ckit_041s_max Board Website:
   https://www.infineon.com/evaluation-board/CY8CKIT-041S-MAX

.. _ModusToolbox:
    https://www.infineon.com/design-resources/development-tools/sdk/modustoolbox-software

.. _ModusToolbox Programming Tools:
    https://www.infineon.com/design-resources/development-tools/sdk/modustoolbox-software/modustoolbox-programming-tools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
