.. zephyr:board:: erbium

Overview
********

The AIFoundry Erbium board configuration is a minimal Zephyr port to a
single core on the 16-core Erbium platform. It exposes one 64-bit RISC-V hart,
an MRAM memory region and controller, a PLIC, a machine timer, and a polling
UART console. This configuration targets the original UART revision supported
by the public ``et-platform`` functional simulator.

Hardware
********

The board configuration describes the following hardware:

* 64-bit RISC-V hart with the ``rv64imfc_zicsr_zifencei`` ISA extensions
* 16 MiB MRAM memory region
* Platform-Level Interrupt Controller
* RISC-V machine timer
* Nekko Shakti UART, using 32-bit accesses at eight-byte register intervals

Supported Features
==================

.. zephyr:board-supported-hw::

The UART supports polling input and output at 115200 baud, 8 data bits, no
parity, and one stop bit. Interrupt-driven and asynchronous UART operations,
runtime line configuration, and hardware flow control are not implemented.

UART revision
=============

The ``aifoundry,shakti-uart-v1`` compatible identifies the original Erbium
integration described by the `original UART register documentation`_. It has
``InterruptEn`` at offset ``0x30`` and ``Rx_Threshold`` at offset ``0x40``.
The driver configures the baud divisor, line format, delay, interrupt enable,
and receive threshold during initialization. To transmit a character, it waits
for room in the transmit FIFO and writes the data register, following the
documented program flow.

The generic ``shakti,uart`` driver uses four-byte register intervals and
mixed 16-bit and 32-bit accesses. Later Erbium RTL also has a different
interrupt register layout, described in the `revised UART documentation`_.
Neither register layout is compatible with this board configuration.

Memory and boot requirements
============================

Zephyr executes from the MRAM window at ``0x40000200``. The first 512 bytes
of the 16 MiB window are reserved for the boot flow. The MRAM controller is
described separately at ``0x02001000``, with PLIC interrupt 1, following the
`CPU memory map`_ and `platform interrupt map`_. The data window is directly
readable, writable, and executable, so it is used as Zephyr's SRAM region.
MRAM controller initialization, ECC handling, and power management remain
the responsibility of the boot firmware; there is no Zephyr MRAM driver.

The boot firmware must enter hart 0 in machine mode with MRAM initialized,
the other harts parked, and the UART enabled through ``SystemConfig`` bit 6.
The board configuration assumes a 400 MHz UART input clock and a 2 MHz
machine timer. Adjust ``uart0.clock-frequency`` and ``/cpus/timebase-frequency``
in a devicetree overlay when the boot firmware uses different clocks.

Erbium does not implement ``WFI``. The SoC idle functions poll for an enabled
pending interrupt while global interrupts are masked, then restore interrupt
delivery. This preserves the idle wakeup contract without using ``WFI``.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

Applications for the ``erbium`` board configuration can be built in the
usual way. For example, with the :zephyr:code-sample:`hello_world` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: erbium
   :goals: build

Flashing and Running
====================

No upstream runner is provided for this board yet. Run the ELF with
``erbium_emu`` from `et-platform`_. The original UART model is available at
revision ``836a4ab600e93c3059bb58c898edbc37744cd8d0``. Build the repository's
``erbium-hal`` package and then ``sw-sysemu`` with that package on
``CMAKE_PREFIX_PATH``.

From the Zephyr application directory, with ``erbium_emu`` on ``PATH``:

.. code-block:: console

   erbium_emu -elf_load build/zephyr/zephyr.elf \
     -reset_pc 0x40000200 -minions 1 -single_thread \
     -mem_write32 0x02000008,0x44 -max_cycles 2000000 \
     -uart_rx_file /dev/null -uart_tx_file uart.log
   cat uart.log

The ``SystemConfig`` write enables the UART and keeps the watchdog disabled
in the simulator. Loading the ELF and setting its entry point bypasses ROM
boot. The UART log should contain ``Hello World! erbium/erbium``. The
simulator reports a cycle-limit error after the sample enters idle; judge
the sample by its UART output, not the simulator's exit code.

UART tests
==========

Build the polling API test with Twister:

.. code-block:: console

   ./scripts/twister -p erbium -T tests/drivers/uart/uart_basic_api \
     -s drivers.uart.basic_api.shakti_nekko --build-only

Run the resulting ``zephyr.elf`` with the same simulator options, increasing
``-max_cycles`` to ``10000000``. Supply a file containing a short line, for
example ``erbium-rx`` followed by a newline, through ``-uart_rx_file``. The
input must fit the 16-byte receive FIFO. Check that the line is echoed and
that the UART log ends with ``PROJECT EXECUTION SUCCESSFUL``. Runtime UART
configuration is unsupported; the generic test accepts that API result.

The functional model verifies software register accesses and console I/O.
It does not validate physical baud timing or silicon behavior.

References
**********

* `Erbium open-source processor <https://github.com/openhwfoundation/core-et-erbium>`_
* `original UART register documentation`_
* `revised UART documentation`_
* `CPU memory map`_
* `platform interrupt map`_
* `et-platform`_

.. _original UART register documentation:
   https://github.com/openhwfoundation/core-et-erbium/blob/325b32b7efaa2c2ab9c91001c89d3f49a4740826/doc/uart.md
.. _revised UART documentation:
   https://github.com/openhwfoundation/core-et-erbium/blob/afa22ae4d30ef5efe52ddf7a8ec7ddb55c3da21f/doc/uart.md
.. _CPU memory map:
   https://github.com/openhwfoundation/core-et-erbium/blob/325b32b7efaa2c2ab9c91001c89d3f49a4740826/doc/cpu_mm.md
.. _platform interrupt map:
   https://github.com/openhwfoundation/core-et-erbium/blob/325b32b7efaa2c2ab9c91001c89d3f49a4740826/doc/interrupts.md
.. _et-platform: https://github.com/aifoundry-org/et-platform
