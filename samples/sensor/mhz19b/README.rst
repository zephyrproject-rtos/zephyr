.. _mhz19b-sample:

MH-Z19B: CO2 Sensor Sample
##########################

Description
***********

This sample application demonstrate the configurations of the MH-Z19B CO2
sensor, then periodically reads CO2 data from the sensor. The driver
currently only support uart interrupt APIs.

Requirements
************

The baudrate of the UART must be configured to 9600, and must support
interrupt driven API.

Building and Running
********************

To build the application, a board with UART interface
has to be chosen, or a custom devicetree overlay has to be provided.
Here Nucleo G0B1RE board is used.
Then, connect the RX and TX from your Zephyr target board to the
MH-Z19B CO2 sensor and power the sensor according to the datasheet.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/mhz19b
   :board: nucleo_g0b1re
   :goals: build
   :compact:

Sample Output
=============

The application will perform runtime configuration of the sensor, then
read back the sensor configuration data. After that it will read the CO2
data every 2 seconds.

.. code-block:: console

   Winsen MH-Z19B CO2 sensor application
   Configuring sensor - OK
   Reading configurations from sensor:
   Sensor range is set to 5000ppm
   Sensor ABC is enabled
   sensor: co2 reading: 758
   sensor: co2 reading: 759
   sensor: co2 reading: 762

   <repeats endlessly every 2 seconds>
