.. zephyr:board:: 3r_tnh_sensor_lite

Overview
********

The ThirdReality Temperature and Humidity Sensor Lite integrates the BL704L SoC from Bouffalo Lab.
Internally, the device breaks out the pinout for JTAG, as well as UART and the bootstrap pin,
making it perfectly suited for development.
BL704L is a highly integrated 802.15.4 (Zigbee/Thread/Matter) and BLE combo chipset for IoT applications.

Hardware
********

Pins 17-20 are broken out on the JTAG header.

For more information about the BouffaloLab BL704L MCU and the device:

- `Bouffalo Lab BL702L MCU Datasheet`_
- `Bouffalo Lab Development Zone`_
- `ThirdReality Temperature and Humidity Sensor Lite`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The Device is configured to run at max speed (128MHz).

Serial Port
===========

The ``3r_tnh_sensor_lite`` board uses UART0 as default serial port.


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Samples
=======

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample
   application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: 3r_tnh_sensor_lite
      :goals: build flash

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyUSB0`. For example:

   .. code-block:: console

      $ screen /dev/ttyUSB0 115200

   Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

   Then, press and release RST button

   .. code-block:: console

      *** Booting Zephyr OS build v4.3 ***
      Hello World! 3r_tnh_sensor_lite/bl704l10q2i

Congratulations, you have ``3r_tnh_sensor_lite`` configured and running Zephyr.

.. _Bouffalo Lab BL702L MCU Datasheet:
	https://dev.bouffalolab.com/document?id=guest

.. _Bouffalo Lab Development Zone:
	https://dev.bouffalolab.com/home?id=guest

.. _ThirdReality Temperature and Humidity Sensor Lite:
	https://thirdreality.com/product/temperature-and-humidity-sensor-lite/
