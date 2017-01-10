[DTLS Client for Arduino 101]
#############################

The DTLS Client can be tested on top of the Arduino 101 board by
adding an Ethernet daughter board.


Requirements
============

- Arduino 101
- ENC28J60
- LAN for testing purposes


Wiring
======

The ENC28J60 is an Ethernet device with SPI interface. The following
pins must be connected from the ENC28J60 device to the Arduino 101
board:

Arduino 101     ENC28J60
------------------------
D13             SCK
D12             SO
D11             SI
D10             CS
D04             INT
3.3V            VCC
GDN             GND


Build and running
=================

By default, make will build the sample for the QEMU-x86 board.
For the Arduino 101 board, type:

.. code-block:: console

	make pristine && make BOARD=arduino_101

To load the binary in the development board follow the steps
indicated here:

  https://www.zephyrproject.org/doc/board/arduino_101.html

