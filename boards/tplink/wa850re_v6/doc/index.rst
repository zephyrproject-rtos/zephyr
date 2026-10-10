.. zephyr:board:: wa850re_v6

Overview
********

The `TL-WA850RE v6`_ is a 2.4 GHz 802.11n range extender by TP-Link. It is based on the MediaTek
MT7628 SoC, a single-core MIPS 24KEc processor with an integrated Wi-Fi radio, a 10/100 Ethernet
switch and the usual peripherals of a router SoC.

Hardware
********

.. list-table::
   :header-rows: 1

   * - Feature
     - Value
   * - SoC
     - MediaTek MT7628
   * - CPU
     - MIPS 24KEc at 575 MHz (25 MHz crystal)
   * - RAM
     - 32 MiB DDR2, 16-bit bus
   * - Flash
     - 4 MiB SPI NOR (GigaDevice GD25Q32B)
   * - Wi-Fi
     - 802.11b/g/n 2x2, integrated
   * - Ethernet
     - 1x 10/100 Mbps, integrated switch

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The board boots from SPI NOR flash through the stock Ralink U-Boot 1.1.3 bootloader. Zephyr is
loaded into DRAM over the serial console with the bootloader's ``loadb`` command and started with
``go``, so the flash content stays untouched.

Serial Console
==============

The console is the first UART of the SoC, which is also used by U-Boot. Connect a 3.3 V TTL UART
adapter to the UART pads on the PCB and use **57600 8N1** without hardware flow control.

Building
========

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: wa850re_v6
   :goals: build

Running from RAM
================

The image is linked at ``0x80100000`` and its entry point is at ``0x80101000``, after the 4 KiB
aligned vector section. ``loadb`` receives a file with the kermit protocol, so a kermit client such
as C-Kermit (the ``ckermit`` package) is needed on the host.

Open the serial console, power on the board and press :kbd:`4` within one second of the U-Boot
menu appearing:

.. code-block:: text

   Please choose the operation:
      1: Load system code to SDRAM via TFTP.
      2: Load system code then write to Flash via TFTP.
      3: Boot system code via Flash (default).
      4: Entr boot command line interface.
      7: Load Boot Loader code then write to Flash via Serial.
      9: Load Boot Loader code then write to Flash via TFTP.

At the ``MT7628 #`` prompt, start the download to the uncached KSEG1 alias of the link address.
U-Boot writes the data through the data cache and does not flush it, so a cached destination would
leave part of the image in the cache when ``go`` starts fetching instructions from memory:

.. code-block:: console

   MT7628 # loadb 0xa0100000

U-Boot now waits for kermit packets. Close the terminal program so that the kermit client can open
the serial port, then send ``build/zephyr/zephyr.bin`` from a shell. C-Kermit needs three settings
for this receiver: no carrier detect and no hardware flow control, since only GND, RX and TX are
connected, and prefixing of every control character, since U-Boot rejects unprefixed ones:

.. code-block:: console

   kermit -C "set carrier-watch off, set line /dev/ttyUSB0, set speed 57600, set flow none, \
     set control-character prefixed all, set file display brief, send build/zephyr/zephyr.bin, exit"

The transfer takes a few seconds and ends with a summary line:

.. code-block:: text

   SEND zephyr.bin (binary) (24836 bytes): OK (7.226 sec, 3436 cps)

If the kermit client is started while U-Boot is still at the prompt, U-Boot reports
``Unknown command 'kermit'`` and nothing is loaded. Reopen the terminal program, press
:kbd:`Enter` to get the prompt back and start the image:

.. code-block:: console

   MT7628 # go 0x80101000

Expected output on the serial console:

.. code-block:: text

   ## Starting application at 0x80101000 ...
   *** Booting Zephyr OS build v4.4.0 ***
   thread_a: Hello World from cpu 0 on wa850re_v6!
   thread_b: Hello World from cpu 0 on wa850re_v6!

Menu entries ``1`` and ``2`` fetch a file through TFTP instead, and entry ``2`` writes it to the
firmware partition of the flash, which replaces the stock firmware. This U-Boot ignores the load
and entry addresses of a legacy image header, so those paths are not supported.

.. _TL-WA850RE v6:
   https://www.tp-link.com/en/home-networking/range-extender/tl-wa850re/
