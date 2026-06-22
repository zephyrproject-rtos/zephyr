.. zephyr:code-sample:: versal_sysmon
   :name: versal sysmon temperature sample

   Read and print die temperature from the AMD Versal SysMon.

Overview
********

This sample demonstrates how to use the Zephyr sensor driver API
to read the die temperature from the AMD Versal System Monitor (SYSMON).

The application periodically fetches sensor data and prints the
die temperature to the console in milli-degrees Celsius (mC).

Building and Running
********************

This sample requires a Versal platform with SysMon enabled node present in
device tree and marked as ``okay``.

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/versal_sysmon
   :host-os: unix
   :board: versal_apu
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   SYSMON sample started
   Sysmon Die temperature: 45000 °mC
   Sysmon Die temperature: 45010 °mC
   Sysmon Die temperature: 44980 °mC
