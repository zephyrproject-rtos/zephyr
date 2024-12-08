.. _phyboard_nash:

phyBOARD-Nash i.MX93
####################

Overview
********

The phyBOARD-Nash is based on the phyCORE-i.MX93 SoM is based on the NXP i.MX93
SoC. It features common industrial interfaces and can be used as a reference for
development or in the final product. It is an entry-level development board,
which helps developers to get familiar with the module before investing a large
amount of resources in more specific designs.

i.MX93 MPU is composed of one cluster of 2x Cortex-A55 cores and a single
Cortex-M33 core. Zephyr OS is ported to run on one of the Cortex-A55 core as
well as the Cortex-M33 core.

- Memory:

   - RAM: 512 MB - 2GB LPDDR4
   - EEPROM: 4 kB - 32 kB
   - eMMC: 8 GB - 256 GB

- Interfaces:

   - Ethernet: 2x 10/100BASE-T (1x TSN Support)
   - USB: 2x 2.0 Host / OTG
   - Serial: 1x RS232 / RS485 Full Duplex / Half Duplex
   - CAN: 1x CAN FD
   - Digital I/O: via Expansion Connector
   - MMX/SD/SDIO: microSD slot
   - Display: LVDS(1x4 or 1x8), MIPI DSI(1x4), HDMI
   - Audio: SAI
   - Camera: 1x MIPI CSI-2 (phyCAM-M), 1x Parallel
   - Expansion Bus: I2C, SPI, SDIO, UART, USB

- Debug:

   - JTAG 10-pin connector
   - USB-C for UART debug, 2x serial ports for A55 and M33


.. image:: img/phyboard_nash.webp
   :width: 720px
   :align: center
   :height: 405px
   :alt: phyBOARD-Nash

More information about the board can be found at the `PHYTEC website`_.

Supported Features
==================

The ``phyboard_nash/mimx9352/a55`` board target supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| GIC-v4    | on-chip    | interrupt controller                |
+-----------+------------+-------------------------------------+
| ARM TIMER | on-chip    | system clock                        |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock_control                       |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+
| TPM       | on-chip    | TPM Counter                         |
+-----------+------------+-------------------------------------+

The ``phyboard_nash/mimx9352/m33`` board target supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | interrupt controller                |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock_control                       |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 24 MHz. Cortex-A55
Core runs up to 1.7 GHz. Cortex-M33 Core runs up to 200MHz in which SYSTICK runs
on same frequency.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART2 for A55 core and M33 core. The u-boot bootloader or Linux use the
second serial port for debug output.

Programming and Debugging (A55)
*******************************

Copy the compiled ``zephyr.bin`` to the ``BOOT`` partition of the SD card and
plug the SD card into the board. Power it up and stop the u-boot execution at
prompt.

Use U-Boot to load and execute zephyr.bin on Cortex-A55 Core0:

.. code-block:: console

    fatload mmc 1:1 0xd0000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; go 0xd0000000


Use this configuration to run basic Zephyr applications and kernel tests,
for example:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: phyboard_nash/mimx9352/a55
   :goals: build

Use this configuration to run basic Zephyr applications, for example:

.. code-block:: console

    *** Booting Zephyr OS build v3.7.0-848-gb4d99b124c6d ***
    Hello World! phyboard_nash/mimx9352/a55

Programming and Debugging (M33)
*******************************

Copy the compiled ``zephyr.bin`` to the ``BOOT`` partition of the SD card and
plug the SD card into the board. Power it up and stop the u-boot execution at
prompt.

Use U-Boot to load and kick zephyr.bin to Cortex-M33 Core:

.. code-block:: console

    load mmc 1:1 0x80000000 zephyr.bin;cp.b 0x80000000 0x201e0000 0x30000;bootaux 0x1ffe0000 0

Use this configuration to run basic Zephyr applications, for example:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: phyboard_nash/mimx9352/m33
   :goals: build

This will build an image with the synchronization sample app, boot it and
display the following console output:

.. code-block:: console

    *** Booting Zephyr OS build v3.7.0-848-gb4d99b124c6d ***
    Hello World! phyboard_nash/mimx9352/m33

Starting the M7-Core from U-Boot and Linux
==========================================

Loading binaries and starting the M33-Core is supported from Linux via
remoteproc. Please check the `phyCORE-i.MX93 BSP Manual`_ for more information.

References
==========

For more information refer to the `PHYTEC website`_.

.. _PHYTEC website:
   https://www.phytec.eu/en/produkte/development-kits/phyboard-nash/
.. _phyCORE-i.MX93 BSP Manual:
   https://phytec.github.io/doc-bsp-yocto/bsp/imx9/imx93/imx93.html
