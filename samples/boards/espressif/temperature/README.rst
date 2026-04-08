.. zephyr:code-sample:: aht30-esp32s3-box3
   :name: AHT30 Temperature and Humidity Sensor
   :relevant-api: sensor_interface

   Read temperature and humidity data from AHT30 sensor on ESP32-S3 Box 3.

Overview
********

This sample demonstrates how to read temperature and humidity data from the
AHT30 sensor that comes with the ESP32-S3 Box 3 sensor accessories dock.

The AHT30 sensor is connected via I2C0 interface:
- **I2C Address**: 0x38
- **SDA Pin**: GPIO 41
- **SCL Pin**: GPIO 40

The application reads sensor data every 5 seconds and displays:
- Temperature in degrees Celsius (°C)
- Relative humidity as percentage (%RH)
- Raw sensor values for debugging

Hardware Requirements
*********************

- ESP32-S3 Box 3 board
- ESP32-S3 Box 3 sensor accessories dock (with AHT30 sensor)
- USB cable for programming and power

Sensor Specifications
*********************

**AHT30 Sensor Features:**
- Temperature range: -40°C to +85°C
- Temperature accuracy: ±0.3°C (typical)
- Humidity range: 0% to 100% RH
- Humidity accuracy: ±2% RH (typical)
- I2C interface with 3.3V supply
- Low power consumption

Building and Running
********************

Build and flash the sample:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/aht30_esp32s3_box3
   :board: esp32s3_box3/esp32s3/procpu
   :goals: build flash monitor
   :compact:

Sample Output
*************

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
   === AHT30 Sensor Reading ===
   Temperature: 23.48 °C
   Humidity: 45.71 %RH
   Raw values - Temp: 23.480000, Humidity: 45.710000
   =============================

Troubleshooting
***************

If the sensor doesn't work:

1. **Check Hardware Connection**: Ensure the sensor accessories dock is properly connected
2. **Verify I2C Address**: The AHT30 should appear at address 0x38 on I2C0
3. **Check Power**: Ensure the sensor dock is receiving 3.3V power
4. **I2C Bus Issues**: Check for proper pull-up resistors on SDA/SCL lines
5. **Device Tree**: Verify the sensor is enabled in the device tree

**Common Error Messages:**
- "AHT30 sensor device not ready" - Hardware connection issue
- "Failed to fetch sensor data" - I2C communication problem
- "Failed to get temperature/humidity" - Sensor reading error

The sensor readings should be stable and update every 5 seconds with current
environmental conditions.