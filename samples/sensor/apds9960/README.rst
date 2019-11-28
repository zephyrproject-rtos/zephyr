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

This sample application uses an APDS9960 sensor connected to a board
(for example as shown in this
`Sparkfun APDS9960 tutorial`_).

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/apds9960
   :board: reel_board
   :goals: flash
   :compact:

Sample Output
=============

.. code-block:: console

        ambient light intensity without trigger is 387
        proxy without trigger is 115
        ambient light intensity without trigger is 386
        proxy without trigger is 112
        ambient light intensity without trigger is 386

.. _Sparkfun APDS9960 tutorial: https://www.sparkfun.com/products/12787
