.. zephyr:board:: cy8ckit_062s4

Overview
********
The PSOC 62S4 Pioneer kit has a CY8C62x4 MCU, which is an ultra-low-power PSOC device specifically designed for battery-operated analog
sensing applications. It includes a 150-MHz Arm® Cortex®-M4 CPU as the primary application processor, a 100-MHz Arm® Cortex®-M0+ CPU that
supports low-power operations, up to 256 KB Flash and 128 KB SRAM, programmable analog sensing,
CapSense™ touch-sensing, and programmable digital peripherals.

The board features an onboard
programmer/debugger (KitProg3), a 512-Mbit Quad SPI NOR flash, a micro-B connector for USB device
interface, a thermistor, an ambient light sensor, a 5-segment CapSense™ slider, two CapSense™ buttons, two
user LEDs, and a push button. The board supports operating voltages from 1.8 V to 3.3 V for PSoC™ 6 MCU.

Hardware
********

* `CY8CKIT 062S4 Pioneer Kit Website`_
* `CY8CKIT 062S4 Pioneer Kit Guide`_
* `CY8CKIT 062S4 Pioneer Kit Schematic`_
* `CY8CKIT 062S4 Pioneer Kit Technical Reference Manual`_
* `CY8CKIT 062S4 Pioneer Kit Datasheet`_

Supported Features
==================

.. zephyr:board-supported-hw::

Clock Configuration
===================

+-----------+------------+-----------------------+
| Clock     | Source     | Output Frequency      |
+===========+============+=======================+
| FLL       | IMO        | 100.0 MHz             |
+-----------+------------+-----------------------+
| PLL       | IMO        | 48.0 MHz              |
+-----------+------------+-----------------------+
| CLK_HF0   | CLK_PATH0  | 100.0 MHz             |
+-----------+------------+-----------------------+

Fetch Binary Blobs
******************

.. code-block:: console

   west blobs fetch hal_infineon

Build blinking led sample
*************************

Here is an example for building the :zephyr:code-sample:`blinky` sample application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: cy8ckit_062s4
   :goals: build

Programming and Debugging
*************************

The CY8CKIT-062S4 includes an onboard programmer/debugger (`KitProg3`_) to provide debugging, flash programming, and serial communication over USB. Flash and debug commands use OpenOCD and require a custom Infineon OpenOCD version, that supports KitProg3, to be installed.

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
            west build -b cy8ckit_062s4 -p always samples/basic/blinky

            west flash
            west debug

      .. group-tab:: Linux

         .. code-block:: shell

            # Run west config once to set permanent CMake argument
            west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

            # Do a pristine build once after setting CMake argument
            west build -b cy8ckit_062s4 -p always samples/basic/blinky

            west flash
            west debug

Once the gdb console starts after executing the west debug command, you may now set breakpoints and perform other standard GDB debugging on the PSOC 6 CM4 core.

References
**********

.. target-notes::

.. _CY8CKIT 062S4 Pioneer Kit Guide:
    https://www.infineon.com/dgdl/Infineon-CY8CKIT_062S4_PSOC62S4_pioneer_kit_guide-UserManual-v01_00-EN.pdf?fileId=8ac78c8c7e7124d1017e962f98992207

.. _CY8CKIT 062S4 Pioneer Kit Website:
    https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-062s4/?redirId=VL1508&utm_medium=referral&utm_source=cypress&utm_campaign=202110_globe_en_all_integration-dev_kit

.. _CY8CKIT 062S4 Pioneer Kit Schematic:
    https://www.infineon.com/dgdl/Infineon-CY8CKIT-062S4_PSOC_62S4_Pioneer_Kit_Schematic-PCBDesignData-v01_00-EN.pdf?fileId=8ac78c8c7d710014017d7153484d2081

.. _CY8CKIT 062S4 Pioneer Kit Technical Reference Manual:
    https://www.infineon.com/dgdl/Infineon-PSOC_6_MCU_CY8C61X4CY8C62X4_REGISTERS_TECHNICAL_REFERENCE_MANUAL_(TRM)_PSOC_61_PSOC_62_MCU-AdditionalTechnicalInformation-v03_00-EN.pdf?fileId=8ac78c8c7d0d8da4017d0fb34f0627a7

.. _CY8CKIT 062S4 Pioneer Kit Datasheet:
   https://www.infineon.com/dgdl/Infineon-PSOC_6_MCU_CY8C62X4-DataSheet-v12_00-EN.pdf?fileId=8ac78c8c7ddc01d7017ddd026d585901

.. _ModusToolbox:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolbox

.. _ModusToolbox Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
