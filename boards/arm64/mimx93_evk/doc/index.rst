.. _imx93_evk:

NXP i.MX93 EVK (Cortex-A55)
############################

Overview
********

The i.MX93 Evaluation Kit (MCIMX93-EVK board) is a platform designed to show
the most commonly used features of the i.MX 93 Applications Processor in a
small and low cost package. The MCIMX93-EVK board is an entry-level development
board, which helps developers to get familiar with the processor before
investing a large amount of resources in more specific designs.

i.MX93 MPU is composed of one cluster of 2x Cortex-A55 cores and a single
Cortex®-M33 core. Zephyr OS is ported to run on one of the Cortex®-A55 core.

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
  - Ethernet
  - PCI-E M.2
  - Connectors:

    - 40-Pin Dual Row Header
  - LEDs:

    - 1x Power status LED
    - 2x UART LED
  - Debug

    - JTAG 20-pin connector
    - MicroUSB for UART debug, two COM ports for A55 and M33


Supported Features
==================

The Zephyr mimx93_evk board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| GIC-v4    | on-chip    | interrupt controller                |
+-----------+------------+-------------------------------------+
| ARM TIMER | on-chip    | system clock                        |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 24 MHz.
Cortex-A55 Core runs up to 1.7 GHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART4.

Programming and Debugging
*************************

Copy the compiled ``zephyr.bin`` to the first FAT partition of the SD card and
plug the SD card into the board. Power it up and stop the u-boot execution at
prompt.

Use U-Boot to load and kick zephyr.bin to Cortex-A55 Core1:

.. code-block:: console

    fatload mmc 1:1 0xc0000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; cpu release 1 0xc0000000


Or use the following command to kick zephyr.bin to Cortex-A55 Core0:

.. code-block:: console

    fatload mmc 1:1 0xc0000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; go 0xc0000000


Use this configuration to run basic Zephyr applications and kernel tests,
for example, with the :ref:`synchronization_sample`:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: mimx93_evk_a55
   :goals: run

This will build an image with the synchronization sample app, boot it and
display the following ram console output:

.. code-block:: console

    *** Booting Zephyr OS build zephyr-v3.2.0-8-g1613870534a0  ***
    thread_a: Hello World from cpu 0 on mimx93_evk_a55!
    thread_b: Hello World from cpu 0 on mimx93_evk_a55!
    thread_a: Hello World from cpu 0 on mimx93_evk_a55!
    thread_b: Hello World from cpu 0 on mimx93_evk_a55!

References
==========

More information can refer to NXP official website:
`NXP website`_.

.. _NXP website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-processors/i-mx-applications-processors/i-mx-9-processors/i-mx-93-applications-processor-family-arm-cortex-a55-ml-acceleration-power-efficient-mpu:i.MX93
