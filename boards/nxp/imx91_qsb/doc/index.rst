.. zephyr:board:: imx91_qsb

Overview
********

The i.MX 91 Quick Start Board (MCIMX91-QSB board) is a platform designed
to display the most commonly used features of the i.MX 91 Application
Processor. The MCIMX91-QSB board is an entry-level development board
with a small and low-cost package. The board can be used by developers
to get familiar with the processor before investing a large amount of
resources in more specific designs.

Hardware
********

- i.MX 91 Application Processor

  - The processor integrates an Arm Cortex-A55 core that can operate at speeds of up to 1.4 GHz.

- RAM: 1GB LPDDR4
- Storage:

  - SanDisk 16GB eMMC5.1
  - microSD Socket
- Wireless:

  - Murata Type-2EL (SDIO+UART+SPI) module. It is based on NXP IW612 SoC,
    which supports dual-band (2.4 GHz /5 GHz) 1x1 Wi-Fi 6, Bluetooth 5.2,
    and 802.15.4
- USB:

  - Two USB 2.0 Type C connectors
- Ethernet:

  - ENET_QoS: 10/100/1000 Mbit/s RGMII Ethernet supporting TSN connected
    with external PHY RTL8211
- PCIe:

  - One M.2/NGFF Key E mini card 75-pin connector
- Connectors:

  - 40-Pin Dual Row Header
- LEDs:

  - 1x Power status LED
  - 2x UART LED
- Debug:

  - JTAG 20-pin connector
  - MicroUSB for UART debug


Supported Features
==================

.. zephyr:board-supported-hw::

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 24 MHz.
Cortex-A55 Core runs up to 1.4 GHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART1 for A55 core.

Programming and Debugging
*******************************

U-Boot "go" command is used to load and kick Zephyr to Cortex-A55 Core.

Stop the board at U-Boot command line, then need to download Zephyr binary image into
DDR memory, it can use tftp:

.. code-block:: console

    tftp 0x80000000 zephyr.bin

Or copy the Zephyr image ``zephyr.bin`` to SD card and plug the card into the board, for example
if copy the image to the FAT partition of the SD card, use the following U-Boot command to load
the image into DDR memory (assuming the SD card is dev 1, fat partition ID is 1, they could be
changed based on actual partitions):

.. code-block:: console

    fatload mmc 1:1 0x80000000 zephyr.bin;


Then use U-Boot to load and kick zephyr.bin to Cortex-A55 Core:

.. code-block:: console

    dcache off; icache flush; go 0x80000000

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: imx91_qsb/mimx9111
   :goals: build

This will build an image with the synchronization sample app, boot it and
display the following console output:

.. code-block:: console

    *** Booting Zephyr OS build v4.0.0-3277-g69f43115c9a8 ***
    thread_a: Hello World from cpu 0 on imx91_qsb!
    thread_b: Hello World from cpu 0 on imx91_qsb!
    thread_a: Hello World from cpu 0 on imx91_qsb!
    thread_b: Hello World from cpu 0 on imx91_qsb!

.. include:: ../../common/board-footer.rst.inc
