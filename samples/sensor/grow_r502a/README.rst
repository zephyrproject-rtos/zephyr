.. _grow_r502a:

GROW_R502A Fingerprint Sensor
#############################

Overview
********

This sample has the below functionalities:

#. Sensor LED is controlled using led APIs from zephyr subsystem.
#. Shows the number of fingerprints stored in the sensor.
#. Shows the sensor device's configurations like baud rate, library size, address and data packet size in UART frame.
#. When SENSOR_ATTR_RECORD_FREE_IDX is set then it search for free index in sensor library.
#. When SENSOR_ATTR_RECORD_ADD is set then it adds a new fingerprint to the sensor.
#. When SENSOR_ATTR_RECORD_FIND is set then it tries to find a match for the input fingerprint. On finding a match it returns ID and confidence.
#. When SENSOR_ATTR_RECORD_DEL is set then it deletes a fingerprint from the sensor.

Note: Fingerprint add functionality work only when SENSOR_TRIG_TOUCH is set.

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
   :board: esp32_devkitc_wroom/esp32/procpu
   :goals: build flash

Sample Output
*************

.. code-block:: console

    *** Booting Zephyr OS build v3.6.0-3147-g8ae1a2e2718e ***
    template count : 4
    baud 57600
    addr 0xffffffff
    lib_size 200
    data_pkt_size 128
    Fingerprint Deleted at ID #3
    Fingerprint template free idx at ID #3
    Waiting for valid finger to enroll as ID #3
    Place your finger
    Fingerprint successfully stored at #3
    template count : 4
    Matched ID : 2
    confidence : 110
    <repeats endlessly>
