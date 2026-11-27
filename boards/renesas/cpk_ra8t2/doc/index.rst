.. zephyr:board:: cpk_ra8t2

Overview
********

The `Renesas CPK-RA8T2 development kit`_ combines the CPKNET-RA8T2 v1.1 core
board and the CPKEXP-ECSMCB v1.0 carrier board. Its R7KA8T2LFLCAC MCU has an
Arm Cortex-M85 (CPU0) running at up to 1 GHz and a Cortex-M33 (CPU1) running
at up to 250 MHz.

Hardware
********

The core board provides a 24 MHz main crystal, 16 MiB of SDRAM, 32 MiB of
QSPI NOR flash, a microSD slot, and an onboard SEGGER J-Link debugger with
a virtual serial port.

The carrier provides CAN FD, RS-485, two EtherCAT ports, two motor inverter
interfaces, and connectors for Pmod modules and an optional LCD. See the
`carrier board schematic`_ and the `core board hardware guide`_ for the wiring.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and I/Os
====================

The CM85 console uses SCI9 through the J-Link virtual serial port. The green
user LED on P110 is active high and available as ``led0``. The core board USER
button on P303 is available as ``sw0``. The carrier board USER button (SW600)
on P903 is available as ``sw1``. Both buttons are active low.

The CM33 console uses SCI1 on the carrier's J609 connector. Connect a 3.3 V
USB-to-UART adapter's RX to J609 pin 3 (P707, board TX), TX to J609 pin 2
(P706, board RX), and GND to J609 pin 1. Both consoles use 115200 baud,
8 data bits, no parity, and one stop bit.

CM33 applications can enable the AGT0 counter (``agt0`` and ``counter0``) in
a devicetree overlay.

CM85 applications can enable the SCI4 SPI bus (``sci4`` and ``spi4``) in a
devicetree overlay. SCI4 is wired to J202 on the core board. Pins 1 through 4
are chip select, MOSI, MISO, and SCK, respectively. For the SPI loopback test,
connect J202 pins 2 and 3.

The CM85 CAN FD controller uses the carrier's J602 connector. J602 pin 1 is
CANH, pin 2 is CANL, and pin 3 is ground.

Dual Core Operation
*******************

CM85 is the boot core. When CM33 is the application target, sysbuild creates
a CM85 launcher image with ``CONFIG_SOC_RA_ENABLE_START_SECOND_CORE`` enabled.
Flash both images so CM85 can initialize the system and start CM33 on reset.

Memory Usage
============

.. list-table:: Default on-chip memory layout
   :header-rows: 1

   * - Core
     - MRAM base address
     - MRAM size
     - SRAM base address
     - SRAM size
   * - CM85 (CPU0)
     - ``0x02000000``
     - 768 KiB
     - ``0x22000000``
     - 1024 KiB
   * - CM33 (CPU1)
     - ``0x020c0000``
     - 256 KiB
     - ``0x22100000``
     - 640 KiB

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

With the motor inverter boards disconnected, connect the host to the USB-C
port marked ``JDBG`` to power the kit and access the onboard J-Link. The orange
debug LED stays on after USB enumeration.

Build and flash Hello World for CM85.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: cpk_ra8t2/r7ka8t2lflcac/cm85
   :build-dir: cpk-cm85
   :goals: build flash

Reset the kit and check the J-Link virtual serial port for the Hello World message.

.. code-block:: console

   Hello World! cpk_ra8t2/r7ka8t2lflcac/cm85

For CM33, build and flash Hello World and the CM85 launcher with sysbuild.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: cpk_ra8t2/r7ka8t2lflcac/cm33
   :build-dir: cpk-cm33
   :west-args: --sysbuild
   :goals: build flash

Reset the kit and check the serial port connected to J609 for the Hello World message.

.. code-block:: console

   Hello World! cpk_ra8t2/r7ka8t2lflcac/cm33

Debugging
=========

Use the build directory for the core you want to debug. For the CM33 example,
flash both images before starting the debug session for the application domain.

.. code-block:: console

   west debug -d build/cpk-cm85
   west debug -d build/cpk-cm33 --domain hello_world

References
**********

* `RA8T2 Group User's Manual`_
* `Renesas CPK-RA8T2 development kit`_
* `core board hardware guide`_
* `carrier board schematic`_

Board photograph from the `Renesas CPK-RA8T2 documentation repository`_.

.. _RA8T2 Group User's Manual:
   https://www.renesas.com/en/document/mah/ra8t2-group-users-manual-hardware

.. _Renesas CPK-RA8T2 development kit:
   https://github.com/renesas/cpk_examples/blob/main/cpk_ra8t2/docs/21_overview.adoc

.. _carrier board schematic:
   https://github.com/renesas/cpk_examples/blob/main/cpk_ra8t2/docs/cpkexp_ecsmcb_v1.0_sch_release.pdf

.. _core board hardware guide:
   https://github.com/renesas/cpk_examples/blob/main/cpknet_ra8t2/docs/03_hardware.adoc

.. _Renesas CPK-RA8T2 documentation repository:
   https://github.com/renesas/cpk_examples/blob/main/cpk_ra8t2/cpk_ra8t2.png
