.. zephyr:code-sample:: aht30_temperature_humidity
   :name: AHT30 Temperature and Humidity Sensor
   :relevant-api: sensor_interface

   Read temperature and humidity data from AHT30 sensor on ESP32-S3 Box 3.

Overview
********

This sample demonstrates how to read temperature and humidity data from the
AHT30 sensor using Zephyr's sensor API. The AHT30 is a high-precision digital
temperature and humidity sensor that communicates via I2C interface.

The application continuously reads sensor data every 5 seconds and displays:
- Temperature in degrees Celsius (°C) with 0.01°C resolution
- Relative humidity as percentage (%RH) with 0.01%RH resolution
- Raw sensor values for debugging purposes

The sample showcases proper sensor initialization, data fetching, and value
conversion using Zephyr's standardized sensor framework.

Requirements
************

* ESP32-S3 Box 3 board
* ESP32-S3 Box 3 sensor accessories dock (with AHT30 sensor)
* USB cable for programming and power

Wiring
******

The AHT30 sensor comes pre-wired on the ESP32-S3 Box 3 sensor accessories dock:

.. list-table::
   :header-rows: 1

   * - AHT30 Pin
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

Building and Running
********************

Build and flash the sample for ESP32S3 Box3:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/temperature
   :board: esp32s3_box3/esp32s3/procpu
   :goals: build flash monitor

Sample Output
*************

The sample will continuously output sensor readings:

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0 ***
   [00:00:00.123,000] <inf> aht30_sensor: AHT30 Temperature & Humidity Sensor Sample
   [00:00:00.123,000] <inf> aht30_sensor: ESP32-S3 Box 3 - I2C0 (SDA=GPIO41, SCL=GPIO40)
   [00:00:00.124,000] <inf> aht30_sensor: AHT30 sensor device ready
   [00:00:00.124,000] <inf> aht30_sensor: Device name: aht30@38
   [00:00:02.124,000] <inf> aht30_sensor: Starting sensor readings every 5 seconds...
   === AHT30 Sensor Reading ===
   Temperature: 23.45 °C
   Humidity: 45.67 %RH
   Raw values - Temp: 23.450000, Humidity: 45.670000
   =============================

   [00:00:02.125,000] <inf> aht30_sensor: Temperature: 23.45°C, Humidity: 45.67%RH

Configuration
*************

The sensor is configured via device tree in the board definition:

.. code-block:: devicetree

   &i2c1 {
       aht30: aht30@38 {
           compatible = "aosong,aht20";
           reg = <0x38>;
           status = "okay";
       };
   };

Troubleshooting
***************

Common issues and solutions:

* **"AHT30 sensor device not ready"**: Check hardware connection and ensure sensor dock is properly connected
* **"Failed to fetch sensor data"**: Verify I2C communication and check for proper pull-up resistors
* **Unstable readings**: Allow sensor to stabilize for 2-3 minutes after power-on

References
**********

* `AHT30 Datasheet <https://server4.eca.ir/eshop/AHT30/Aosong_AHT30_en_draft_0c.pdf>`_
* :ref:`sensor_api`