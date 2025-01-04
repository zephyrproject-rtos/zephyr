.. _ath20_bmp280_board_sample:

ATH20+BMP280 Board
##################

Overview
********

This sample shows how to use the Zephyr :ref:`sensor_api` API driver and configure the Devicetree for the *ATH20+BMP280 Board* that has both the
`Bosch BMP280 Digital Pressure Sensor`_ and `Aosong AHT20`_ temperature and humidity sensor.

.. _Bosch BMP280 Digital Pressure Sensor:
   https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf

.. _Aosong AHT20:
   http://www.aosong.com/en/products-32.html

Building and Running
********************

The devicetree overlay :zephyr_file:`samples/sensor/aht20_bmp280_board/app.overlay` should work on any board with a properly configured ``i2c0`` bus.
The devicetree must have an enabled node with ``compatible = "bosch,bmp280";``.

Make sure this node has ``status = "okay";``, then build and run with:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/aht20_bmp280_board
   :goals: build flash
   :board: esp32


Sample Output
*************

The sample prints output to the serial console. Both of the sensor's device driver messages
are also logged. Refer to your board's documentation for information on
connecting to its serial console.

Here is example output:

.. code-block:: none

   [00:00:00.171,000] <dbg> BMP280: bmp280_init: Chip id: 0x58
   [00:00:00.176,000] <dbg> BMP280: bmp280_get_compensation_params: t1: 28219
   [00:00:00.176,000] <dbg> BMP280: bmp280_get_compensation_params: t2: 27080
   [00:00:00.176,000] <dbg> BMP280: bmp280_get_compensation_params: t3: -1000
   [00:00:00.176,000] <dbg> BMP280: bmp280_get_compensation_params: p1: 36710
   [00:00:00.176,000] <dbg> BMP280: bmp280_get_compensation_params: p2: -10531
   [00:00:00.176,000] <dbg> BMP280: bmp280_get_compensation_params: p3: 3024
   [00:00:00.176,000] <dbg> BMP280: bmp280_get_compensation_params: p4: 3818
   [00:00:00.176,000] <dbg> BMP280: bmp280_get_compensation_params: p5: 149
   [00:00:00.176,000] <dbg> BMP280: bmp280_get_compensation_params: p6: -7
   [00:00:00.176,000] <dbg> BMP280: bmp280_get_compensation_params: p7: 15500
   [00:00:00.176,000] <dbg> BMP280: bmp280_get_compensation_params: p8: -14600
   [00:00:00.176,000] <dbg> BMP280: bmp280_get_compensation_params: p9: 6000
   *** Booting Zephyr OS build v4.0.0-28-g2d852bba87e8 ***
   [00:00:00.177,000] <inf> app: ATH20+BMP280 board sample started
   [00:00:00.216,000] <dbg> BMP280: bmp280_sample_fetch: STATUS: 0x0
   [00:00:00.217,000] <inf> app: bmp280@77: Pressure (kPa): 101.1292000
   [00:00:00.217,000] <inf> app: bmp280@77: Temperature (C): 27.430000
   [00:00:02.256,000] <dbg> BMP280: bmp280_sample_fetch: STATUS: 0x0
   [00:00:02.257,000] <inf> app: bmp280@77: Pressure (kPa): 101.1284280
   [00:00:02.257,000] <inf> app: bmp280@77: Temperature (C): 27.440000
