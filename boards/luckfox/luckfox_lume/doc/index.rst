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
See the `Luckfox Lume overview`_ for more information about the board.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

This board configuration uses 3 MiB of reserved DRAM at ``0x421c5000`` and
UART3 as the console. The ``mmio-sram`` entry in the table refers to this
DRAM region.

Zephyr support is limited to the E907 core and the features listed above.
RPMsg and shared-memory logging with Linux are not supported. UART3 uses
fixed pins and clocks; runtime pin or clock reconfiguration and power
management are not supported.

Connections and IOs
===================

Power the board through USB-C and use a USB-to-TTL adapter with 3.3 V signals.
Connect TX, RX, and ground as shown below, leaving the adapter's power
output disconnected.

.. list-table:: UART3 wiring (physical 40-pin header numbers)
   :header-rows: 1

   * - Board pin
     - Board signal
     - Adapter signal
   * - 15
     - PD14 / UART3 TX
     - RX
   * - 13
     - PD15 / UART3 RX
     - TX
   * - 6
     - Ground
     - Ground

Configure the serial terminal for 115200 baud, 8 data bits, no parity,
one stop bit, and no flow control. Use the `Luckfox UART pinout`_ to locate
the physical pins. Linux's debug console uses UART0 on PB9/PB10 (pins 8/10),
separate from Zephyr's UART3 console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Install the vendor Linux image following `Luckfox OS installation`_.
Applications are loaded through Linux remoteproc, as described in
`Luckfox RISC-V subsystem`_.

Building and loading
====================

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

Debugging
=========

Use UART3 for application output and Linux ``dmesg`` to diagnose remoteproc
loading errors. A ``west debug`` runner is not provided, and JTAG debugging
has not been validated.

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
