.. zephyr:board:: rak11161

Overview
********

RAK11161 Breakout Board provides easy access to the RAK11160 stamp module's
pins via 2.54 mm pitch headers, simplifying development and testing of the
RAK11160.

The RAK11160 module combines an STM32WLE5CC MCU with integrated LoRa
transceiver and an Espressif ESP8684 (ESP32-C2) co-processor that adds
Bluetooth LE and Wi-Fi connectivity. Dedicated debug headers for both
processors are exposed on the breakout board, making it straightforward
to flash and debug either SoC independently.

Hardware
********

- STM32WLE5CC ARM Cortex-M4 at 48 MHz, 256 KB Flash, 64 KB SRAM
- Integrated LoRa transceiver with programmable TX power up to +22 dBm
- LoRaWAN 1.0.4 compliant (Class A/B/C); IN865 / EU868 / AU915 / US915 /
  KR920 / RU864 / AS923-1/2/3/4
- Hardware AES-256 encryption and true random number generator
- ESP8684 (ESP32-C2) RISC-V at up to 120 MHz, 2 MB Flash, 272 KB SRAM
- IEEE 802.11 b/g/n Wi-Fi and Bluetooth LE 5.0 via ESP8684
- SMA connectors for LoRa and BLE/Wi-Fi antennas
- ST_UART2 header: USART2 console and BOOT0 for STM32WLE5
- ST_DEBUG header: SWD interface for STM32WLE5
- ESP_DEBUG header: UART0 console and boot mode for ESP8684
- Supply voltage: 3.0 V ~ 3.6 V; operating temperature: −40 °C ~ 85 °C

For more information about the RAK11161 Breakout Board and RAK11160 stamp module:

- `WisDuo RAK11161 Breakout Board`_
- `WisDuo RAK11160 Module`_
- `STM32WLE5CC on www.st.com`_
- `ESP8684 on www.espressif.com`_

Connections and IOs
===================

The RAK11161 exposes the RAK11160 stamp module pins through six headers.

ST_GPIO 1 — STM32WLE5 SPI / I2C / GPIO
----------------------------------------

+-----+-----------+--------+-------------------+---------+
| Pin | Name      | Type   | Description       | MCU Pin |
+=====+===========+========+===================+=========+
| 1   | NSS       | I/O    | SPI1 chip select  | PA4     |
+-----+-----------+--------+-------------------+---------+
| 2   | SCK       | I/O    | SPI1 clock        | PA5     |
+-----+-----------+--------+-------------------+---------+
| 3   | MISO      | I/O    | SPI1 master-in    | PA6     |
+-----+-----------+--------+-------------------+---------+
| 4   | MOSI      | I/O    | SPI1 master-out   | PA7     |
+-----+-----------+--------+-------------------+---------+
| 5   | PA8       | I/O    | GPIO              | PA8     |
+-----+-----------+--------+-------------------+---------+
| 6   | PA10      | I/O    | GPIO              | PA10    |
+-----+-----------+--------+-------------------+---------+
| 7   | SDA       | I/O    | I2C2 data         | PA11    |
+-----+-----------+--------+-------------------+---------+
| 8   | SCL       | I/O    | I2C2 clock        | PA12    |
+-----+-----------+--------+-------------------+---------+
| 9   | GND       | Power  | Ground            | —       |
+-----+-----------+--------+-------------------+---------+

ST_GPIO 2 — STM32WLE5 GPIO
----------------------------

+-----+-----------+--------+-------------------+---------+
| Pin | Name      | Type   | Description       | MCU Pin |
+=====+===========+========+===================+=========+
| 1   | PB5       | I/O    | GPIO              | PB5     |
+-----+-----------+--------+-------------------+---------+
| 2   | PA15      | I/O    | GPIO              | PA15    |
+-----+-----------+--------+-------------------+---------+
| 3   | PA1       | I/O    | GPIO              | PA1     |
+-----+-----------+--------+-------------------+---------+
| 4   | PB4       | I/O    | GPIO              | PB4     |
+-----+-----------+--------+-------------------+---------+
| 5   | PB3       | I/O    | GPIO              | PB3     |
+-----+-----------+--------+-------------------+---------+
| 6   | PA9       | I/O    | GPIO              | PA9     |
+-----+-----------+--------+-------------------+---------+
| 7   | PB2       | I/O    | GPIO              | PB2     |
+-----+-----------+--------+-------------------+---------+
| 8   | PB12      | I/O    | GPIO              | PB12    |
+-----+-----------+--------+-------------------+---------+
| 9   | VCC       | Power  | 3.3 V supply      | —       |
+-----+-----------+--------+-------------------+---------+

ST_UART2 — STM32WLE5 Console / BOOT
-------------------------------------

+-----+-----------+--------+----------------------------------+---------+
| Pin | Name      | Type   | Description                      | MCU Pin |
+=====+===========+========+==================================+=========+
| 1   | BOOT0     | Input  | Boot mode select (active high)   | BOOT0   |
+-----+-----------+--------+----------------------------------+---------+
| 2   | VCC       | Power  | 3.3 V supply                     | —       |
+-----+-----------+--------+----------------------------------+---------+
| 3   | RX2       | Input  | USART2 receive / console RX      | PA3     |
+-----+-----------+--------+----------------------------------+---------+
| 4   | TX2       | Output | USART2 transmit / console TX     | PA2     |
+-----+-----------+--------+----------------------------------+---------+
| 5   | GND       | Power  | Ground                           | —       |
+-----+-----------+--------+----------------------------------+---------+

ST_DEBUG — STM32WLE5 SWD
--------------------------

+-----+-----------+--------+----------------------------------+---------+
| Pin | Name      | Type   | Description                      | MCU Pin |
+=====+===========+========+==================================+=========+
| 1   | VCC       | Power  | 3.3 V supply                     | —       |
+-----+-----------+--------+----------------------------------+---------+
| 2   | SWDIO     | I/O    | SWD debug data                   | PA13    |
+-----+-----------+--------+----------------------------------+---------+
| 3   | SWDCLK    | Input  | SWD debug clock                  | PA14    |
+-----+-----------+--------+----------------------------------+---------+
| 4   | GND       | Power  | Ground                           | —       |
+-----+-----------+--------+----------------------------------+---------+
| 5   | RESET     | Input  | MCU reset (active low)           | NRST    |
+-----+-----------+--------+----------------------------------+---------+

ESP_GPIO — ESP8684 GPIO
------------------------

+-----+-----------+--------+-------------------+---------+
| Pin | Name      | Type   | Description       | MCU Pin |
+=====+===========+========+===================+=========+
| 1   | IO0       | I/O    | GPIO              | GPIO0   |
+-----+-----------+--------+-------------------+---------+
| 2   | IO1       | I/O    | GPIO              | GPIO1   |
+-----+-----------+--------+-------------------+---------+
| 3   | IO2       | I/O    | GPIO              | GPIO2   |
+-----+-----------+--------+-------------------+---------+
| 4   | IO3       | I/O    | GPIO              | GPIO3   |
+-----+-----------+--------+-------------------+---------+
| 5   | IO4       | I/O    | GPIO              | GPIO4   |
+-----+-----------+--------+-------------------+---------+
| 6   | IO5       | I/O    | GPIO              | GPIO5   |
+-----+-----------+--------+-------------------+---------+
| 7   | IO8       | I/O    | GPIO              | GPIO8   |
+-----+-----------+--------+-------------------+---------+
| 8   | IO10      | I/O    | GPIO              | GPIO10  |
+-----+-----------+--------+-------------------+---------+
| 9   | IO18      | I/O    | GPIO              | GPIO18  |
+-----+-----------+--------+-------------------+---------+

ESP_DEBUG — ESP8684 Console / BOOT
------------------------------------

+-----+-----------+--------+------------------------------------------+---------+
| Pin | Name      | Type   | Description                              | MCU Pin |
+=====+===========+========+==========================================+=========+
| 1   | VCC       | Power  | 3.3 V supply                             | —       |
+-----+-----------+--------+------------------------------------------+---------+
| 2   | TX        | Output | UART0 transmit / console TX              | GPIO20  |
+-----+-----------+--------+------------------------------------------+---------+
| 3   | RX        | Input  | UART0 receive / console RX               | GPIO19  |
+-----+-----------+--------+------------------------------------------+---------+
| 4   | GND       | Power  | Ground                                   | —       |
+-----+-----------+--------+------------------------------------------+---------+
| 5   | ESP_BOOT  | Input  | Boot mode select (pull low during reset) | GPIO9   |
+-----+-----------+--------+------------------------------------------+---------+

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The RAK11161 exposes separate debug interfaces for each processor. Connect a
USB-to-TTL adapter or a debug probe to the appropriate header depending on
which SoC you are targeting.

STM32WLE5 (rak11161/stm32wle5xx)
=================================

The STM32WLE5 can be debugged and flashed with an external debug probe
connected to the ST_DEBUG header (SWD). It can also be flashed via
`pyOCD`_, but an additional pack must be installed first:

.. code-block:: console

   $ pyocd pack --update
   $ pyocd pack --install stm32wl

Flashing an application
-----------------------

Connect a debug probe to the ST_DEBUG header and build and flash an
application. The sample application :zephyr:code-sample:`hello_world` is
used for this example:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak11161/stm32wle5xx
   :goals: build flash

Run a serial terminal connected to the ST_UART2 header (TX2 / RX2, pins 4
and 3):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

.. code-block:: console

   Hello World! rak11161/stm32wle5xx

ESP8684 (rak11161/esp32c2)
==========================

The ESP8684 can be flashed via the ESP_DEBUG header using a USB-to-TTL
adapter. Pull ESP_BOOT (pin 5) low during reset to enter download mode.

Flashing an application
-----------------------

Connect a USB-to-TTL adapter to the ESP_DEBUG header (TX / RX, pins 2 and
3) with ESP_BOOT held low during power-up, then build and flash:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak11161/esp32c2
   :goals: build flash

Open a serial terminal on the same adapter after releasing ESP_BOOT:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

.. code-block:: console

   Hello World! rak11161/esp32c2

References
**********

.. target-notes::

.. _WisDuo RAK11161 Breakout Board:
   https://docs.rakwireless.com/product-categories/wisduo/rak11161-breakout-board/overview/

.. _WisDuo RAK11160 Module:
   https://docs.rakwireless.com/product-categories/wisduo/rak11160-module/overview/

.. _STM32WLE5CC on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32wle5cc.html

.. _ESP8684 on www.espressif.com:
   https://www.espressif.com/sites/default/files/documentation/esp8684_datasheet_en.pdf

.. _pyOCD:
   https://github.com/pyocd/pyOCD
