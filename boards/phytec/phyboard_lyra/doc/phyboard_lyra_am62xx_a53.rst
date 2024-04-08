.. _phyboard_lyra_am62xx_a53:

phyBOARD-Lyra AM62x A53 Core
############################

Overview
********

PHYTEC phyBOARD-Lyra AM62x board is based on TI Sitara applications
processor, composed of a quad Cortex®-A53 cluster and a single Cortex®-M4 core.
Zephyr OS is ported to run on the Cortex®-A53 core.

- Board features:

  - RAM: 2GB DDR4
  - Storage:

    - 16GB eMMC
    - 64MB OSPI NOR
    - 4KB EEPROM
  - Ethernet

See the `PHYTEC AM62x Product Page`_ for details.

.. figure:: img/phyCORE-AM62x_Lyra_frontside.webp
   :align: center
   :alt: phyBOARD-Lyra AM62x

   PHYTEC phyBOARD-Lyra with the phyCORE-AM62x SoM

Supported Features
==================

The Zephyr phyboard_lyra/am6234/a53 board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| GIC-v3    | on-chip    | interrupt controller                |
+-----------+------------+-------------------------------------+
| ARM TIMER | on-chip    | system clock                        |
+-----------+------------+-------------------------------------+
| PINCTRL   | on-chip    | pinctrl                             |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+
| Mailbox   | on-chip    | IPC Mailbox                         |
+-----------+------------+-------------------------------------+

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 200 MHz.

DDR RAM
-------

The board has 2GB of DDR RAM available. This board configuration
allocates Zephyr 1MB of RAM (0x82000000 to 0x82100000).

Serial Port
-----------

This board configuration uses a single serial communication channel with the
CPU's UART0.

SD Card
*******

Download PHYTEC's official `WIC`_ and `bmap`_ files and flash the WIC file with
bmap-tools on a SD-card.

.. code-block:: console

    bmaptool copy phytec-qt5demo-image-phyboard-lyra-am62xx-3.wic.xz /dev/sdX

Building
********

You can build an application in the usual way. Refer to
:ref:`build_an_application` for more details. Here is an example for
:zephyr:code-sample:`hello_world`.

.. zephyr-app-commands::
   :board: phyboard_lyra/am6234/a53
   :zephyr-app: samples/hello_world
   :goals: build

Programming
***********

Copy the compiled ``zephyr.bin`` to the first FAT partition of the SD card and
plug the SD card into the board. Power it up and stop the u-boot execution at
prompt.

Use U-Boot to load and kick zephyr.bin:

.. code-block:: console

    fatload mmc 1:1 0x82000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; go 0x82000000


..
  References

.. _PHYTEC AM62x Product Page:
   https://www.phytec.com/product/phycore-am62x/

.. _WIC:
   https://download.phytec.de/Software/Linux/BSP-Yocto-AM62x/BSP-Yocto-Ampliphy-AM62x-PD23.2.1/images/ampliphy-xwayland/phyboard-lyra-am62xx-3/phytec-qt5demo-image-phyboard-lyra-am62xx-3.wic.xz

.. _Bmap:
   https://download.phytec.de/Software/Linux/BSP-Yocto-AM62x/BSP-Yocto-Ampliphy-AM62x-PD23.2.1/images/ampliphy-xwayland/phyboard-lyra-am62xx-3/phytec-qt5demo-image-phyboard-lyra-am62xx-3.wic.bmap
