.. zephyr:board:: adafruit_feather_rfm95_rp2040

Overview
********

The `Adafruit Feather RP2040 with RFM95 LoRa Radio`_ is a board based on the RP2040
microcontroller from Raspberry Pi Ltd. The board has a RFM95 LoRa radio
operating at 915 MHz, and a Stemma QT connector for easy sensor usage.
It is compatible with the Feather board form factor, and has
a USB type C connector.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 8 Mbyte QSPI flash
- 17 GPIO pins
- 4 ADC pins
- I2C
- SPI
- UART
- USB type C connector
- Reset and boot buttons
- Red LED
- RGB LED (Neopixel)
- Stemma QT I2C connector
- RFM95 LoRa 915 MHz radio with antenna connector
- Built-in lithium ion battery charger


Default Zephyr Peripheral Mapping
=================================

.. rst-class:: rst-columns

    - A0 ADC0 : GPIO26
    - A1 ADC1 : GPIO27
    - A2 ADC2 : GPIO28
    - A3 ADC3 : GPIO29
    - D24 : GPIO24
    - D25 : GPIO25
    - SCK SPI1 SCK : GPIO14
    - MO SPI1 MOSI : GPIO15
    - MI SPI1 MISO : GPIO8
    - RX UART0 : GPIO1
    - TX UART0 : GPIO0
    - GND : -
    - SDA I2C1 : GPIO2
    - SCL I2C1 : GPIO3
    - D5 : GPIO5
    - D6 : GPIO6
    - D9 : GPIO9
    - D10 : GPIO10
    - D11 : GPIO11
    - D12 : GPIO12
    - D13 and red LED : GPIO13
    - Button BOOT : GPIO7
    - RGB LED (Neopixel) signal : GPIO4
    - LORA radio CS : GPIO16
    - LORA radio RESET : GPIO17
    - LORA radio IO0 : GPIO21
    - LORA radio IO1 : GPIO22
    - LORA radio IO2 : GPIO23
    - LORA radio IO3 : GPIO19
    - LORA radio IO4 : GPIO20
    - LORA radio IO5 : GPIO18

The LORA radio is also connected to the SPI1 pins SCK, MOSI and MISO.

See also `pinout`_ and `schematic`_.


Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The Adafruit Feather RP2040 with RFM95 LoRa Radio board does not expose
the SWDIO and SWCLK pins, so programming must be done via the USB
port. Press and hold the BOOT button, and then press the RST button,
and the device will appear as a USB mass storage unit.
Building your application will result in a :file:`build/zephyr/zephyr.uf2` file.
Drag and drop the file to the USB mass storage unit, and the board
will be reprogrammed.

For more details on programming RP2040-based boards, see
:ref:`rpi_pico_programming_and_debugging`.


Flashing
========

To run the :zephyr:code-sample:`blinky` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky/
   :board: adafruit_feather_rfm95_rp2040
   :goals: build flash

Also try the :zephyr:code-sample:`led-strip`, :zephyr:code-sample:`input-dump`,
:zephyr:code-sample:`input-dump`, :zephyr:code-sample:`usb-cdc-acm-console`,
:zephyr:code-sample:`adc_dt`,
:zephyr:code-sample:`lora-send` and :zephyr:code-sample:`lora-receive` samples.

The Stemma QT connector can be used to read sensor data via I2C, for
example the :zephyr:code-sample:`accel_polling` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/accel_polling
   :board: adafruit_feather_rfm95_rp2040
   :shield: adafruit_lis3dh
   :goals: build flash


References
**********

.. target-notes::

.. _Adafruit Feather RP2040 with RFM95 LoRa Radio:
   https://learn.adafruit.com/feather-rp2040-rfm95

.. _pinout:
   https://learn.adafruit.com/feather-rp2040-rfm95/pinouts

.. _schematic:
   https://learn.adafruit.com/feather-rp2040-rfm95/downloads
