.. zephyr:board:: khadas_edge2

Overview
********

See `Product page`_

.. _Product page: https://www.khadas.com/edge2

Hardware
********

See `Hardware details`_

.. _Hardware details: https://docs.khadas.com/products/sbc/edge2/hardware/start

Supported Features
==================

.. zephyr:board-supported-hw::

There are multiple serial ports on the board: Zephyr is using
uart2 as serial console.

Programming and Debugging
*************************

Use the following configuration to run basic Zephyr applications and
kernel tests on Khadas Edge2 board. For example, with the :zephyr:code-sample:`hello_world`:

1. Non-SMP mode

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: khadas_edge2
   :goals: build

This will build an image with the hello world sample app.

Build the zephyr image:

.. code-block:: console

   mkimage -C none -A arm64 -O linux -a 0x10000000 -e 0x10000000 -d build/zephyr/zephyr.bin build/zephyr/zephyr.img

Burn the image on the board (we choose to use Rockchip burning tool `rkdeveloptool <https://github.com/rockchip-linux/rkdeveloptool.git>`_, you will need a `SPL <http://dl.khadas.com/products/edge2/firmware/boot/>`_ which is provided by khadas:

.. code-block:: console

   rkdeveloptool db rk3588_spl_loader_*; rkdeveloptool wl 0x100000 zephyr.img; rkdeveloptool rd

The sector 0x100000 was chosen arbitrarily (far away from U-Boot image)

Use U-Boot to load and run Zephyr:

.. code-block:: console

   mmc read ${pxefile_addr_r} 0x100000 0x1000; bootm start ${pxefile_addr_r}; bootm loados; bootm go

0x1000 is the size (in number of sectors) or your image. Increase it if needed.

It will display the following console output:

.. code-block:: console

   *** Booting Zephyr OS build XXXXXXXXXXXX  ***
   Hello World! khadas_edge2

Flashing
========

Zephyr image can be loaded in DDR memory at address 0x10000000 from SD Card,
EMMC, QSPI Flash or downloaded from network in uboot.

References
==========

`Edge2 Documentation`_

.. _Edge2 Documentation: https://docs.khadas.com/products/sbc/edge2/start
