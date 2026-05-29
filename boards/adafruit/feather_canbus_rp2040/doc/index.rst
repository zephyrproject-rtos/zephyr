.. zephyr:board:: adafruit_feather_canbus_rp2040

Overview
********

The `Adafruit RP2040 CANBUS Feather`_ board is based on the RP2040
microcontroller from Raspberry Pi Ltd. The board has a CAN bus
controller, and a Stemma QT connector for easy sensor usage.
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
- CAN bus controller
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
    - D4 : GPIO4
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
    - RGB LED (Neopixel) signal : GPIO21
    - RGB LED (Neopixel) power : GPIO20
    - CAN controller CS : GPIO19
    - CAN controller INTERRUPT : GPIO22
    - CAN controller RESET (not used) : GPIO18
    - CAN controller STANDBY (not used) : GPIO16
    - CAN controller TX0RTS (not used) : GPIO17
    - CAN controller RX0BF (not used) : GPIO23

The CAN controller is also connected to the SPI1 pins SCK, MOSI and MISO.

See also `pinout`_ and `schematic`_.


Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The Adafruit RP2040 CANBUS Feather board does not expose
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
   :board: adafruit_feather_canbus_rp2040
   :goals: build flash

Try also the :zephyr:code-sample:`led-strip`, :zephyr:code-sample:`input-dump`,
:zephyr:code-sample:`usb-cdc-acm-console` and :zephyr:code-sample:`adc_dt` samples.

To run the :zephyr:code-sample:`can-counter` CAN sample:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/can/counter
   :board: adafruit_feather_canbus_rp2040
   :goals: build flash
   :build-args: -DCONFIG_LOOPBACK_MODE=n

Connect the other end of the CAN cable to a CAN interface connected to your laptop.
If you are using a Linux laptop, receive the CAN frames by::

   sudo ip link set can0 up type can bitrate 125000
   candump can0

Send CAN frames to control the LED on the board::

   cansend can0 010#00
   cansend can0 010#01


References
**********

.. target-notes::

.. _Adafruit RP2040 CANBUS Feather:
   https://learn.adafruit.com/adafruit-rp2040-can-bus-feather

.. _pinout:
    https://learn.adafruit.com/adafruit-rp2040-can-bus-feather/pinouts

.. _schematic:
    https://learn.adafruit.com/adafruit-rp2040-can-bus-feather/downloads
