.. zephyr:code-sample:: qmi8658c
   :name: QMI8658C motion tracking device
   :relevant-api: sensor_interface

   Get acceleration and angular velocity from a QMI8658C 6-axis IMU sensor (polling mode).

Description
***********

This sample application periodically (10 Hz) measures the sensor acceleration
and angular velocity, displaying the values on the console along with calculated
angles from the accelerometer data.

The QMI8658C is a 6-axis IMU sensor that combines a 3-axis accelerometer and
a 3-axis gyroscope. The sensor communicates via I2C interface at address 0x6A.

The sample application:
- Reads accelerometer data in m/s²
- Reads gyroscope data in rad/s
- Calculates tilt angles (X, Y, Z) from accelerometer data
- Displays all values on the console

Requirements
************

This sample uses the QSTCORP QMI8658C 6-axis IMU sensor, which provides:
- 3-axis accelerometer with ±4g full-scale range
- 3-axis gyroscope with ±512dps full-scale range
- I2C interface at 7-bit address 0x6A

A devicetree overlay must be provided to identify the I2C bus and sensor
configuration.

Wiring
******

This sample uses an external breakout board for the sensor. The sensor must
be connected to an I2C bus on the target board.

Typical connections:
- VCC: 3.3V power supply
- GND: Ground
- SDA: I2C data line
- SCL: I2C clock line

The I2C address is 0x6A (7-bit).

Building and Running
********************

After providing a devicetree overlay that specifies the sensor location,
build this sample app using:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/qmi8658c
   :board: esp32c3_lckfb
   :goals: build flash

For example, create a devicetree overlay file (e.g., ``boards/esp32c3_lckfb.overlay``):

.. code-block:: devicetree

   &i2c0 {
       qmi8658c@6a {
           compatible = "qstcorp,qmi8658c";
           reg = <0x6a>;
           status = "okay";
       };
   };

   aliases {
       qmi8658c_0 = &qmi8658c_6a;
   };

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0-1121-g4feeaa973f02 ***
   [00:00:00.129,000] <inf> qmi8658c_test: QMI8658C sensor test started
   Accel: X=1.10, Y=-0.54, Z=17.62 m/s² | Gyro: X=-8.94, Y=5.87, Z=-0.09 rad/s | Angle: X=3.6°, Y=-1.8°, Z=86.0°
   Accel: X=1.10, Y=-0.54, Z=17.63 m/s² | Gyro: X=-8.94, Y=5.87, Z=-0.06 rad/s | Angle: X=3.6°, Y=-1.7°, Z=86.0°
   Accel: X=1.10, Y=-0.55, Z=17.60 m/s² | Gyro: X=-8.94, Y=5.90, Z=-0.07 rad/s | Angle: X=3.6°, Y=-1.8°, Z=86.0°
   Accel: X=1.07, Y=-0.52, Z=17.62 m/s² | Gyro: X=-8.94, Y=5.92, Z=-0.11 rad/s | Angle: X=3.5°, Y=-1.7°, Z=86.1°
   Accel: X=1.09, Y=-0.54, Z=17.65 m/s² | Gyro: X=-8.94, Y=5.89, Z=-0.03 rad/s | Angle: X=3.5°, Y=-1.7°, Z=86.1°

   <repeats endlessly every 100ms>

Output Explanation
==================

The sample output shows:
- **Accel**: Acceleration values in m/s² for X, Y, and Z axes
  - When the sensor is at rest, the Z-axis should show approximately 9.8 m/s² (gravity)
- **Gyro**: Angular velocity values in rad/s for X, Y, and Z axes
  - When the sensor is at rest, values should be close to 0 rad/s
- **Angle**: Calculated tilt angles in degrees for X, Y, and Z axes
  - Calculated from accelerometer data using atan2 function
  - Z-axis angle of ~86° indicates the sensor is oriented with Z-axis pointing upward

References
**********

For more information about the QMI8658C sensor, see the QSTCORP datasheet.

For more information about Zephyr sensor API, see:
- :ref:`sensor_api`
- :ref:`sensor_drivers`

