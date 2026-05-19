.. zephyr:board:: dnn647

Overview
********

The Alientek DNN647 is a feature-rich development board built around the
STM32N647X0H3Q microcontroller (Arm® Cortex®-M55, 800 MHz, 4.2 MB SRAM).
It combines an 800×480 RGB LCD panel, 32 MB NOR Flash (XIP-capable), 32 MB
HyperRAM, and a full set of expansion interfaces.

The Zephyr port supports a complete MCUBoot XIP boot flow:

- **ROM Bootloader** loads the signed FSBL (MCUBoot) from NOR Flash into SRAM
- **MCUBoot FSBL** verifies the XIP application with RSA-2048 and jumps to it
- **XIP Application** runs code directly from NOR Flash; data/stack in AXISRAM1

Hardware
********

- STM32N647X0H3Q, Arm® Cortex®-M55, 800 MHz, VFBGA264 package
- 4.2 MB on-chip SRAM (AXISRAM1 2 MB + AXISRAM2 512 KB + FLEXRAM 1.6 MB)
- 32 MB Macronix MX25UM25645G Octo-SPI NOR Flash (XSPI2, 0x70000000)
- 32 MB HyperRAM (XSPI1, 0x90000000)
- 800×480 RGB888 LCD with backlight control (PA3)
- USB Full-Speed device (USB-SLAVE connector for DFU download)
- USART1 console via USB-UART bridge (PE5/PE6, 115200 baud)
- User LED0 (PG10) and LED1 (PE10), active-high
- User button WK_UP (PC13, active-high, pull-down) — also MCUBoot serial recovery trigger
- JTAG/SWD debug header

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Default Zephyr peripheral mapping:

- USART1_TX : PE5
- USART1_RX : PE6
- LED0       : PG10
- LED1       : PE10
- WK_UP      : PC13
- XSPI2_NCS1 : PN1
- XSPI2_DQS0 : PN0
- XSPI2_CLK  : PN6
- XSPI2_IO0  : PN2
- XSPI2_IO1  : PN3
- XSPI2_IO2  : PN4
- XSPI2_IO3  : PN5
- XSPI2_IO4  : PN8
- XSPI2_IO5  : PN9
- XSPI2_IO6  : PN10
- XSPI2_IO7  : PN11
- LTDC_CLK   : PA5
- LTDC_HSYNC : PF9
- LTDC_VSYNC : PG0
- LTDC_DE    : PG13
- LTDC_BL    : PA3

System Clock
============

The DNN647 system clock is driven by a 48 MHz external crystal (HSE) with
internal 2× pre-divider, giving a 24 MHz PLL1 input. PLL1 is configured for a
2400 MHz VCO:

- **CPU (IC1)**: 2400 / 3 = **800 MHz**
- **System bus (IC2)**: 2400 / 6 = **400 MHz**; AHB prescaler 2 → 200 MHz
- **LTDC pixel clock (IC16)**: 2400 / 72 ≈ **33.3 MHz** (≈60 Hz for 800×480)

Serial Port
===========

USART1 provides the Zephyr console at 115200 8N1 on PE5 (TX) / PE6 (RX),
connected to the on-board USB-UART bridge.

.. note::

   When MCUBoot serial recovery mode is active (WK_UP held at reset),
   ``CONFIG_UART_CONSOLE`` is disabled and the UART is used exclusively for
   the SMP protocol.  Normal boot produces no UART output in that build.

NOR Flash Partition Layout
==========================

The 32 MB MX25UM25645G NOR Flash is mapped at 0x70000000:

+--------------------+--------------------+----------+---------------------+
| Partition          | Address            | Size     | Contents            |
+====================+====================+==========+=====================+
| boot_partition     | 0x70000000         | 64 KB    | MCUBoot FSBL        |
+--------------------+--------------------+----------+---------------------+
| slot0_partition    | 0x70010000         | 1.5 MB   | Primary app (XIP)   |
+--------------------+--------------------+----------+---------------------+
| slot1_partition    | 0x70210000         | 1.5 MB   | OTA update image    |
+--------------------+--------------------+----------+---------------------+
| storage_partition  | 0x70410000         | 64 KB    | Settings / NVS      |
+--------------------+--------------------+----------+---------------------+

Board Variants
**************

Three variants are available:

- **dnn647** (default): XIP application, chain-loaded by MCUBoot from
  ``slot0_partition``.  Must be built with ``--sysbuild``.  Code runs
  directly from NOR Flash; data/stack in AXISRAM1 (2 MB, 0x34000000).

- **dnn647/stm32n647xx/fsbl**: First Stage Boot Loader variant.  Intended
  for MCUBoot.  Runs from AXISRAM2 (ROM Bootloader copies it there).
  ``CONFIG_XIP=n``, ``CONFIG_TRUSTED_EXECUTION_SECURE=y``.  The build
  system auto-signs the binary as FSBL type via ``STM32_SigningTool_CLI``.

- **dnn647/stm32n647xx/sb**: Serial-Boot variant, functionally identical
  to ``fsbl`` but loaded directly into SRAM over USB without changing BOOT
  pins.  Useful for rapid development iterations.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The DNN647 board is programmed with `STM32CubeProgrammer`_ (version 2.18.0
or later required).  Ensure ``STM32_Programmer_CLI`` is in ``PATH``.

The board uses a Macronix external NOR Flash.  Place the external loader
``MX25UM25645G_ATK-CNN647B_ExtMemLoader.stldr`` in the
``<CubeProgrammer>/bin/ExternalLoader/`` directory before flashing.

.. note::

   The STM32N6 ROM Bootloader verifies the FSBL image signature before
   loading it.  The build system generates ``zephyr.bin`` (raw) for MCUBoot
   and ``zephyr.signed.bin`` (MCUBoot-signed) for the application.

Boot Pin Configuration
======================

+------------------+-----------+-----------+-------------------------------+
| Mode             | BOOT0     | BOOT1     | Description                   |
+==================+===========+===========+===============================+
| Normal run       | 0 (Low)   | 0 (Low)   | ROM BL boots from NOR Flash   |
+------------------+-----------+-----------+-------------------------------+
| USB DFU download | 1 (High)  | 0 (Low)   | ROM BL enters USB DFU mode    |
+------------------+-----------+-----------+-------------------------------+

Flashing
========

Full application (MCUBoot + App) using sysbuild:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: dnn647
   :west-args: --sysbuild
   :goals: build

After building, flash MCUBoot and the application separately using
``STM32_Programmer_CLI`` (run from the ``zephyrproject`` directory):

.. tabs::

   .. group-tab:: Step 1 — Flash MCUBoot FSBL

      Set BOOT0=1, BOOT1=0, connect USB-SLAVE, then run:

      .. code-block:: console

         STM32_Programmer_CLI -c port=usb1 -d build\mcuboot\zephyr\zephyr.bin 0x70000000

      Set BOOT0=0, BOOT1=0 and power-cycle to continue.

   .. group-tab:: Step 2 — Flash Application

      .. code-block:: console

         STM32_Programmer_CLI -c port=usb1 -d build\app\zephyr\zephyr.signed.bin 0x70010000 -hardRst

Serial Boot variant (no BOOT pin changes required):

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: dnn647//sb
   :goals: build flash

MCUBoot Serial Recovery (OTA upload)
=====================================

The MCUBoot FSBL supports serial recovery over USART1 using the SMP protocol.

**Entering serial recovery mode:**

1. Hold the **WK_UP** button (PC13).
2. Press **RESET** (or power-cycle).
3. Keep WK_UP held for at least **500 ms**, then release.

**Install mcumgr:**

.. code-block:: console

   go install github.com/apache/mynewt-mcumgr-cli/mcumgr@latest

**Usage:**

.. code-block:: console

   # Configure connection (replace COM3 with actual port)
   mcumgr conn add dnn647 type=serial connstring="dev=COM3,baud=115200"

   # List current images
   mcumgr -c dnn647 image list

   # Upload new application to OTA slot
   mcumgr -c dnn647 image upload build\app\zephyr\zephyr.signed.bin

   # Mark slot1 for next boot and reset
   mcumgr -c dnn647 image test <slot1-hash>
   mcumgr -c dnn647 reset

Debugging
=========

Connect a SWD debugger to the debug header.  Set BOOT0=0, BOOT1=1 before
powering the board to enable JTAG/SWD access.

To debug the full MCUBoot + XIP app stack:

.. code-block:: console

   west debug --domain mcuboot

To add application symbols for chain-loaded debugging:

.. code-block:: console

   (gdb) b do_boot
   (gdb) c
   (gdb) add-symbol-file build/hello_world/zephyr/zephyr.elf
   (gdb) b main
   (gdb) c

References
**********

.. target-notes::

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html

- `Alientek DNN647 product page <https://wiki.alientek.com/docs/Boards/STM32/DNN647/start-guide/stm32n647-board-introduction/>`_
- `STM32N647XX reference manual <https://www.st.com/resource/en/reference_manual/rm0486-stm32n647657xx-armbased-32bit-mcus-stmicroelectronics.pdf>`_
- `STM32N657X0 datasheet <https://www.st.com/en/microcontrollers-microprocessors/stm32n657x0.html>`_
