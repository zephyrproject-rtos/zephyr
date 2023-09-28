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

+-----------+------------+--------------------------------------+
| Interface | Controller | Driver/Component                     |
+===========+============+======================================+
| GIC-400   | on-chip    | GICv2 interrupt controller           |
+-----------+------------+--------------------------------------+
| UART      | on-chip    | Mini uart serial port                |
+-----------+------------+--------------------------------------+

Other hardware features have not been enabled yet for this board.

The default configuration can be found in the defconfig file:

        ``boards/arm/rpi_4b/rpi_4b_defconfig``

Programming and Debugging
*************************

Flashing
========

1. Install Raspberry Pi OS using Raspberry Pi Imager. see <https://www.raspberrypi.com/software/>.

2. add `kernel=zephyr.bin` in `config.txt`. see <https://www.raspberrypi.com/documentation/computers/config_txt.html#kernel>

.. code-block:: console

	*** Booting Zephyr OS build XXXXXXXXXXXX  ***
	Hello World! Raspberry Pi 4 Model B!
