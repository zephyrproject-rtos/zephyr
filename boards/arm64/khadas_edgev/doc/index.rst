.. _khadas_edgev:

Khadas Edge-V
#################################

Overview
********

See <https://www.khadas.com/edge-v>

Hardware
********

See <https://docs.khadas.com/linux/edge/Hardware.html#Edge-V-1>

Supported Features
==================

Khadas Edge-V board default configuration supports the following
hardware features:

+-----------+------------+--------------------------------------+
| Interface | Controller | Driver/Component                     |
+===========+============+======================================+
| GIC-500   | on-chip    | GICv3 interrupt controller           |
+-----------+------------+--------------------------------------+
| ARM TIMER | on-chip    | System Clock                         |
+-----------+------------+--------------------------------------+
| UART      | on-chip    | Synopsys DesignWare 8250 serial port |
+-----------+------------+--------------------------------------+

Other hardware features have not been enabled yet for this board.

The default configuration can be found in the defconfig file for NON-SMP:

        ``boards/arm64/khadas_edgev/khadas_edgev_defconfig``

There are multiple serial ports on the board: Zephyr is using
uart2 as serial console.

Programming and Debugging
*************************

Use the following configuration to run basic Zephyr applications and
kernel tests on Khadas Edge-V board. For example, with the :ref:`hello_world`:

1. Non-SMP mode

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: khadas_edgev
   :goals: build

This will build an image with the synchronization sample app.

Build the zephyr image:

.. code-block:: console

	mkimage -C none -A arm64 -O linux -a 0x10000000 -e 0x10000000 -d build/zephyr/zephyr.bin build/zephyr/zephyr.img

Use u-boot to load and kick Zephyr.bin to CPU Core0:

.. code-block:: console

	tftpboot ${pxefile_addr_r} zephyr.img; bootm start ${pxefile_addr_r}; bootm loados; bootm go

It will display the following console output:

.. code-block:: console

	*** Booting Zephyr OS build XXXXXXXXXXXX  ***
	Hello World! khadas_edgev

Flashing
========

Zephyr image can be loaded in DDR memory at address 0x10000000 from SD Card,
EMMC, QSPI Flash or downloaded from network in uboot.

References
==========

`Documentation: <https://docs.khadas.com/linux/edge/>`_
