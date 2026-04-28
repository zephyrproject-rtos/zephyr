.. zephyr:board:: rpi_4b

Overview
********

The `Raspberry Pi 4`_ is the fourth generation of the Raspberry Pi flagship series of
single-board computers. It is based on the Broadcom BCM2711 SoC which features a
quad-core 64-bit ARM Cortex-A72 CPU and is available in 1, 2, 4 or 8 GB of LPDDR4 RAM.

Hardware
********

- Broadcom BCM2711, Quad core Cortex-A72 (ARM v8) 64-bit SoC @ 1.8GHz
- 1GB, 2GB, 4GB or 8GB LPDDR4-3200 SDRAM (depending on model)
- 2.4 GHz and 5.0 GHz IEEE 802.11ac wireless, Bluetooth 5.0, BLE
- Gigabit Ethernet
- 2 USB 3.0 ports; 2 USB 2.0 ports.
- Raspberry Pi standard 40 pin GPIO header (fully backwards compatible with previous boards)
- 2 × micro-HDMI® ports (up to 4kp60 supported)
- 2-lane MIPI DSI display port
- 2-lane MIPI CSI camera port
- 4-pole stereo audio and composite video port
- H.265 (4kp60 decode), H264 (1080p60 decode, 1080p30 encode)
- OpenGL ES 3.1, Vulkan 1.0
- Micro-SD card slot for loading operating system and data storage
- 5V DC via USB-C connector (minimum 3A)
- 5V DC via GPIO header (minimum 3A)
- Power over Ethernet (PoE) enabled (requires separate PoE HAT)

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

TF Card
=======

Prepare a TF card with MBR and FAT32. In the root directory of the TF card:

1. Download and place these firmware files:

   * `bcm2711-rpi-4-b.dtb <https://raw.githubusercontent.com/raspberrypi/firmware/master/boot/bcm2711-rpi-4-b.dtb>`_
   * `bootcode.bin <https://raw.githubusercontent.com/raspberrypi/firmware/master/boot/bootcode.bin>`_
   * `start4.elf <https://raw.githubusercontent.com/raspberrypi/firmware/master/boot/start4.elf>`_

2. Copy ``build/zephyr/zephyr.bin``
3. Create a ``config.txt``:

   .. code-block:: text

      kernel=zephyr.bin
      arm_64bit=1
      enable_uart=1
      uart_2ndstage=1

Insert the card and power on the board. You should see the following output on
the serial console (GPIO 14/15):

.. code-block:: text

   *** Booting Zephyr OS build XXXXXXXXXXXX  ***
   Hello World! Raspberry Pi 4 Model B!

References
**********

.. target-notes::

.. _Raspberry Pi 4:
   https://pip.raspberrypi.com/documents/RP-008344-DS-raspberry-pi-4-product-brief.pdf/

.. _Raspberry Pi 4 Schematic:
   https://datasheets.raspberrypi.com/rpi4/raspberry-pi-4-reduced-schematics.pdf
