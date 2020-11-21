.. _si7021:

SI7021 Relative Humidity and Temperature Sensor
###############################################

Overview
********

The Si7021 is a combined relative humidity and temperature sensor which features an I2C interface.
This sample application specifically targets the Thunderboard 2 demonstration board. The Mighty Gecko
EFR32MG microcontroller on that board offers location rerouting during runtime, i.e. the same peripheral
can be connected to different pins. Currently, the i2c driver does not support this in a generic manner,
therefore a devicetree overlay is needed. This means, that this sensor can currently not be used in
combination with any other sensor that uses the same i2c peripheral, but on a different pin location
(e.g. the :ref:`ccs811` sensor driver).

Building and Running
********************

.. zephyr-app-commands::
    :zephyr-app: samples/sensor/si7021
    :board: efr32mg_sltb004
    :goals: build
    :compact:

To flash, move the created zephyr.bin to the connected USB device (refer to :ref:`efr32mg_sltb004a`).

Connect the EFR32MG-SLTB004A to your host computer using the USB port and copy
the generated zephyr.bin in the SLTB004A drive.

Sample Output
*************

Open a serial terminal with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

The output should look like this:

.. code-block:: console

    Found device "SI7021"
    Temp: 21.210000
    RH: 38.040000
    Temp: 21.230000
    RH: 38.080000
    Temp: 21.260000
    ...
