.. _duos:

Milk-V Duo S
############

Overview
********

See https://milkv.io/duo-s

Supported Features
==================
The Milk-V Duo S board configuration supports the following hardware features:

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

The default configuration can be found in the defconfig file:

        ``boards/milkv/duos/milkv_duos_defconfig``

Programming and Debugging
*************************

Prepare a TF card and follow the instructions in the
`official SDK <https://github.com/milkv-duo/duo-buildroot-sdk>`_ to build and
flash the image.

After that, you will get a ``fip.bin`` file at the root of the TF card. Use the
following command to replace the RTOS image with Zephyr:

.. code-block:: sh

   python3 /path/to/duo-buildroot-sdk/fsbl/plat/cv181x/fiptool.py \
       -v genfip "/path/to/target/fip.bin" \
       --OLD_FIP="/path/to/original/fip.bin" \
       --BLCP_2ND="build/zephyr/zephyr.bin"

Replace the original ``fip.bin`` on the TF card with the new one.

.. note::

   Notes for the official buildroot SDK

   1. The Linux running on the big core uses UART0 (A16/A17) for the console,
      while to avoid conflict, the Zephyr application (in the default board
      configuration) uses UART2 (B11/B12).
   2. Thus, you need to recompile the Buildroot SDK to disable any unused
      `PINMUX configs <https://github.com/milkv-duo/duo-buildroot-sdk/blob/develop/build/boards/cv181x/cv1813h_milkv_duos_sd/u-boot/cvi_board_init.c>`_
      from the U-Boot source code, so they won't override the configuration
      set by Zephyr.
