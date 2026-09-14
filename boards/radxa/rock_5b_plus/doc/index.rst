.. zephyr:board:: rock_5b_plus

Overview
********

The Radxa ROCK 5B+ is a single-board computer based on the Rockchip RK3588.
The SoC combines four Cortex-A76 and four Cortex-A55 CPU cores. The board is
available with up to 32 GB of LPDDR5 memory and provides eMMC, microSD, PCIe,
USB, Ethernet, and a 40-pin expansion header.

The SMP variant supports all four Cortex-A55 and four Cortex-A76 cores.

Supported Features
==================

.. zephyr:board-supported-hw::

The onboard Ethernet interface is not currently supported by Zephyr.

Serial Port
===========

Zephyr uses UART2 as the serial console at 1.5 Mbit/s.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications are built with the standard configuration, for example:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: rock_5b_plus
   :goals: build

To run Zephyr on all eight CPU cores, build the SMP variant:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: rock_5b_plus//smp
   :goals: build

Create a legacy U-Boot image whose payload loads and runs at ``0x10000000``:

.. code-block:: console

   mkimage -C none -A arm64 -O linux -a 0x10000000 -e 0x10000000 \
     -d build/zephyr/zephyr.bin build/zephyr/zephyr.img

The Radxa U-Boot AArch64 Linux boot path requires a devicetree. Obtain
``rk3588-rock-5b-plus.dtb`` from the Radxa Linux distribution and copy it and
``zephyr.img`` to a FAT partition accessible from U-Boot.

Check the temporary kernel and devicetree load addresses provided by U-Boot:

.. code-block:: console

   printenv kernel_addr_r fdt_addr_r

Load each file at its corresponding address, then pass the devicetree address
to ``bootm``. Adjust the MMC device and partition numbers for the selected boot
media:

.. code-block:: console

   fatload mmc 1:1 ${kernel_addr_r} zephyr.img
   fatload mmc 1:1 ${fdt_addr_r} rk3588-rock-5b-plus.dtb
   bootm ${kernel_addr_r} - ${fdt_addr_r}

The legacy image header instructs U-Boot to move the Zephyr payload from
``kernel_addr_r`` to ``0x10000000`` before jumping to the entry point. Keeping
the temporary image, devicetree, and Zephyr runtime regions separate avoids
overwriting any of them during the handoff.

References
==========

More information is available from the `Radxa ROCK 5B+ documentation`_.

.. _Radxa ROCK 5B+ documentation:
   https://docs.radxa.com/en/rock5/rock5b
