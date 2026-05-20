.. zephyr:board:: nrf52_sparkfun

Overview
********

The SparkFun nRF52832 Breakout is a compact breakout board based on the Nordic Semiconductor
nRF52832 SoC. The nRF52832 integrates an Arm Cortex-M4F CPU, 512 KiB of flash, 64 KiB of RAM, and
a 2.4 GHz multiprotocol radio supporting Bluetooth Low Energy, ANT, and Nordic proprietary 2.4 GHz
protocols.

Hardware
********

The SparkFun nRF52832 Breakout has the following hardware features:

* Nordic Semiconductor nRF52832 SoC
* Arm Cortex-M4F processor
* 512 KiB flash and 64 KiB RAM
* 2.4 GHz multiprotocol radio
* PCB trace antenna
* 32.768 kHz low-frequency crystal
* User-programmable LED
* User-programmable push button
* UART serial header
* SWD programming/debug test points
* 3.3 V regulator

For more information about the board, refer to these documents:

- `SparkFun nRF52832 Breakout product page`_
- `SparkFun nRF52832 Breakout Hookup Guide`_
- `SparkFun nRF52832 Breakout Schematics`_
- `SparkFun nRF52832 Breakout GitHub Repository`_
- `nRF52832 Product Specification`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The SparkFun nRF52832 Breakout does not include an on-board debug probe. Use an external SWD
debugger, such as a J-Link or an nRF52 DK used as an external debugger.

Flashing
========

For example, to build and flash the :zephyr:code-sample:`blinky` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nrf52_sparkfun/nrf52832
   :goals: build flash

Debugging
=========

To start a debug session:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nrf52_sparkfun/nrf52832
   :goals: debug

References
**********

.. target-notes::

.. _SparkFun nRF52832 Breakout product page:
   https://www.sparkfun.com/sparkfun-nrf52832-breakout.html

.. _SparkFun nRF52832 Breakout Hookup Guide:
   https://learn.sparkfun.com/tutorials/nrf52832-breakout-board-hookup-guide

.. _SparkFun nRF52832 Breakout Schematics:
   https://cdn.sparkfun.com/assets/learn_tutorials/5/4/9/sparkfun-nrf52832-breakout-schematic-v10.pdf

.. _SparkFun nRF52832 Breakout GitHub Repository:
   https://github.com/sparkfun/nRF52832_Breakout

.. _nRF52832 Product Specification:
   https://docs.nordicsemi.com/bundle/ps_nrf52832/page/nrf52832_ps.html
