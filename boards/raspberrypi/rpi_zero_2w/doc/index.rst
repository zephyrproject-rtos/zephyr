.. zephyr:board:: rpi_zero_2w

Overview
********

The Raspberry Pi Zero 2 W is a small SBC built around the Broadcom BCM2710A1
SoC (the same silicon as the original Pi 3, in an RP3A0-AU SiP). The CPU is a
quad-core Cortex-A53; the board has 512 MiB DRAM and a CYW43439 Wi-Fi/BT chip
on SDIO.

Zephyr currently supports the BCM2710A1 in single-core mode, with the BCM2836
ARM-local interrupt controller and BCM2835 ARMC peripheral interrupt
controller wired up natively (no GIC). The mini-UART (GPIO 14/15) is the
default console. Wi-Fi/BT, USB, SDHCI and SMP are out of scope.

Hardware
********

- BCM2710A1 -- 4x Cortex-A53 @ up to 1 GHz, ARMv8-A AArch64
- 512 MiB LPDDR2 SDRAM
- CYW43439 Wi-Fi 802.11b/g/n + Bluetooth 4.2 (not yet supported in Zephyr)
- Mini-HDMI, USB OTG, microSD slot
- 40-pin GPIO header

Supported features
==================

+----------------------------+--------------+----------------------------+
| Interface                  | Status       | Driver                     |
+============================+==============+============================+
| ARM generic timer          | yes          | ``arm_arch_timer``         |
+----------------------------+--------------+----------------------------+
| BCM2836 ARM-local intc     | yes          | SoC-internal               |
+----------------------------+--------------+----------------------------+
| BCM2835 ARMC peripheral    | yes          | SoC-internal               |
| intc cascade               |              |                            |
+----------------------------+--------------+----------------------------+
| Mini-UART (uart1)          | yes          | ``brcm,bcm2711-aux-uart``  |
+----------------------------+--------------+----------------------------+
| GPIO                       | yes (driver) | ``brcm,bcm2711-gpio``      |
+----------------------------+--------------+----------------------------+
| Pinctrl                    | yes          | ``brcm,bcm2711-pinctrl``   |
+----------------------------+--------------+----------------------------+
| iproc RNG                  | yes          | ``brcm,iproc-rng200``      |
+----------------------------+--------------+----------------------------+
| PL011 UARTs                | binding only | not validated on this SoC  |
+----------------------------+--------------+----------------------------+
| SMP, SDHCI, USB, Wi-Fi/BT  | no           |                            |
+----------------------------+--------------+----------------------------+

Building
********

Builds with the upstream Zephyr SDK or a host ``aarch64-zephyr-elf`` cross
toolchain:

.. code-block:: console

   west build -p always -b rpi_zero_2w samples/hello_world

The resulting ``zephyr.bin`` is linked at ``0x200000`` and is loaded by Pi
firmware via ``kernel_address=0x200000`` in ``config.txt`` (see below).

Producing a microSD card
************************

Flash any **Raspberry Pi OS Lite (64-bit)** image to a microSD card with
Raspberry Pi Imager. This creates the FAT boot partition with the Pi
firmware blobs (``bootcode.bin``, ``fixup.dat``, ``start.elf``) Zephyr
needs to boot. Re-insert the card; the boot partition auto-mounts
(``/Volumes/bootfs`` on macOS, ``/media/$USER/bootfs`` on most Linux).

On that partition:

1. Copy the built ``build/zephyr/zephyr.bin`` to the partition root.
2. Replace ``config.txt`` with the Zephyr boot parameters below.

Eject the card and boot the Pi. The console comes up on the mini-UART
(GPIO 14/15) at 115200 baud.

config.txt reference
====================

The Zephyr-required ``config.txt`` content::

   arm_64bit=1
   enable_uart=1
   core_freq=250
   kernel_address=0x200000
   kernel=zephyr.bin

- ``arm_64bit=1`` -- load and execute kernel in AArch64 mode. Required
  whenever the kernel filename does not start with ``kernel8``.
- ``enable_uart=1`` -- routes the mini-UART to GPIO 14/15 (instead of to
  Bluetooth, which is the default on Pi Zero 2 W since the BT chip lives
  on the same UART otherwise).
- ``core_freq=250`` -- explicitly locks the VPU clock so the mini-UART's
  baud-rate divisor stays valid. ``enable_uart=1`` is supposed to imply
  this but is unreliable; see
  `raspberrypi/linux issue #4123 <https://github.com/raspberrypi/linux/issues/4123>`_.
- ``kernel_address=0x200000`` -- match the linker base set in
  ``dts/arm64/broadcom/bcm2710.dtsi``.
- ``kernel=zephyr.bin`` -- load this filename instead of the default
  ``kernel8.img``.

Console connection
******************

Wire a 3.3 V USB-UART adapter to the GPIO header (looking at the board
with the microSD slot at the top):

================== ============= =====================
Adapter            Pi header pin Pi GPIO / function
================== ============= =====================
GND                6 (or 9, 14)  GND
RX                 8             GPIO 14 / mini-UART TX
TX                 10            GPIO 15 / mini-UART RX
================== ============= =====================

115200 8N1, no flow control. The mini-UART output looks like::

   *** Booting Zephyr OS build v4.x.x-... ***
   Hello World! rpi_zero_2w

References
**********

- `BCM2837 ARM Peripherals manual <https://datasheets.raspberrypi.com/bcm2837/bcm2837-peripherals.pdf>`_
- `QA7 ARM Quad-A7 Core (BCM2836/2837 ARM-local intc) <https://datasheets.raspberrypi.com/bcm2836/QA7_rev3.4.pdf>`_
- `Pi config.txt reference <https://www.raspberrypi.com/documentation/computers/config_txt.html>`_
