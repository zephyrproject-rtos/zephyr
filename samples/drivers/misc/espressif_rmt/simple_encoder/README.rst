.. zephyr:code-sample:: espressif_rmt_simple_encoder
   :name: Espressif RMT with simple encoder
   :relevant-api: espressif_rmt

   Espressif RMT loopback application to demonstrate API usage with simple encoder.

Overview
********

This sample sends a RMT signal on one pin and read it back on another pin using Espressif RMT and simple encoder.
Demonstration is done with and without DMA support.

Requirements
************

To use this sample, an ESP32 DevKitC/M supporting RMT is required.

Wiring
******

Pins 13 (Tx) and 12 (Rx) of the board must be connected together to establish the loopback.

Building and Running
********************

To build the sample on ESP32-S3-DevKitC:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/misc/espressif_rmt/simple_encoder
   :board: esp32s3_devkitc/esp32s3/procpu
   :goals: flash
   :compact:

The following logs will be available:

.. code-block:: console

   Enable RMT RX channel
   Enable RMT TX channel
   RMT TX value 0x00
   RMT RX value 0x00
   RMT TX value 0x01
   RMT RX value 0x01
   RMT TX value 0x02
   RMT RX value 0x02
   RMT TX value 0x03
   RMT RX value 0x03
   RMT TX value 0x04
   RMT RX value 0x04

.. _Espressif RMT: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/rmt.html
