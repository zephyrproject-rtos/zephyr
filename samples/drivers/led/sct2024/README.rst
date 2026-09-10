.. zephyr:code-sample:: sct2024
   :name: SCT2024 LED Controller
   :relevant-api: led_interface

   Control up to 16 LEDs connected to an SCT2024 chip.

Overview
********

This sample controls up to 16 LEDs connected to a SCT2024 driver.

In an infinite loop, a test pattern is applied to each LED one by one. The
pattern is a sequence of turning each LED on; when all LEDs are on, each of the
LEDs is turned off.

Building and Running
********************

This sample can be built and executed on boards with an SCT2024 driver
connected. A node matching the device type binding should be defined in the
board DTS files.

As an example this sample provides a DTS overlay for the :zephyr:board:`nrf52840dk`
board (:file:`boards/nrf52840dk_nrf52840.overlay`). It assumes that an SCT2024
LED driver (with 16 LEDs wired) is connected to the SPI bus at selected pins.

References
**********

- SCT2024: http://www.starchips.com.tw/pdf/datasheet/SCT2024V01_03.pdf
