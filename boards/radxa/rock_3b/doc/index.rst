.. zephyr:board:: rock_3b

Overview
********

The `Radxa ROCK 3B`_ is a single-board computer based on the Rockchip RK3568 SoC, featuring
a quad-core Arm Cortex-A55 CPU, LPDDR4 memory, dual Gigabit Ethernet, USB, display, storage,
and expansion interfaces.

Hardware
********

The ROCK 3B includes the following hardware features:

* Rockchip RK3568 SoC
* Quad-core Arm Cortex-A55 CPU, up to 2 GHz
* Arm Mali-G52 GPU
* 64-bit LPDDR4 RAM, commonly available in 2 GB, 4 GB, and 8 GB variants
* eMMC storage (optional)
* microSD card slot
* HDMI, MIPI DSI, and eDP display interfaces
* MIPI CSI camera interface
* Two Gigabit Ethernet ports
* Two USB 2.0 host ports
* One USB 3.0 host port
* One USB 3.0 OTG port
* M.2 M Key slot with PCIe 3.0 x2 for NVMe SSDs
* M.2 E Key slot for wireless modules
* M.2 B Key slot and SIM slot for cellular modules
* 40-pin Raspberry Pi compatible expansion header
* 2-pin 5 V fan connector

More details can be found in `Rockchip RK3568 TRM (Part1)`_ and `Rockchip RK3568 TRM (Part2)`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Run program from U-Boot
=======================

To execute the program, it must be loaded from U-Boot into memory and then run.
Here, we show how to load an image from an SD card and execute it using the U-Boot console.
The UART console can be connected via pins 8 and 10 of the 40-pin connector.

.. code-block:: console

    => setenv zephyr_addr 0x40000000
    => load mmc 1:1 ${zephyr_addr} /zephyr.bin
    reading /zephyr.bin
    36988 bytes read in 7 ms (5 MiB/s)
    => echo ${filesize}
    0x907c
    => go ${zephyr_addr}
    ## Starting application at 0x40000000 ...


Building & Flashing
===================

You can build the :zephyr:code-sample:`hello_world` sample by following the steps below.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rock_3b
   :goals: build

When you launch the program from U-Boot using the procedure described above,
you will see output similar to the following.

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0-6528-g087c82b397b6 ***
   Hello World! rock_3b/rk3568

.. note::

   Debugging and flashing via west command are not supported at this time.

The ``rock_3b//smp`` target is also supported, use this configuration to run smp application.

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :board: rock_3b//smp
   :goals: build


.. target-notes::

.. _Radxa ROCK 3B:
   https://radxa.com/products/rock3/3b/

.. _Rockchip RK3568 TRM (Part1):
   https://dl.radxa.com/rock3/docs/hw/datasheet/Rockchip%20RK3568%20TRM%20Part1%20V1.1-20210301.pdf

.. _Rockchip RK3568 TRM (Part2):
   https://dl.radxa.com/rock3/docs/hw/datasheet/Rockchip%20RK3568%20TRM%20Part2%20V1.1-20210301.pdf
