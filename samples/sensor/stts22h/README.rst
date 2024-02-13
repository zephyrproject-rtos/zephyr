.. zephyr:code-sample:: stts22h
   :name: STTS22H
   :relevant-api: i2c

   A demo code for STTTS22H temerature sensor

Overview
********

The `stts22h` is a demo application to demonstrate the STTS22H
usage in Zephyr.


Requirements
************

Your board must:

#. Have an STTS22H sensor connected via a I2C bus to the MCU.
#. Have the `st,stts22h` compatible node in the devicetree.

Building and Running
********************

Build and flash as follows, changing ``reel_board`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/stts22h
   :board: reel_board
   :goals: build flash
   :compact:

After flashing, ambient temperature in are printed on the console.
If a runtime error occurs, the sample exits without printing to the console.


Adding board support
********************

To add support for your board, add something like this to your devicetree:

.. code-block:: DTS

   / {

	temp_sensor:stts22h@38 {
		compatible = "st,stts22h";
		reg = <0x38>;
		drdy-gpios = <&gpioe 1 GPIO_ACTIVE_HIGH>;
		status = "okay";
	};

	};
