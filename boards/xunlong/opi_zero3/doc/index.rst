.. SPDX-FileCopyrightText: Copyright (c) 2026 Eugene Cheng
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: opi_zero3

Orange Pi Zero 3
#################

Overview
********

The `Orange Pi Zero 3`_ is a small single-board computer built around
the Allwinner H618: four Cortex-A53 cores, a Mali-T860 GPU and 1 GB
to 4 GB of LPDDR4 depending on the variant. The 4 GB variant was used
for the testing described here.

The board has a green status LED on PC13 and a red power LED on PC12.
The debug console is UART0 on the 3-pin header, at 115200 8N1.

.. image:: img/opi_zero3.webp
   :align: center
   :alt: Orange Pi Zero 3

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The H618 cannot run Zephyr straight from the ROM. The clock tree and
the DRAM controller have to be brought up first, so a second stage
loader is required. The flow below builds an SD card image from Das
U-Boot SPL, Arm Trusted Firmware-A and Das U-Boot proper, and lets
U-Boot load the Zephyr image for us.

Building the boot images
========================

Build TF-A first. U-Boot needs the BL31 blob and the build stops in
``binman`` with an ``atf-bl31`` error when it is missing:

.. code-block:: console

   git clone https://git.trustedfirmware.org/TF-A/trusted-firmware-a.git
   cd trusted-firmware-a
   make CROSS_COMPILE=aarch64-linux-gnu- PLAT=sun50i_h616
   export BL31="$(pwd)/build/sun50i_h616/release/bl31.bin"

Then build U-Boot. Master was tested here, at 2026.07-rc3:

.. code-block:: console

   git clone https://source.denx.de/u-boot/u-boot.git
   cd u-boot
   export CROSS_COMPILE=aarch64-linux-gnu-
   make orangepi_zero3_defconfig
   make

The resulting image is ``u-boot-sunxi-with-spl.bin``.

.. note::

   U-Boot mainline builds the Zero 3 on the sun50i_h616 platform
   (``CONFIG_MACH_SUN50I_H616``). The H618 shares that platform code,
   so this is expected.

Writing the card
================

Write the SPL/U-Boot image at the 8 KiB offset. It must go to the
whole disk device, not to a partition device:

.. code-block:: console

   sudo dd if=u-boot-sunxi-with-spl.bin of=/dev/sdX bs=1024 seek=8

Then create a FAT32 partition that U-Boot reads to find ``zephyr.bin``:

.. code-block:: console

   sudo fdisk /dev/sdX

Inside ``fdisk``:

.. code-block:: console

   n
   p
   1
   (accept the default first sector)
   +100M
   t
   6
   w

Format and mount it:

.. code-block:: console

   sudo mkfs.vfat -F 32 /dev/sdX1
   sudo mount /dev/sdX1 /mnt

If ``fdisk`` warns that it is reusing an existing partition table,
delete it first with ``d``, otherwise the new partition can land on top
of the SPL.

Building and Flashing
=====================

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: opi_zero3
   :goals: build

Copy the built image onto the FAT partition and unmount it:

.. code-block:: console

   cp build/zephyr/zephyr.bin /mnt
   sudo umount /mnt

Serial console
==============

Connect a USB serial adapter to the 3-pin debug header at 3.3 V level:
header TX to adapter RX, header RX to adapter TX, and GND to GND.
Settings are 115200 8N1 with no flow control.

Booting
=======

Insert the card and power the board. At the U-Boot prompt:

.. code-block:: console

   => fatload mmc 0:1 0x40080000 zephyr.bin
   => go 0x40080000

The load address is the start of the DRAM node in the device tree
(``dram0`` at ``0x40080000``, 1 GB), so U-Boot does not have to relocate
the image. Expected console output:

.. code-block:: text

   *** Booting Zephyr OS vx.x.x ***
   Hello World! opi_zero3/sun50i_h618

Other samples can be loaded the same way. ``samples/basic/blinky``
toggles the green LED through the ``led0`` alias.

References
==========

- `Orange Pi Zero 3`_
- `Das U-Boot documentation`_
- `Allwinner sunxi board documentation`_
- `Arm Trusted Firmware-A`_

.. _Orange Pi Zero 3:
   http://www.orangepi.org/html/hardWare/computerAndMicrocontrollers/details/Orange-Pi-Zero-3.html

.. _Das U-Boot documentation:
   https://docs.u-boot.org/en/latest/

.. _Allwinner sunxi board documentation:
   https://docs.u-boot.org/en/stable/board/allwinner/sunxi.html

.. _Arm Trusted Firmware-A:
   https://git.trustedfirmware.org/TF-A/trusted-firmware-a
