.. zephyr:board:: am62x_m4_bl350

Overview
********

The BLIIoT BL352BW (AM625SIP Cortex-M4F) is part of the BL350 series of
Industrial Embedded Controllers powered by the TI Sitara AM62x (AM6254) SoC.
It features a quad-core Cortex-A53 cluster (up to 1.4 GHz) and a dedicated
Cortex-M4F MCU core. Built for robust industrial automation, it supports a
wide operating temperature range of -40°C to 85°C.

The Zephyr port targets the **Cortex-M4F MCU domain**, providing
real-time deterministic performance for industrial I/O control while the main
Cortex-A53 cluster runs Linux. Communication between the two domains is handled
via OpenAMP/RPMsg IPC over a shared DDR carveout.

.. figure:: img/ARMxy-3-1.webp
   :align: center
   :alt: BLIIoT BL350 Industrial Controller

   BLIIoT BL350 Industrial Controller

Hardware
********

The board is based on the TI AM6254 SoC:

- **CPU (Linux domain):** Quad-core ARM Cortex-A53 @ up to 1.4 GHz
- **MCU (Zephyr domain):** ARM Cortex-M4F @ 400 MHz
- **Memory:**

  - 192 KB SRAM (MCU domain)
  - Up to 2 GB DDR4 (shared; 4 MB carved out for IPC, 2 MB for M4 code/data)
  - Up to 8 GB eMMC (Linux)

- **Industrial I/O:** RS232/RS485, CAN FD (×2), DI/DO, RTD, Thermocouple,
  configurable X/Y-Series expansion modules

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The BL350 uses X-Series and Y-Series I/O expansion connectors for
RS232/RS485, CAN, DI/DO, RTD, TC, and more. The Zephyr M4F image communicates
with the A53 Linux application layer through the RPMsg IPC channel.

System Clock
------------

The M4F MCU domain runs at **400 MHz**, configured via
``CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC=400000000``.

Serial Port
-----------

The default Zephyr console uses the MCU domain UART (``uart0``). The main
domain UARTs (``main_uart0``, ``main_uart1``, ``main_uart2``) are also
available for application use (e.g., Modbus over ``main_uart0``).

Programming and Debugging
*************************

Building
========

Build Zephyr applications for the M4F core using the board qualifier
``am62x_m4_bl350/am6234/m4``:

.. code-block:: console

   west build -b am62x_m4_bl350/am6234/m4 samples/hello_world

Flashing
========

The M4F binary is loaded by the A53 Linux cores using the ``remoteproc``
framework. Transfer the compiled ELF to the target filesystem and start
the core via ``sysfs``:

.. code-block:: console

   # On the target Linux system (as root):
   cp zephyr.elf /lib/firmware/
   echo zephyr.elf > /sys/class/remoteproc/remoteproc0/firmware
   echo start      > /sys/class/remoteproc/remoteproc0/state

To stop the core:

.. code-block:: console

   echo stop > /sys/class/remoteproc/remoteproc0/state

Debugging
=========

Crash and log output from the M4F core is available via the MCU UART
(``/dev/ttyS0`` or similar on the Linux side at 115200 baud).

For JTAG-based debugging, attach an XDS110 or compatible probe to the
AM62x JTAG header and use Code Composer Studio (CCS) or OpenOCD with a
TI-specific target configuration. The M4F core is accessible as the
``Cortex_M4_0`` target within the AM62x debug chain.

References
**********

- `BLIIoT BL350 Product Page <https://bliiot.com/products/industrial-controller-bl350>`_
- `TI AM62x Technical Reference Manual <https://www.ti.com/lit/ug/spruj40c/spruj40c.pdf>`_
- `TI AM62x Datasheet <https://www.ti.com/lit/ds/symlink/am6254.pdf>`_
- `Zephyr remoteproc documentation <https://docs.zephyrproject.org/latest/services/ipc/openamp.html>`_
