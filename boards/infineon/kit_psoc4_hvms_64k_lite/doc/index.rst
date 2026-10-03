.. zephyr:board:: kit_psoc4_hvms_64k_lite

Overview
********

The `KIT_PSOC4-HVMS-64K_LITE`_ is a lite kit based on the PSOC™ 4 HVMS 64K
family, featuring a PSOC™ 4 HVMS 64K (CY8C4146LWE-HVS115X) microcontroller
with an Arm® Cortex®-M0+ core running at up to 48 MHz.

Key features include 64 KB flash, 8 KB SRAM, and an onboard KitProg3
programmer/debugger with USB Micro-B connectivity.

Hardware
********

- **SoC:** PSOC™ 4 HVMS 64K (CY8C4146LWE-HVS115X)
- **CPU:** Arm® Cortex®-M0+ at 48 MHz
- **Flash:** 64 KB
- **SRAM:** 8 KB
- **Peripherals:** SAR ADC, TCPWM, SCB (UART/SPI/I2C)
- **Debug:** Onboard KitProg3 (SWD + UART bridge)
- **Power:** USB powered via Micro-B connector

For more information about the PSOC™ 4 HVMS 64K and KIT_PSOC4-HVMS-64K_LITE:

- `PSOC 4 SoC Website`_
- `KIT_PSOC4-HVMS-64K_LITE Board Website`_

Kit Contents
============

- PSOC™ 4 HVMS 64K Lite kit board
- USB-A to Micro-B cable
- Quick start guide

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LEDs
----

+---------+--------------------+
| Name    | GPIO Pin           |
+=========+====================+
| LED6    | P0.7 (active high) |
+---------+--------------------+
| LED7    | P0.6 (active high) |
+---------+--------------------+

Buttons
-------

+---------+--------------------+
| Name    | GPIO Pin           |
+=========+====================+
| SW5     | P5.5 (active low)  |
+---------+--------------------+

Default Zephyr Peripheral Mapping
----------------------------------

+-----------+-----------------+----------------------------+
| Pin       | Function        | Usage                      |
+===========+=================+============================+
| P5.0      | SCB1 UART TX    | Console TX                 |
+-----------+-----------------+----------------------------+
| P5.1      | SCB1 UART RX    | Console RX                 |
+-----------+-----------------+----------------------------+
| P0.7      | GPIO            | LED6                       |
+-----------+-----------------+----------------------------+
| P0.6      | GPIO            | LED7                       |
+-----------+-----------------+----------------------------+
| P5.5      | GPIO            | SW5                        |
+-----------+-----------------+----------------------------+

System Clock
============

The PSOC™ 4 HVMS 64K uses the Internal Main Oscillator (IMO) as the default
system clock source. The clock path is:

- **IMO** (Internal Main Oscillator): 48 MHz
- **CLK_HF**: 48 MHz (system clock)

Serial Port
============

The PSOC™ 4 HVMS 64K has two SCB (Serial Communication Block) interfaces
that can be configured as UART, SPI, or I2C. The Zephyr console output is
assigned to **SCB1** (``uart1``), which is routed through the KitProg3
USB-UART bridge.

Default communication settings are **115200 8N1**.

Building
********

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_psoc4_hvms_64k_lite
   :goals: build

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The `KIT_PSOC4-HVMS-64K_LITE`_ includes an onboard programmer/debugger
(`KitProg3`_) which can be used to program and debug the PSOC™ 4 HVMS 64K
Cortex-M0+ core.

Infineon OpenOCD Installation
=============================

The `ModusToolbox™ Programming Tools`_ package includes Infineon OpenOCD.
Alternatively, a standalone installation can be done by downloading the
`Infineon OpenOCD`_ release for your system and extracting the files to a
location of your choice.

.. note::

   Linux requires device access rights to be set up for KitProg3. This is
   handled automatically by the ModusToolbox™ Programming Tools installation.
   When doing a standalone OpenOCD installation, this can be done
   manually by executing the script ``openocd/udev_rules/install_rules.sh``.

Configuring a Console
=====================

Connect a USB cable from your PC to the KitProg3 USB Micro-B connector on
the `KIT_PSOC4-HVMS-64K_LITE`_. Use the serial terminal of your choice
(minicom, PuTTY, etc.) with the following settings:

- **Speed:** 115200
- **Data:** 8 bits
- **Parity:** None
- **Stop bits:** 1

Flashing
========

.. tabs::

   .. group-tab:: Windows

      One time, set the Infineon OpenOCD path:

      .. code-block:: shell

         west config build.cmake-args -- "-DOPENOCD=path/to/infineon/openocd/bin/openocd.exe"

      Build and flash the application:

      .. code-block:: shell

         west build -b kit_psoc4_hvms_64k_lite -p always samples/hello_world
         west flash

   .. group-tab:: Linux

      One time, set the Infineon OpenOCD path:

      .. code-block:: shell

         west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

      Build and flash the application:

      .. code-block:: shell

         west build -b kit_psoc4_hvms_64k_lite -p always samples/hello_world
         west flash

You should see the following message on the console:

.. code-block:: console

   *** Booting Zephyr OS build vX.Y.Z ***
   Hello World! kit_psoc4_hvms_64k_lite

Debugging
=========

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_psoc4_hvms_64k_lite
   :goals: debug

Once the GDB console starts, you may set breakpoints and perform standard
GDB debugging on the PSOC™ 4 HVMS 64K Cortex-M0+ core.

References
**********

.. _KIT_PSOC4-HVMS-64K_LITE:
    https://www.infineon.com/cms/en/product/evaluation-boards/kit_psoc4-hvms-64k-lite/

.. _PSOC 4 SoC Website:
    https://www.infineon.com/cms/en/product/microcontroller/32-bit-psoc-arm-cortex-microcontroller/psoc-4-32-bit-arm-cortex-m0-mcu/

.. _KIT_PSOC4-HVMS-64K_LITE Board Website:
    https://www.infineon.com/cms/en/product/evaluation-boards/kit_psoc4-hvms-64k-lite/

.. _ModusToolbox™ Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
