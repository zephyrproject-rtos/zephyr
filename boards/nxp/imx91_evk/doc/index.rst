.. zephyr:board:: imx91_evk

Overview
********

The i.MX 91 Evaluation Kit (MCIMX91-EVK board) is a platform designed
to display the most commonly used features of the i.MX 91 applications
processor. The MCIMX91-EVK board is an entry-level development board
with a small and low-cost package. The board can be used by developers
to get familiar with the processor before investing a large amount of
resources in more specific designs.

The i.MX 91 applications processor features an Arm Cortex-A55 core
that can operate at speeds of up to 1.4 GHz.

- Board features:

  - RAM: 2GB LPDDR4
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

    - ENET: 10/100/1000 Mbit/s RGMII Ethernet connected with external PHY
      RTL8211
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

.. zephyr:board-supported-runners::

There are multiple methods to program and debug Zephyr

Option 1. Boot Zephyr by Using JLink Runner
===========================================

The default runner for the board is JLink, connect the EVK board's JTAG connector to
the host computer using a J-Link debugger, power up the board and stop the board at
U-Boot command line.

Then use "west flash" or "west debug" command to load the zephyr.bin
image from the host computer and start the Zephyr application on A55 core0.

Flash and Run
-------------

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: imx91_evk/mimx9131
   :goals: flash

Then the following log could be found on UART1 console:

.. code-block:: console


    *** Booting Zephyr OS build v4.1.0-3063-g2c7ef313ac38 ***
    Hello World! imx91_evk/mimx9131

Debug
-----

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: imx91_evk/mimx9131
   :goals: debug

Option 2. Boot Zephyr by Using U-Boot Command
=============================================

U-Boot "go" command is used to load and kick Zephyr to Cortex-A55 Core.

Copy the compiled ``zephyr.bin`` to the first FAT partition of the SD card and
plug the SD card into the board. Power it up and stop the u-boot execution at
prompt.

Use U-Boot to load and kick zephyr.bin to Cortex-A55 Core:

.. code-block:: console

    fatload mmc 1:1 0x80000000 zephyr.bin; dcache flush; icache flush; go 0x80000000

Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: imx91_evk/mimx9131
   :goals: build

This will build an image with the synchronization sample app, boot it and
display the following console output:

.. code-block:: console

    *** Booting Zephyr OS build v4.0.0-3277-g69f43115c9a8 ***
    thread_a: Hello World from cpu 0 on imx91_evk!
    thread_b: Hello World from cpu 0 on imx91_evk!
    thread_a: Hello World from cpu 0 on imx91_evk!
    thread_b: Hello World from cpu 0 on imx91_evk!

.. include:: ../../common/board-footer.rst.inc
