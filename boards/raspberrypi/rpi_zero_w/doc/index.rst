.. zephyr:board:: rpi_zero_w

Overview
********

The `Raspberry Pi Zero W`_ is a small single-board computer built around the
Broadcom BCM2835 SoC, the same silicon as the original Raspberry Pi, with a
single ARM1176JZF-S core and 512 MB of SDRAM.

The BCM2835 has no GIC and no BCM2836 ARM-local interrupt controller. The
ARMC peripheral interrupt controller (`BCM2835 ARM Peripherals`_) is the root
interrupt controller and the free-running system timer is the system clock.
The mini-UART on GPIO 14/15 is the default console.

Hardware
********

- 1GHz single-core 32-bit ARM1176JZF-S CPU
- 512MB SDRAM
- 2.4GHz 802.11 b/g/n wireless LAN
- Bluetooth 4.1, Bluetooth Low Energy (BLE)
- Mini HDMI port and micro USB On-The-Go (OTG) port
- microSD card slot
- CSI camera connector
- HAT-compatible 40-pin header footprint (unpopulated)
- Micro USB power
- Composite video and reset pins via solder test points

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

microSD card
============

Flash a **Raspberry Pi OS Lite (32-bit)** image to a microSD card with
Raspberry Pi Imager. This creates the FAT boot partition holding the
firmware blobs (``bootcode.bin``, ``fixup.dat``, ``start.elf``) that the
BCM2835 needs in order to bring up the ARM core.

In the root directory of that boot partition:

1. Copy ``build/zephyr/zephyr.bin``
2. Replace ``config.txt`` with:

   .. code-block:: text

      kernel=zephyr.bin
      kernel_address=0x8000
      disable_commandline_tags=1
      enable_uart=1
      core_freq=250

``kernel_address`` must match the DRAM base in the SoC devicetree.
``disable_commandline_tags=1`` stops the firmware writing ATAGS, which this
port does not use. ``enable_uart=1`` routes the mini-UART to GPIO 14/15, which
the Bluetooth radio would otherwise use. ``core_freq=250`` locks the VPU clock
so the mini-UART baud-rate divisor stays valid. See the
`config.txt reference`_ for the full set of firmware options.

Eject the card and boot the Pi. The console comes up on the mini-UART at
115200 8N1, no flow control:

.. code-block:: console

   *** Booting Zephyr OS build v4.x.x-... ***
   Hello World! rpi_zero_w/bcm2835

Console connection
==================

Wire a 3.3 V USB-UART adapter to the GPIO header:

================== ============= =======================
Adapter            Pi header pin Pi GPIO / function
================== ============= =======================
GND                6 (or 9, 14)  GND
RX                 8             GPIO 14 / mini-UART TX
TX                 10            GPIO 15 / mini-UART RX
================== ============= =======================

QEMU
====

The board also runs on the QEMU ``raspi0`` machine:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rpi_zero_w
   :goals: run

QEMU paces the guest clock from the host by default, so timing-sensitive
tests are more stable with instruction counting enabled, for example
``QEMU_EXTRA_FLAGS="-icount shift=5,align=off,sleep=off"``.

References
**********

.. target-notes::

.. _Raspberry Pi Zero W:
   https://www.raspberrypi.com/products/raspberry-pi-zero-w/

.. _BCM2835 ARM Peripherals:
   https://datasheets.raspberrypi.com/bcm2835/bcm2835-peripherals.pdf

.. _config.txt reference:
   https://www.raspberrypi.com/documentation/computers/config_txt.html
