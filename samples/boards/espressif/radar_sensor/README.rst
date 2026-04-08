.. _at581x_radar_sample:

AT581X Radar Sensor Sample
##########################

Overview
********

This sample demonstrates how to use the AT581X radar sensor driver with Zephyr.
The AT581X is a microwave radar sensor that can detect motion and presence.

The sample configures the sensor via I2C and monitors a GPIO pin for detection
interrupts. When motion is detected, the sensor pulls the interrupt pin low.

Requirements
************

* ESP32-S3 based board (ESP32S3 DevKitC or ESP32S3 Box3)
* AT581X radar sensor connected via I2C
* GPIO pin connected to sensor's detection output

Hardware Setup
**************

Connect the AT581X sensor to your ESP32-S3 Box3 board:

* VCC: 3.3V
* GND: Ground
* SDA: GPIO41 (I2C1 SDA)
* SCL: GPIO40 (I2C1 SCL)  
* OUT: GPIO21 (Detection interrupt)

Building and Running
********************

Build and flash the sample for ESP32S3 Box3:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/Radar_sensor
   :board: esp32s3_box3/esp32s3/procpu
   :goals: build flash monitor

Sample Output
*************

.. code-block:: console

   [00:00:00.123,000] <inf> main: AT581X radar sensor initialized
   [00:00:00.124,000] <inf> main: Monitoring Radar on IO21...
   [00:00:05.234,000] <inf> main: >>> MOTION DETECTED!
   [00:00:07.456,000] <inf> main: <<< Area Clear

Configuration
*************

The sensor can be configured via device tree properties:

* ``threshold``: Detection sensitivity (0-1023, default 200)
* ``gain``: Amplifier gain (0-255, default 27)  
* ``base-time-ms``: Base detection time (default 500ms)
* ``keep-time-ms``: Keep detection time (default 1500ms)

Driver Architecture
*******************

This sample uses the proper Zephyr sensor driver located in 
``drivers/sensor/at581x/`` instead of a custom driver implementation.
The driver follows Zephyr's sensor API and integrates with the device tree
system for configuration.