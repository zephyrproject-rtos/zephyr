.. zephyr:board:: kit_psoc4_hvpa_144k_lite

Overview
********

The KIT_PSoC4_HVPA_144K_LITE is a development kit based on the PSOC™ 4 HVPA
144K family, featuring a PSOC™ 4 HVPA 144K (CY8C4147LCE-HV423) microcontroller
with an Arm® Cortex®-M0+ core running at up to 48 MHz.

Hardware
********

- **SoC:** PSOC™ 4 HVPA 144K (CY8C4147LCE-HV423)
- **CPU:** Arm® Cortex®-M0+ at 48 MHz
- **Flash:** 128 KB
- **SRAM:** 8 KB
- **Peripherals:** SCB (UART/SPI/I2C), TCPWM, LIN, HVSS (High-Voltage Subsystem)
- **Debug:** Onboard KitProg3 (SWD + UART bridge)

For more information about the PSOC™ 4 SoC family:

- `PSOC 4 SoC Website`_

Supported Features
==================

.. zephyr:board-supported-hw::

Default Zephyr Peripheral Mapping
----------------------------------

+-----------+-----------------+----------------------------+
| Pin       | Function        | Usage                      |
+===========+=================+============================+
| P0.1      | SCB0 UART TX    | Console TX                 |
+-----------+-----------------+----------------------------+
| P0.0      | SCB0 UART RX    | Console RX                 |
+-----------+-----------------+----------------------------+
| P0.3      | GPIO            | LED0 (active low)          |
+-----------+-----------------+----------------------------+
| P0.5      | GPIO            | SW0 / User Button          |
+-----------+-----------------+----------------------------+

System Clock
============

The PSOC™ 4 HVPA 144K uses the Internal Main Oscillator (IMO) as the default
system clock source. The clock path is:

- **IMO** (Internal Main Oscillator): 48 MHz
- **CLK_HF**: 48 MHz (system clock)

Serial Port
============

The PSOC™ 4 HVPA 144K has a single SCB (Serial Communication Block)
interface that can be configured as UART, SPI, or I2C. The Zephyr console
output is assigned to **SCB0** (``uart0``), which is routed through the
KitProg3 USB-UART bridge.

Default communication settings are **115200 8N1**.

Building
********

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_psoc4_hvpa_144k_lite
   :goals: build

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The KIT_PSoC4_HVPA_144K_LITE includes an onboard programmer/debugger
(`KitProg3`_) which can be used to program and debug the PSOC™ 4 HVPA 144K
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

Connect a USB cable from your PC to the KitProg3 USB-UART bridge on the
KIT_PSoC4_HVPA_144K_LITE. Use the serial terminal of your choice (minicom,
PuTTY, etc.) with the following settings:

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

         west build -b kit_psoc4_hvpa_144k_lite -p always samples/hello_world
         west flash

   .. group-tab:: Linux

      One time, set the Infineon OpenOCD path:

      .. code-block:: shell

         west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

      Build and flash the application:

      .. code-block:: shell

         west build -b kit_psoc4_hvpa_144k_lite -p always samples/hello_world
         west flash

You should see the following message on the console:

.. code-block:: console

   *** Booting Zephyr OS build vX.Y.Z ***
   Hello World! kit_psoc4_hvpa_144k_lite

Debugging
=========

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_psoc4_hvpa_144k_lite
   :goals: debug

Once the GDB console starts, you may set breakpoints and perform standard
GDB debugging on the PSOC™ 4 HVPA 144K Cortex-M0+ core.

References
**********

.. _PSOC 4 SoC Website:
    https://www.infineon.com/cms/en/product/microcontroller/32-bit-psoc-arm-cortex-microcontroller/psoc-4-32-bit-arm-cortex-m0-mcu/

.. _ModusToolbox™ Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
