.. zephyr:board:: cy8cproto_040t_auto

Overview
********

The `CY8CPROTO-040T-AUTO`_ is a prototyping kit based on the PSOC™ 4000T
family, featuring a PSOC™ 4000T (CY8C4046LQI-T452) microcontroller with
an Arm® Cortex®-M0+ core running at up to 48 MHz.

Key features include 64 KB flash, 8 KB SRAM, CAPSENSE™ low-power sensing,
and an onboard `KitProg3`_ programmer/debugger with USB Micro-B connectivity.

Hardware
********

- **SoC:** PSOC™ 4000T (CY8C4046LQI-T452)
- **CPU:** Arm® Cortex®-M0+ at 48 MHz
- **Flash:** 64 KB
- **SRAM:** 8 KB
- **Peripherals:** TCPWM, SCB (UART/SPI/I2C), MSCLP (CAPSENSE™)
- **Debug:** Onboard KitProg3 (SWD + UART bridge)
- **Power:** USB powered via Micro-B connector (5V operating)

For more information about the PSOC™ 4000T and CY8CPROTO-040T-AUTO:

- `PSOC 4 SoC Website`_
- `CY8CPROTO-040T-AUTO Board Website`_

Kit Contents
============

- PSOC™ 4000T Prototyping Kit board
- USB-A to Micro-B cable
- Quick start guide

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Default Zephyr Peripheral Mapping
----------------------------------

+-----------+-----------------+----------------------------+
| Pin       | Function        | Usage                      |
+===========+=================+============================+
| P2.3      | SCB0 UART TX    | Console TX                 |
+-----------+-----------------+----------------------------+
| P2.2      | SCB0 UART RX    | Console RX                 |
+-----------+-----------------+----------------------------+

.. note::

   Verify the UART pin mapping above against the official board schematic
   before relying on it for production use.

System Clock
============

The PSOC™ 4000T uses the Internal Main Oscillator (IMO) as the default
system clock source. The clock path is:

- **IMO** (Internal Main Oscillator): 48 MHz
- **CLK_HF**: 48 MHz (system clock)

Serial Port
============

The PSOC™ 4000T has two SCB (Serial Communication Block) interfaces that
can each be configured as UART, SPI, or I2C. The Zephyr console output is
assigned to **SCB0** (``uart0``), which is routed through the KitProg3
USB-UART bridge.

Default communication settings are **115200 8N1**.

Building
********

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: cy8cproto_040t_auto
   :goals: build

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The `CY8CPROTO-040T-AUTO`_ includes an onboard programmer/debugger (`KitProg3`_)
which can be used to program and debug the PSOC™ 4000T Cortex-M0+
core.

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
the `CY8CPROTO-040T-AUTO`_. Use the serial terminal of your choice (minicom, PuTTY,
etc.) with the following settings:

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

         west build -b cy8cproto_040t_auto -p always samples/hello_world
         west flash

   .. group-tab:: Linux

      One time, set the Infineon OpenOCD path:

      .. code-block:: shell

         west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

      Build and flash the application:

      .. code-block:: shell

         west build -b cy8cproto_040t_auto -p always samples/hello_world
         west flash

You should see the following message on the console:

.. code-block:: console

   *** Booting Zephyr OS build vX.Y.Z ***
   Hello World! cy8cproto_040t_auto

Debugging
=========

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: cy8cproto_040t_auto
   :goals: debug

Once the GDB console starts, you may set breakpoints and perform standard
GDB debugging on the PSOC™ 4000T Cortex-M0+ core.

References
**********

.. _CY8CPROTO-040T-AUTO:
    https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-040t-auto/

.. _PSOC 4 SoC Website:
    https://www.infineon.com/cms/en/product/microcontroller/32-bit-psoc-arm-cortex-microcontroller/psoc-4-32-bit-arm-cortex-m0-mcu/

.. _CY8CPROTO-040T-AUTO Board Website:
    https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-040t-auto/

.. _ModusToolbox™ Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
