.. _rcar_spider_a55:

R-CAR Spider S4 (ARM64)
#######################

Overview
********
R-Car S4 enables to launch Car Server/CoGW with high performance, high-speed networking,
high security and high functional safety levels that are required as E/E architectures
evolve into domains and zones. The R-Car S4 solution allows designers to re-use up to 88
percent of software code developed for 3rd generation R-Car SoCs and RH850 MCU applications.
The software package supports the real-time cores with various drivers and basic software
such as Linux BSP and hypervisors.

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
The Renesas ``rcar_spider_s4/r8a779f0/a55`` board configuration supports the following
hardware features:

+-----------+------------------------------+--------------------------------+
| Interface | Driver/components            | Support level                  |
+===========+==============================+================================+
| PINCTRL   | pinctrl                      |                                |
+-----------+------------------------------+--------------------------------+
| CLOCK     | clock_control                |                                |
+-----------+------------------------------+--------------------------------+
| UART      | serial                       | interrupt-driven/polling       |
+-----------+------------------------------+--------------------------------+

Other hardware features have not been enabled yet for this board.

Programming and Debugging
*************************

The onboard flash is not supported by Zephyr at this time. However, it is possible to
load the Zephyr binary using U-Boot commands.

One of the ways to load Zephyr is shown below.

.. code-block:: console

   tftp 0x48000000 <tftp_server_path/zephyr.bin>
   booti 0x48000000

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rcar_spider_s4/r8a779f0/a55
   :goals: build

References
**********

- `Renesas R-Car Development Support website`_
- `eLinux Spider page`_

.. _Renesas R-Car Development Support website:
   https://www.renesas.com/us/en/support/partners/r-car-consortium/r-car-development-support

.. _eLinux Spider page:
   https://elinux.org/R-Car/Boards/Spider
