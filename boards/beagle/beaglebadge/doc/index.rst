.. zephyr:board:: beaglebadge

Overview
********

BeagleBoard.org BeagleBadge brings a new vision to wearables by combining
powerful built-in sensing with unmatched expansion flexibility. Based on a
TI Sitara AM62L dual-core ARM Cortex-A53 SoC

See the `BeagleBoard.org BeagleBadge Product Page`_ for details.

Hardware
********

* Processor

  * TI Sitara AM62L SoC with 2x ARM Cortex-A53

* Memory

  * 256MB LPDDR4
  * 256Mb OSPI flash
  * 32Kb EEPROM

* Wireless connectivity

  * BeagleMod CC3301-1216 Module WiFi6 (2.4GHz)
  * Bluetooth 5.2 with MHF4 Connector
  * LoRaWAN (Wio SX1262 module) with u.FL Connector

* Built-in Sensors

  * 1x IMU Sensor
  * 1x Ambient light sensor
  * 1x Temperature & humidity sensor

* User interface

  * 4.2″ ePaper Display
  * 1x 4-way Joystick button with press
  * 2x Tactile buttons (Back and Select)
  * 1x Passive Buzzer
  * 1x RGB status LED
  * 2x 7-segment LED displays

* Expansion

  * MIPI DSI connector for LCD displays
  * USB Type-C for Power input and debug
  * 1x mikroBus Connector
  * 1x USB2.0 Type-A host
  * 1x Micro-SD socket
  * 2x QWIIC Connector
  * 1x Grove Connector

Supported Features
==================

.. zephyr:board-supported-hw::

Devices
========
Serial Port
-----------

This board configuration uses a single serial communication channel with the
MAIN domain UART (main_uart0).

SD Card
*******

Build the AM62L SDK as described here `Building the SDK with Yocto`_ using
MACHINE=beaglebadge-ti. Flash the resulting WIC file with an etching software
onto an SD card.

Build Zephyr, for example build the :zephyr:code-sample:`hello_world` sample
with the following command:

.. zephyr-app-commands::
   :board: beaglebadge/am62l3/a53
   :zephyr-app: samples/hello_world
   :goals: build

This builds the program and the binary is present in the :file:`build/zephyr`
directory as :file:`zephyr.bin`.

Copy the compiled ``zephyr.bin`` to the first FAT partition of the SD card and
plug the SD card into the board. Power it up and stop the U-Boot execution at
prompt.

Use U-Boot to load and start zephyr.bin:

.. code-block:: console

    fatload mmc 1:1 0x82000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; go 0x82000000

The Zephyr application should start running on the A53 cores.

Debugging
*********

The board is equipped with JTAG exposed over an edge SOICbite connector.

References
**********

* `BeagleBoard.org BeagleBadge Product Page`_

.. _BeagleBoard.org BeagleBadge Product Page:
   https://www.beagleboard.org/boards/beaglebadge

.. _TI AM62L Product Page:
   https://www.ti.com/product/AM62L

.. _Building the SDK with Yocto:
   https://software-dl.ti.com/processor-sdk-linux-rt/esd/AM62LX/11_02_08_02/exports/docs/linux/Overview_Building_the_SDK.html
