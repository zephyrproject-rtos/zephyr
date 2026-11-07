.. zephyr:board:: cy8ckit_149_plus

Overview
********

The `CY8CKIT-149`_ is a prototyping kit based on the PSOC™ 4100S Plus
family, featuring a PSOC™ 4100S Plus (CY8C4147AZI-S475) microcontroller with
an Arm® Cortex®-M0+ core running at up to 48 MHz. It is designed for
mixed-signal and capacitive touch sensing applications.

The kit includes an onboard `KitProg3`_ programmer/debugger with USB
connectivity, a user LED and a user push button.

Hardware
********

- **SoC:** PSOC™ 4100S Plus (CY8C4147AZI-S475)
- **CPU:** Arm® Cortex®-M0+ at 48 MHz
- **Flash:** 128 KB
- **SRAM:** 16 KB
- **Peripherals:** SAR ADC, comparators, CAPSENSE™, TCPWM, SCB (UART/SPI/I2C)
- **User I/O:** one user LED and one user push button
- **Debug:** Onboard KitProg3 (SWD + UART bridge)

For more information about the PSOC™ 4100S Plus and CY8CKIT-149:

- `PSOC 4 SoC Website`_
- `CY8CKIT-149 Board Website`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Default Zephyr Peripheral Mapping
---------------------------------

+-----------+-----------------+----------------------------+
| Pin       | Function        | Usage                      |
+===========+=================+============================+
| P7.1      | SCB3 UART TX    | Console TX                 |
+-----------+-----------------+----------------------------+
| P7.0      | SCB3 UART RX    | Console RX                 |
+-----------+-----------------+----------------------------+
| P3.4      | GPIO            | LED0                       |
+-----------+-----------------+----------------------------+
| P3.7      | GPIO            | SW0 (pull-up)              |
+-----------+-----------------+----------------------------+

System Clock
============

The PSOC™ 4100S Plus uses the Internal Main Oscillator (IMO) as the default
system clock source. The clock path is:

- **IMO** (Internal Main Oscillator): 48 MHz
- **CLK_HF**: 48 MHz (system clock)

Serial Port
===========

The PSOC™ 4100S Plus has multiple SCB (Serial Communication Block) interfaces
that can be configured as UART, SPI, or I2C. The Zephyr console output is
assigned to **SCB3** (``uart3``), which is routed through the KitProg3
USB-UART bridge.

Default communication settings are **115200 8N1**.

Building
********

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: cy8ckit_149_plus
   :goals: build

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The `CY8CKIT-149`_ includes an onboard programmer/debugger (`KitProg3`_)
which can be used to program and debug the PSOC™ 4100S Plus Cortex-M0+
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

Connect a USB cable from your PC to the KitProg3 USB connector on the
`CY8CKIT-149`_. Use the serial terminal of your choice (minicom, PuTTY,
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

         west build -b cy8ckit_149_plus -p always samples/hello_world
         west flash

   .. group-tab:: Linux

      One time, set the Infineon OpenOCD path:

      .. code-block:: shell

         west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

      Build and flash the application:

      .. code-block:: shell

         west build -b cy8ckit_149_plus -p always samples/hello_world
         west flash

You should see the following message on the console:

.. code-block:: console

   *** Booting Zephyr OS build vX.Y.Z ***
   Hello World! cy8ckit_149_plus

Debugging
=========

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: cy8ckit_149_plus
   :goals: debug

Once the GDB console starts, you may set breakpoints and perform standard
GDB debugging on the PSOC™ 4100S Plus Cortex-M0+ core.

References
**********

.. _CY8CKIT-149:
    https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-149/

.. _PSOC 4 SoC Website:
    https://www.infineon.com/cms/en/product/microcontroller/32-bit-psoc-arm-cortex-microcontroller/psoc-4-32-bit-arm-cortex-m0-mcu/

.. _CY8CKIT-149 Board Website:
    https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-149/

.. _ModusToolbox™ Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
