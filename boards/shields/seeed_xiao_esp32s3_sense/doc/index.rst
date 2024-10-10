.. _seeed_xiao_esp32s3_sense:

Seeed Studio XIAO ESP32S3 Sense
#################################

Overview
********

The Seeed Studio XIAO ESP32S3 Sense is a compact development board designed for AI and IoT applications, integrating a camera sensor,
digital microphone, and an SD card slot. It fits the base XIAO ESP32S3 board equipped with the B2B connector.

.. figure:: img/xiao-esp32s3-sense-full.png
     :align: center
     :alt: Seeed Studio XIAO ESP32S3 Sense

     Seeed Studio XIAO ESP32S3 Sense Board (Credit: Seeed Studio)

Requirements
************

This shield can only bse used with Seeed Studio XIAO ESP32S3 with soldered B2B connector

Programming
***********

Set ``--shield seeed_xiao_esp32s3_sense`` when you invoke ``west build``. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/video/capture
   :board: xiao_esp32s3/esp32s3/procpu
   :shield: seeed_xiao_esp32s3_sense
   :goals: build
