.. zephyr:board:: rak11161

Overview
********

The RAK11161 Breakout Board carries a RAK11160 WisDuo stamp module, which pairs
an STM32WLE5CC with its own LoRa transceiver and an Espressif ESP8684
co-processor for Wi-Fi and Bluetooth, and transfers its pins to 2.54 mm
headers.

Zephyr runs on the STM32WLE5CC. The ESP8684 ships with AT firmware and is driven
over the module's internal UART by the :kconfig:option:`CONFIG_WIFI_ESP_AT`
driver.

- `WisDuo RAK11161 Website`_
- `WisDuo RAK11160 Website`_

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The headers carry the module pins of both processors:

+--------+-------------------------------------------------------------+
| Header | Signals                                                     |
+========+=============================================================+
| J1     | ESP8684 UART and boot pin, for writing its firmware         |
+--------+-------------------------------------------------------------+
| J2     | STM32WLE5 SWDIO, SWCLK and reset                            |
+--------+-------------------------------------------------------------+
| J3     | STM32WLE5 UART2 and BOOT0                                   |
+--------+-------------------------------------------------------------+
| J4     | ESP8684 GPIO                                                |
+--------+-------------------------------------------------------------+
| J5     | STM32WLE5 SPI1, I2C2, PA8 and PA10                          |
+--------+-------------------------------------------------------------+
| J6     | STM32WLE5 PA1, PA9, PA15, PB2, PB3, PB4, PB5 and PB12       |
+--------+-------------------------------------------------------------+

The console is UART2 on PA2 and PA3, on the J3 header, so a serial adapter is
needed to read it.

The board carries no LEDs. Its three buttons drive the STM32WLE5 reset and BOOT0
pins and the ESP8684 boot pin, none of which an application can read.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The board exposes SWD on the J2 header. Build and flash an application with an
external debug probe:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak11161/stm32wle5xx
   :goals: build flash

Run a serial terminal on the J3 header at 115200 8N1 and reset the board:

.. code-block:: console

   Hello World! rak11161/stm32wle5xx

The ESP8684 is programmed over the J1 header. Replacing the AT firmware stops
the Wi-Fi driver from working until it is written back.

References
**********

.. target-notes::

.. _WisDuo RAK11161 Website:
   https://docs.rakwireless.com/product-categories/wisduo/rak11161-breakout-board/overview

.. _WisDuo RAK11160 Website:
   https://docs.rakwireless.com/product-categories/wisduo/rak11160-module/overview
