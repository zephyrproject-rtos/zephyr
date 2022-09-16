.. _wsen-itds:

WSEN-ITDS: 3-axis acceleration sensor
#####################################

Overview
********
 This sample periodically measures acceleration in 3-axis and die temperature for
 5 sec in the interval of 300msec in polling mode. Then data ready trigger mode
 is enabled with the sample frequency of 400Hz and 3-axis data is fetched based
 on interrupt. The result is displayed on the console.

Requirements
************

 This sample uses the WSEN-ITDS sensor controlled using the I2C interface.
 Connect sensor IN1 for interrupt(data ready trigger) to Disco board D8 pin
 Connect sensor CS pin high to enable I2C communication, SAO to ground for 0x18
 I2C address selection.

References
**********

 - WSEN-ITDS: https://www.we-online.com/catalog/manual/2533020201601_WSEN-ITDS%202533020201601%20Manual_rev1.pdf

Building and Running
********************

 This project outputs sensor data to the console. It requires a WSEN-ITDS
 sensor, which is present on the disco_l475_iot1 board.

 .. zephyr-app-commands::
    :app: samples/sensor/wsen_itds/
    :goals: build flash


Sample Output
=============

 .. code-block:: console

    Testing the polling mode.
    Accl (m/s): X=2.311466, Y=-39.433716, Z=2.636890
    Temperature (Celsius): 28.000000

    <repeats 5sec every 300ms>

    Polling mode test finished.
    Testing the trigger mode.
    Testing data ready trigger.
    any drdy.
    Accl (m/s): X=8.039883, Y=-1.780260, Z=6.063412

    <repeats 5sec every 300ms>

    Data ready trigger test finished.
    Trigger mode test finished.
