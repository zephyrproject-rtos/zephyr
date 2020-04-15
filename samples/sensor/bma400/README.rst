.. _bma400:

BMA400: Ultra low-power accelerometer
#####################################################

Description
***********

This sample application configures the accelerometer to measure data at 100Hz,
with a range of 8G with the highest oversampling. The result is written to
the console.

References
**********

 - BMA400: https://www.bosch-sensortec.com/products/motion-sensors/accelerometers/bma400.html

Wiring
*******

This sample uses the BMA400 sensor controlled using the I2C interface.
Connect Supply: **VDD**, **VDDIO**, **GND** and Interface: **SDA**, **SCL**.
The supply voltage can be in the 1.8V to 3.6V range.
Depending on the baseboard used, the **SDA** and **SCL** lines require Pull-Up
resistors.

Building and Running
********************

This project outputs sensor data to the console. It requires a BMA400
sensor. It should work with any platform featuring a I2C peripheral interface.
It does not work on QEMU.
In this example below the :ref:`nrf52840dk_nrf52840` board is used.


.. zephyr-app-commands::
   :zephyr-app: samples/sensor/bma400
   :board: nrf52840dk_nrf52840
   :goals: build flash

Sample Output
=============

.. code-block:: console

   Device 0x200014dc name is BMA400
   X: 0.268150; Y: 0.076614; Z: 9.730035;
   X: 0.229843; Y: 0.076614; Z: 9.806650;
   X: 0.229843; Y: 0.076614; Z: 9.806650;
   X: 0.306457; Y: 0.038307; Z: 9.768342;

   <repeats endlessly>
