.. zephyr:board:: rak11162

Overview
********

The RAK11162 WisBlock Core Module is a WisBlock Core module for RAK WisBlock. It
combines an STM32WLE5CC MCU with integrated LoRa transceiver and an Espressif
ESP8684 (ESP32-C2) co-processor for Bluetooth LE and Wi-Fi connectivity, packaged
with a high-quality connector compatible with WisBlock Base boards.

- `WisBlock overview`_
- `RAK11162 datasheet`_

Hardware
********

Supported Features
==================

- STM32WLE5CC ARM Cortex-M4 at 48 MHz, 256 KB Flash, 64 KB SRAM
- Integrated LoRa transceiver with programmable TX power up to +22 dBm
- LoRaWAN 1.0.4 compliant (Class A/B/C); IN865 / EU868 / AU915 / US915 /
  KR920 / RU864 / AS923-1/2/3/4
- Hardware AES-256 encryption and true random number generator
- ESP8684 (ESP32-C2) RISC-V at up to 120 MHz, 2 MB Flash, 272 KB SRAM
- IEEE 802.11 b/g/n Wi-Fi and Bluetooth LE 5.0 via ESP8684
- I/O ports: UART/I2C/SPI/ADC/GPIO
- Supply voltage: 3.0 V ~ 3.6 V; operating temperature: −40 °C ~ 85 °C

.. zephyr:board-supported-hw::

Connections and IOs
===================

The RAK11162 features a 40-pin header with various I/O interfaces for the WisBlock
ecosystem. Pins prefixed ``PA``/``PB`` belong to the STM32WLE5, and pins prefixed
``GPIO`` belong to the ESP8684. The pinout is as follows:

+-----------------------------+----------+-----+-----+----------+-----------------------------+
| Used                        | Name     | Pin | Pin | Name     | Used                        |
+=============================+==========+=====+=====+==========+=============================+
| NC                          | VBAT     | 1   | 2   | VBAT     | NC                          |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GND                         | GND      | 3   | 4   | GND      | GND                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| 3V3                         | 3V3      | 5   | 6   | 3V3      | 3V3                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| CH340_P / UART2             | USB_P    | 7   | 8   | USB_N    | CH340_N / UART2             |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| NC                          | VBUS     | 9   | 10  | SW1      | GPIO3                       |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO5 / soft_uart0_TX       | TXD0     | 11  | 12  | RXD0     | GPIO10 / soft_uart0_RX      |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| RESET                       | RESET    | 13  | 14  | LED1     | PA10                        |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PA1                         | LED2     | 15  | 16  | LED3     | GPIO4                       |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| 3V3                         | VDD      | 17  | 18  | VDD      | 3V3                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PA11 / I2C2_SDA             | I2C1_SDA | 19  | 20  | I2C1_SCL | PA12 / I2C2_SCL             |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PB3 / ADC_VBAT              | AIN0     | 21  | 22  | AIN1     | PB4 / ADC                   |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| BOOT                        | BOOT0    | 23  | 24  | IO7      | GPIO1                       |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PA4 / SPI1_CS               | SPI_CS   | 25  | 26  | SPI_CLK  | PA5 / SPI1_SCK              |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PA6 / SPI1_MISO             | SPI_MISO | 27  | 28  | SPI_MOSI | PA7 / SPI1_MOSI             |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PB5                         | IO1      | 29  | 30  | IO2      | PA8                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PB12                        | IO3      | 31  | 32  | IO4      | PB2                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO20 / UART0_TX           | TXD1     | 33  | 34  | RXD1     | GPIO19 / UART0_RX           |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GPIO0 / I2C0_SDA            | I2C2_SDA | 35  | 36  | I2C2_SCL | GPIO18 / I2C0_SCL           |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| PA15                        | IO5      | 37  | 38  | IO6      | PA9                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+
| GND                         | GND      | 39  | 40  | GND      | GND                         |
+-----------------------------+----------+-----+-----+----------+-----------------------------+

.. note::

   The STM32WLE5 console (USART2) is accessible via USB_P/USB_N (pins 7–8) through
   an on-module CH340 USB-to-serial converter. The ESP8684 console (UART0) is
   accessible via TXD1/RXD1 (pins 33–34) using a USB-to-TTL adapter.

Connecting to a Baseboard
=========================

The RAK11162 can be mounted on a baseboard using the 40-pin header, called WisBlock
I/O connector. It is compatible with the WisBlock ecosystem, allowing for easy
integration with various WisBlock modules and sensors.

.. figure:: img/mounting.webp
   :align: center
   :alt: RAK11162 mounting

Programming and debugging
*************************

.. zephyr:board-supported-runners::

The RAK11162 exposes separate debug interfaces for each processor.

STM32WLE5 (rak11162/stm32wle5xx)
=================================

Building & Flashing
-------------------

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: rak11162/stm32wle5xx
   :shield: rakwireless_rak19010,rakwireless_rak19012
   :goals: build flash

Debugging
---------

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak11162/stm32wle5xx
   :shield: rakwireless_rak19007
   :maybe-skip-config:
   :goals: debug

ESP8684 (rak11162/esp32c2)
==========================

Building & Flashing
-------------------

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak11162/esp32c2
   :shield: rakwireless_rak19007
   :goals: build flash

Debugging
---------

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak11162/esp32c2
   :shield: rakwireless_rak19007
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _WisBlock overview:
   https://www.rakwireless.com/en-us/products/wisblock

.. _RAK11162 datasheet:
   https://docs.rakwireless.com/product-categories/wisblock/rak11162/datasheet
