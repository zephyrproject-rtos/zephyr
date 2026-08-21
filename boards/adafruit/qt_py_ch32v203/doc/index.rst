.. zephyr:board:: adafruit_qt_py_ch32v203

Overview
********

The `Adafruit QT Py CH32V203`_ is a small, low-cost board from Adafruit
built around the WCH CH32V203G6U6 QFN-28 RISC-V microcontroller. It features
a USB-C connector, an on-board NeoPixel RGB LED, and a STEMMA QT connector
for I2C peripherals.

Hardware
********
- QingKe V4B 32-bit RISC-V processor running up to 144 MHz
- 10KB on-chip SRAM
- 32KB on-chip flash (224KB total flash address space)
- USB-C connector (USB 2.0 Full Speed)
- STEMMA QT / Qwiic I2C connector
- On-board NeoPixel (WS2812B) RGB LED
- SPI, I2C, and UART peripherals exposed on headers
- Boot button (cannot be used as input)

The `WCH webpage on CH32V203`_ contains the processor's information and
manuals.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

RGB NeoPixel LED
----------------

The board has one NeoPixel WS2812B RGB LED connected to pin ``PA04``. Unfortunately no Zephyr
driver currently exists for this configuration of MCU and pin.

Default Zephyr Peripheral Mapping
----------------------------------

.. rst-class:: rst-columns

- USART2_TX : PA2
- USART2_RX : PA3
- I2C1_SCL : PB6
- I2C1_SDA : PB7
- SPI1_SCK : PA5
- SPI1_MISO : PA6
- SPI1_MOSI : PA7

Programming and Debugging
*************************

Applications for the ``adafruit_qt_py_ch32v203`` board can be built and flashed
in the usual way (see :ref:`build_an_application` and :ref:`application_run`
for more details); however, an external programmer is required for debugging since
the board does not have a built-in debug probe.

When using a debugger, the following pins of the external programmer must be connected
to the following pins on the PCB:

* VCC = 3.3V
* GND = GND
* SWIO = PA13
* SWCLK = PA14

Flashing
========

.. zephyr:board-supported-runners::

You can use ``minichlink``, ``wlink``, or ``wchisp`` to flash the board. Once
the tool has been set up, build and flash applications as usual (see
:ref:`build_an_application` and :ref:`application_run` for more details).

Applications can be flashed over USB by holding down the boot button at
power on and using the ``wchisp`` runner.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: adafruit_qt_py_ch32v203
   :goals: build flash

References
**********

.. target-notes::

.. _Adafruit QT Py CH32V203: https://www.adafruit.com/product/5996
.. _WCH webpage on CH32V203: https://www.wch-ic.com/products/CH32V203.html
