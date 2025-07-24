.. zephyr:board:: kit_psc3m5_evk

Overview
********

This is the standard evaluation board for the PSOC™ Control C3 family of MCU's. PSOC™ Control C3M5
is a high-performance and low-power 32-bit single-core Arm® Cortex® M33-based MCU.
In addition to the CPU subsystem, the devices contain advanced real-time control peripherals,
such as a high-performance programmable analog subsystem, comparators, advanced timers with
high-resolution capability, up to six SCBs, and two CAN FDs for communication.

Hardware
********

For more information about the PSC3M5 SoC and KIT_PSC3M5_EVK board:

- `PSOC Control C3 SoC Website`_
- `KIT_PSC3M5_EVK Board Website`_

Kit Features:
=============

- Onboard programmer/debugger (KitProg3)
- PSC3M5FDS2AFQ1
- Type-C USB device interface
- Operating voltage of 3.3 V
- CAN FD interface
- Headers for MIKROE's mikroBUS shields
- Headers compatible with Arduino Uno R3
- Mode button and a mode LED for KitProg3
- Potentionmeter to simulate analog output
- Two (40-pin) expansion headers

Kit Contents:
=============

- EVK board
- Type C to USB A cable
- Quick Start Guide


Supported Features
==================

.. zephyr:board-supported-hw::


Build blinking led sample
*************************

Here is an example for building the :zephyr:code-sample:`blinky` sample application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: kit_psc3m5_evk
   :goals: build

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The KIT_PSC3M5_EVK includes an onboard programmer/debugger (`KitProg3`_) to provide debugging, flash programming, and serial communication over USB. Flash and debug commands use OpenOCD and require a custom Infineon OpenOCD version, that supports KitProg3, to be installed.

Infineon OpenOCD Installation
=============================

Both the full `ModusToolbox`_ and the `ModusToolbox Programming Tools`_ packages include Infineon OpenOCD. Installing either of these packages will also install Infineon OpenOCD. If neither package is installed, a minimal installation can be done by downloading the `Infineon OpenOCD`_ release for your system and manually extract the files to a location of your choice.

.. note:: Linux requires device access rights to be set up for KitProg3. This is handled automatically by the ModusToolbox and ModusToolbox Programming Tools installations. When doing a minimal installation, this can be done manually by executing the script ``openocd/udev_rules/install_rules.sh``.

West Commands
=============

The path to the installed Infineon OpenOCD executable must be available to the ``west`` tool commands. There are multiple ways of doing this. The example below uses a permanent CMake argument to set the CMake variable ``OPENOCD``.

   .. tabs::
      .. group-tab:: Windows

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd.exe

            # Do a pristine build once after setting CMake argument
            west build -b kit_psc3m5_evk -p always samples/basic/blinky

            west flash
            west debug

      .. group-tab:: Linux

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

            # Do a pristine build once after setting CMake argument
            west build -b kit_psc3m5_evk -p always samples/basic/blinky

            west flash
            west debug

Once the gdb console starts after executing the west debug command, you may now set breakpoints and perform other standard GDB debugging on the CYW20829 CM33 core.

.. _PSOC Control C3 SoC Website:
    https://www.infineon.com/cms/en/product/microcontroller/32-bit-psoc-arm-cortex-microcontroller/32-bit-psoc-control-arm-cortex-m33-mcu/

.. _KIT_PSC3M5_EVK Board Website:
    https://www.infineon.com/cms/en/product/evaluation-boards/kit_psc3m5_evk/

.. _ModusToolbox:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolbox

.. _ModusToolbox Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
