.. zephyr:board:: rcar_spider_s4

Overview
********

R-Car S4 Spider board is based on the R-Car S4 SoC made for Car
Server/Communication Gateway and that is composed of a octo Cortex |reg|-A55, a
dual lockstep Cortex |reg|-R52 and a double dual lockstep G4MH.

The R-Car S4 SoC enables the launch of Car Server/CoGW with high performance,
high-speed networking, high security and high functional safety levels that are
required as E/E architectures evolve into domains and zones.

The R-Car S4 solution allows designers to re-use up to 88 percent of software
code developed for 3rd generation R-Car SoCs and RH850 MCU applications.
The software package supports the real-time cores with various drivers and
basic software such as Linux BSP and hypervisors.

The Renesas R-Car Spider board is the Renesas R-Car S4 reference board and is designed for
evaluating features and performance of this SoC.

Zephyr OS support is available for both Cortex |reg|-A cores & Cortex |reg|-R52 core.

More information about the S4 SoC can be found at `Renesas R-Car S4 chip`_.

Hardware
********

- Spider features:

  - Connectors

    - CPU Board:

      - CN1 JTAG1
      - CN2 JTAG2
      - CN3 EX-SPI (QSPI0)
      - CN4 MicroSD Slot (back side)
      - CN11 EXIO Connector A (back side)
      - CN12 EXIO Connector B (back side)
      - CN14 EVT
      - CN16 OcuLink (PCIe0,PCIe1)
      - CN24 CAN 4pin
      - CN20 USB microAB (SCIF0)
      - CN21 USB microAB (HSCIF0)
      - CN22 SW Board
      - CN23 CPLD JTAG
      - CN27 FAN
      - CN30 Buck3
      - CN31 Buck1
      - CN32 CAN 8pin (back side)
    - Breakout Board:

      - CN11 EXIO Connector A
      - CN12 EXIO Connector B
      - CN13 CAN 0/1
      - CN15 CAN 3/4/5
      - CN18 CAN 6/7/8
      - CN21 CAN 2/9/10/11
      - CN24 CAN 12/13/14/15
      - CN28 LIN0
      - CN29 LIN1
      - CN30 LIN2
      - CN31 LIN3
      - CN32 LIN4
      - CN33 LIN5
      - CN34 LIN6
      - CN35 LIN7
      - CN36 EtherTS
      - CN37 MSIOF0
      - CN38 CAN/LIN BOARD
      - CN39 GPIO CN_A
      - CN40 GPIO
      - CN41 I2C
      - CN42 HSCIF0
      - CN43 SCIF0
      - CN44 TSN_CN
      - CN45 Legacy 12V-in
      - CN46 AC Adapter
      - CN48 POWER CONTROL
      - CN50 Debug Serial
      - CN51 FAN
  - Input

    - SW1 (SPI Flash Memory / EX-SPI connector)
    - SW2 (Hyper Flash Memory / SPI Flash Memory)
    - SW3 (MicroSD Card Slot / eMMC Memory)
    - SW4 (PRESETn)
    - SW6 (Interface Voltage Setting for MMC/JTAG2)
    - SW8 Mode Setting
    - SW10 (Software Switch)
    - SW11 (Board Power-Supply Circuit Control)
    - SW12 (AURORES#)
    - SW13 (CANFD0 RX)
    - SW14 (CANFD0 TX)
    - SW15 (System Reset Switch)
  - Output

    - LED7 Software Controllable LED
    - LED8 Software Controllable LED


Supported Features
==================

.. zephyr:board-supported-hw::

.. note::

   It is recommended to disable peripherals used by the R52 core on the Linux host.

Connections and IOs
===================

The Spider board consists of a CPU board plugged on top of a Breakout board.

Here are the official IOs figures from eLinux for S4 board:

`S4 Spider CPU board IOs`_

`S4 Spider breakout board IOs`_

GPIO
----

By running Zephyr on S4 Spider, the software controllable LED 'LED8' can be used as output.

UART
----

Here is information about both serial ports provided on the S4 Spider board :

+--------------------+----------+--------------------+-------------+------------------------+
| Physical Interface | Location | Software Interface | Converter   | Further Information    |
+====================+==========+====================+=============+========================+
| CN20 USB Port      | CPU Board| SCIF0/HSCIF1       | FT232HQ     | Default Zephyr serial  |
+--------------------+----------+--------------------+-------------+------------------------+
| CN21 USB Port      | CPU Board| SCIF3/HSCIF0       | FT2232H-56Q | Used by U-BOOT & Linux |
+--------------------+----------+--------------------+-------------+------------------------+

.. note::
   The Zephyr console output is assigned to SCIF0 (CN20 USB Port) with settings:
   115200 8N1 without hardware flow control by default.

I2C
---

I2C is mainly used to manage and power-on some onboard chips on the S4 Spider board.

Embedded I2C devices and I/O expanders are not yet supported.
The current I2C support therefore does not make any devices available to the user at this time.

Programming and Debugging (A55)
*******************************

At that time, no direct flashing method is officially supported by this Zephyr port.
However, it is possible to load the Zephyr binary using U-Boot commands.

One of the ways to load Zephyr is shown below.

.. code-block:: console

   tftp 0x48000000 <tftp_server_path/zephyr.bin>
   booti 0x48000000

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rcar_spider_s4/r8a779f0/a55
   :goals: build

Programming and Debugging (R52)
*******************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Supported Debug Probe
=====================

| The "Olimex ARM-USB-OCD-H" probe is the only officially supported probe.
| This probe is supported by OpenOCD that is shipped with the Zephyr SDK.

The "Olimex ARM-USB-OCD-H" probe needs to be connected with a "Coresight 20 pins"
adapter to CN1 connector on Spider board.

Configuring a Console
=====================

Connect a USB cable from your PC to CN20 USB port of your Spider board.

Use the following settings with your serial terminal of choice (minicom, putty,
etc.):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

First of all, open your serial terminal.

Applications for the ``rcar_spider_s4/r8a779f0/r52`` board configuration can be built in the
usual way (see :ref:`build_an_application` for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rcar_spider_s4/r8a779f0/r52
   :goals: flash

You should see the following message in the terminal:

.. code-block:: console

	*** Booting Zephyr OS build v3.3.0-rc2 ***
	Hello World! rcar_spider_s4

Debugging
=========

First of all, open your serial terminal.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rcar_spider_s4/r8a779f0/r52
   :goals: debug

You will then get access to a GDB session for debugging.

By continuing the app, you should see the following message in the terminal:

.. code-block:: console

	*** Booting Zephyr OS build v3.3.0-rc2 ***
	Hello World! rcar_spider_s4


References
**********

- `Renesas R-Car S4 Spider`_
- `Renesas R-Car S4 chip`_
- `eLinux S4 Spider`_

.. _Renesas R-Car S4 Spider:
   https://www.renesas.com/us/en/products/automotive-products/automotive-system-chips-socs/rtp8a779f0askb0sp2s-r-car-s4-reference-boardspider

.. _Renesas R-Car S4 chip:
	https://www.renesas.com/us/en/products/automotive-products/automotive-system-chips-socs/r-car-s4-automotive-system-chip-soc-car-servercommunication-gateway

.. _eLinux S4 Spider:
	https://elinux.org/R-Car/Boards/Spider

.. _S4 Spider CPU board IOs:
	https://elinux.org/images/6/6d/Rcar_s4_spider_cpu_board.jpg

.. _S4 Spider breakout board IOs:
	https://elinux.org/images/2/29/Rcar_s4_spider_breakout_board.jpg
