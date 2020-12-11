.. _bq274xx-sample:

BQ274XX Sensor Sample
#####################

Overview
********

This sample application retrieves all the fuel gauge parameters,
- Voltage, in volts
- Average current, in amps
- Standby current, in amps
- Maximum load current, in amps
- Gauge temperature
- State of charge measurement in percentage
- Full Charge Capacity in mAh
- Remaining Charge Capacity in mAh
- Nominal Available Capacity in mAh
- Full Available Capacity in mAh
- Average power in mW
- State of health measurement in percentage

from BQ274XX sensor and prints this information to the UART console.

Requirements
************

- nrf9160_innblue22 board with BQ274XX sensor `BQ274XX Sensor`_

Building and Running
********************

Build this sample using the following commands:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/bq274xx
   :board: nrf9160_innblue22
   :goals: build flash

References
**********

.. target-notes::

.. _BQ274XX Sensor: http://www.ti.com/lit/ds/symlink/bq27421-g1.pdf
