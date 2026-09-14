.. zephyr:board:: rpi_zero_2w

Overview
********

The `Raspberry Pi Zero 2 W`_ is a small single-board computer built around the
Broadcom BCM2710A1 SoC, the same silicon as the original Raspberry Pi 3,
packaged in an RP3A0-AU SiP together with 512 MB of LPDDR2 SDRAM.

Zephyr runs the BCM2710A1 in single-core mode, using the BCM2836 ARM-local
interrupt controller (`QA7 ARM Quad-A7 Core`_) and the BCM2835 ARMC peripheral
interrupt controller (`BCM2837 ARM Peripherals`_) natively, as this SoC has no
GIC. The mini-UART on GPIO 14/15 is the default console.

Hardware
********

- 1GHz quad-core 64-bit Arm Cortex-A53 CPU
- 512MB SDRAM
- 2.4GHz 802.11 b/g/n wireless LAN
- Bluetooth 4.2, Bluetooth Low Energy (BLE), onboard antenna
- Mini HDMI port and micro USB On-The-Go (OTG) port
- microSD card slot
- CSI-2 camera connector
- HAT-compatible 40-pin header footprint (unpopulated)
- H.264, MPEG-4 decode (1080p30); H.264 encode (1080p30)
- OpenGL ES 1.1, 2.0 graphics
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

Flash a **Raspberry Pi OS Lite (64-bit)** image to a microSD card with
Raspberry Pi Imager. This creates the FAT boot partition holding the
firmware blobs (``bootcode.bin``, ``fixup.dat``, ``start.elf``) that the
BCM2710A1 needs in order to bring up the ARM cores.

In the root directory of that boot partition:

1. Copy ``build/zephyr/zephyr.bin``
2. Replace ``config.txt`` with:

   .. code-block:: text

      arm_64bit=1
      enable_uart=1
      core_freq=250
      kernel_address=0x200000
      kernel=zephyr.bin

``arm_64bit=1`` is required whenever the kernel filename does not start with
``kernel8``. ``enable_uart=1`` routes the mini-UART to GPIO 14/15, which the
Bluetooth radio would otherwise use. ``core_freq=250`` locks the VPU clock so
the mini-UART baud-rate divisor stays valid; ``enable_uart=1`` is supposed to
imply this but does not always (see `raspberrypi/linux issue #4123`_).
``kernel_address`` must match the DRAM base in the board devicetree. See the
`config.txt reference`_ for the full set of firmware options.

Eject the card and boot the Pi. The console comes up on the mini-UART at
115200 8N1, no flow control:

.. code-block:: console

   *** Booting Zephyr OS build v4.x.x-... ***
   Hello World! rpi_zero_2w

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

References
**********

.. target-notes::

.. _Raspberry Pi Zero 2 W:
   https://www.raspberrypi.com/products/raspberry-pi-zero-2-w/

.. _raspberrypi/linux issue #4123:
   https://github.com/raspberrypi/linux/issues/4123

.. _BCM2837 ARM Peripherals:
   https://datasheets.raspberrypi.com/bcm2837/bcm2837-peripherals.pdf

.. _QA7 ARM Quad-A7 Core:
   https://datasheets.raspberrypi.com/bcm2836/QA7_rev3.4.pdf

.. _config.txt reference:
   https://www.raspberrypi.com/documentation/computers/config_txt.html
