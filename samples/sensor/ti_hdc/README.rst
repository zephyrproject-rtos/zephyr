.. _ti_hdc_sample:

TI_HDC Sample
##############

Description
***********

This sample application periodically takes Temperature and Humidity
using the ti_hdc sensor driver. The result is written to the console.

Requirements
************

This sample needs a compatible sensor like HDC1010 or HDC1080
connected to the target board's I2C connector.

Example Breakout Boards:

* Pmod HYGRO: Humidity and Temperature Sensor Breakout board


Wiring
******

This sample is tested with the STM32L496ZG nucleo and the Pmod HYGRO
Temp/RH breakout board.

The sensor operates at 3.3V and uses I2C to communicate with the board.

External Wires:

* Breakout **GND** pin <--> Nucleo **GND** pin
* Breakout **VCC** pin <--> Nucleo **3V3** pin
* Breakout **SDA** pin <--> Nucleo **CN7-D14** pin
* Breakout **SCL** pin <--> Nucleo **CN7-D15** pin

Building and Running
********************

This sample builds one application for the HDC1080 sensor.
Build/Flash Steps:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ti_hdc/
   :board: nucleo_l496zg
   :goals: build flash
   :compact:

Sample Output
*************
.. code-block:: console

    Running on arm!
    Dev 0x20001160 name HDC1080 is ready!
    Fetching...
    Raw Temp = 25144, Temp = 23.305053 C, Raw RH = 32292, RH = 49.273681 %
    Fetching...
    Raw Temp = 25148, Temp = 23.315124 C, Raw RH = 32424, RH = 49.475097 %
    ...

Build Testing
**************

.. code-block:: bash

    $ZEPHYR_BASE/scripts/sanitycheck -T $ZEPHYR_BASE/samples/sensor/ti_hdc/ -p nucleo_l496zg --device-testing --device-serial /dev/ttyACM0 -t build

Target Testing
**************

.. code-block:: bash

    $ZEPHYR_BASE/scripts/sanitycheck -T $ZEPHYR_BASE/samples/sensor/ti_hdc/ -p nucleo_l496zg --device-testing --device-serial /dev/ttyACM0 -t target


References
**********

.. _Nucleo STM32L496ZG board: https://www.st.com/en/evaluation-tools/nucleo-l496zg.html
.. _HDC1080 Breakout board: https://store.digilentinc.com/pmod-hygro-digital-humidity-and-temperature-sensor/
