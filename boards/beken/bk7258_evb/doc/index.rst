.. zephyr:board:: bk7258_evb

Overview
********

The BK7258 is a 2.4 GHz Wi-Fi 6 (IEEE 802.11b/g/n/ax 1x1) and Bluetooth Low
Energy 5.4 combo SoC, built around an Armv8-M STAR-MC1 core running at up to
480 MHz.

The ``bk7258_evb`` board is the Beken evaluation board for this SoC.

References
**********

- `BK7258 product page <https://www.bekencorp.com/en/goods/detail/cid/60.html>`_:
  the SoC on the vendor's website.
- `Beken documentation portal <https://docs.bekencorp.com/>`_: the datasheet and
  the reference manual for the SoC, published by the vendor.

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

Boot Flow
*********

The BootROM starts the image at flash offset 0, which is not where the
application lives: flash entry has to hand execution over to the application.
The board therefore uses two images:

- At flash offset 0, a first stage built from
  :file:`soc/beken/bk7258/first_stage.S`. It disables the watchdogs the BootROM
  leaves running, starts the application at ``0x02150000``, and then waits.
- At flash offset ``0x165000``, the Zephyr application, linked at
  ``0x02150000``.

The code images are not stored flat: instruction fetch consumes the stored
bytes as they are, so every 32 bytes of code are followed by their CRC-16, and
a partial last block is padded with ``0xff``. Data partitions are not stored
this way. :file:`soc/beken/bk7258/bkimage.py` converts both images and merges
them into one ``zephyr_full.bin``, which is what ``west flash`` writes at
offset 0.

Devicetree
==========

SRAM
----

The SoC has 640 KB of shared SRAM. Zephyr claims the 256 KB region at
``0x28010000``, which is where the image's data lives. The 64 KB bank below it,
at ``0x28000000``, is where the first stage keeps its stack.

Flash
-----

``flash0``
   The code region, at ``0x02150000``, 1156 KB. This is the window from which the
   core fetches instructions, and Zephyr loads the image there. Reads through the
   window are handled by hardware. The size is the application slot, not the
   capacity of the flash device.

The application is linked at the base of its slot rather than at the base of the
XIP window, because the first stage occupies the bottom of that window.

There is no flash controller driver yet, so no device node for the controller
exists and ``zephyr,flash-controller`` is not set.

System Clock
============

Out of reset the core runs on the 26 MHz crystal, and the first stage leaves it
there. The SoC init then switches the core to 120 MHz, divided down from the
480 MHz PLL, and the flash controller to 80 MHz, before the timer driver is
initialised.

SysTick runs from the core clock, so the ``clock-frequency`` property of the
``cpu@0`` node is 120000000 and ``CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC`` takes its
value from that property.

Serial Port
===========

UART0 is the console, at 115200 baud, 8 data bits, no parity, one stop bit. The
driver initialises the port: clock source, pin muxing and divider.

Running alongside the firmware the device ships with
====================================================

The image at flash offset 0 is normally the firmware the device ships with,
which drives the radio and owns the flash controller. That firmware can be kept
by enabling ``CONFIG_SOC_BK7258_VENDOR_FIRMWARE``. In that configuration the
first stage is not built, the SoC is expected to run at 120 MHz, UART0 is
expected to be configured already and is left alone, and the image keeps the
mailbox heartbeat that firmware supervises. Programming that configuration
writes only the application slot and leaves the rest of the flash alone.

Requirements
************

Building and running this board needs nothing outside this tree: the first stage,
the boot header it carries, the image conversion and the flasher are all part of
Zephyr.

Programming and Debugging
*************************

Building
========

Build the :zephyr:code-sample:`hello_world` sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: bk7258_evb
   :goals: build
   :compact:

Flashing
========

#. Connect the board's USB-UART bridge to the host.
#. Run ``west flash`` with the serial port the bridge appeared as:

   .. code-block:: console

      west flash --port /dev/ttyUSB0

   The runner speaks the BootROM download protocol over that port. It erases and
   writes 4 KB sectors as it goes, verifies each written sector, and then
   restarts the chip.
#. Open the serial port at 115200 8N1 to see the console output.

.. note::
   In the default configuration the image covers the flash from offset 0, so
   programming replaces the firmware the device ships with, together with its
   bootloader. Going back to that firmware means programming it with the vendor
   tooling. The radio calibration data near the end of the flash lies outside
   the written range and is not affected.

Debugging
=========

The SoC supports SWD, but no runner is configured. See ``board.cmake`` for the
OpenOCD lines to enable if a probe is attached.
