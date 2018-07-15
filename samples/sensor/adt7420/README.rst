.. _adt7420:

ADT7420: High accuracy digital I2C temperature sensor
#####################################################

Description
***********

This sample application periodically (1Hz) measures the ambient temperature
in degrees Celsius. The result is written to the console.
Optionally, it also shows how to use the upper and lower threshold triggers.

References
**********

 - ADT7420: http://www.analog.com/adt7420

Wiring
*******

This sample uses the ADT7420 sensor controlled using the I2C interface.
Connect Supply: **VDD**, **GND** and Interface: **SDA**, **SCL**
and optionally connect the **INT** to a interrupt capable GPIO.
The supply voltage can be in the 2.7V to 5.5V range.
Depending on the baseboard used, the **SDA** and **SCL** lines require Pull-Up
resistors.

Building and Running
********************

This project outputs sensor data to the console. It requires an ADT7420
sensor. It should work with any platform featuring a I2C peripheral interface.
It does not work on QEMU.
In this example below the :ref:`nrf52_pca10040` board is used.


.. zephyr-app-commands::
   :zephyr-app: samples/sensors/adt7420
   :board: nrf52_pca10040
   :goals: build flash

Sample Output
=============

.. code-block:: console

   device is 0x20002b74, name is ADT7420
   temperature 24.984375 C
   temperature 24.968750 C
   temperature 24.968750 C
   temperature 24.968750 C
   temperature 24.953125 C

<repeats endlessly>

