.. _grow_r502a:

GROW_R502A Fingerprint Sensor
#############################

Overview
********

This sample has the below functionalities:

#. Shows the number of fingerprints stored in the sensor.
#. When SENSOR_ATTR_RECORD_FREE_IDX is set then it search for free index in sensor library.
#. When SENSOR_ATTR_RECORD_ADD is set then it adds a new fingerprint to the sensor.
#. When SENSOR_ATTR_RECORD_FIND is set then it tries to find a match for the input fingerprint. On finding a match it returns ID and confidence.
#. When SENSOR_ATTR_RECORD_DEL is set then it deletes a fingerprint from the sensor.

Note: Fingerprint add & delete functionalities work only when SENSOR_TRIG_TOUCH is set.
Tricolored LED in the sensor hardware will, flash on the below conditions:

#. On successful addition or deletion of fingerprint it will flash in blue three times.
#. On finding a match for the input fingerprint it will flash in purple.
#. In all other cases it will flash in red.

Wiring
*******

This sample uses an external breakout for the sensor.  A devicetree
overlay must be provided to identify the UART bus and the GPIO
used to control the sensor. Sensor can be powered using mentioned optional GPIO.

References
**********

For more information about the GROW_R502A Fingerprint Sensor see
http://en.hzgrow.com/product/143.html

Building and Running
********************

After providing a devicetree overlay that specifies the sensor location,
build this sample app using:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/grow_r502a
   :board: nrf52840dk_nrf52840
   :goals: build flash

Sample Output
*************

.. code-block:: console

    *** Booting Zephyr OS build zephyr-v3.1.0-2640-g328bb73113d4  ***
    template count : 0
    Fingerprint Deleted at ID #3
    Fingerprint template free idx at ID #0
    Waiting for valid finger to enroll as ID #0
    Place your finger
    Fingerprint successfully stored at #0
    template count : 1
    Matched ID : 0
    confidence : 170
    template count : 1
    Matched ID : 0
    confidence : 136
    template count : 1
    Matched ID : 0
    confidence : 318
    <repeats endlessly>
