.. _cy8ckit_062s4:

INFINEON PSOC 62S4 Pioneer Kit
##############################

Overview
********
The PSOC 62S4 Pioneer kit has a CY8C62x4 MCU, which is an ultra-low-power PSoC device specifically designed for battery-operated analog
sensing applications. It includes a 150-MHz Arm® Cortex®-M4 CPU as the primary application processor, a 100-MHz Arm® Cortex®-M0+ CPU that
supports low-power operations, up to 256 KB Flash and 128 KB SRAM, programmable analog sensing,
CapSense™ touch-sensing, and programmable digital peripherals.

The board features an onboard
programmer/debugger (KitProg3), a 512-Mbit Quad SPI NOR flash, a micro-B connector for USB device
interface, a thermistor, an ambient light sensor, a 5-segment CapSense™ slider, two CapSense™ buttons, two
user LEDs, and a push button. The board supports operating voltages from 1.8 V to 3.3 V for PSoC™ 6 MCU.

.. figure:: img/cy8ckit_062s4.png
   :align: center
   :alt: INFINEON PSOC 62S4 Pioneer Kit

   INFINEON PSOC 62S4 Pioneer Kit (Credit: Infineon)

Hardware
********

* `CY8CKIT 062S4 Pioneer Kit Website`_
* `CY8CKIT 062S4 Pioneer Kit Guide`_
* `CY8CKIT 062S4 Pioneer Kit Schematic`_
* `CY8CKIT 062S4 Pioneer Kit Technical Reference Manual`_
* `CY8CKIT 062S4 Pioneer Kit Datasheet`_

Supported Features
==================

The board configuration supports the following hardware features:

+-----------+------------+-----------------------+
| Interface | Controller | Driver/Component      |
+===========+============+=======================+
| NVIC      | on-chip    | nested vectored       |
|           |            | interrupt controller  |
+-----------+------------+-----------------------+
| SYSTICK   | on-chip    | system clock          |
+-----------+------------+-----------------------+
| PINCTRL   | on-chip    | pin control           |
+-----------+------------+-----------------------+
| UART      | on-chip    | serial port-polling;  |
+-----------+------------+-----------------------+

The default configuration can be found in the Kconfig
:zephyr_file:`boards/infineon/cy8ckit_062s4/cy8ckit_062s4_defconfig`.

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
==================

.. code-block:: console

   west blobs fetch hal_infineon


Build and flash hello world sample
**********************************


.. code-block:: console

   cd zephyr/samples/hello_world
   west build -p auto -b cy8ckit_062s4 --pristine
   west flash
   picocom /dev/ttyACM0 -b 115200

OpenOCD Installation
====================

To get the OpenOCD package, it is required that you

1. Download the software ModusToolbox 3.1. https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolbox
2. Once downloaded add the path to access the Scripts folder provided by ModusToolbox::

      export PATH=$PATH:/path/to/ModusToolbox/tools_3.1/openocd/scripts

3. Add the OpenOCD executable file's path to west flash/debug.
4. Flash using: ``west flash --openocd path/to/infineon/openocd/bin/openocd``
5. Debug using: ``west debug --openocd path/to/infineon/openocd/bin/openocd``

References
**********

.. _CY8CKIT 062S4 Pioneer Kit Guide:
    https://www.infineon.com/dgdl/Infineon-CY8CKIT_062S4_PSoC62S4_pioneer_kit_guide-UserManual-v01_00-EN.pdf?fileId=8ac78c8c7e7124d1017e962f98992207

.. _CY8CKIT 062S4 Pioneer Kit Website:
    https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-062s4/?redirId=VL1508&utm_medium=referral&utm_source=cypress&utm_campaign=202110_globe_en_all_integration-dev_kit

.. _CY8CKIT 062S4 Pioneer Kit Schematic:
    https://www.infineon.com/dgdl/Infineon-CY8CKIT-062S4_PSoC_62S4_Pioneer_Kit_Schematic-PCBDesignData-v01_00-EN.pdf?fileId=8ac78c8c7d710014017d7153484d2081

.. _CY8CKIT 062S4 Pioneer Kit Technical Reference Manual:
    https://www.infineon.com/dgdl/Infineon-PSOC_6_MCU_CY8C61X4CY8C62X4_REGISTERS_TECHNICAL_REFERENCE_MANUAL_(TRM)_PSOC_61_PSOC_62_MCU-AdditionalTechnicalInformation-v03_00-EN.pdf?fileId=8ac78c8c7d0d8da4017d0fb34f0627a7

.. _CY8CKIT 062S4 Pioneer Kit Datasheet:
   https://www.infineon.com/dgdl/Infineon-PSoC_6_MCU_CY8C62X4-DataSheet-v12_00-EN.pdf?fileId=8ac78c8c7ddc01d7017ddd026d585901
