MAX22190 8 channel industrial diginal input sensor
###########################################

Overview
********

This is sample applicatino will read 8ch input with ebaled
wire break functionality for all channels

Requirements
************

- MAX22190 wired to your board SPI bus
- MAX22190 FAULTB, READYB, LATCH wired to your board GPIOs

References
**********

 - MAX22190: https://www.analog.com/media/en/technical-documentation/data-sheets/MAX22190.pdf

Building and Running
********************

This sample can be built with any board that supports SPI. A sample overlay is
provided for the ADI-SDP-K1 with SDP-I-PMOD adapted board board.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/max22190_sdp_120_pmod
   :board: adi_sdp_k1
   :goals: build
   :compact:

Sample Output
=============

The application will read and print all IN channels state and WB condition every second
Normal operating state is all WB_n=L which mean, there is not wire break condition.
IN_n could be H and L which indicate state of the input.

.. code-block:: console

   IN_1=H WB_1=L IN_2=L WB_2=L IN_3=L WB_3=L IN_4=L WB_4=L IN_5=L WB_5=L IN_6=L WB_6=L IN_7=L WB_7=L IN_8=L WB_8=L
   IN_1=L WB_1=L IN_2=L WB_2=L IN_3=L WB_3=L IN_4=L WB_4=L IN_5=L WB_5=L IN_6=L WB_6=L IN_7=L WB_7=L IN_8=L WB_8=L
   IN_1=L WB_1=L IN_2=L WB_2=H IN_3=L WB_3=H IN_4=L WB_4=L IN_5=L WB_5=L IN_6=L WB_6=L IN_7=L WB_7=L IN_8=L WB_8=L

   <repeats endlessly every second>
