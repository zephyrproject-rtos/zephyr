.. zephyr:board:: s32k5xxcvb

Overview
********

The NXP S32K5XXCVB board is based on the `NXP S32K5 Series`_ family of automotive microcontrollers (MCUs),
which expands the S32K3 series to deliver higher performance, larger memory, and increased vehicle network
communication capability, all with lower power consumption.

The S32K5 MCU features:
- Compute-focused Arm Cortex-R52 cores
- Platform-focused Arm Cortex-M7 cores
- Low-power-focused Arm Cortex-M4 core

Zephyr OS is ported to run on both the Cortex-M7 and Cortex-R52 cores.

- ``s32k5xxcvb/s32k566/m7``, for S32K566 Cortex-M7, code executed from code MRAM by default.
- ``s32k5xxcvb/s32k566/r52``, for S32K566 Cortex-R52, code executed from code MRAM by default.

Hardware
********

- NXP S32K566
    - Arm Cortex-M7 (6 cores with two options 3 x Lock-Step or 2 x Lock-Step + 2 x Single-Core,
      option 2 is configured by default).
    - Arm Cortex-R52 (contains an R52 cluster with two Cortex-R52 cores).
    - 1.5 MB SRAM for Cortex-M7, with ECC
    - 1 MB of SRAM connected to the Cortex-R52, with ECC
    - 32 MB of Code Magnetic RAM for all cores, with ECC
    - Ethernet switch integrated, CAN FD/XL, QSPI
    - 14-bit 3 Msps ADC, 24-bit eMIOS timer and more.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The Port columns in the Reference Manual (RM) are organized into ports and pins to ensure consistency with
the GPIO driver used in this Zephyr port.

The table below summarizes the mapping between the Port columns in the RM and Zephyr's ports and pins.
Please consider this mapping when using the GPIO driver or configuring pin multiplexing for device drivers.

+-------------------+--------------------+
|  Ports in RM      | Zephyr Ports/Pins  |
+===================+====================+
| GPIO0 - GPIO15    | PA0 - PA15         |
+-------------------+--------------------+
| GPIO16 - GPIO31   | PB0 - PB15         |
+-------------------+--------------------+
| GPIO32 - GPIO47   | PC0 - PC15         |
+-------------------+--------------------+
| GPIO48 - GPIO63   | PD0 - PD15         |
+-------------------+--------------------+
| GPIO64 - GPIO78   | PE0 - PE14         |
+-------------------+--------------------+
| GPIO80 - GPIO95   | PF0 - PF15         |
+-------------------+--------------------+
| GPIO96 - GPIO111  | PG0 - PG15         |
+-------------------+--------------------+
| GPIO112 - GPIO127 | PH0 - PH15         |
+-------------------+--------------------+
| GPIO128 - GPIO138 | PI0 - PI10         |
+-------------------+--------------------+
| GPIO160 - GPIO175 | PK0 - PK15         |
+-------------------+--------------------+
| GPIO176 - GPIO191 | PL0 - PL15         |
+-------------------+--------------------+
| GPIO192 - GPIO207 | PM0 - PM15         |
+-------------------+--------------------+
| GPIO208 - GPIO223 | PN0 - PN15         |
+-------------------+--------------------+
| GPIO224 - GPIO239 | PO0 - PO15         |
+-------------------+--------------------+
| GPIO240 - GPIO254 | PP0 - PP14         |
+-------------------+--------------------+
| GPIO256 - GPIO263 | PQ0 - PQ7          |
+-------------------+--------------------+
| GPIO304 - GPIO319 | PT0 - PT15         |
+-------------------+--------------------+
| GPIO320 - GPIO335 | PU0 - PU15         |
+-------------------+--------------------+
| GPIO336 - GPIO351 | PV0 - PV15         |
+-------------------+--------------------+
| GPIO352 - GPIO363 | PW0 - PW11         |
+-------------------+--------------------+

LEDs
----

The board has three user RGB LEDs:

=======================  =====  ====
Devicetree node          Color  Pin
=======================  =====  ====
led0 / user_led_red      Red    PP1
led1 / user_led_green    Green  PO2
led2 / user_led_blue     Blue   PN10
=======================  =====  ====

The user can control the LEDs in any way. An output of ``1`` illuminates the LED.

Buttons
-------

The board has two user buttons:

=======================  =======  ===
Devicetree node          Label    Pin
=======================  =======  ===
sw0 / user_button_0      USERSW0  PF3
sw1 / user_button_1      USERSW1  PF9
=======================  =======  ===

System Clock
============

- The Arm Cortex-M7: 200 MHz.
- The Arm Cortex-R52: 800 MHz.

Set-up the Board
================

Connect the external debugger probe to the board's JTAG Cortex ETM connector (``J17``)
and to the host computer via USB or Ethernet, as supported by the probe.

NXP S32K5XX-MB Shield
=====================

This Zephyr shield, :ref:`nxp_s32k5xx_mb` expands the board's available I/O connectivity

The shield provides access to the following feature:
- Serial UART

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``s32k5xxcvb`` board can be built in the usual way as
documented in :ref:`build_an_application`.

This board supports West runners for the following debug tools:

- :ref:`Lauterbach TRACE32 <lauterbach-trace32-debug-host-tools>`

Follow the installation steps of the debug tool you plan to use before loading
your firmware.

Debugging
=========

You can build and debug the :zephyr:code-sample:`hello_world` sample for the board
``s32k5xxcvb/s32k566/m7`` with:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: s32k5xxcvb/s32k566/m7
   :goals: build debug

Flashing
========

Follow these steps if you just want to download the application to the board and run.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: s32k5xxcvb/s32k566/m7
   :goals: build flash
   :compact:

Core selection
==============

This Zephyr port can only run single core. By default, Zephyr runs on the first core
of the selected board. To execute Zephyr application on other core, all runner parameters
must be overridden from command line:

.. code-block:: console

   west <debug/flash> --startup-args elfFile=<elf_path> coreType=<m7/r52> loadTo=<mram/sram> core=<core_id>

Where ``<elf_path>`` is the path to the Zephyr application ELF in the output
directory.

.. include:: ../../common/board-footer.rst.inc

References
**********

.. target-notes::

.. _NXP S32K5 Series:
   https://www.nxp.com/products/S32K5
