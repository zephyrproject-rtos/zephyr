.. zephyr:code-sample:: uart-rs485
   :name: UART RS485 echo bot
   :relevant-api: uart_interface

   Read data from RS485 bus and echo it back.

Overview
********

This sample demonstrates how to use the UART serial driver with RS485 transceiver
control. It reads data from the RS485 bus and echoes the characters back after
an end of line (return key) is received.

The sample automatically configures the UART in RS485 mode and controls the
Driver Enable (DE) and Receiver Enable (RE) signals to manage bus direction.

The polling API is used for sending data and the interrupt-driven API
for receiving, so that in theory the thread could do something else
while waiting for incoming data.

Hardware Requirements
*********************

This sample requires a UART peripheral with RS485 transceiver control support.
The hardware must have:

* A UART peripheral (e.g., SERCOM on SAM devices)
* GPIO pins for DE (Driver Enable) and RE (Receiver Enable) control
* An RS485 transceiver chip connected to the UART

Test Hardware
=============

This sample was tested with the following hardware:

* **Development Board**: SAME54 Xplained Pro
* **RS485 Transceiver**: `SparkFun Transceiver Breakout - RS-485 <https://www.sparkfun.com/sparkfun-transceiver-breakout-rs-485.html>`_
* **USB-to-RS485 Adapter**: `DTECH USB to RS422/RS485 Converter Cable <https://www.dtechelectronics.com/high-quality-usb-2-0-to-rs422-rs485-converter-cable-1-2m-gold-plated-db9-male-usb-to-rs422-rs485-adapter-cable_p368.html>`_

SAME54 Xplained Pro Configuration
**********************************

For the SAME54 Xplained Pro board, the sample is configured to use:

* **SERCOM0** as UART
* **PA04** - TX (SERCOM0 PAD0)
* **PA05** - RX (SERCOM0 PAD1)
* **PA22** - DE/RE (Driver/Receiver Enable, shared pin)
* **Baud rate**: 115200

Connect your RS485 transceiver as follows:

1. Connect SERCOM0 TX (PA04) to transceiver DI (Driver Input)
2. Connect SERCOM0 RX (PA05) to transceiver RO (Receiver Output)
3. Connect PA22 to both DE and ~RE pins on the transceiver
4. Connect A and B lines to your RS485 bus
5. Add 120Ω termination resistor if at bus end

Building and Running
********************

Build and flash the sample for SAME54 Xplained Pro:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/uart/echo_bot_rs485
   :board: same54_xpro
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    *** Booting Zephyr OS build v4.3.0-rc3 ***
    RS485 mode enabled
    Hello! I'm your echo bot.
    Tell me something and press enter:
    # Type e.g. "Hi there!" and hit enter!
    Echo: Hi there!

Testing
*******

To test the RS485 functionality:

1. Connect another RS485 device to the bus
2. Configure it with 115200 baud, 8N1
3. Send text data followed by newline
4. The echo bot will respond with "Echo: <your message>"

The DE/RE GPIO will automatically:

* Switch to transmit mode (DE=HIGH, RE=LOW) when sending
* Switch to receive mode (DE=LOW, RE=HIGH) when receiving
* Transition happens automatically in the driver
