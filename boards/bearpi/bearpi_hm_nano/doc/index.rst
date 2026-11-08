.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: bearpi_hm_nano

Overview
********

The BearPi-HM Nano uses a HiSilicon Hi3861 RISC-V Wi-Fi SoC. Initial
Zephyr support provides UART0, a system timer, and the Hi3861 local
interrupt controller. Wi-Fi, GPIO, flash programming, and a GDB runner
are not implemented.

The console is UART0 at 115200 baud, 8N1. GPIO3 is TX and GPIO4 is RX;
the board's CH340 USB serial bridge connects to these pins.

UART support uses the ``hisilicon,hi3861-uart`` driver rather than an
ARM PL011 compatible. It implements polling, interrupt-driven RX/TX,
error reporting, and runtime configuration. Supported formats are
5 to 8 data bits, one or two stop bits, and none, odd, even, mark, or
space parity. UART0 has no hardware RTS/CTS; requests to enable it are
rejected. UART1 and UART2 support RTS/CTS but remain disabled because
their board routing has not been enabled or validated. Asynchronous
DMA transfers are not implemented.

The TX interrupt requires a FIFO threshold crossing. A one-shot kernel
timer seeds the FIFO in ISR context when transmission starts from a
thread; subsequent transfers use UART interrupts. Interrupt-driven
operation therefore requires the system clock.

Supported Features
******************

.. zephyr:board-supported-hw::

UART validation
***************

The UART basic API keyboard-harness variant is
``drivers.uart.basic_api.hi3861``. It tests runtime configuration, polling,
interrupt-driven FIFO transfers, and receive-interrupt pending status.
Build it with:

.. code-block:: console

   west build -b bearpi_hm_nano tests/drivers/uart/uart_basic_api -- -DCONF_FILE=prj_hi3861.conf

Download it using the RAM runner described below. Send a newline-terminated
line when the test asks for characters. The separate interrupt API suite
tests transmission with increasing FIFO-fill sizes:

.. code-block:: console

   west build -b bearpi_hm_nano tests/drivers/uart/uart_interrupt_api

Neither suite requires connecting TX to RX; keep the CH340 console wiring.
Press RESET after testing to return to the factory firmware.

RAM execution
*************

The board boots through the Hi3861 ROM serial loader. The image entry is
``0x0010a040``. This port reserves the RAM window ending at ``0x0011a000``
for Zephyr, leaving the ROM loader's work areas untouched. This gives
65472 bytes for code, data, and stacks, rather than exposing the entire
352 KiB of physical SRAM. The SoC also has 2 MiB of flash.

The board metadata uses 63 KiB as a conservative code-size budget and
63 KiB as its RAM budget. These are limits on the same RAM window, not
two separate memory regions, and must not be added together. The
physical flash capacity is not available for executing this port's
image. The linker and runner additionally check the actual RAM bounds.

The ``hi3861`` runner wraps the binary in the ROM's SHA-256 image format
and sends it over YMODEM. Despite using the standard ``west flash``
command, it only loads RAM: it never erases or writes flash or efuses,
and does not replace the factory bootloader. The image is volatile.
The port does not produce an OpenHarmony all-in-one flash image.

From the Zephyr repository directory:

.. code-block:: console

   west build -b bearpi_hm_nano samples/hello_world
   west flash --runner hi3861 --port COM12

Replace ``COM12`` with the board's serial port. Close other serial tools
before downloading. The runner initially sends the factory firmware's
``AT+RST`` command to enter ROM download mode. When the board is already
in ROM or running an application without AT commands, use:

.. code-block:: console

   west flash --runner hi3861 --port COM12 --no-at-reset

Press the board's RESET button while the runner waits if necessary.
After transferring the image, the runner prints the initial UART output
and releases the port. Use a serial terminal at 115200 baud for further
output.

.. zephyr:board-supported-runners::

Hardware notes
**************

The system timer uses APB timer 0 for alarms and timer 1 for the
free-running cycle count. On this board the crystal clock is 40 MHz.
The SoC early-init hook enables both timer clocks before driver
initialization; the timer driver gets its register ranges from devicetree.
The counter value is a latched snapshot: software must request a snapshot
using control bit 3 and wait for it to clear before reading the value.

Hi3861 interrupts 32 through 61 use the ``LOCIE0`` CSR (``0xbe0``),
not the 32-bit ``mie`` register. UART0 uses interrupt 38. The SoC
initializes the local interrupt priorities and installs the Zephyr trap
vector instead of retaining the ROM vector.

Software reboot uses the ROM's PMU global-reset sequence. A CPU-only
watchdog reset does not restore all state needed by the factory boot
process after RAM execution.

The CH340 is a serial adapter, not a debug probe. GDB requires a separately
wired RISC-V JTAG probe; this serial runner does not provide breakpoints
or a debug server.
