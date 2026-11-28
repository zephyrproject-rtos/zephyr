.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: luckfox_lume

Overview
********

Luckfox Lume is a development board based on the Allwinner T153 SoC. It has
quad-core Cortex-A7 processors, a XuanTie E907 RISC-V core, 128 MiB DDR3,
256 MiB SPI NAND, and a 40-pin expansion header.

The ``luckfox_lume/t153/e907`` target runs Zephyr on the E907. Linux on the
Cortex-A7 loads the application through the vendor remoteproc driver.
The ``luckfox_lume/t153/a7`` target runs Zephyr on a single Cortex-A7 core,
booted from U-Boot instead of Linux.
See the `Luckfox Lume overview`_ for more information about the board.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

The E907 target uses 3 MiB of reserved DRAM at ``0x421c5000`` and UART3 as
the console. The Cortex-A7 target uses 22 MiB of DRAM at ``0x40a00000`` and
UART0 as the console. The ``mmio-sram`` entries in the table refer to these
DRAM regions.

Connections and IOs
===================

Power the board through USB-C and use a USB-to-TTL adapter with 3.3 V signals,
leaving the adapter's power output disconnected. Configure the serial terminal
for 115200 baud, 8 data bits, no parity, one stop bit, and no flow control.
Use the `Luckfox UART pinout`_ to locate the physical pins on the 40-pin
header.

* **E907** uses UART3 (separate from the Linux debug console).
* **Cortex-A7** uses UART0 (shared with U-Boot and the Linux debug console).

.. list-table:: UART wiring (physical 40-pin header numbers)
   :header-rows: 1

   * - Target
     - Board pin
     - Board signal
     - Adapter signal
   * - E907 (UART3)
     - 15
     - PD14 / UART3 TX
     - RX
   * - E907 (UART3)
     - 13
     - PD15 / UART3 RX
     - TX
   * - Cortex-A7 (UART0)
     - 8
     - PB9 / UART0 TX
     - RX
   * - Cortex-A7 (UART0)
     - 10
     - PB10 / UART0 RX
     - TX
   * - Both
     - 6
     - Ground
     - Ground

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Install the vendor Linux image following `Luckfox OS installation`_.
E907 applications are loaded through Linux remoteproc, as described in
`Luckfox RISC-V subsystem`_. Cortex-A7 applications are loaded from U-Boot.

Building and loading (E907)
===========================

From the Zephyr repository, build the :zephyr:code-sample:`hello_world` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: luckfox_lume/t153/e907
   :goals: build

Load ``build/zephyr/zephyr.elf``. The vendor remoteproc driver reads its load
addresses, entry point, and ``.resource_table`` section. A raw binary
(``zephyr.bin``) cannot be used with this loading method.

The following example uses ADB to transfer the image and open a Linux shell:

.. code-block:: console

   adb devices -l
   export ANDROID_SERIAL='<serial-from-adb-devices>'
   adb push build/zephyr/zephyr.elf /tmp/luckfox-lume-zephyr.elf
   adb shell

Run the remaining commands as root in the same board-side shell. SSH or
the Linux UART0 console can also be used, with the ELF copied separately.
Find the remoteproc device named ``e907_rproc``; its number may vary:

.. code-block:: sh

   for dev in /sys/class/remoteproc/remoteproc*; do
       printf '%s: ' "$dev"
       cat "$dev/name"
   done
   rproc=/sys/class/remoteproc/remoteproc0  # Use the discovered e907_rproc.
   cat "$rproc/state"

The core must be ``offline`` before loading. If it is ``running``, stop it
with ``echo stop > "$rproc/state"``.

Open the UART3 serial terminal. Copy the ELF to ``/lib/firmware`` and start
Zephyr:

.. code-block:: sh

   cp /tmp/luckfox-lume-zephyr.elf /lib/firmware/luckfox-lume-zephyr.elf
   echo luckfox-lume-zephyr.elf > "$rproc/firmware"
   echo start > "$rproc/state"

The UART3 console should display:

.. code-block:: text

   *** Booting Zephyr OS build ... ***
   Hello World! luckfox_lume/t153/e907

To stop Zephyr:

.. code-block:: sh

   echo stop > "$rproc/state"

To run another build, copy the new ELF and repeat the loading steps.

Building and loading (Cortex-A7)
================================

The Cortex-A7 target boots from the stock vendor U-Boot. Build the
:zephyr:code-sample:`hello_world` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: luckfox_lume/t153/a7
   :goals: build

Package ``build/zephyr/zephyr.bin`` as a legacy U-Boot image using ``mkimage``:

.. code-block:: console

   mkimage -A arm -O linux -T kernel -C none \
       -a 0x40a00000 -e 0x40a00000 -n "Zephyr" \
       -d build/zephyr/zephyr.bin build/zephyr/zephyr.uImage

Interrupt autoboot on the UART0 console to reach the U-Boot prompt and start a
YMODEM transfer:

.. code-block:: console

   => loady 0x40000000 115200

Send ``build/zephyr/zephyr.uImage`` from your serial terminal, then boot it
(the vendor image keeps its FDT at ``0x42000000``, which can be checked with
``fdt addr``):

.. code-block:: console

   => bootm 0x40000000 - 0x42000000

U-Boot requires the FDT argument for ``bootm``, while Zephyr uses its own
built-in devicetree.

The UART0 console should display:

.. code-block:: text

   *** Booting Zephyr OS build ... ***
   Hello World! luckfox_lume/t153/a7

This loads Zephyr into RAM only; reset the board and interrupt autoboot to
run another build.

Debugging
=========

Use UART3 and Linux ``dmesg`` to diagnose E907 remoteproc loading errors, and
UART0 for U-Boot and Cortex-A7 console output. A ``west debug`` runner is not
provided, and JTAG debugging has not been validated.

References
**********

* `Luckfox Lume overview`_
* `Luckfox UART pinout`_
* `Luckfox OS installation`_
* `Luckfox RISC-V subsystem`_
* `Luckfox hardware resources`_

.. _Luckfox Lume overview: https://wiki.luckfox.com/Luckfox-Lume/Introduction/
.. _Luckfox UART pinout: https://wiki.luckfox.com/Luckfox%20Lume/Lume-pinout/UART/
.. _Luckfox OS installation: https://wiki.luckfox.com/Luckfox-Lume/Getting-Started/
.. _Luckfox RISC-V subsystem: https://wiki.luckfox.com/Luckfox-Lume/RV-Subsystem/
.. _Luckfox hardware resources: https://github.com/LuckfoxTECH/Luckfox-Lume-docs
