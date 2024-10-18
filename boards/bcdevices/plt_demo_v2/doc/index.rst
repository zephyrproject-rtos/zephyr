.. zephyr:board:: blueclover_plt_demo_v2

Overview
********

The Blue Clover PLT Demo V2 is an open source (OSWHA certified) hardware
product, featuring the Nordic Semiconductor nRF52832 ARM Cortex-M4F MCU
and several useful external peripherals.

The Nordic Semiconductor nRF52832 ARM Cortex-M4F MCU features the following:

* :abbr:`ADC (Analog to Digital Converter)`
* CLOCK
* FLASH
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`I2C (Inter-Integrated Circuit)`
* :abbr:`MPU (Memory Protection Unit)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`PWM (Pulse Width Modulation)`
* RADIO (Bluetooth Low Energy)
* :abbr:`RTC (nRF RTC System Clock)`
* Segger RTT (RTT Console)
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UART (Universal asynchronous receiver-transmitter)`
* :abbr:`WDT (Watchdog Timer)`

Hardware
********

- nRF52832 ARM Cortex-M4F processor at 64 MHz
- 512 KB flash memory and 64 KB of SRAM
- Bosch BMI270 IMU
- Sensiron SHT30 Humidity and Temperature sensor
- Murata PKLCS1212E4001R1 Piezo Buzzer
- Battery connector and charger for 3.7 V lithium polymer batteries
- 4 APA102C Addressable LEDs
- Reset button (can be configured as user button)
- 1 User button
- Tag-Connect TC2030-FP 6-pin Debug Connector

Supported Features
==================

The Blue Clover PLT Demo V2 board configuration supports the
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
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| RADIO     | on-chip    | Bluetooth            |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| RTT       | Segger     | console              |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Connections and IOs
===================

Push buttons
------------

* RESET = P0.21
* STATUS = P0.26

UART
----

* TXD = P0.06
* RXD = P0.08

Power
-----

* USB-C Connector
* JST-PH Battery Connector

NFC
---

* U.FL Connector, on NFC1/P0.09, NFC2/P0.10

Programming and Debugging
*************************

Applications for the ``blueclover_plt_demo_v2/nrf52832`` board configuration
can be built and flashed in the usual way (see :ref:`build_an_application`
and :ref:`application_run` for more details).

Flashing
========

Flashing Zephyr onto the ``blueclover_plt_demo_v2/nrf52832`` board requires
an external programmer. The programmer is attached to the SWD header.

Build the Zephyr kernel and the :zephyr:code-sample:`led-strip` sample application.

   .. zephyr-app-commands::
      :zephyr-app: samples/drivers/led/led_strip
      :board: blueclover_plt_demo_v2/nrf52832
      :goals: build
      :compact:

Flash the image.

   .. zephyr-app-commands::
      :zephyr-app: samples/drivers/led/led_strip
      :board: blueclover_plt_demo_v2/nrf52832
      :goals: flash
      :compact:

References
**********

.. target-notes::

.. _Blue Clover PLT Demo V2 Product site:
    https://bcdevices.com/products/plt-demo-board

.. _Blue Clover PLT Demo V2 OSWHA Certification:
    https://certification.oshwa.org/us002054.html

.. _Schematic, layout, and gerbers:
    https://github.com/bcdevices/plt-docs/tree/master/PLT-DEMOv2
