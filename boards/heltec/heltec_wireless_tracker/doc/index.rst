.. zephyr:board:: heltec_wireless_tracker

Overview
********

The Heltec Wireless Tracker V1.1 is a development kit based on ESP32-S3FN8. It integrates both SX1262 LoRa transceiver and UC6580 GNSS chip to provide fast GNSS solution for IoT. [1]_

.. note::

   This board support package targets the Heltec Wireless Tracker V1.1. In this revision, the GNSS power-control pin has changed: GPIO3 must be driven HIGH to enable the GNSS module. For additional hardware differences between versions, refer to the official hardware revision logs.

Hardware
********

The main hardware features are:

- ESP32-S3FN8 low-power MCU-based SoC (dual-core Xtensa® 32-bit LX7 microprocessor, five stage pipeline rack Structure, main frequency up to 240 MHz).
- Semtech SX1262 LoRa node chip
- UC6580 GNSS chip with L1 + L5/L2 dual-frequency multi-system support (GPS, GLONASS, BDS, Galileo, NAVIC, QZSS)
- 0.96-inch 160x80 dot matrix LCD display (ST7735) for debugging information, battery power, and other information
- Type-C USB interface with a complete voltage regulator, ESD protection, short circuit protection, RF shielding, and other protection measures
- Onboard SH1.25-2 battery interface, integrated lithium battery management system (charge and discharge management, overcharge protection, battery power detection, USB / battery power automatic switching)
- Integrated WiFi and Bluetooth interfaces with 2.4GHz metal spring antenna and reserved IPEX (U.FL) interface for LoRa and GNSS use
- Good RF circuit design and low-power design

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

.. table:: HelTec Wireless Tracker Pinout
   :widths: auto

   +--------+---------+-----------------------------+
   | Header | Function| Description                 |
   +========+=========+=============================+
   | J2.1   | 5V      |                             |
   +--------+---------+-----------------------------+
   | J2.2   | GND     |                             |
   +--------+---------+-----------------------------+
   | J2.3   | 3V3     |                             |
   +--------+---------+-----------------------------+
   | J2.4   | GND     |                             |
   +--------+---------+-----------------------------+
   | J2.5   | 3V3     |                             |
   +--------+---------+-----------------------------+
   | J2.6   | GND     |                             |
   +--------+---------+-----------------------------+
   | J2.7   | Vext    |                             |
   +--------+---------+-----------------------------+
   | J2.8   | GND     |                             |
   +--------+---------+-----------------------------+
   | J2.1   | RST     | Reset Switch                |
   +--------+---------+-----------------------------+
   | J2.2   | GPIO0   | User Switch                 |
   +--------+---------+-----------------------------+
   | J2.3   | ADC1_CH0| Battery Voltage Measurement |
   +--------+---------+-----------------------------+
   | J2.4   | GPIO2   | ADC Control                 |
   +--------+---------+-----------------------------+
   | J2.5   | GPIO3   | Vext Control                |
   +--------+---------+-----------------------------+
   | J2.6   | GPIO19  |                             |
   +--------+---------+-----------------------------+
   | J2.7   | GPIO20  |                             |
   +--------+---------+-----------------------------+
   | J2.8   | GPIO21  | TFT Backlight Control       |
   +--------+---------+-----------------------------+
   | J2.9   | GPIO26  |                             |
   +--------+---------+-----------------------------+
   | J2.10  | GPIO48  |                             |
   +--------+---------+-----------------------------+
   | J2.11  | GPIO47  |                             |
   +--------+---------+-----------------------------+
   | J2.12  | GPIO33  | UART2 GNSS RX               |
   +--------+---------+-----------------------------+
   | J2.13  | GPIO34  | UART2 GNSS TX               |
   +--------+---------+-----------------------------+
   | J2.14  | GPIO35  |                             |
   +--------+---------+-----------------------------+
   | J2.15  | GPIO36  |                             |
   +--------+---------+-----------------------------+
   | J2.16  | GPIO37  |                             |
   +--------+---------+-----------------------------+
   | J3.1   | GPIO18  | PWM LED Control             |
   +--------+---------+-----------------------------+
   | J3.2   | GPIO17  |                             |
   +--------+---------+-----------------------------+
   | J3.3   | GPIO16  | XTAL_32K N                  |
   +--------+---------+-----------------------------+
   | J3.4   | GPIO15  | XTAL_32K P                  |
   +--------+---------+-----------------------------+
   | J3.5   | GPIO7   |                             |
   +--------+---------+-----------------------------+
   | J3.6   | GPIO6   |                             |
   +--------+---------+-----------------------------+
   | J3.7   | GPIO5   |                             |
   +--------+---------+-----------------------------+
   | J3.8   | GPIO4   |                             |
   +--------+---------+-----------------------------+
   | J3.1   | GPIO46  |                             |
   +--------+---------+-----------------------------+
   | J3.2   | GPIO45  |                             |
   +--------+---------+-----------------------------+
   | J3.3   | GPIO44  | UART0 RX                    |
   +--------+---------+-----------------------------+
   | J3.4   | GPIO43  | UART0 TX                    |
   +--------+---------+-----------------------------+
   | J3.5   | GPIO14  | LoRa DIO1                   |
   +--------+---------+-----------------------------+
   | J3.6   | GPIO13  | LoRa Busy                   |
   +--------+---------+-----------------------------+
   | J3.7   | GPIO12  | LoRa RST                    |
   +--------+---------+-----------------------------+
   | J3.8   | GPIO11  | LoRa MISO                   |
   +--------+---------+-----------------------------+
   | J3.9   | GPIO10  | LoRa MOSI                   |
   +--------+---------+-----------------------------+
   | J3.10  | GPIO9   | LoRa SCK                    |
   +--------+---------+-----------------------------+
   | J3.11  | GPIO8   | LoRa NSS                    |
   +--------+---------+-----------------------------+
   | J3.12  | GPIO42  | TFT_SDIN (MOSI)             |
   +--------+---------+-----------------------------+
   | J3.13  | GPIO41  | TFT_SCLK                    |
   +--------+---------+-----------------------------+
   | J3.14  | GPIO40  | TFT_RS (DC)                 |
   +--------+---------+-----------------------------+
   | J3.15  | GPIO39  | TFT_RES (RST)               |
   +--------+---------+-----------------------------+
   | J3.16  | GPIO38  | TFT_CS                      |
   +--------+---------+-----------------------------+


Key Pin Assignments
===================

**GNSS (GPS) Module:**
- GNSS_TX (MCU UART2 TX): GPIO 34 (connects to GNSS RX)
- GNSS_RX (MCU UART2 RX): GPIO 33 (connects to GNSS TX)
- GNSS_Power_Control (VEXT): GPIO 3 (V1.1 specific - set to HIGH to enable GNSS)

.. note::

   VEXT (GPIO 3) is automatically configured in board initialization code to be active at boot, ensuring the GNSS module is powered on startup.

**LoRa Module (SX1262):**
- LoRa_DIO1: GPIO 14
- LoRa_Busy: GPIO 13
- LoRa_RST: GPIO 12
- LoRa_MISO: GPIO 11
- LoRa_MOSI: GPIO 10
- LoRa_SCK: GPIO 9
- LoRa_NSS: GPIO 8

**TFT Display (ST7735) - Optional:**
- TFT_SDIN (MOSI): GPIO 42
- TFT_SCLK: GPIO 41
- TFT_RS (DC): GPIO 40
- TFT_RES (RST): GPIO 39
- TFT_CS: GPIO 38
- TFT_LED_K (Backlight): GPIO 21
- TFT_VTFT_CTRL (Power): GPIO 3 (shared with GNSS power control)

**UART Interfaces:**
- UART0: U0RXD (GPIO 44), U0TXD (GPIO 43) - Console
- UART1: U1RXD (GPIO 18), U1TXD (GPIO 17) - General purpose
- UART2: U2RXD (GPIO 33), U2TXD (GPIO 34) - GNSS module

**Power and Control:**
- VEXT Control: GPIO 3 (GNSS and TFT power, active HIGH)
- ADC Control: GPIO 37
- User Button: GPIO 0
- Reset Button: RST

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

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

Applications
************

This board is suitable for:

* GPS tracking applications
* IoT sensor monitoring with location data
* LoRaWAN networks with positioning
* LoRa mesh networks with positioning (e.g., Meshtastic)
* Display-based user interfaces (optional)
* Battery-powered tracking devices
* Asset tracking and monitoring
* Environmental monitoring with GPS coordinates

Specifications
**************

.. table:: Heltec Wireless Tracker V1.1 Specifications
   :widths: auto

   +----------------------------+---------------------------------------------------------------------------------------+
   | Parameters                 | Description                                                                           |
   +============================+=======================================================================================+
   | Master Chip                | ESP32-S3FN8 (Xtensa(R) 32-bit lx7 dual core processor)                                |
   +----------------------------+---------------------------------------------------------------------------------------+
   | LoRa Chipset               | SX1262                                                                                |
   +----------------------------+---------------------------------------------------------------------------------------+
   | GNSS Chipset               | UC6580                                                                                |
   +----------------------------+---------------------------------------------------------------------------------------+
   | LoRa Frequency             | 470~510MHz, 863~928MHz                                                                |
   +----------------------------+---------------------------------------------------------------------------------------+
   | Max. TX Power              | 21+/-1dBm                                                                             |
   +----------------------------+---------------------------------------------------------------------------------------+
   | Max. Receiving Sensitivity | -134dBm                                                                               |
   +----------------------------+---------------------------------------------------------------------------------------+
   | Wi-Fi                      | 802.11 b/g/n, up to 150Mbps                                                           |
   +----------------------------+---------------------------------------------------------------------------------------+
   | Bluetooth LE               | Bluetooth 5, Bluetooth mesh                                                           |
   +----------------------------+---------------------------------------------------------------------------------------+
   | Hardware Resource          | 10xADC1 + 10xADC2; 12xTouch; 3xUART; 2xI2C; 2xSPI; etc.                               |
   +----------------------------+---------------------------------------------------------------------------------------+
   | Interface                  | Type-C USB; 2x1.25 lithium battery interface; LoRa ANT(IPEX1.0); 2x18x2.54 Header Pin |
   +----------------------------+---------------------------------------------------------------------------------------+
   | Battery                    | 3.7V lithium battery power supply and charging                                        |
   +----------------------------+---------------------------------------------------------------------------------------+
   | Operating Temperature      | -20 ~ 70 degrees C                                                                    |
   +----------------------------+---------------------------------------------------------------------------------------+
   | Dimensions                 | 65.48 x 28.06 x 13.52mm                                                               |
   +----------------------------+---------------------------------------------------------------------------------------+

References
**********

.. target-notes::

- `Heltec Wireless Tracker Official Page <https://heltec.org/project/wireless-tracker/>`_
- `Heltec Wireless Tracker Hardware Update Log <https://docs.heltec.org/en/node/esp32/wireless_tracker/hardware_update_log.html>`_ (Important for V1.1 changes)
- `Heltec Wireless Tracker FAQ <https://docs.heltec.org/en/node/esp32/wireless_tracker/frequently_asked_questions.html>`_
- `ESP-IDF Programming Guide <https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/index.html>`_
- `esptool documentation <https://github.com/espressif/esptool/blob/master/README.md>`_
- `OpenOCD ESP32 <https://github.com/espressif/openocd-esp32/releases>`_
- `Community Repository <https://github.com/jhiggason/lorawirelesstracker>`_

.. [1] https://heltec.org/project/wireless-tracker/
