.. _arduino_nicla_sense_me:

Arduino Nicla Sense ME
######################

Overview
********
The `Arduino Nicla Sense ME`_ is designed around Nordic Semiconductor's
nrf52832 ARM Cortex-M4F CPU. The board houses 4 low power industrial grade sensors
that can measure rotation, acceleration, pressure, humidity, temperature, air quality
and CO2 levels.

.. figure:: arduino_nicla_sense_me.jpg
   :align: center
   :alt: Arduino Nicla Sense ME

   Arduino Nicla Sense ME (Credit: Arduino)

Hardware
********

- nRF52832 ARM Cortex-M4 processor at 64 MHz
- 512 kB flash memory, 64 kB SRAM
- Bluetooth Low Energy
- Micro USB (USB-B)
- JST 3-pin 1.2 mm pitch battery connector
- 10 Digital I/O pins
- 2 Analog input pins
- 12 PWM pins
- One reset button
- RGB LED (I2C)
- On board sensors:

  - Accelerometer/Gyroscope: Bosch BHI260AP
  - Gas/Pressure/Temperature/Humidity: Bosch BME688
  - Geomagnetic: Bosch BMM150
  - Digital Pressure: Bosch BMP390

Supported Features
==================

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
| I2C(M/S)  | on-chip    | i2c                  |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| RADIO     | on-chip    | Bluetooth Low Energy |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Connections and IOs
===================

Available pins:
---------------
.. figure:: arduino_nicla_sense_me_pinout.jpg
   :align: center
   :alt: Arduino Nicla Sense ME pinout

   Arduino Nicla Sense ME pinout (Credit: Arduino)

For more details please refer to the `datasheet`_, `full pinout`_ and the `schematics`_.

References
**********

.. target-notes::

.. _Arduino Nicla Sense ME:
    https://docs.arduino.cc/hardware/nicla-sense-me

.. _datasheet:
   https://docs.arduino.cc/resources/datasheets/ABX00050-datasheet.pdf

.. _full pinout:
    https://docs.arduino.cc/static/b35956b631d757a0455c286da441641b/ABX00050-full-pinout.pdf

.. _schematics:
    https://docs.arduino.cc/static/ebd652e859efba8536a7e275c79d5f79/ABX00050-schematics.pdf
