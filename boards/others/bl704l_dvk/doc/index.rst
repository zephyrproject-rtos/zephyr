.. zephyr:board:: bl704l_dvk

Overview
********

This board is a development platform designed for usage with Zephyr RTOS, filling in for the absence
of official BL70xL development boards, it features BL704L and CH32V203.
BL704L is a highly integrated 802.15.4 (Zigbee/Thread/Matter) and BLE combo chipset for IoT applications.
CH32V203 is used as the interfacing chip, providing UART and JTAG conversion to USB.

Hardware
********

BL704L_DVK provides the following hardware components:

- BL704L20Q2IW low-power RF microcontroller. 32MHz and 32.768KHz crystals are populated.

- CH32V203F8U6 general purpose microcontroller.

- Eight Amber-colored LEDs:

   - Two connected to GPIO17 and GPIO18 of BL704L
   - Five forming a line connected to PB14, PB15, PA8, PA9, and PA10 of CH32V203
   - One additional LED indicating DAPLink status, connected to PA5 of CH32V203

- Three Buttons:

   - Reset Button for BL704L (SW1)
   - Boot Select Button for BL704L (SW2, GPIO16)
   - User Button (SW3, GPIO13)

- FPC connector for Reflective LCD displays using 24Pin headers. A 2.13" ST7305 display is provided by default.

- Pads for I2C DFN-6 sensors (CHT8315, HTU21D) and various formats of storage chips are provided.

For more information about the BouffaloLab BL704L MCU and the device:

- `Bouffalo Lab BL702L MCU Datasheet`_
- `Bouffalo Lab Development Zone`_
- `BL704L_DVK Schematic`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The Device is configured to run at max speed (128MHz).

Serial Port
===========

The ``bl704l_dvk`` board uses ACM0 as default serial port.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Samples
=======

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample
   application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: bl704l_dvk
      :goals: build flash

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyACM0`. For example:

   .. code-block:: console

      $ screen /dev/ttyACM0 115200

   Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

   Then, press and release RST button

   .. code-block:: console

      *** Booting Zephyr OS build v4.4 ***
      Hello World! bl704l_dvk/bl704l20q2iw

Congratulations, you have ``bl704l_dvk`` configured and running Zephyr.

.. _Bouffalo Lab BL702L MCU Datasheet:
	https://dev.bouffalolab.com/document?id=guest

.. _Bouffalo Lab Development Zone:
	https://dev.bouffalolab.com/home?id=guest

.. _BL704L_DVK Schematic:
	https://github.com/VynDragon/bl704l_dvk/blob/main/output/BL704L_DVK.pdf
