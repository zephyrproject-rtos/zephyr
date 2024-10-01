.. zephyr:code-sample:: max17048
   :name: MAX17048 Li-Ion battery fuel gauge

   Read battery percentage and power status using MAX17048 fuel gauge.

Overview
********

This sample shows how to use the Zephyr :ref:`fuel_gauge_api` API driver for the MAX17048 fuel gauge.

.. _MAX17048: https://www.maximintegrated.com/en/products/power/battery-management/MAX17048.html

The sample periodically reads battery percentage and power status

Building and Running
********************

The sample can be configured to support MAX17048 fuel gauge connected via either I2C. It only needs
an I2C pin configuration

Features
********
By using this fuel gauge you can get the following information:
  * Battery charge status as percentage
  * Total time until battery is fully charged or discharged
  * Battery voltage
  * Charging state: if charging or discharging


Notes
*****
The charging state and the time to full/empty are estimated and based on the last consumption average. That means that
if you plug/unplug a charger it will take some time until it is actually detected by the chip. Don't try to plug/unplug
to see in real time the charging status change because it will not work. If you really need to know exactly the moment
when the battery is being charged you will need other method.

Sample output
*************

```
*** Booting Zephyr OS build 16043f62a40a ***
Found device "max17048@36", getting fuel gauge data
Time to empty 1911
Time to full 0
Charge 72%
Voltage 3968
Time to empty 1911
Time to full 0
Charge 72%
Voltage 3968
```
