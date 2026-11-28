.. SPDX-License-Identifier: Apache-2.0
.. SPDX-FileCopyrightText: Copyright (c) 2026 Filip Stojanovic <filipembedded@gmail.com>

.. zephyr:board:: frdm_imx8mp

Overview
********

The NXP FRDM-IMX8MPLUS development board uses the i.MX 8M Plus applications
processor, with four Arm Cortex-A53 cores and one Arm Cortex-M7 core. This board
port supports Zephyr on the Cortex-M7 core, running at 800 MHz. The Cortex-A53
cores initialize the system through U-Boot and can run Linux alongside Zephyr.

See the `FRDM-IMX8MPLUS product page`_ and `NXP getting started guide`_ for board
information and boot switch settings.

Supported Features
******************

.. zephyr:board-supported-hw::

The supported board targets are:

* ``frdm_imx8mp/mimx8ml8/m7``: code in 128 KiB ITCM and data in 128 KiB DTCM.
* ``frdm_imx8mp/mimx8ml8/m7/ddr``: code in 2 MiB DDR at ``0x80000000`` and data
  in 2 MiB DDR at ``0x80200000``.

The ``zephyr,flash`` chosen node describes the code RAM region. Applications are
loaded into RAM by U-Boot, Linux remoteproc, or a debugger.

Serial Console
==============

The M7 console uses UART4 at 115200 baud, 8 data bits, no parity, and one stop bit,
without hardware flow control. Its pins are ``UART4_RXD`` and ``UART4_TXD``.
The board's USB debug interface provides separate serial ports for A53 and M7;
use the M7 port for Zephyr output. The A53 U-Boot/Linux console uses UART2.

Peripheral Connections
======================

The pin assignments follow NXP's `FRDM Linux device tree`_ and
`FRDM root device tree`_. The latter describes the native UART4 pads.

.. list-table:: M7 peripheral pins
   :header-rows: 1
   :widths: 20 30 50

   * - Peripheral
     - Signal
     - SoC pad
   * - UART4
     - RX / TX
     - UART4_RXD / UART4_TXD
   * - ECSPI2
     - SCLK / MOSI / MISO
     - ECSPI2_SCLK / ECSPI2_MOSI / ECSPI2_MISO
   * - ECSPI2
     - CS0, active low
     - ECSPI2_SS0 as GPIO5_IO13
   * - I2C3
     - SCL / SDA
     - I2C3_SCL / I2C3_SDA

ECSPI2 has no default child device. Add the attached device in an application
overlay and enable :kconfig:option:`CONFIG_SPI`. I2C3 is configured for 400 kHz;
enable :kconfig:option:`CONFIG_I2C` to use it. The WM8962 codec is connected to
I2C3 at address ``0x1a``, but is not instantiated by this board port.

The ``led0`` alias refers to the green status LED on pin 14 of the I2C1 PCAL6416
expander at address ``0x20``. The LED, expander, and I2C1 controller are disabled
by default because Linux uses the same bus and expander. The Blinky sample's
board overlays enable these nodes for both M7 targets. The expander uses
``no-auto-reset`` to preserve the other outputs configured by the boot firmware.

The user button uses GPIO3_IO14 and remains owned by Linux in NXP's FRDM RPMsg
configuration, so this port does not provide a ``sw0`` alias. The CAN transceiver
also depends on the I2C1 expander and is not enabled.

System Initialization and Linux Coexistence
===========================================

Boot the board through its FRDM U-Boot firmware before starting M7. U-Boot must
initialize the system clocks, DDR, and resource domain controller (RDC) access
permissions for the M7 peripherals. The SoC support sets the M7, UART4, and
ECSPI2 clock roots. I2C1 and I2C3 use the clock roots supplied by the boot firmware;
the clock driver supports the 24 MHz oscillator and System PLL1 DIV5 sources.

Linux and Zephyr must have exclusive ownership of each peripheral and its pins.
NXP's `FRDM RPMsg device tree`_ disables Linux access to UART4 and ECSPI2 and
removes the physical I2C3 controller. It reserves 16 MiB at ``0x80000000`` for M7,
covering both DDR regions used by this port. If using another Linux device tree,
apply equivalent resource exclusions and DDR reservations before running M7.

The RPMsg device tree also describes audio services provided by NXP's M7
firmware. Those services are not implemented by the Zephyr hello-world sample.
Enabling the messaging unit in this port does not itself implement RPMsg or
the audio services.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build Hello World
=================

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_imx8mp/mimx8ml8/m7
   :goals: build

For a DDR image, use ``frdm_imx8mp/mimx8ml8/m7/ddr`` as the board target.

Build Blinky
============

The :zephyr:code-sample:`blinky` sample automatically enables the ``led0``
hardware through
:zephyr_file:`samples/basic/blinky/boards/frdm_imx8mp_mimx8ml8_m7.overlay` and its
DDR counterpart.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: frdm_imx8mp/mimx8ml8/m7
   :build-dir: blinky
   :goals: build

Use ``frdm_imx8mp/mimx8ml8/m7/ddr`` for the DDR variant. For an initial hardware
test, load the image with U-Boot and leave the A53 cores at the U-Boot prompt.
NXP's unmodified FRDM RPMsg Linux device tree retains I2C1 and the PCAL6416
expander, along with the PMIC and USB-C controller on I2C1. Concurrent LED control
requires an ownership and inter-core communication arrangement; the standard
remoteproc recipe is intended for firmware that leaves these nodes disabled.

Load and Run with U-Boot
========================

Copy ``build/zephyr/zephyr.bin`` to a FAT partition on an SD card, then interrupt
U-Boot on the A53 console. The examples use MMC device 1, partition 1; adjust
``1:1`` for the boot media on your board.

For the ITCM target, stage the binary in DDR and copy it to the A53-visible
ITCM address ``0x007e0000``:

.. code-block:: console

   fatload mmc 1:1 0x48000000 zephyr.bin
   cp.b 0x48000000 0x7e0000 ${filesize}
   dcache flush
   bootaux 0x7e0000

For the DDR target:

.. code-block:: console

   fatload mmc 1:1 0x80000000 zephyr.bin
   dcache flush
   bootaux 0x80000000

UART4 should then print the hello-world message. To boot Linux afterward, select
the FRDM RPMsg device tree and prepare the M7 resources:

.. code-block:: console

   setenv fdtfile imx8mp-frdm-rpmsg.dtb
   run prepare_mcore
   boot

Load and Run with Linux Remoteproc
==================================

Before booting Linux, run these commands in U-Boot, as described in the
`NXP getting started guide`_:

.. code-block:: console

   setenv fdtfile imx8mp-frdm-rpmsg.dtb
   run prepare_mcore
   boot

Copy the ITCM build's ``zephyr.elf`` to ``/lib/firmware/zephyr.elf`` on the board,
then select and start it from the A53 Linux console:

.. code-block:: console

   echo zephyr.elf > /sys/class/remoteproc/remoteproc0/firmware
   echo start > /sys/class/remoteproc/remoteproc0/state

The example assumes ``remoteproc0`` is the i.MX M7 remote processor. Check its
``name`` and ``state`` files on your system before starting firmware.

For DDR firmware loaded through remoteproc, also enable
:kconfig:option:`CONFIG_ROMSTART_RELOCATION_ROM` to place the boot vectors in
ITCM. This option is not enabled in the DDR board defaults because it creates
a large gap in the raw binary. Build only the ELF output for this configuration:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_imx8mp/mimx8ml8/m7/ddr
   :build-dir: remoteproc
   :gen-args: -DCONFIG_ROMSTART_RELOCATION_ROM=y -DCONFIG_BUILD_OUTPUT_BIN=n
   :goals: build

Use ``build/remoteproc/zephyr/zephyr.elf`` with remoteproc.

Debug with J-Link
=================

Connect an external J-Link probe to the board's debug interface. Initialize the
board with U-Boot before loading an image, especially when using DDR.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_imx8mp/mimx8ml8/m7
   :goals: debug

.. include:: ../../common/board-footer.rst.inc

.. _FRDM-IMX8MPLUS product page:
   https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-IMX8MPLUS
.. _NXP getting started guide:
   https://www.nxp.com/document/guide/getting-started-with-frdm-imx8mplus:GS-FRDM-IMX8MPLUS
.. _FRDM Linux device tree:
   https://github.com/nxp-imx/linux-imx/blob/lf-6.18.y/arch/arm64/boot/dts/freescale/imx8mp-frdm.dts
.. _FRDM root device tree:
   https://github.com/nxp-imx/linux-imx/blob/lf-6.18.y/arch/arm64/boot/dts/freescale/imx8mp-frdm-root.dts
.. _FRDM RPMsg device tree:
   https://github.com/nxp-imx/linux-imx/blob/lf-6.18.y/arch/arm64/boot/dts/freescale/imx8mp-frdm-rpmsg.dts
