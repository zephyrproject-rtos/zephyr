.. _kineis-kim1-sample:

Satellite Kineis KIM1 module
####################################

Overview
********

This sample demonstrates how to use the Satellite API.
with the module Kineis KIM1. Without adaptions this example runs on STM32F103 discovery
board. You need to establish a physical connection between pins PA9 (UART1_TX) and
PA10 (UART1_RX) on Kineis Arduino board RX_KIM1 and TX_KIM1 respectively according
to the STM32 or Arduino mode jumper.
Don't forget to rely GND pin.

Building and Running
********************

Build and flash as follows, replacing ``nucleo_f103rb`` with your board:

 .. zephyr-app-commands::
    :zephyr-app: samples/drivers/satellite/kineis_kim1
    :board: nucleo_f103rb
    :goals: build flash
    :compact:

Sample output
=============

.. code-block:: console

   Modem ID:
   Modem FW version:
   Modem SN:
   Synchronous send OK
   Launch send pool of 10 messages with 60 seconds delay between them
   Finished sending pool of messages successfully
