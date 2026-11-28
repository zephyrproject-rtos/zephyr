.. zephyr:board:: kit_psoc_hvpa_spm1_lite

Overview
********

The KIT_PSOC-HVPA-SPM1_LITE is a low cost evaluation kit for the PSOC™ 4 HVPA-SPM 1.0
(high voltage precision analog smart battery pack monitor) family. It carries a
PSOC™ 4 HVPA-SPM 1.0 (CY8C4147LDE-HV734) microcontroller with an Arm® Cortex®-M0+ core
and an onboard KitProg3 programmer/debugger.

Hardware
********

- **SoC:** PSOC™ 4 HVPA-SPM 1.0 (CY8C4147LDE-HV734), 48-pin QFN
- **CPU:** Arm® Cortex®-M0+, up to 49.152 MHz (48 MHz in Zephyr)
- **Flash:** 128 KB code flash (the 16 KB data flash is not described yet)
- **SRAM:** 8 KB
- **Peripherals:** 2x SCB (UART/SPI/I2C), 5x 16-bit TCPWM, precision analog subsystem
  (PACSS), isoUART, CAN FD, LIN, Crypto
- **Debug:** Onboard KitProg3 (SWD + UART bridge)

Only the SCB, GPIO and clock blocks are supported by Zephyr on this board so far. See the
PSOC™ 4 HVPA-SPM 1.0 architecture reference manual (Infineon document 002-39965) for the
silicon description.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The pin assignment below is taken from the BSP of the kit (``TARGET_APP_KIT_PSOC-HVPA-SPM1_LITE``).

LEDs
----

+---------+--------------------+
| Name    | GPIO Pin           |
+=========+====================+
| LED6    | P0.5 (active high) |
+---------+--------------------+

Default Zephyr Peripheral Mapping
----------------------------------

+-----------+-----------------+----------------------------+
| Pin       | Function        | Usage                      |
+===========+=================+============================+
| P0.0      | SCB0 UART RX    | Console RX                 |
+-----------+-----------------+----------------------------+
| P0.1      | SCB0 UART TX    | Console TX                 |
+-----------+-----------------+----------------------------+
| P0.5      | GPIO            | LED6                       |
+-----------+-----------------+----------------------------+
| P0.6      | SWD IO          | KitProg3 SWDIO             |
+-----------+-----------------+----------------------------+
| P0.7      | SWD CLK         | KitProg3 SWDCK             |
+-----------+-----------------+----------------------------+

System Clock
============

The IMO (Internal Main Oscillator) is the default system clock source:

- **IMO**: 48 MHz
- **CLK_HF**: 48 MHz (system clock)

Serial Port
===========

The PSOC™ 4 HVPA-SPM 1.0 has two SCB (Serial Communication Block) interfaces that can be
configured as UART, SPI, or I2C. The Zephyr console output is assigned to **SCB0**
(``uart0``), which is routed through the KitProg3 USB-UART bridge.

Default communication settings are **115200 8N1**.

Supported SoCs
**************

The PSOC™ 4 HVPA-SPM 1.0 SoC series (``psoc4hvpaspm10``) is defined for all six
48-QFN SPM 1.0 part numbers. They share the same memories, peripherals and package pinout and
differ only in the optional isoUART, CAN FD and Crypto blocks. Only CY8C4147LDE-HV734 is
used by a board in Zephyr.

+--------------------+-------------+---------+--------+--------------------+
| MPN                | isoUART     | CAN FD  | Crypto | Silicon ID         |
+====================+=============+=========+========+====================+
| CY8C4147LDE-HV504  | no          | yes     | no     | 0x370011C7         |
+--------------------+-------------+---------+--------+--------------------+
| CY8C4147LDE-HV534  | no          | yes     | yes    | 0x370111C7         |
+--------------------+-------------+---------+--------+--------------------+
| CY8C4147LDE-HV604  | yes         | no      | no     | 0x370211C7         |
+--------------------+-------------+---------+--------+--------------------+
| CY8C4147LDE-HV634  | yes         | no      | yes    | 0x370311C7         |
+--------------------+-------------+---------+--------+--------------------+
| CY8C4147LDE-HV704  | yes         | yes     | no     | 0x370411C7         |
+--------------------+-------------+---------+--------+--------------------+
| CY8C4147LDE-HV734  | yes         | yes     | yes    | 0x370511C7         |
+--------------------+-------------+---------+--------+--------------------+

The PSOC™ 4 HVPA-SPM 2.0 part numbers (CY8C4147LDE-HVA04 to -HVC34) are not supported: the
Peripheral Driver Library has no device support for them.

Building
********

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_psoc_hvpa_spm1_lite
   :goals: build

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The kit includes an onboard programmer/debugger (`KitProg3`_) which can be used to program
and debug the PSOC™ 4 HVPA-SPM 1.0 Cortex-M0+ core through Infineon OpenOCD.

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

Connect a USB cable from your PC to the KitProg3 USB Micro-B connector on the kit. Use the
serial terminal of your choice (minicom, PuTTY, etc.) with the following settings:

- **Speed:** 115200
- **Data:** 8 bits
- **Parity:** None
- **Stop bits:** 1

Flashing
========

One time, set the Infineon OpenOCD path:

.. code-block:: shell

   west config build.cmake-args -- -DOPENOCD=path/to/infineon/openocd/bin/openocd

Build and flash the application:

.. code-block:: shell

   west build -b kit_psoc_hvpa_spm1_lite -p always samples/hello_world
   west flash

You should see the following message on the console:

.. code-block:: console

   *** Booting Zephyr OS build vX.Y.Z ***
   Hello World! kit_psoc_hvpa_spm1_lite/cy8c4147lde_hv734

Debugging
=========

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: kit_psoc_hvpa_spm1_lite
   :goals: debug

References
**********

.. _ModusToolbox™ Programming Tools:
    https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolboxprogtools

.. _Infineon OpenOCD:
    https://github.com/Infineon/openocd/releases/latest

.. _KitProg3:
    https://github.com/Infineon/KitProg3
