.. zephyr:board:: rpi_4b

Overview
********
see <https://www.raspberrypi.com/products/raspberry-pi-4-model-b/specifications/>

Hardware
********
see <https://www.raspberrypi.com/documentation/computers/raspberry-pi.html>

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

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
