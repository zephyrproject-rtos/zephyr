.. _light_polling:

Generic light level sensor polling sample
#########################################

Overview
********

This sample application periodically (2 Hz) measures the ambient light value in lux. The results are written to the console.

Building and Running
********************

This sample supports any luminosity sensor who's driver is included in the Zephyr project.
The sensor must be aliased as ``light0`` to be detected. For example:

.. code-block:: devicetree

  / {
	aliases {
			light0 = &ltr390;
		};
	};

After providing a devicetree overlay that specifies the sensor information,
and the sensor alias, build this sample app using:

.. zephyr-app-commands::
    :zephyr-app: samples/sensor/ltr390
    :board: nucleo_f411re
    :goals: build flash
    :compact:

Sample Output
=============

.. code-block:: console

    *** Booting Zephyr OS build v3.2.0-rc1-24-gb10c52635237  ***
	Polling light level data from ltr390@53.
	Light:  229.200000 lux
	Light:  231.200000 lux
	Light:  232.400000 lux
	Light:  230.400000 lux

<repeats endlessly>
