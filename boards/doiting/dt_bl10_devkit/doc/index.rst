.. zephyr:board:: dt_bl10_devkit

DT-BL10 Development Kit
#######################

Overview
********

DT-BL10 Wi-Fi and BLE coexistence Module is a highly integrated single-chip
low power 802.11n Wireless LAN (WLAN) network controller. It combines an RISC
CPU, WLAN MAC, a lT1R capable WLAN baseband, RF, and Bluetooth in a single chip.
It also provides a bunch of configurable GPIO, which are configured as digital
peripherals for different applications and control usage.

DT-BL10 WiFi Module use BL602 as Wi-Fi and BLE coexistence soc chip. DT-BL10
WiFi Module integrates internal memories for complete WIFI protocol functions.
The embedded memory configuration also provides simple application developments.

DT-BL10 WiFi module supports the standard IEEE 802.11 b/g/n/e/i protocol and the
complete TCP/IP protocol stack. User can use it to add the WiFi function for the
installed devices, and also can be viewed as a independent network controller.

Hardware
********

For more information about the Bouffalo Lab BL-602 MCU:

- `Bouffalo Lab BL602 MCU Website`_
- `Bouffalo Lab BL602 MCU Datasheet`_
- `Bouffalo Lab Development Zone`_
- `dt_bl10_devkit Schematic`_
- `Doctors of Intelligence & Technology (www.doiting.com)`_
- `The RISC-V BL602 Book`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The DT-BL10 board is configured to run at max speed (192MHz).

Serial Port
===========

The ``dt_bl10_devkit`` board uses UART0 as default serial port. It is connected
to USB Serial converter and port is used for both program and console.


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Samples
=======

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample
application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: dt_bl10_devkit
      :goals: build

#. To flash an image using blflash runner:

   #. Press D8 button

   #. Press and release EN button

   #. Release D8 button

   .. code-block:: console

      west flash

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyUSB0`. For example:

   .. code-block:: console

      $ minicom -D /dev/ttyUSB0 -o

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

   Then, press and release EN button

   .. code-block:: console

      *** Booting Zephyr OS build v4.1.0-4682-g21b20de1eb34 ***
      Hello World! dt_bl10_devkit/bl602c20q2i

Congratulations, you have ``dt_bl10_devkit`` configured and running Zephyr.


.. _Bouffalo Lab BL602 MCU Website:
	https://www.bouffalolab.com/bl602

.. _Bouffalo Lab BL602 MCU Datasheet:
	https://github.com/bouffalolab/bl_docs/tree/main/BL602_DS/en

.. _Bouffalo Lab Development Zone:
	https://dev.bouffalolab.com/home?id=guest

.. _dt_bl10_devkit Schematic:
	https://github.com/SmartArduino/Doiting_BL/blob/master/board/DT-BL10%20User%20Mannual.pdf

.. _Doctors of Intelligence & Technology (www.doiting.com):
	https://www.doiting.com

.. _The RISC-V BL602 Book:
	https://lupyuen.github.io/articles/book

.. _Flashing Firmware to BL602:
	https://lupyuen.github.io/articles/book#flashing-firmware-to-bl602
