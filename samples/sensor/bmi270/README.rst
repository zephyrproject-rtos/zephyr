.. _bmi270:

BMI270: 6 axis inertial measurement unit
########################################

Description
***********

This sample application configures the accelerometer and gyroscope to
measure data at 100Hz. The result is written to the console.

References
**********

 - BMI270: https://www.bosch-sensortec.com/products/motion-sensors/imus/bmi270.html

Wiring
*******

This sample uses the BMI270 sensor controlled using the I2C interface.
Connect Supply: **VDD**, **VDDIO**, **GND** and Interface: **SDA**, **SCL**.
The supply voltage can be in the 1.8V to 3.6V range.
Depending on the baseboard used, the **SDA** and **SCL** lines require Pull-Up
resistors.

Building and Running
********************

This project outputs sensor data to the console. It requires a BMI270
sensor. It should work with any platform featuring a I2C peripheral interface.
It does not work on QEMU.
In this example below the :ref:`nrf52840dk_nrf52840` board is used.


.. zephyr-app-commands::
   :zephyr-app: samples/sensor/bmi270
   :board: nrf52840dk_nrf52840
   :goals: build flash

Sample Output
=============

.. code-block:: console

   Device 0x200014cc name is BMI270
   AX: 0.268150; AY: 0.076614; AZ: 9.730035; GX: 0.001065; GY: -0.005326; GZ: -0.004261;
   AX: 0.229843; AY: 0.076614; AZ: 9.806650; GX: 0.000532; GY: -0.005592; GZ: -0.002929;
   AX: 0.229843; AY: 0.076614; AZ: 9.806650; GX: 0.000266; GY: -0.006125; GZ: -0.002663;
   AX: 0.306457; AY: 0.038307; AZ: 9.768342; GX: 0.001331; GY: -0.005326; GZ: -0.004793;

   <repeats endlessly>
