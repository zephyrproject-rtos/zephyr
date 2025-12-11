.. zephyr:code-sample:: jc42
   :name: JEDEC JC 42.4 compliant Temperature Sensor
   :relevant-api: sensor_interface

   Get ambient temperature from a JEDEC JC 42.4 compliant temperature sensor (polling & trigger
   mode).

Overview
********

This sample application periodically (0.5 Hz) measures the ambient
temperature. The result is written to the console.

If triggered measurements are enabled the sample initializes and
maintains a |plusminus| 2 |deg| C window around the current temperature.
When the temperature moves outside the window an alert is given, and the
window is moved to center on the new temperature.

Requirements
************

The sample requires a JEDEC JC 42.4 compliant temperature sensor. The
sample is configured to use the MCP9808 sensor.

The MCP9808 digital temperature sensor converts temperatures between -20 |deg|
C and +100 |deg| C to a digital word with |plusminus| 0.5 |deg| C (max.)
accuracy. It is I2C compatible and supports up to 16 devices on the bus.

Wiring
*******

The MCP9808, which is a JEDEC JC 42.4 compliant temperature sensor, is
available in a discrete component form but it is much easier to
use it mounted on a breakout board.  We used the Adafruit `MCP9808
Sensor`_ breakout board.

.. _`MCP9808 Sensor`: https://www.adafruit.com/product/1782

Building and Running
********************

After providing a devicetree overlay that specifies the sensor I2C bus
and alert GPIO, build this sample app using:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/jc42
   :board: particle_xenon
   :goals: build flash

Sample Output
=============

Note that in the capture below output from the trigger callback and the
main thread are interleaved.

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v2.1.0-537-gbbdeaa1ae5bb  ***
   Alert on temp outside [24500, 25500] milli-Celsius
   Trtrigger fired 1, temp 15.9375 C
   iggAlert on temp outside [15437, 16437] milli-Celsius
   er set got 0
   0:00:00.017: 15.9375 C
   0:00:02.020: 16 C
   0:00:04.022: 16.125 C
   0:00:06.024: 16.1875 C
   trigger fired 2, temp 16.3125 C
   Alert on temp outside [15812, 16812] milli-Celsius
   0:00:08.027: 16.3125 C
   0:00:10.029: 16.375 C
   0:00:12.032: 16.5 C
   0:00:14.034: 16.5625 C
   0:00:16.037: 16.5625 C
   0:00:18.039: 16.625 C
   0:00:20.042: 16.6875 C
   trigger fired 3, temp 16.8125 C
   Alert on temp outside [16312, 17312] milli-Celsius
   0:00:22.044: 16.8125 C
   0:00:24.047: 16.875 C
