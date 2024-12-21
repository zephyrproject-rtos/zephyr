.. zephyr:code-sample:: th02
   :name: TH02 Temperature and Humidity Sensor
   :relevant-api: sensor_interface

   Get temperature and humidity data from a TH02 sensor (polling mode).

Overview
********
This sample periodically reads temperature and humidity from the Grove
Temperature & Humidity Sensor (TH02) and display the results on the Grove LCD
display.


Requirements
************

This sample uses the TH02 sensor and the grove LCD display. Both devices are
controlled using the I2C interface.

More details about the sensor and the display can be found here:

- `Grove Temperature And Humidity`_
- `Grove LCD Module`_

Wiring
******

The easiest way to get this wired is to use the Grove shield and connect both
devices to I2C. No additional wiring is required. Depending on the board you are
using you might need to connect two 10K ohm resistors to SDL and SDA (I2C).
The LCD display requires 5 volts, so the voltage switch on the shield needs to
be on 5v.


References
**********

 - TH02 (Si7005): https://www.silabs.com/documents/public/data-sheets/Si7005.pdf


.. _Grove LCD Module: http://wiki.seeed.cc/Grove-LCD_RGB_Backlight/
.. _Grove Temperature And Humidity: http://wiki.seeed.cc/Grove-TemptureAndHumidity_Sensor-High-Accuracy_AndMini-v1.0/
