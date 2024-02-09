.. rpi_4b:

Raspberry Pi 4 Model B (Cortex-A72)
###################################

Overview
********
see <https://www.raspberrypi.com/products/raspberry-pi-4-model-b/specifications/>

Hardware
********
see <https://www.raspberrypi.com/documentation/computers/raspberry-pi.html>

Supported Features
==================
The Raspberry Pi 4 Model B board configuration supports the following
hardware features:

.. list-table::
   :header-rows: 1

   * - Peripheral
     - Kconfig option
     - Devicetree compatible
   * - GIC-400
     - N/A
     - :dtcompatible:`arm,gic-v2`
   * - GPIO
     - :kconfig:option:`CONFIG_GPIO`
     - :dtcompatible:`brcm,bcm2711-gpio`
   * - UART (Mini UART)
     - :kconfig:option:`CONFIG_SERIAL`
     - :dtcompatible:`brcm,bcm2711-aux-uart`

Other hardware features have not been enabled yet for this board.

The default configuration can be found in
:zephyr_file:`boards/raspberrypi/rpi_4b/rpi_4b_defconfig`

Programming and Debugging
*************************

TF Card
=======

Prepare a TF card with MBR and FAT32. In the root directory of the TF card:

1. Download and place these firmware files:

   * `bcm2711-rpi-4-b.dtb <https://raw.githubusercontent.com/raspberrypi/firmware/master/boot/bcm2711-rpi-4-b.dtb>`_
   * `bootcode.bin <https://raw.githubusercontent.com/raspberrypi/firmware/master/boot/bootcode.bin>`_
   * `start4.elf <https://raw.githubusercontent.com/raspberrypi/firmware/master/boot/start4.elf>`_

2. Copy ``build/zephyr/zephyr.bin``
3. Create a ``config.txt``:

   .. code-block:: text

      kernel=zephyr.bin
      arm_64bit=1
      enable_uart=1
      uart_2ndstage=1

Insert the card and power on the board. You should see the following output on
the serial console (GPIO 14/15):

.. code-block:: text

   *** Booting Zephyr OS build XXXXXXXXXXXX  ***
   Hello World! Raspberry Pi 4 Model B!
