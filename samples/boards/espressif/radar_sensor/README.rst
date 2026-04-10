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
* AT581X radar sensor module
* Jumper wires for connections

Wiring
******

Connect the AT581X sensor to your ESP32-S3 Box3 board as follows:

.. list-table::
   :header-rows: 1

   * - AT581X Pin
     - ESP32S3 Box3 Pin
     - Description
   * - VCC
     - 3.3V
     - Power supply
   * - GND
     - Ground
     - Ground connection
   * - SDA
     - GPIO41
     - I2C1 data line
   * - SCL
     - GPIO40
     - I2C1 clock line
   * - OUT
     - GPIO21
     - Detection output (interrupt)

Building and Running
********************

Build and flash the sample for ESP32S3 Box3:

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
* :ref:`sensor_api`
* :ref:`gpio_api`