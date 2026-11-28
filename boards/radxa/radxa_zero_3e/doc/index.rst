.. zephyr:board:: radxa_zero_3e

Overview
********

The `Radxa ZERO 3E`_ is a compact single-board computer based on the Rockchip RK3566 SoC,
featuring a quad-core Arm Cortex-A55 CPU, LPDDR4 memory, Gigabit Ethernet, USB, a microSD
card slot and a 40-pin expansion header.

Hardware
********

The ZERO 3E includes the following hardware features:

* Rockchip RK3566 SoC with a quad-core Arm Cortex-A55 CPU running at up to 1.6 GHz
* 1, 2, 4 or 8 GB of LPDDR4 RAM, depending on the variant
* microSD card slot (no eMMC)
* Gigabit Ethernet
* Two USB Type-C ports (one USB 3.0 host and one USB 2.0 OTG)
* 40-pin expansion header

More details can be found in the `Radxa ZERO 3E documentation`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Run program from U-Boot
=======================

To execute the program, it must be loaded from U-Boot into memory and then run.
Here, we show how to load an image from the SD card (replace ``<dev>:<part>`` with the
MMC device and partition holding ``zephyr.bin``) and execute it using the U-Boot console.
The image is linked and loaded at ``0x40000000``.

.. note::

   This was tested on a 2 GB board. On the 1 GB variant DRAM ends at ``0x40000000``, so a
   different load address would be needed.

The debug UART is UART2 on the 40-pin expansion header: pin 8 (TX), pin 10 (RX) and pin 6
(GND), at 1500000 baud, 8N1. Connect a USB-to-serial adapter that supports 1.5 Mbaud to
these pins. Zephyr does not configure the pin muxing; it relies on U-Boot having set up UART2
as its console.

.. code-block:: console

    => setenv zephyr_addr 0x40000000
    => load mmc <dev>:<part> ${zephyr_addr} /zephyr.bin
    => go ${zephyr_addr}
    ## Starting application at 0x40000000 ...

Building & Flashing
===================

You can build the :zephyr:code-sample:`hello_world` sample by following the steps below.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: radxa_zero_3e/rk3566
   :goals: build

Copy ``build/zephyr/zephyr.bin`` to a partition of the SD card that is readable from U-Boot, and
run it as described above. The following output is displayed on the UART console:

.. code-block:: console

   *** Booting Zephyr OS build v4.5.99-xxxx ***
   Hello World! radxa_zero_3e/rk3566

.. note::

   Debugging and flashing via west command are not supported at this time.

The ``radxa_zero_3e/rk3566/smp`` target is also supported, use this configuration to run SMP
applications on all four cores.

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :board: radxa_zero_3e/rk3566/smp
   :goals: build

.. target-notes::

.. _Radxa ZERO 3E:
   https://radxa.com/products/zeros/zero3e/

.. _Radxa ZERO 3E documentation:
   https://docs.radxa.com/en/zero/zero3
