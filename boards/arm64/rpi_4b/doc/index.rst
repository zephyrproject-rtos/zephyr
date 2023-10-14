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
   * - UART (Mini UART)
     - :kconfig:option:`CONFIG_SERIAL`
     - :dtcompatible:`brcm,bcm2711-aux-uart`

Other hardware features have not been enabled yet for this board.

The default configuration can be found in the defconfig file:

        ``boards/arm64/rpi_4b/rpi_4b_defconfig``

Programming and Debugging
*************************

Flashing
========

1. Install Raspberry Pi OS using Raspberry Pi Imager. see <https://www.raspberrypi.com/software/>.

2. Add `kernel=zephyr.bin` and `arm_64bit=1` in `config.txt`. see <https://www.raspberrypi.com/documentation/computers/config_txt.html#kernel>

.. code-block:: console

	*** Booting Zephyr OS build XXXXXXXXXXXX  ***
	Hello World! Raspberry Pi 4 Model B!
