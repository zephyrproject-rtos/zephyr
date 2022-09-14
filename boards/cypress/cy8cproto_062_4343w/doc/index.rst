.. _cy8cproto_062_4343w:

INFINEON CY8CPROTO-062-4343W
############################

Overview
********

The CY8CPROTO-062-4343W PSoC 6 Wi-Fi BT Prototyping Kit is a low-cost hardware
platform that enables design and debug of PSoC 6 MCUs. It comes with a Murata
LBEE5KL1DX module, based on the CYW4343W combo device, industry-leading CAPSENSE
for touch buttons and slider, on-board debugger/programmer with KitProg3, microSD
card interface, 512-Mb Quad-SPI NOR flash, PDM-PCM microphone, and a thermistor.

This kit is designed with a snap-away form-factor, allowing the user to separate
the different components and features that come with this kit and use independently.
In addition, support for Digilent's Pmod interface is also provided with this kit.

.. image:: img/board.jpg
     :align: center
     :alt: CY8CPROTO-062-4343W

Hardware
********

For more information about the PSoC 62 MCU SoC and CY8CPROTO-062-4343W board:

- `PSoC 62 MCU SoC Website`_
- `PSoC 62 MCU Datasheet`_
- `PSoC 62 MCU Architecture Reference Manual`_
- `PSoC 62 MCU Register Reference Manual`_
- `CY8CPROTO-062-4343W PSoC 6 Wi-Fi BT Website`_
- `CY8CPROTO-062-4343W PSoC 6 Wi-Fi BT User Guide`_
- `CY8CPROTO-062-4343W PSoC 6 Wi-Fi BT Schematics`_

Kit Features:
=============

- Support of up to 2MB Flash and 1MB SRAM
- Dedicated SDHC to interface with WICED wireless devices.
- Delivers dual-cores, with a 150-MHz Arm Cortex-M4 as the primary
  application processor and a 100-MHz Arm Cortex-M0+ as the secondary
  processor for low-power operations.
- Supports Full-Speed USB, capacitive-sensing with CAPSENSE, a PDM-PCM
  digital microphone interface, a Quad-SPI interface, 13 serial communication
  blocks, 7 programmable analog blocks, and 56 programmable digital blocks.

Kit Contents:
=============

- PSoC 6 Wi-Fi BT Prototyping Board
- USB Type-A to Micro-B cable
- Quick Start Guide

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
| GPIO      | on-chip    | GPIO                  |
+-----------+------------+-----------------------+
| UART      | on-chip    | serial port-polling;  |
|           |            | serial port-interrupt |
+-----------+------------+-----------------------+


The default configuration can be found in the Kconfig

:zephyr_file:`boards/cypress/cy8cproto_062_4343w/cy8cproto_062_4343w_defconfig`


System Clock
============

The PSoC 62 MCU SoC is configured to use the internal IMO+FLL as a source for
the system clock. CM0+ works at 50MHz, CM4 - at 100MHz. Other sources for the
system clock are provided in the SOC, depending on your system requirements.


Fetch Binary Blobs
******************

cy8cproto_062_4343w board optionally uses binary blobs for features
(e.g WIFI/Bluetooth chip firmware, CM0p prebuilt images, etc).

To fetch Binary Blobs:

.. code-block:: console

   west blobs fetch hal_infineon


Build blinking led sample
*************************

Here is an example for the :zephyr:code-sample:`blinky` application.

.. code-block:: console

   cd zephyr
   west build -p auto -b cy8cproto_062_4343w samples/basic/blink

OpenOCD Installation
====================

To get the OpenOCD package, it is required that you

1. Download the software ModusToolbox 3.1. https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolbox
2. Once downloaded add the path to access the Scripts folder provided by ModusToolbox
   export PATH=$PATH:/path/to/ModusToolbox/tools_3.1/openocd/scripts
3. Add the OpenOCD executable file's path to west flash/debug.
4. Flash using: west flash --openocd path/to/infineon/openocd/bin/openocd
5. Debug using: west debug --openocd path/to/infineon/openocd/bin/openocd


Programming and Debugging
*************************

The CY8CPROTO-062-4343W includes an onboard programmer/debugger (KitProg2) with
mass storage programming to provide debugging, flash programming, and serial
communication over USB. Flash and debug commands must be pointed to the Cypress
OpenOCD you downloaded above.

On Windows:

.. code-block:: console

   west flash --openocd path/to/infineon/openocd/bin/openocd.exe
   west debug --openocd path/to/infineon/openocd/bin/openocd.exe

On Linux:

.. code-block:: console

   west flash --openocd path/to/infineon/openocd/bin/openocd
   west debug --openocd path/to/infineon/openocd/bin/openocd

Once the gdb console starts after executing the west debug command, you may
now set breakpoints and perform other standard GDB debugging on the PSoC 6 CM4 core.

Errata
======

+------------------------------------------------+----------------------------------------+
| Problem                                        | Solution                               |
+================================================+========================================+
| The GPIO_INT_TRIG_BOTH interrupt is not raised | This will be fixed in a future release.|
| when the associated GPIO is asserted.          |                                        |
+------------------------------------------------+----------------------------------------+
| GDB experiences a timeout error connecting to  | This will be fixed in a future release.|
| a server instance started by west debugserver. |                                        |
+------------------------------------------------+----------------------------------------+

.. _PSoC 62 MCU SoC Website:
    https://www.cypress.com/products/32-bit-arm-cortex-m4-psoc-6

.. _PSoC 62 MCU Datasheet:
    https://www.cypress.com/documentation/datasheets/psoc-6-mcu-psoc-62-datasheet-programmable-system-chip-psoc-preliminary

.. _PSoC 62 MCU Architecture Reference Manual:
    https://www.cypress.com/documentation/technical-reference-manuals/psoc-6-mcu-psoc-62-architecture-technical-reference-manual

.. _PSoC 62 MCU Register Reference Manual:
    https://www.cypress.com/documentation/technical-reference-manuals/psoc-6-mcu-psoc-62-register-technical-reference-manual-trm

.. _CY8CPROTO-062-4343W PSoC 6 Wi-Fi BT Website:
    https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-062-4343w/

.. _CY8CPROTO-062-4343W PSoC 6 Wi-Fi BT User Guide:
    https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-062-4343w/#!?fileId=8ac78c8c7d0d8da4017d0f0118571844

.. _CY8CPROTO-062-4343W PSoC 6 Wi-Fi BT Schematics:
    https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-062-4343w/#!?fileId=8ac78c8c7d0d8da4017d0f01126b183f

.. _Infineon OpenOCD:
    https://github.com/infineon/openocd/releases/tag/release-v4.3.0
