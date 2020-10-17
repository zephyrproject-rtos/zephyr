.. _adafruit_clue_nrf52840:

Adafruit Clue nRF52840 Express
#################################

Overview
********

The Adafruit Clue nRF52840 provides support for the Nordic Semiconductor
nRF52840 ARM Cortex-M4F CPU and the following devices:

* :abbr:`ADC (Analog to Digital Converter)`
* CLOCK
* FLASH
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`I2C (Inter-Integrated Circuit)`
* :abbr:`MPU (Memory Protection Unit)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`PWM (Pulse Width Modulation)`
* RADIO (Bluetooth Low Energy and 802.15.4)
* :abbr:`RTC (nRF RTC System Clock)`
* Segger RTT (RTT Console)
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UART (Universal asynchronous receiver-transmitter)`
* :abbr:`USB (Universal Serial Bus)`
* :abbr:`WDT (Watchdog Timer)`

.. figure:: img/adafruit_feather_nrf52840.jpg
     :width: 640px
     :align: center
     :alt: Adafruit Feather nRF52840 Express

QSPI: GD25Q16C (working)
  * DATA0: P0.17
  * DATA1: P0.22
  * DATA2: P0.23
  * DATA3: P0.21
  * SCK  : P0.19 
  * CS   : P0.20
Neopixel:
  * P0.16
HW LED (working):
  * P1.01
I2C:
  * SCL: P0.25
  * SDA: P0.24
SPI:
  * SCK: P0.08
  * MOSI: P0.26
  * MISO: P0.06
UART:
  * RX: P0.04
  * TX: P0.05
Microphone:
  * CLK : P0.01
  * DATA: P0.00
Speaker:
  * P1.00
Proximity/Light Interrupt:
  * P0.09
Accelerometer/Gyro Interrupt:
  * P1.06
White LEDs:
  * P0.10
Buttons:
  * Button A: P1.02
  * Button B: P1.10
TFT ST7789: (working)
  * Reset: P1.03
  * Backlight: P1.05
  * CS: P0.12
  * DC: P0.13
  * SCK: P0.14
  * MOSI: P0.15

TODO: describe UF2 bootloader 
https://github.com/microsoft/uf2

```
uf2conv.py -c -f 0xada52840 build/zephyr/zephyr.hex
```
file created flash.uf2, drag-n-drop to CLUEBOOT
double-press reset button to go into bootloader.



Hardware
********

- nRF52840 ARM Cortex-M4F processor at 64 MHz
- 1 MB flash memory and 256 KB of SRAM
- Battery connector and charger for 3.7 V lithium polymer batteries
- Charging indicator LED
- 2 User LEDs
- 1 NeoPixel LED
- Reset button
- SWD connector

Supported Features
==================

The Adafruit Feather nRF52840 board configuration supports the
following hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| ADC       | on-chip    | adc                  |
+-----------+------------+----------------------+
| CLOCK     | on-chip    | clock_control        |
+-----------+------------+----------------------+
| FLASH     | on-chip    | flash                |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| I2C       | on-chip    | i2c                  |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| RADIO     | on-chip    | Bluetooth,           |
|           |            | ieee802154           |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| SPI       | on-chip    | spi                  |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| USB       | on-chip    | usb                  |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Other hardware features are not supported by the Zephyr kernel.

Connections and IOs
===================

The `Adafruit Feather nRF52840 Express Learn site`_ has detailed
information about the board including `pinouts`_ and the `schematic`_.

LED
---

* LED0 (red) = P1.15
* LED1 (blue) = P1.10

Push buttons
------------

* SWITCH = P1.02
* RESET = P0.18

Programming and Debugging
*************************

Applications for the ``adafruit_feather_nrf52840`` board configuration
can be built and flashed in the usual way (see :ref:`build_an_application`
and :ref:`application_run` for more details).

Flashing
========

Flashing Zephyr onto the ``adafruit_feather_nrf52480`` board requires
an external programmer. The programmer is attached to the SWD header.

Build the Zephyr kernel and the :ref:`blinky-sample` sample application.

   .. zephyr-app-commands::
      :zephyr-app: samples/blinky
      :board: adafruit_feather_nrf52840
      :goals: build
      :compact:

Flash the image.

   .. zephyr-app-commands::
      :zephyr-app: samples/blinky
      :board: adafruit_feather_nrf52840
      :goals: flash
      :compact:

You should see the the red LED blink.

References
**********

.. target-notes::

.. _Adafruit Feather nRF52840 Express Learn site:
    https://learn.adafruit.com/introducing-the-adafruit-nrf52840-feather/

.. _pinouts:
    https://learn.adafruit.com/introducing-the-adafruit-nrf52840-feather/pinouts

.. _schematic:
    https://learn.adafruit.com/introducing-the-adafruit-nrf52840-feather/downloads
