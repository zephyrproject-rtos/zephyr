.. zephyr:board:: kit_psoc4_hvpa_144k_lite

Overview
********

The `KIT_PSOC4-HVPA-144K_LITE`_ is a lite kit based on the PSOC™ 4 HVPA 144K
family, featuring a PSOC™ 4 HVPA 144K (CY8C4147LCE-HV423) high-voltage precision
analog microcontroller with an Arm® Cortex®-M0+ core, intended for lead-acid battery
monitoring and management.

Key features include 128 KB flash, 8 KB SRAM, and an onboard KitProg3
programmer/debugger with USB Micro-B connectivity.

Hardware
********

- **SoC:** PSOC™ 4 HVPA 144K (CY8C4147LCE-HV423, 32-QFN)
- **CPU:** Arm® Cortex®-M0+ at 48 MHz
- **Flash:** 128 KB
- **SRAM:** 8 KB
- **Peripherals:** Precision analog subsystem, TCPWM, SCB (UART/SPI/I2C), LIN with
  integrated PHY
- **Debug:** Onboard KitProg3 (SWD + UART bridge)
- **Power:** USB powered via Micro-B connector, or 12 V wall adapter

For more information about the PSOC™ 4 HVPA 144K and KIT_PSOC4-HVPA-144K_LITE:

- `PSOC 4 SoC Website`_
- `KIT_PSOC4-HVPA-144K_LITE Board Website`_

Kit Contents
============

- PSOC™ 4 HVPA 144K Lite kit board
- USB cable

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
| LED6    | P0.3 (active high) |
+---------+--------------------+
| LED7    | P0.2 (active high) |
+---------+--------------------+

Buttons
-------

+---------+--------------------+
| Name    | GPIO Pin           |
+=========+====================+
| SW5     | P0.5 (active low)  |
+---------+--------------------+

P0.5 is also the external clock input (J13).

Default Zephyr Peripheral Mapping
----------------------------------

+-----------+-----------------+----------------------------+
| Pin       | Function        | Usage                      |
+===========+=================+============================+
| P0.1      | SCB0 UART TX    | Console TX                 |
+-----------+-----------------+----------------------------+
| P0.0      | SCB0 UART RX    | Console RX                 |
+-----------+-----------------+----------------------------+
| P0.3      | GPIO            | LED6                       |
+-----------+-----------------+----------------------------+
| P0.2      | GPIO            | LED7                       |
+-----------+-----------------+----------------------------+
| P0.5      | GPIO            | SW5                        |
+-----------+-----------------+----------------------------+

System Clock
============

The PSOC™ 4 HVPA 144K uses the Internal Main Oscillator (IMO) as the default
system clock source. The clock path is:

- **IMO** (Internal Main Oscillator): 48 MHz
- **CLK_HF**: 48 MHz (system clock)
- **CLK_PUMP**: IMO (analog pump clock)

SRAM ECC
========

The PSOC™ 4 HVPA 144K SRAM has error correction and detection (ECC). In Zephyr the
SRAM ECC is **disabled by default**: it is turned off early during reset, so
uninitialized SRAM can be read without raising a fault.

To enable the SRAM ECC, set the ``infineon,sram-ecc`` property on the ``sram0`` node, for
example in an application devicetree overlay:

.. code-block:: devicetree

   &sram0 {
       infineon,sram-ecc;
   };

and pass the overlay to the build:

.. code-block:: shell

   west build -b kit_psoc4_hvpa_144k_lite samples/hello_world -- -DEXTRA_DTC_OVERLAY_FILE=ecc.overlay

With the ECC enabled, the whole SRAM is written at reset to generate valid ECC. This is
skipped after a reset that preserves the SRAM (for example a watchdog reset), so ``.noinit``
data survives such a reset. After a power-on or a supply loss the whole SRAM is always
initialized.

Serial Port
============

The PSOC™ 4 HVPA 144K has one SCB (Serial Communication Block) interface that can
be configured as UART, SPI, or I2C. The Zephyr console output is assigned to
**SCB0** (``uart0``), which is routed through the KitProg3 USB-UART bridge. The
bridge is connected through resistors R99 and R101 (UART position, factory default);
the I2C position (R98 and R103) is not populated.

Default communication settings are **115200 8N1**.

Flash
=====

The 128 KB code flash (``flash0``) is written in 128-byte rows. It has no separate erase
operation: every row write erases the row first. Applications that store data define a
partition at the end of the flash that the application image does not use, as in
:zephyr_file:`tests/drivers/flash/common/boards/kit_psoc4_hvpa_144k_lite.overlay`, which uses
the last 32 KB. The 8 KB data flash at 0x1F000000 is not supported.

Power
=====

By default the kit is powered from the KitProg3 USB connector with jumper J36 on
position 2-3. To power it from a 12 V wall adapter on J14, set J36 to position 1-2
and slide switch SW4 to the adapter side.

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

The `KIT_PSOC4-HVPA-144K_LITE`_ includes an onboard programmer/debugger
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

Connect a USB cable from your PC to the KitProg3 USB Micro-B connector on
the `KIT_PSOC4-HVPA-144K_LITE`_. Use the serial terminal of your choice
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

.. _KIT_PSOC4-HVPA-144K_LITE:
    https://www.infineon.com/psochvpalitekit

.. _PSOC 4 SoC Website:
    https://www.infineon.com/cms/en/product/microcontroller/32-bit-psoc-arm-cortex-microcontroller/psoc-4-32-bit-arm-cortex-m0-mcu/

.. _KIT_PSOC4-HVPA-144K_LITE Board Website:
    https://www.infineon.com/psochvpalitekit

.. _ModusToolbox™ Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
