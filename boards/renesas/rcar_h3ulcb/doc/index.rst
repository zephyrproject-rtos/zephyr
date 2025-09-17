.. zephyr:board:: rcar_h3ulcb

Overview
********
R-Car H3ULCB starter kit board is based on the R-Car H3 SoC that features basic
functions for next-generation car navigation systems.
It is composed of a quad Cortex |reg|-A57, a quad Cortex |reg|-A53 cluster and a
dual lockstep Cortex |reg|-R7.

Zephyr OS support is available for both Cortex |reg|-A cores & Cortex |reg|-R7 core.

More information about the H3 SoC can be found at `Renesas R-Car H3 chip`_.

Hardware
********

- H3ULCB features:

  - Storage:

    - 384KB System RAM
    - 4/8 GB LPDDR4
    - 64 MB HYPER FLASH (512 MBITS, 160 MHZ, 320 MBYTES/S)
    - 16MB QSPI FLASH (128 MBITS,80 MHZ,80 MBYTES/S)1 HEADER QSPI MODULE
    - 8/32/64/128 GB EMMC (HS400 240 MBYTES/S)
    - MICROSD-CARD SLOT (SDR104 100 MBYTES/S)
  - Connectors

    - CN1 COM Express type connector 440pin
    - CN2 QSPI Flash module
    - CN3 DEBUG JTAG
    - CN4 HDMI (HDMI-0)
    - CN5 USB 2.0 (USB2.0-1)
    - CN6 Push-Pull microSD Card Socket (SDHI-0)
    - CN7 Ethernet, Connector, RJ45
    - CN8 LINE Out
    - CN9 MIC Input
    - CN10 DEBUG SERIAL (not populated)
    - CN11 CPLD Programming JTAG
    - CN12 DEBUG SERIAL (serial)
    - CN13 Main Power Supply input (5VDC)
    - CN14 CPU Fan
  - Input

    - SW1 Hyper Flash
    - SW2 Software Readable DIPSWITCHES (4x)
    - SW3 Software Readable Push button
    - SW4 Software Readable Push button
    - SW5 Software Readable Push button
    - SW6 Mode Settings
    - SW7 CPLD Reset
    - SW8 Power
    - SW9 Reset
  - Output

    - LED1 HDMI / Hot Plug Sync Detect
    - LED4 Software Controllable LED
    - LED5 Software Controllable LED
    - LED6 Software Controllable LED
    - LED9 5V Main Supply
    - LED14 Backup LED
    - LED15 System Reset


Complete list of the H3ULCB board capabilities can be found on the `eLinux H3SK page`_ of the board.

More information about the board can be found at `Renesas R-Car Starter Kit website`_.

Supported Features
==================

.. zephyr:board-supported-hw::

.. note::

   It is recommended to disable peripherals used by the R7 core on the Linux host.

Connections and IOs
===================

The H3ULCB Starter Kit can be plugged on a Kingfisher daughter board.

H3ULCB Board
------------

Here are official IOs figures from eLinux for H3ULCB board:

`H3SK top view`_

`H3SK bottom view`_

Kingfisher Infotainment daughter board
--------------------------------------

When connected to Kingfisher Infotainment board through COMExpress connector, the board is exposing much more IOs.

Here are official IOs figures from eLinux for Kingfisher Infotainment board:

`Kingfisher top view`_

`Kingfisher bottom view`_

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
| CN04 DEBUG SERIAL  | Kingfisher        | SCIF1              |           | Secondary UART // Through ComExpress |
+--------------------+-------------------+--------------------+-----------+--------------------------------------+

H3ULCB A53 support is assigning SCIF2 as UART while R7 supports is using SCIF1. In both cases, console are set to 115200 8N1 without hardware flow control by default.

To access SCIF1 using CN04 UART interface, please follow the following pinout (depending on your Kingfisher board version):

+--------+----------+----------+
| Signal | Pin KF03 | Pin KF04 |
+========+==========+==========+
| RXD    | 3        | 4        |
+--------+----------+----------+
| TXD    | 5        | 2        |
+--------+----------+----------+
| RTS    | 4        | 1        |
+--------+----------+----------+
| CTS    | 6        | 3        |
+--------+----------+----------+
| GND    | 9        | 6        |
+--------+----------+----------+

CAN
---

H3ULCB board provides two CAN interfaces. Both interfaces are available on the Kingfisher daughter board.

+--------------------+--------------------+--------------+
| Physical Interface | Software Interface | Transceiver  |
+====================+====================+==============+
| CN17               | CAN0               | TCAN332GDCNT |
+--------------------+--------------------+--------------+
| CN18               | CAN1               | TCAN332GDCNT |
+--------------------+--------------------+--------------+

.. note:: Interfaces are set to 125 kbit/s by default.

The following table lists CAN physical interfaces pinout:

+-----+--------+
| Pin | Signal |
+=====+========+
| 1   | CANH   |
+-----+--------+
| 2   | CANL   |
+-----+--------+
| 3   | GND    |
+-----+--------+

I2C
---

H3ULCB board provides two I2C buses. Unfortunately direct access to these buses is not available through connectors.

I2C is mainly used to manage and power on multiple of onboard chips on the H3ULCB and Kingfisher daughter board.

Embedded I2C devices and I/O expanders are not yet supported. The current I2C support therefore does not make any devices available to the user at this time.

PWM
---

ULCB boards provide one PWM controller with a maximum of 7 channels [0..6]. H3ULCB does provide the pwm0 from test pin CP8 only.

When plugged on a Kingfisher daughter board, pwm4 channel is available on CN7 LVDS connector.

Programming and Debugging (A53)
*******************************

Flashing
========

At that time, no flashing method is officially supported by this Zephyr port.

Programming and Debugging (R7)
******************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Supported Debug Probe
=====================

The "Olimex ARM-USB-OCD-H" probe is the only officially supported probe. This probe is supported by OpenOCD that is shipped with the Zephyr SDK.

The "Olimex ARM-USB-OCD-H" probe needs to be connected with a SICA20I2P adapter to CN3 on H3ULCB.

.. note::
    See `eLinux Kingfisher page`_ "Known issues" section if you encounter problem with JTAG.

Configuring a Console
=====================

Connect a USB cable from your PC to CN04 of your Kingfisher daughter board.

Use the following settings with your serial terminal of choice (minicom, putty,
etc.):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

First of all, open your serial terminal.

Applications for the ``rcar_h3ulcb/r8a77951/r7`` board configuration can be built in the usual way (see :ref:`build_an_application` for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rcar_h3ulcb/r8a77951/r7
   :goals: flash

You should see the following message in the terminal:

.. code-block:: console

	*** Booting Zephyr OS build v2.6.0-rc1 ***
	Hello World! rcar_h3ulcb

Debugging
=========

First of all, open your serial terminal.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rcar_h3ulcb/r8a77951/r7
   :goals: debug

You will then get access to a GDB session for debug.

By continuing the app, you should see the following message in the terminal:

.. code-block:: console

	*** Booting Zephyr OS build v2.6.0-rc1 ***
	Hello World! rcar_h3ulcb

References
**********

- `Renesas R-Car Starter Kit website`_
- `Renesas R-Car H3 chip`_
- `eLinux H3SK page`_
- `eLinux Kingfisher page`_

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

.. _eLinux Kingfisher page:
	https://elinux.org/R-Car/Boards/Kingfisher

.. _Kingfisher top view:
	https://elinux.org/images/0/08/Kfisher_top_specs.png

.. _Kingfisher bottom view:
	https://elinux.org/images/0/06/Kfisher_bot_specs.png
