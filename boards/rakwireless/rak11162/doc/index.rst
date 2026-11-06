.. zephyr:board:: rak11162

Overview
********

The RAK11162 WisBlock Core Module carries a RAK11160 WisDuo stamp module, which
pairs an STM32WLE5CC with its own LoRa transceiver and an Espressif ESP8684
co-processor for Wi-Fi and Bluetooth, on a board that ends in the 40-pin
WisBlock connector.

Zephyr runs on the STM32WLE5CC. The ESP8684 ships with AT firmware and is driven
over the module's internal UART by the :kconfig:option:`CONFIG_WIFI_ESP_AT`
driver.

- `WisBlock RAK11162 Website`_
- `WisDuo RAK11160 Website`_

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The WisBlock connector is split between the two processors. Zephyr only reaches
the signals the STM32WLE5CC drives:

+---------------------+-----------+
| WisBlock signal     | STM32WLE5 |
+=====================+===========+
| RESET               | NRST      |
+---------------------+-----------+
| LED1                | PA10      |
+---------------------+-----------+
| LED2                | PA1       |
+---------------------+-----------+
| I2C1 SDA/SCL        | PA11/PA12 |
+---------------------+-----------+
| AIN0                | PB3       |
+---------------------+-----------+
| AIN1                | PB4       |
+---------------------+-----------+
| BOOT0               | BOOT0     |
+---------------------+-----------+
| SPI CS/CLK/MISO/MOSI| PA4/PA5/  |
|                     | PA6/PA7   |
+---------------------+-----------+
| IO1                 | PB5       |
+---------------------+-----------+
| IO2                 | PA8       |
+---------------------+-----------+
| IO3                 | PB12      |
+---------------------+-----------+
| IO4                 | PB2       |
+---------------------+-----------+
| IO5                 | PA15      |
+---------------------+-----------+
| IO6                 | PA9       |
+---------------------+-----------+

The remaining connector signals belong to the ESP8684 and have no STM32WLE5 pin
behind them: SW1, TXD0, RXD0, LED3, IO7, TXD1, RXD1 and I2C2. A WisBlock module
that needs one of them cannot be driven from Zephyr.

LED1 and LED2 drive LEDs on the base board rather than on the module, so the
board defines no ``led0`` alias. An application that needs one, such as
:zephyr:code-sample:`blinky`, gets it from an overlay naming the LEDs of the
base board in use.

The console is UART2 on PA2 and PA3. It does not appear on the connector, and
reaches the host through the CH340E bridge behind the base board's USB port, so
no serial adapter is needed.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The board exposes SWD on the P1 header. Build and flash an application with an
external debug probe:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rak11162/stm32wle5xx
   :goals: build flash

Run a serial terminal on the USB port of the base board at 115200 8N1 and reset
the board:

.. code-block:: console

   Hello World! rak11162/stm32wle5xx

The ESP8684 is programmed over its own UART, which the board brings out on test
points TP1 to TP4 together with its boot pin. Replacing the AT firmware stops
the Wi-Fi driver from working until it is written back.

References
**********

.. target-notes::

.. _WisBlock RAK11162 Website:
   https://docs.rakwireless.com/product-categories/wisblock/rak11162/overview

.. _WisDuo RAK11160 Website:
   https://docs.rakwireless.com/product-categories/wisduo/rak11160-module/overview
