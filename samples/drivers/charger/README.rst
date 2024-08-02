.. zephyr:code-sample:: charger
   :name: Charger
   :relevant-api: charger_interface

   Charge a battery using the charger driver API.

Overview
********

This sample demonstrates how to use the :ref:`charger_api` API.

The sample application performs a simple charging task loop.

- The application will first poll for external power provided to the charger device.
- If power is provided to the charger, then the sample application will attempt to enable the charge
  cycle.
- After the charge cycle is initiated, the sample application will check the status property of the
  charger device and report any relevant information to the log.
- Once the charger device reports that the charge cycle has completed, the application returns.

Note that this sample terminates once the charge cycle completes and does not attempt to "top-off"
the battery pack. Additionally, the sample intentionally does not respond to the reported charger
health state and the implications the environment may have on the charge cycle execution. The
responsibility of responding to these events falls on the user or the charger device implementation.
