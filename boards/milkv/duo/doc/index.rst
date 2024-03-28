.. _duo:

Milk-V Duo
##########

Overview
********

See https://milkv.io/duo

Supported Features
==================
The Milk-V Duo board configuration supports the following hardware features:

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

        ``boards/milkv/duo/milkv_duo_defconfig``

Programming and Debugging
*************************

Prepare a TF card and follow the instructions in the
`official SDK <https://github.com/milkv-duo/duo-buildroot-sdk>`_ to build and
flash the image.

After that, you will get a ``fip.bin`` file at the root of the TF card. Use the
following command to replace the RTOS image with Zephyr:

.. code-block:: sh

   python3 /path/to/duo-buildroot-sdk/fsbl/plat/cv180x/fiptool.py \
       -v genfip "/path/to/target/fip.bin" \
       --OLD_FIP="/path/to/original/fip.bin" \
       --BLCP_2ND="build/zephyr/zephyr.bin"

Replace the original ``fip.bin`` on the TF card with the new one.

.. note::

   Notes for the official buildroot SDK

   1. The Linux running on the big core uses UART0 (GP12/GP13) for the console,
      while to avoid conflict, the Zephyr application (in the default board
      configuration) uses UART1 (GP0/GP1).
   2. Thus, you need to recompile the Buildroot SDK to disable any unused
      `PINMUX configs <https://github.com/milkv-duo/duo-buildroot-sdk/blob/develop/build/boards/cv180x/cv1800b_milkv_duo_sd/u-boot/cvi_board_init.c>`_
      from the U-Boot source code, so they won't override the configuration
      set by Zephyr.
   3. By default, the Linux running on the big core will blink the LED on the
      board. For the demonstration of Zephyr (more specifically, the
      ``samples/basic/blinky`` sample), you need to delete that script from the
      Linux filesystem. The script is located at ``/mnt/system/blink.sh``.
