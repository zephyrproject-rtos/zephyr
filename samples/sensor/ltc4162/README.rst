.. _ltc4162:

LTC4162: Step-Down battery charger
##################################

Description
***********

This sample application retrieves all the charger parameters,
- Charger Status
- Charger Type
- Charger Health
- Input Voltage in Volt
- Input Current in Amps
- Constant Current in Amps
- Constant Current Max in Amps
- Constant Voltage in Volt
- Constant Voltage Max in Volt
- Input Current limit in Amps
- Charger die temperature in Celcius

from LTC4162 sensor and print this information to the UART console.

Requirements
************

This sample uses the LTC4162 charger controllered by using the I2C interface.

References
**********

 - LTC4162S - https://www.analog.com/media/en/technical-documentation/data-sheets/LTC4162-S.pdf

Building and Running
********************

 It requires a LTC4162S charger controller. It should work with any platforms featuring a
 I2C peripheral interface. It does not work on QEMU. In this example below the
 :ref:`frdm_k64f` board is used.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/ltc4162
   :board: frdm_k64f
   :goals: build flash

Sample Output
=============

 .. code-block:: console

    Charger_status: charging
    Charger_type: fast
    Charger_health: good
    Input Voltage in Volt: 14.227000
    Input Current in Amps: 0.336000
    Constant Charge Current: 0.40000000
    Constant current max in Amps: 0.40000000
    Constant voltage in Volt: 14.457000
    Constant voltage max in Volt: 13.199000
    Input current limit in Amps: 3.050000
    Charger die temperature in Celsius: 39.337000

    <repeats endlessly every 2 seconds>
