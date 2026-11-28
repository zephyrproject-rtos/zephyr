.. zephyr:code-sample:: presence_sensor
   :name: Presence sensor
   :relevant-api: sensor_interface

   Read a presence/motion sensor with the Sensor Read & Decode API.

Overview
********

The sample reads the sensor node with the ``presence-sensor`` devicetree alias.
If the node has an ``int-gpios`` property the read is started by the
``SENSOR_TRIG_DATA_READY`` trigger, otherwise the sensor is polled once per second.
Every channel the driver can decode is printed; channels it does not support are skipped.

Building
********

Define the alias in a board overlay, for example for the STHS34PF80 on I2C:

.. code-block:: devicetree

   #include <zephyr/dt-bindings/sensor/sths34pf80.h>

   / {
           aliases {
                   presence-sensor = &sths34pf80;
           };
   };

   &arduino_i2c {
           sths34pf80: sths34pf80@5a {
                   compatible = "st,sths34pf80";
                   reg = <0x5a>;
                   odr = <STHS34PF80_DT_ODR_1HZ>;
                   int-gpios = <&arduino_header 8 GPIO_ACTIVE_HIGH>;
           };
   };

A ready-made overlay for the nRF9151 DK is in the :file:`boards` folder of the
sample. It polls the sensor; uncomment ``int-gpios`` in it to read on data-ready.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/presence_sensor
   :board: <your_board>
   :goals: build flash
