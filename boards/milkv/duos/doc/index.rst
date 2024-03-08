.. zephyr:board:: milkv_duos

Overview
********

See https://milkv.io/duo-s

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

Prepare a TF card and follow the instructions in the `official SDK`_ to build
and flash the image.

After that, you will get a ``fip.bin`` file at the root of the TF card, where
the RTOS image is packed.

The following steps demonstrate how to build the blinky sample and update the
``fip.bin`` file on the TF card.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: milkv_duos/sg2000/c906_1
   :goals: build flash
   :flash-args: --fiptool /path/to/duo-buildroot-sdk/fsbl/plat/cv181x/fiptool.py --fip-bin /path/to/tfcard/fip.bin

Eject the TF card and insert it into the board. Power on the board and you will
see the LED blinking.

.. note::

   Notes for the official buildroot SDK

   1. The Linux running on the big core uses UART0 (A16/A17) for the console,
      while to avoid conflict, the Zephyr application (in the default board
      configuration) uses UART2 (A19/A18).
   2. Pin multiplexing can be handled either by U-Boot or Zephyr. To utilize
      Zephyr's pin multiplexing, enable :kconfig:option:`CONFIG_PINCTRL` and
      recompile U-Boot without the `PINMUX configs`_. Note that U-Boot boots
      several seconds after Zephyr.

.. _official SDK:
   https://github.com/milkv-duo/duo-buildroot-sdk

.. _PINMUX configs:
   https://github.com/milkv-duo/duo-buildroot-sdk/blob/develop/build/boards/cv181x/cv1813h_milkv_duos_sd/u-boot/cvi_board_init.c
