.. zephyr:board:: dt_xt_zb1_devkit

Overview
********

XT-ZB1 Zigbee 3.0 and BLE 5.0 coexistence Module is a highly integrated single-chip solution
providing Zigbee and BLE in a single chip.
It also provides a bunch of configurable GPIO, which are configured as digital
peripherals for different applications and control usage.

The XT-ZB1 Module use BL702 as Zigbee and BLE coexistence soc chip.
The embedded memory configuration provides simple application developments.

Hardware
********

For more information about the Bouffalo Lab BL702 MCU:

- `Bouffalo Lab BL702 MCU Website`_
- `Bouffalo Lab BL702 MCU Datasheet`_
- `Bouffalo Lab Development Zone`_
- `Doctors of Intelligence & Technology (www.doiting.com)`_

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The XT-ZB1 board is configured to run at max speed by default (144 MHz).

Serial Port
===========

The ``dt_xt_zb1_devkit`` board uses UART0 as default serial port. It is connected
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
      :board: dt_xt_zb1_devkit
      :goals: build flash

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyUSB0`. For example:

   .. code-block:: console

      $ screen /dev/ttyUSB0 115200

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

   Then, press and release RST button

   .. code-block:: console

      *** Booting Zephyr OS build v4.2.0-1031-g63c9db88d01a ***
      Hello World! dt_xt_zb1_devkit/bl702c10q2h

Congratulations, you have ``dt_xt_zb1_devkit`` configured and running Zephyr.


.. _Bouffalo Lab BL702 MCU Website:
	https://www.bouffalolab.com/product/?type=detail&id=8

.. _Bouffalo Lab BL702 MCU Datasheet:
	https://github.com/bouffalolab/bl_docs/tree/main/BL702_DS/en

.. _Bouffalo Lab Development Zone:
	https://dev.bouffalolab.com/home?id=guest

.. _Doctors of Intelligence & Technology (www.doiting.com):
	https://www.doiting.com
