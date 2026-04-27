.. zephyr:code-sample:: at581x_radar_sensor
   :name: AT581X Radar Motion Detection
   :relevant-api: sensor_interface gpio_interface

   Demonstrate motion detection using the AT581X microwave radar sensor with GPIO interrupt handling.

Overview
********

This sample demonstrates how to use the AT581X radar sensor driver for motion detection
on ESP32-S3 based boards. The AT581X is a microwave radar sensor specifically designed
for presence and motion detection, not distance measurement.

Unlike distance sensors that measure actual distances to objects, the AT581X radar sensor
detects motion and presence within its detection range. The sensor provides a digital
output signal that changes state when motion is detected or when the area becomes clear.

The sample configures the sensor via I2C and monitors a GPIO pin for detection state changes.
When motion is detected, the sensor pulls the interrupt pin low, and when the area clears,
the pin goes high again.

Requirements
************

* ESP32-S3 based board (ESP32S3 DevKitC or ESP32S3 Box3)
* Sensor Bottom

Connect the sensor bottom to your ESP32-S3 Box3 board


Building and Running
********************

The AT581X sensor is available as a shield (``esp32s3_box3_at581x``) for the ESP32-S3-BOX-3
board. Using the shield is the recommended approach as it automatically applies the correct
device tree overlay (I2C1 on GPIO40/GPIO41, interrupt on GPIO21).

Build and flash using the shield:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/radar_sensor
   :board: esp32s3_box3/esp32s3/procpu
   :shield: esp32s3_box3_at581x
   :goals: build flash monitor

Or equivalently using the ``west`` CLI directly:

.. code-block:: console

  west build -b esp32s3_box3/esp32s3/procpu -- -DSHIELD=esp32s3_box3_at581x

The ``-DSHIELD`` flag loads ``boards/shields/esp32s3_box3_at581x/esp32s3_box3_at581x.overlay``
which configures I2C1 and the AT581X node automatically — no manual overlay needed.

Alternatively, without the shield (manual overlay):

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/radar_sensor
   :board: esp32s3_box3/esp32s3/procpu
   :goals: build flash monitor

Sample Output
*************

The sample will output detection events to the console:

.. code-block:: console

   [00:00:00.123,000] <inf> main: AT581X radar sensor initialized
   [00:00:00.124,000] <inf> main: Monitoring Radar on IO21...
   [00:00:05.234,000] <inf> main: >>> MOTION DETECTED!
   [00:00:07.456,000] <inf> main: <<< Area Clear

Configuration
*************

The sensor behavior can be customized via device tree properties in your board overlay:

.. code-block:: devicetree

   &i2c1 {
       at581x: at581x@28 {
           compatible = "andar,at581x";
           reg = <0x28>;
           threshold = <150>;        /* Lower for higher sensitivity */
           gain = <30>;              /* Adjust amplification */
           base-time-ms = <300>;     /* Faster response time */
           keep-time-ms = <2000>;    /* Longer detection persistence */
           status = "okay";
       };
   };

Available properties:

* ``threshold``: Detection sensitivity (0-1023, default 200)
* ``gain``: Amplifier gain (0-255, default 27)  
* ``base-time-ms``: Base detection time (default 500ms)
* ``keep-time-ms``: Keep detection time (default 1500ms)

References
**********

* `AT581X Datasheet <https://www.andar.com.cn/>`_