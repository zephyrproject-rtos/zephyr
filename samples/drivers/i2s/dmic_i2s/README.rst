.. zephyr:code-sample:: dmic_i2s
   :name: DMIC to I2S
   :relevant-api: i2s_interface

   Process an audio stream to I2S.

Overview
********

This sample demonstrates how to use an I2S driver in a simple processing of
an audio stream. It configures and starts from memory buffer (sample sine
wave or sequential number) or from DMIC to i2s data streaming out.
It can also be used to test the I2S loopback functionality.

Requirements
************

This sample has been tested on Apollo510 EVB only (loopback)
or Apollo510 EVB + mikroBUS DMIC board. (e.g., TDK583x MICs)

Building and Running
********************

The code can be found in :zephyr_file:`samples/drivers/i2s/dmic_i2s`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/i2s/dmic_i2s
   :board: apollo510_evb
   :goals: build flash
   :compact:

Input audio data (PDM, Sine wave or sequalcial number) is streamed via I2S0
Interface pin for PDM to I2S. - I2S TX: CLK_P5, SDOUT_P6, WS_P7
Interface pin for I2S loopback.
- I2S0 TX: CLK_P5, SDOUT_P6, WS_P7 / I2S1 RX: CLK_P16, SDIN_P19, WS_P18
