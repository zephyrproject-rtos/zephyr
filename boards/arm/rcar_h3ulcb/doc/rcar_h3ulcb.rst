.. _rcar_h3ulcb_board:

Renesas R-Car H3ULCB
####################

Overview
********
- The H3 Starter Kit board is designed for evaluating the features and performance of the R-CAR H3 device from Renesas Electronics and it is also used for developing and evaluating application software for these R-CAR H3.

- The H3 Starter Kit, based on the R-CAR H3 SIP, comes with LPDDR4 @4GB in 2-channel, each 64-bit wide+Hyperflash @64MB, CSI2 interfaces and several communication interfaces like USB, Ethernet, HDMI and can work standalone or can be adapted to other boards, via 440pin connector on bottom side.

It is possible to order 2 different types of H3 Starter Kit Boards, one with Ethernet connection onboard and one with Ethernet connection on ComExpress.

.. figure:: img/rcar_h3ulcb_starter_kit.jpg
   :width: 460px
   :align: center
   :height: 288px
   :alt: R-Car starter kit

.. note:: The H3ULCB board can be plugged on a Renesas Kingfisher Infotainment daughter board through COM Express connector in order to physically access more I/O. |br| To use Kingfisher I/O please use :ref:`rcar_h3_kingfisher_board` board.

CAUTION: In this case, power supply is managed by the daughter board.

More information about the board can be found at `Renesas R-Car Starter Kit website`_.

Hardware
********

Hardware capabilities for the H3ULCB can be found on the `eLinux H3SK page`_ of the board.

.. figure:: img/rcar_h3ulcb_features.jpg
   :width: 286px
   :align: center
   :height: 280px
   :alt: R-Car starter kit features

.. note:: Zephyr will be booted on the CR7 processor provided for RTOS purpose.

More information about the SoC that equips the board can be found here:

- `Renesas R-Car H3 chip`_

Supported Features
==================

Here is the current supported features when running Zephyr Project on the R-Car H3ULCB CR7:

+-----------+------------------------------+--------------------------------+
| Interface | Driver/components            | Support level                  |
+===========+==============================+================================+
| PINMUX    | pinmux                       |                                |
+-----------+------------------------------+--------------------------------+
| CLOCK     | clock_control                |                                |
+-----------+------------------------------+--------------------------------+
| GPIO      | gpio                         |                                |
+-----------+------------------------------+--------------------------------+
| UART      | uart                         | serial port-polling            |
+           +                              +                                +
|           | FT232RQ / CP2102             | serial port-interrupt          |
+-----------+------------------------------+--------------------------------+

It's also currently possible to write on the ram console.

More features will be supported soon.

Connections and IOs
===================

H3ULCB Board
------------

Here are official IOs figures from eLinux for H3ULCB board:

`H3SK top view`_

`H3SK bottom view`_

GPIO
----

By running Zephyr on H3ULCB, the software readable push button 'SW3' can be used as input, and the software controllable LED 'LED5' can be used as output.

UART
----

H3ULCB board is providing two serial ports, only one is commonly available on the board, however, the second one can be made available either by welding components or by plugging the board on a Kingfisher Infotainment daughter board.

Here is information about these serial ports:

+--------------------+-------------------+--------------------+-----------+--------------------------------------+
| Physical Interface | Physical Location | Software Interface | Converter | Further Information                  |
+====================+===================+====================+===========+======================================+
| CN12 DEBUG SERIAL  | ULCB Board        | SCIF2              | FT232RQ   | Used by U-BOOT & Linux               |
+--------------------+-------------------+--------------------+-----------+--------------------------------------+
| CN10 DEBUG SERIAL  | ULCB Board        | SCIF1              | CP2102    | Non-welded                           |
+--------------------+-------------------+--------------------+-----------+--------------------------------------+

.. note:: The Zephyr console output is assigned to SCIF2 with settings 115200 8N1 without hardware flow control by default. |br| As U-BOOT and Linux are also using this UART, it may be complicated to use app that need UART input.

CN12 connector is a standard micro USB connector.

Programming and Debugging
*************************

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Supported Debug Probe
=====================

The "Olimex ARM-USB-OCD-H" probe is the only officially supported probe. This probe is supported by OpenOCD that is shipped with the Zephyr SDK.

The "Olimex ARM-USB-OCD-H" probe needs to be connected with a SICA20I2P adapter to CN3 on H3ULCB.

Configuring a Console
=====================

Connect a USB cable from your PC to CN12 of your H3ULCB board.

Use the following settings with your serial terminal of choice (minicom, putty,
etc.):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

First of all, open your serial terminal.

Applications for the ``rcar_h3ulcb_cr7`` board configuration can be built in the usual way (see :ref:`build_an_application` for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rcar_h3ulcb_cr7
   :goals: flash

You should see the following message in the terminal:

.. code-block:: console

	*** Booting Zephyr OS build v2.6.0-rc1 ***
	Hello World! rcar_h3ulcb_cr7

Debugging
=========

First of all, open your serial terminal.

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rcar_h3ulcb_cr7
   :goals: debug

You will then get access to a GDB session for debug.

By continuing the app, you should see the following message in the terminal:

.. code-block:: console

	*** Booting Zephyr OS build v2.6.0-rc1 ***
	Hello World! rcar_h3ulcb_cr7

References
**********

- `Renesas R-Car Starter Kit website`_
- `Renesas R-Car H3 chip`_
- `eLinux H3SK page`_

.. _Renesas R-Car Starter Kit website:
   https://www.renesas.com/br/en/products/automotive-products/automotive-system-chips-socs/r-car-h3-m3-starter-kit

.. _Renesas R-Car H3 chip:
	https://www.renesas.com/eu/en/products/automotive-products/automotive-system-chips-socs/r-car-h3-high-end-automotive-system-chip-soc-vehicle-infotainment-and-driving-safety-support

.. _eLinux H3SK page:
	https://elinux.org/R-Car/Boards/H3SK

.. _H3SK top view:
	https://elinux.org/images/1/1f/R-Car-H3-topview.jpg

.. _H3SK bottom view:
	https://elinux.org/images/c/c2/R-Car-H3-bottomview.jpg
