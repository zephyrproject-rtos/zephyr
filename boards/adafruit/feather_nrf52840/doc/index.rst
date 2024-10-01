.. _adafruit_feather_nrf52840:

Adafruit Feather nRF52840 (Express, Sense)
##########################################

Overview
********

The Adafruit Feather nRF52840 provides support for the Nordic Semiconductor
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

.. tabs::

   .. group-tab:: Express

      .. figure:: img/adafruit_feather_nrf52840_express.jpg
           :align: center
           :alt: Adafruit Feather nRF52840 Express

   .. group-tab:: Sense

      .. figure:: img/adafruit_feather_nrf52840_sense.jpg
           :align: center
           :alt: Adafruit Feather nRF52840 Sense

Hardware
********

- nRF52840 ARM Cortex-M4F processor at 64 MHz
- 1 MB flash memory and 256 KB of SRAM
- Battery connector and charger for 3.7 V lithium polymer batteries
- Charging indicator LED
- 2 User LEDs
- 1 NeoPixel LED
- Reset button
- SWD connector (Express only)
- SWD solder pads on bottom of PCB (Sense only)
- LSM6DS33 Accel/Gyro (Sense only)
- LIS3MDL magnetometer (Sense only)
- APDS9960 Proximity, Light, Color, and Gesture Sensor (Sense only)
- MP34DT01-M PDM Microphone sound sensor (Sense only)
- SHT3X Humidity sensor (Sense only)
- BMP280 temperature and barometric pressure/altitude (Sense only)

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

Other hardware features have not been enabled yet for this board.

Connections and IOs
===================

.. tabs::

   .. group-tab:: Express

      The `Adafruit Feather nRF52840 Express Learn site`_ has
      detailed information about the board including
      `pinouts (Express)`_ and the `schematic (Express)`_.

   .. group-tab:: Sense

      The `Adafruit Feather nRF52840 Sense Learn site`_ has
      detailed information about the board including
      `pinouts (Sense)`_ and the `schematic (Sense)`_.

LED
---

* LED0 (red) = P1.15 (Express)
* LED0 (red) = P1.9 (Sense)
* LED1 (blue) = P1.10

Push buttons
------------

* SWITCH = P1.02
* RESET = P0.18

Programming and Debugging
*************************

Flashing
========

Flashing Zephyr onto both the Feather nRF52840 Express and Sense is possible
using the SWD headers. Only the Express board has an SWD connector however.

Both the Feather nRF52840 Express and Sense ship with the `Adafruit nRF52 Bootloader`_
which supports flashing using `UF2`_. This allows easy flashing of new images,
but does not support debugging the device.

#. Build the Zephyr kernel and the :zephyr:code-sample:`blinky` sample application.

.. tabs::

   .. group-tab:: Express

      .. zephyr-app-commands::
         :zephyr-app: samples/basic/blinky
         :board: adafruit_feather_nrf52840
         :goals: build
         :compact:

   .. group-tab:: Express UF2

      .. zephyr-app-commands::
         :zephyr-app: samples/basic/blinky
         :board: adafruit_feather_nrf52840/nrf52840/uf2
         :goals: build
         :compact:

   .. group-tab:: Sense

      .. zephyr-app-commands::
         :zephyr-app: samples/basic/blinky
         :board: adafruit_feather_nrf52840/nrf52840/sense
         :goals: build
         :compact:

   .. group-tab:: Sense UF2

      .. zephyr-app-commands::
         :zephyr-app: samples/basic/blinky
         :board: adafruit_feather_nrf52840/nrf52840/sense/uf2
         :goals: build
         :compact:

#. If using UF2, connect the board to your host computer using USB.

#. Tap the reset button twice quickly to enter bootloader mode.
   A mass storage device named ``FTHR840BOOT`` for (Express) or
   ``FTHRSNSBOOT`` (Sense) should appear on the host. Ensure this is
   mounted.

#. Flash the image.

.. tabs::

   .. group-tab:: Express

      .. zephyr-app-commands::
         :zephyr-app: samples/basic/blinky
         :board: adafruit_feather_nrf52840
         :goals: flash
         :compact:

   .. group-tab:: Express UF2

      .. zephyr-app-commands::
         :zephyr-app: samples/basic/blinky
         :board: adafruit_feather_nrf52840/nrf52840/uf2
         :goals: flash
         :compact:


   .. group-tab:: Sense

      .. zephyr-app-commands::
         :zephyr-app: samples/basic/blinky
         :board: adafruit_feather_nrf52840/nrf52840/sense
         :goals: flash
         :compact:

   .. group-tab:: Sense UF2

      .. zephyr-app-commands::
         :zephyr-app: samples/basic/blinky
         :board: adafruit_feather_nrf52840/nrf52840/sense/uf2
         :goals: flash
         :compact:

#. You should see the red LED blink.

References
**********

.. target-notes::

.. _Adafruit Feather nRF52840 Express Learn site:
    https://learn.adafruit.com/introducing-the-adafruit-nrf52840-feather/

.. _pinouts (Express):
    https://learn.adafruit.com/introducing-the-adafruit-nrf52840-feather/pinouts

.. _schematic (Express):
    https://learn.adafruit.com/introducing-the-adafruit-nrf52840-feather/downloads

.. _Adafruit Feather nRF52840 Sense Learn site:
    https://learn.adafruit.com/adafruit-feather-sense

.. _pinouts (Sense):
    https://learn.adafruit.com/adafruit-feather-sense/pinouts

.. _schematic (Sense):
    https://learn.adafruit.com/adafruit-feather-sense/downloads

.. _Adafruit nRF52 Bootloader:
    https://github.com/adafruit/Adafruit_nRF52_Bootloader

.. _UF2:
    https://github.com/microsoft/uf2
