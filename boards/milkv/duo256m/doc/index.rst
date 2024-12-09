.. _duo256m:

Milk-V Duo 256M
###############

Overview
********

See https://milkv.io/duo

Supported Features
==================
The Milk-V Duo 256M board configuration supports the following hardware features:

.. list-table::
   :header-rows: 1

   * - Peripheral
     - Kconfig option
     - Devicetree compatible
   * - Mailbox
     - :kconfig:option:`CONFIG_MBOX`
     - :dtcompatible:`sophgo,cvi-mailbox`
   * - Pin controller
     - :kconfig:option:`CONFIG_PINCTRL`
     - :dtcompatible:`sophgo,cvi-pinctrl`
   * - GPIO
     - :kconfig:option:`CONFIG_GPIO`
     - :dtcompatible:`snps,designware-gpio`
   * - PWM
     - :kconfig:option:`CONFIG_PWM`
     - :dtcompatible:`sophgo,cvi-pwm`
   * - UART
     - :kconfig:option:`CONFIG_SERIAL`
     - :dtcompatible:`ns16550`
   * - PLIC
     - N/A
     - :dtcompatible:`sifive,plic-1.0.0`
   * - SysTick
     - N/A
     - :dtcompatible:`thead,machine-timer`

Other hardware features have not been enabled yet for this board.

The default configuration can be found in
:zephyr_file:`boards/milkv/duo256m/milkv_duo256m_defconfig`.

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
   :board: milkv_duo256m
   :goals: build flash
   :flash-args: --fiptool /path/to/duo-buildroot-sdk/fsbl/plat/cv181x/fiptool.py --fip-bin /path/to/tfcard/fip.bin

Eject the TF card and insert it into the board. Power on the board and you will
see the LED blinking.

.. note::

   Notes for the official buildroot SDK

   1. The Linux running on the big core uses UART0 (GP12/GP13) for the console,
      while to avoid conflict, the Zephyr application (in the default board
      configuration) uses UART1 (GP0/GP1).
   2. Pin multiplexing can be handled either by U-Boot or Zephyr. To utilize
      Zephyr's pin multiplexing, enable :kconfig:option:`CONFIG_PINCTRL` and
      recompile U-Boot without the `PINMUX configs`_. Note that U-Boot boots
      several seconds after Zephyr.
   3. By default, the Linux running on the big core will blink the LED on the
      board. To demonstrate Zephyr (specifically, the ``samples/basic/blinky``
      sample), you should remove the script from the Linux filesystem located at
      ``/mnt/system/blink.sh``.

.. _official SDK:
   https://github.com/milkv-duo/duo-buildroot-sdk

.. _PINMUX configs:
   https://github.com/milkv-duo/duo-buildroot-sdk/blob/develop/build/boards/cv181x/cv1812cp_milkv_duo256m_sd/u-boot/cvi_board_init.c
