.. _BMM350:

BMM350: Magnetometers
########################################

Description
***********

This sample application configures the Magnetometers to
measure data at 25Hz. The result is written to the console.

References
**********

 - BMM350: https://www.bosch-sensortec.com/products/motion-sensors/magnetometers/bmm350/

Wiring
*******

This sample uses the BMM350 sensor controlled using the I2C interface.
Connect Supply: **VDD**, **VDDIO**, **GND** and Interface: **SDA**, **SCL**.
The supply voltage can be in the 1.8V to 3.6V range.
Depending on the baseboard used, the **SDA** and **SCL** lines require Pull-Up
resistors.

Building and Running
********************

This project outputs sensor data to the console. It requires a BMM350
sensor. It should work with any platform featuring a I2C peripheral interface.
It does not work on QEMU.
In this example below the :ref:`nrf52840dk_nrf52840` board is used.


.. zephyr-app-commands::
   :zephyr-app: samples/sensor/bmm350
   :board: nrf52840dk_nrf52840
   :goals: build flash

Sample Output
=============

.. code-block:: console

   disable enable CONFIG_BMM350_TRIGGER_OWN_THREAD
         and CONFIG_BMM350_TRIGGER_GLOBAL_THREAD
   Polling TIME(ms) 6913 MX: -0.070000; MY: 0.120000; MZ: 0.400000;
   Polling TIME(ms) 6960 MX: -0.070000; MY: 0.120000; MZ: 0.400000;
   Polling TIME(ms) 7008 MX: -0.080000; MY: 0.120000; MZ: 0.400000;
   Polling TIME(ms) 7055 MX: -0.080000; MY: 0.120000; MZ: 0.400000;
   Polling TIME(ms) 7103 MX: -0.080000; MY: 0.120000; MZ: 0.400000;
   Polling TIME(ms) 7150 MX: -0.080000; MY: 0.120000; MZ: 0.400000;
   Polling TIME(ms) 7198 MX: -0.080000; MY: 0.120000; MZ: 0.400000;
   Polling TIME(ms) 7245 MX: -0.080000; MY: 0.120000; MZ: 0.400000;
   Polling TIME(ms) 7293 MX: -0.080000; MY: 0.110000; MZ: 0.400000;
   Polling TIME(ms) 7340 MX: -0.080000; MY: 0.120000; MZ: 0.400000;

   enable CONFIG_BMM350_TRIGGER_OWN_THREAD
         or CONFIG_BMM350_TRIGGER_GLOBAL_THREAD
   Data_ready TIME(ms) 4165 MX: -0.080000; MY: 0.120000; MZ: 0.400000;
   Data_ready TIME(ms) 4205 MX: -0.070000; MY: 0.120000; MZ: 0.400000;
   Data_ready TIME(ms) 4245 MX: -0.070000; MY: 0.120000; MZ: 0.400000;
   Data_ready TIME(ms) 4285 MX: -0.070000; MY: 0.120000; MZ: 0.400000;
   Data_ready TIME(ms) 4325 MX: -0.070000; MY: 0.120000; MZ: 0.400000;
   Data_ready TIME(ms) 4364 MX: -0.070000; MY: 0.120000; MZ: 0.400000;
   Data_ready TIME(ms) 4404 MX: -0.070000; MY: 0.120000; MZ: 0.400000;
   Data_ready TIME(ms) 4444 MX: -0.070000; MY: 0.120000; MZ: 0.400000;
   Data_ready TIME(ms) 4484 MX: -0.080000; MY: 0.120000; MZ: 0.400000;
   Data_ready TIME(ms) 4524 MX: -0.070000; MY: 0.120000; MZ: 0.400000;
