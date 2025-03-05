.. zephyr:board:: khadas_edgev

Overview
********

See <https://www.khadas.com/edge-v>

Hardware
********

See <https://docs.khadas.com/linux/edge/Hardware.html#Edge-V-1>

Supported Features
==================

.. zephyr:board-supported-hw::

There are multiple serial ports on the board: Zephyr is using
uart2 as serial console.

Programming and Debugging
*************************

Use the following configuration to run basic Zephyr applications and
kernel tests on Khadas Edge-V board. For example, with the :zephyr:code-sample:`hello_world`:

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
