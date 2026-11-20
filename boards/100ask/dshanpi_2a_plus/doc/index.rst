.. zephyr:board:: dshanpi_2a_plus

Overview
********

The `DShanPi 2A+`_ is an embedded board based on the Rockchip RK3506B SoC.
The SoC contains three Arm Cortex-A7 cores and one Cortex-M0 core. This board
port supports the three Cortex-A7 cores in SMP mode; the Cortex-M0 core is not
supported.

The board provides 512 MiB of DDR3 memory and boots from on-board eMMC. DDR,
clock, and pinmux initialization must be performed by the bootloader before
starting Zephyr.

Hardware
********

* Rockchip RK3506B SoC
* 3 x Arm Cortex-A7 cores
* 512 MiB DDR3
* On-board eMMC
* UART0 debug console

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

There is currently no Zephyr flash or debug runner for this board. The RK3506
requires boot firmware to initialize DRAM, clocks, and pinmux. In the tested
setup, U-Boot loads OP-TEE, which provides the PSCI services used to start the
secondary Cortex-A7 cores, and then starts Zephyr from DDR.

The board boots from on-board eMMC. The following example assumes that
``zephyr.bin`` has already been written to eMMC block ``0x8000`` and that
Zephyr is linked at ``0x00108000``:

.. code-block:: console

   => mmc dev 0
   => mmc read 0x00108000 0x8000 0x1000
   => go 0x00108000

In this example, ``0x8000`` is the starting block in the eMMC user area
(16 MiB), and ``0x1000`` is the number of 512-byte sectors reserved for the
Zephyr image (2 MiB). The eMMC offset is not defined by the Zephyr board port;
adjust these values to match the complete boot image and bootloader layout.

The UART0 console runs at 1500000 baud.

.. note::

   USB MaskROM recovery is board- and vendor-specific and is not a Zephyr
   runner. If it is used, the complete boot image and eMMC layout must match
   the commands used by the boot firmware.

Building
========

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: dshanpi_2a_plus/rk3506/a7/smp
   :goals: build

The expected output is:

.. code-block:: console

   *** Booting Zephyr OS build ... ***
   Hello World! dshanpi_2a_plus/rk3506/a7/smp

.. _DShanPi 2A+:
   https://wiki.dshanpi.org
