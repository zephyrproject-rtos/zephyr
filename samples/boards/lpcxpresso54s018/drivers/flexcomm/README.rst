.. _lpc54s018_flexcomm_sample:

LPC54S018 FLEXCOMM Sample
########################

Overview
********

This sample demonstrates the FLEXCOMM interfaces on the LPC54S018 board.
FLEXCOMM interfaces can be configured as UART, I2C, or SPI.

The sample shows:
- UART communication on FLEXCOMM0 (console)
- I2C communication on FLEXCOMM4
- SPI communication on FLEXCOMM5

Hardware Requirements
********************

- LPCXpresso54S018M development board
- Optional: I2C device connected to FLEXCOMM4 (P0.25=SCL, P0.26=SDA)
- Optional: SPI device connected to FLEXCOMM5 (P0.19=SCK, P0.18=MISO, P0.20=MOSI, P1.1=CS)

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/flexcomm
   :board: lpcxpresso54s018
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** LPC54S018 FLEXCOMM Sample ***
   
   UART (FLEXCOMM0) - Console interface ready
   Baudrate: 115200
   
   I2C (FLEXCOMM4) Configuration:
   - Speed: 400 kHz (Fast mode)
   - Pins: P0.25 (SCL), P0.26 (SDA)
   
   SPI (FLEXCOMM5) Configuration:
   - Mode: Master
   - Speed: 8 MHz
   - Pins: P0.19 (SCK), P0.18 (MISO), P0.20 (MOSI), P1.1 (CS)