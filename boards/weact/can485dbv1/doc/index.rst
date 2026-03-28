.. zephyr:board:: can485dbv1

Overview
********

The WeAct Studio CAN485 DevBoard V1 `ESP32`_ is a wireless WiFi/Bluetooth board
with CAN 2.0 Bus and RS485/Modbus interfaces implemented through transceivers
with 2.5kV isolation, `CA-IS2062A`_ for CAN and `CA-IS2092A`_ for RS485.

More information can be found on the `CAN485 DevBoard V1 ESP32 website`_.

Hardware
********

.. include:: ../../../espressif/common/soc-esp32-features.rst
   :start-after: espressif-soc-esp32-features

The WeAct Studio CAN485 DevBoard V1 ESP32 is equipped with an ESP32-D0WD-V3
microcontroller, 8MB external QSPI flash memory, and includes the following
features:

- One terminal block for connecting to the CAN bus (TX:GPIO27 and RX:GPIO26)
- One terminal block for connecting to the serial RS485 line (DE: GPIO17,
  DI(TX): GPIO22 and RO(RX): GPIO21)
- USB-C connector with CH343P USB-to-UART adapter for the standard serial
  RS232 console (TX:GPIO1 and RX:GPIO3)
- Onboard SD card slot (CS: GPIO13, SCK: GPIO14, MOSI: GPIO15, MISO: GPIO2)
- Onboard PCB antenna for WiFI/Bluetooth
- Onboard input voltage sensing (ADC0: GPIO36, with 1:12 divider)
- Onboard RGB WS2812 LED (GPIO4)
- BOOT/USER button (GPIO0)

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The ESP32-D0WD-V3 PLL is driven by an external crystal oscillator running at
40 MHz and configured to provide a system clock of 240 MHz and an APB clock
of 80 MHz. This allows generating a TWAI internal core clock of 40 MHz (half
of the APB clock).

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants


Debugging
=========

Due to the inaccessibility of the JTAG signals, there is no debugging option.

References
**********

.. target-notes::

.. _CAN485 DevBoard V1 ESP32 website:
   https://github.com/WeActStudio/WeActStudio.CAN485DevBoardV1_ESP32

.. _CA-IS2062A:
   https://e.chipanalog.com/products/interface/isolated/isolated-can-transceivers-with-iso-power/1314

.. _CA-IS2092A:
   https://e.chipanalog.com/products/interface/isolated/iso4/1129

.. _ESP32: https://www.espressif.com/en/products/socs?id=ESP32
