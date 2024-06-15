.. _cy8cproto_063_ble:

INFINEON CY8CPROTO-063-BLE
###########################

Overview
********

The PSoC 6 BLE Proto Kit (CY8CPROTO-063-BLE) is a hardware platform that
enables design and debug of the Cypress PSoC 63 BLE MCU.

.. image:: img/cy8cproto-063-ble.jpg
     :align: center
     :alt: CY8CPROTO-063-BLE

Hardware
********

For more information about the PSoC 63 BLE MCU SoC and CY8CPROTO-063-BLE board:

- `PSoC 63 BLE MCU SoC Website`_
- `PSoC 63 BLE MCU Datasheet`_
- `PSoC 63 BLE MCU Architecture Reference Manual`_
- `PSoC 63 BLE MCU Register Reference Manual`_
- `CY8CPROTO-063-BLE Website`_
- `CY8CPROTO-063-BLE User Guide`_
- `CY8CPROTO-063-BLE Schematics`_

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
| GPIO      | on-chip    | gpio                  |
+-----------+------------+-----------------------+
| PINCTRL   | on-chip    | pin control           |
+-----------+------------+-----------------------+
| SPI       | on-chip    | spi                   |
+-----------+------------+-----------------------+
| UART      | on-chip    | serial port-polling;  |
|           |            | serial port-interrupt |
+-----------+------------+-----------------------+
| I2C       | on-chip    | I2C                   |
+-----------+------------+-----------------------+
| PWM       | on-chip    | PWM                   |
+-----------+------------+-----------------------+
| Counter   | on-chip    | Counter               |
+-----------+------------+-----------------------+
| Bluetooth | on-chip    | Bluetooth             |
+-----------+------------+-----------------------+


The default configurations can be found in
:zephyr_file:`boards/infineon/cy8cproto_063_ble/cy8cproto_063_ble_defconfig`

System Clock
============

The PSoC 63 BLE MCU SoC is configured to use the internal IMO+FLL as a source for
the system clock. CM0+ works at 50MHz, CM4 - at 100MHz. Other sources for the
system clock are provided in the SOC, depending on your system requirements.


OpenOCD Installation
====================

To get the OpenOCD package, it is required that you

1. Download the software ModusToolbox 3.1. https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolbox
2. Once downloaded add the path to access the Scripts folder provided by ModusToolbox
   export PATH=$PATH:/path/to/ModusToolbox/tools_3.1/openocd/scripts
3. Add the OpenOCD executable file's path to west flash/debug.
4. Flash using: west flash --openocd path/to/infineon/openocd/bin/openocd
5. Debug using: west debug --openocd path/to/infineon/openocd/bin/openocd


Fetch Binary Blobs
******************

cy8cproto_063_ble board requires fetch binary files
(e.g Bluetooth controller firmware, CM0p prebuilt images, etc).

To fetch Binary Blobs:

.. code-block:: console

   west blobs fetch hal_infineon

Programming and Debugging
*************************

The CY8CPROTO-063-BLE includes an onboard programmer/debugger (KitProg3) with
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

References
**********

.. _PSoC 63 BLE MCU SoC Website:
    https://www.cypress.com/products/32-bit-arm-cortex-m4-psoc-6

.. _PSoC 63 BLE MCU Datasheet:
    https://www.infineon.com/dgdl/Infineon-PSoC_6_MCU_PSoC_63_with_BLE_Datasheet_Programmable_System-on-Chip_(PSoC)-DataSheet-v16_00-EN.pdf?fileId=8ac78c8c7d0d8da4017d0ee4efe46c37&utm_source=cypress&utm_medium=referral&utm_campaign=202110_globe_en_all_integration-files

.. _PSoC 63 BLE MCU Architecture Reference Manual:
    https://documentation.infineon.com/html/psoc6/zrs1651212645947.html

.. _PSoC 63 BLE MCU Register Reference Manual:
    https://documentation.infineon.com/html/psoc6/bnm1651211483724.html

.. _CY8CPROTO-063-BLE Website:
    https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-063-ble/

.. _CY8CPROTO-063-BLE User Guide:
    https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-063-ble/#!?fileId=8ac78c8c7d0d8da4017d0f00d7eb1812

.. _CY8CPROTO-063-BLE Schematics:
    https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-063-ble/#!?fileId=8ac78c8c7d0d8da4017d0f00ea3c1821

.. _Infineon OpenOCD:
    https://github.com/infineon/openocd/releases/tag/release-v4.3.0
