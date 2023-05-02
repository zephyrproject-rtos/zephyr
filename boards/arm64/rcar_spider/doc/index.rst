.. _rcar_spider:

R-CAR Spider (ARMv8)
#####################################
Note: currently, the board can be used only as a Xen Initial Domain, i.e. Dom0,
because there isn't support of UART driver. So, it can be built only with ``xen_dom0``
shield.

Overview
********
The R-Car Spider (S4) is an SOC that features the basic functions for
next-generation car Gateway systems.

Hardware
********
The R-Car S4 includes:

* eight 1.2GHz Arm Cortex-A55 cores, 2 cores x 4 clusters;
* 1.0 GHz Arm Cortex-R52 core (hardware Lock step is supported);
* two 400MHz G4MH cores (hardware Lock step is supported);
* memory controller for LPDDR4X-3200 with 32bit bus (16bit x 1ch + 16bit x 1ch) with ECC;
* SD card host interface / eMMC;
* UFS 3.0 x 1 channel;
* PCI Express Gen4.0 interface (Dual lane x 2ch);
* ICUMX;
* ICUMH;
* SHIP-S x 3 channels;
* AES Accerator x 8 channels;
* CAN FD interface x 16 channels;
* R-Switch2 (Ether);
* 100base EtherAVB x 1 channel;
* Gbit-EtherTSN x 3 channels;
* 1 unit FlexRay (A,B 2ch) interface.

Supported Features
==================
The Renesas rcar_spider board configuration supports the following
hardware features:

+-----------+------------------------------+--------------------------------+
| Interface | Driver/components            | Support level                  |
+===========+==============================+================================+
| PINCTRL   | pinctrl                      |                                |
+-----------+------------------------------+--------------------------------+
| CLOCK     | clock_control                |                                |
+-----------+------------------------------+--------------------------------+
| MMC       | renesas_rcar_mmc             |                                |
+-----------+------------------------------+--------------------------------+

Other hardware features have not been enabled yet for this board.

The default configuration can be found in the defconfig file:

        ``boards/arm64/rcar_spider/rcar_spider_defconfig``

Programming and Debugging
*************************

Correct shield designation for Xen must be entered when you invoke ``west build``.
For example:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :board: rcar_spider
   :shield: xen_dom0
   :goals: build

For more details, look at documentation of ``xen_dom0`` shield.

References
**********

- `Renesas R-Car Development Support website`_
- `eLinux Spider page`_

.. _Renesas R-Car Development Support website:
   https://www.renesas.com/us/en/support/partners/r-car-consortium/r-car-development-support

.. _eLinux Spider page:
   https://elinux.org/R-Car/Boards/Spider
