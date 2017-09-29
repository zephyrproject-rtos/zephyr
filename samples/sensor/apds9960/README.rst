.. _apds9960:

APDS9960 RGB, Ambient Light, Gesture Sensor
###########################################

Overview
********

This sample application demonstrates how to use the APDS9960 sensor to get
ambient light, RGB, and proximity (or gesture) data.  This sample checks the
sensor in polling mode (without an interrupt trigger).

Building and Running
********************

This sample application uses an APDS9960 sensor connected to an Arduino 101 board
(for example as shown in this
`Sparkfun APDS9960 tutorial`_).
There are two processor cores (x86 and ARC) on the Arduino 101.  You'll need to flash
both this sample's code on the ARC core (using the ``arduino101_sss`` board target),
and stub code on the x86 core (using the ``arduino_101`` board target), as shown below:

.. code-block:: console

        $ cd $ZEPHYR_BASE/samples/sensors/apds9960
        $ make BOARD=arduino_101_sss flash

        $ cd $ZEPHYR_BASE/tests/booting/stub
        $ make BOARD=arduino_101 flash

Sample Output
=============

.. code-block:: console

        ambient light intensity without trigger is 387
        proxy without trigger is 115
        ambient light intensity without trigger is 386
        proxy without trigger is 112
        ambient light intensity without trigger is 386

.. _Sparkfun APDS9960 tutorial: https://www.sparkfun.com/products/12787
