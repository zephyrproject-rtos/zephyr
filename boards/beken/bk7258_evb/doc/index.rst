:orphan:

.. zephyr:board:: bk7258_evb

Overview
********

The BK7258 is a 2.4 GHz Wi-Fi 6 (IEEE 802.11b/g/n/ax 1x1) and Bluetooth Low
Energy 5.4 combo SoC, built around an Armv8-M STAR-MC1 core running at up to
480 MHz.

The ``bk7258_evb`` board is the Beken evaluation board for this SoC.

Hardware
********

- Beken BK7258 SoC

  - Armv8-M STAR-MC1 at up to 480 MHz, with TrustZone and a double-precision FPU
  - 640 KB of shared SRAM
  - System-in-package flash, 8 MB on this board
  - IEEE 802.11b/g/n/ax 1x1, 2.4 GHz, 20 MHz and 40 MHz channels
  - Bluetooth Low Energy 5.4, 1 Mbps, 2 Mbps and long range

- CH340 USB-UART bridge, connected to UART0 (TX on GPIO11, RX on GPIO10)
- Reset button and boot-mode button

Devicetree
==========

SRAM
----

The SoC has 640 KB of SRAM shared between the two cores and the bootloader.
The vendor's own memory map divides it like this: 64 KB at 0x28000000 for
spinlocks, 320 KB at 0x28010000 for this core, and the rest from 0x28060000 for
the firmware on the other core, except for a small power management area and a
swap buffer at the very top.

This port claims the lower 256 KB of this core's region -- 0x28010000 to
0x2804FFFF -- which is where the bootloader loads the image's data, and leaves
the 64 KB above it unclaimed for now.

Flash
-----

``flash0``
   The code region, at ``0x02150000``, 1156 KB. This is the window from which the
   core fetches instructions, and Zephyr loads the image there. Reads through the
   window are handled by hardware. The size is the application slot that the
   bootloader reserves, not the capacity of the flash device.

``flash_ap``
   The device that backs read, write and erase, at ``0x44030000``, 8 MB. It is
   not driven from this core: the firmware the device ships with owns the
   controller, and the driver asks it to perform each operation and waits for the
   result. Offsets here are physical, and what comes back is the stream as
   stored -- 32 bytes of data followed by 2 bytes of checksum -- because the
   driver neither inserts nor strips those checksum bytes.

   This is the node named by ``zephyr,flash-controller``. It is not another name
   for ``flash0``: ``flash0`` is a window computed from the physical stream, and
   it is valid only where the stream was written in that form.

System Clock
============

The bootloader leaves the core clock at 120 MHz, which is what
``CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC`` defaults to. SysTick runs from it.

Serial Port
===========

UART0 is the console, at 115200 baud, 8 data bits, no parity, one stop bit, TX on
GPIO11 and RX on GPIO10, brought out through a CH340 USB-UART bridge.

The port is configured by the firmware the device ships with, which also uses it
as its own console and is the only reader of it. This image only borrows the port
to print, which is why the board enables ``CONFIG_UART_BK7258_NO_HW_INIT``: the
UART driver then registers the device without writing a single register. Running
the normal initialisation (soft reset, pin muxing, divider) would break that
console mid-line and the board would go silent.

For the same reason the shell does not sit on UART0. ``zephyr,shell-uart`` points
at a console transport over the mailbox instead: the firmware forwards a line
typed at its own prompt, and prints what the shell writes back. A shell on UART0
would mean two readers on one port, and the second one would receive nothing.

Requirements
************

Building an image needs nothing beyond Zephyr. Running one needs two things that
are neither built from this tree nor redistributed with it, and that come with
the evaluation board:

- The vendor bootloader. It starts the image, and it owns the partition table and
  the checksums that the vendor packaging tool inserts.
- A prebuilt firmware image that drives the radio, initialises the flash and owns
  UART0. This patch does not replace it, and it is not rebuildable from this tree.

Only the application slot may be erased when programming. Erasing the whole
device removes both of the above together with the radio calibration data.

Boot flow
*********

A BK7258 device ships with the vendor's first-stage bootloader in flash, together
with the firmware that runs on the other core.  Zephyr builds only the
application image; the bootloader loads it, and the application has to honour
what follows.

**Where the image goes.**  The application is linked to execute from the flash
window: it sits at flash offset 0x165000 and runs at 0x02150000.

**Start-up handshake.**  The firmware on the other core supervises this one.  It
waits 2 s for a power-up indication and then expects a heartbeat every 2 s, and
about 6 s after either stops it resets the whole chip -- with no output from this
core at all.  Both go over the mailbox, and the heartbeat entry has to carry at
least four bytes of payload: a zero-length entry is accepted by the mailbox and
silently ignored, so the reset simply repeats.  ``CONFIG_SOC_BK7258_IPC_HEARTBEAT``
implements this, and it has to stay enabled when running on the shipped firmware.

**UART0 belongs to the other core.**  It is that core's console, and the
application must not re-initialise it: the pins, the clock divider and the baud
rate are not ours to change, and a normal UART init truncates the console output
mid-line, after which the board is silent.  ``UART_BK7258_NO_HW_INIT`` keeps this
driver off the registers; the application either hands its output to the mailbox
LOG channel or writes into the transmit FIFO as configured.

Programming and Debugging
*************************

Building
========

Build the board bring-up sample:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/beken/bk7258_evb
   :board: bk7258_evb
   :goals: build
   :compact:

For a board that needs no board-specific configuration, any application that
builds for the architecture will do, for example
:zephyr:code-sample:`hello_world`.

Flashing
========

The board has a ``beken`` runner.  It programs the SoC through the BootROM UART
download protocol, using the vendor's loader tool (BKFIL's ``bk_loader``, which
Beken distributes for Windows, macOS and Linux):

.. code-block:: console

   west flash --runner beken --bkfil-tool /path/to/bk_loader --port COM17

The built ``zephyr.bin`` cannot be programmed as it is.  The flash controller
checks a CRC on every read, so every 32 bytes of payload has to be followed by a
2-byte big-endian CRC-16 -- and the trailing partial block padded with 0xFF
before its CRC is computed -- which is what the vendor packaging tool produces.
Pass ``--package-script`` and ``--pad-script`` to have the runner package the
image for you, or hand it a ready-to-program file with ``--image``.

Only the application slot at 0x165000 is written.  The bootloader, the firmware
on the other core and the radio calibration data are left alone; erasing the
whole device removes all three.

The runner resets the device at the end.  The console appears on the serial port
at 115200 8N1 -- see Boot flow above for why the application's own output shares
that port.

Debugging
=========

The SoC supports SWD, but no runner is configured. See ``board.cmake`` for the
OpenOCD lines to enable if a probe is attached.
