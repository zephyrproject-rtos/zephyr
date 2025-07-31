.. _watchdog_sample:

Watchdog Timer Sample
#####################

Overview
********

This sample demonstrates the Windowed Watchdog Timer (WWDT) functionality on the
LPC54S018 board. The application shows how to configure and use the watchdog timer
to reset the system if the software fails to "feed" the watchdog within the
specified timeout period.

The sample demonstrates:

- Configuring the watchdog with a 5-second timeout
- Installing a watchdog callback (called before reset)
- Feeding the watchdog periodically
- Demonstrating a watchdog timeout and system reset

Requirements
************

- LPC54S018 board
- LED for visual feedback

Building and Running
********************

Build and flash this sample as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/watchdog
   :board: lpcxpresso54s018
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v3.7.0 ***
   [00:00:00.010,000] <inf> watchdog_sample: Watchdog Sample Application
   [00:00:00.010,000] <inf> watchdog_sample: Press button or wait 10 seconds to stop feeding watchdog
   [00:00:00.020,000] <inf> watchdog_sample: Watchdog configured with 5 second timeout
   [00:00:00.020,000] <inf> watchdog_sample: Watchdog will be fed every second for the first 10 seconds
   [00:00:01.020,000] <inf> watchdog_sample: Fed watchdog (1/10)
   [00:00:02.020,000] <inf> watchdog_sample: Fed watchdog (2/10)
   ...
   [00:00:10.020,000] <inf> watchdog_sample: Fed watchdog (10/10)
   [00:00:11.020,000] <wrn> watchdog_sample: Stopped feeding watchdog - system will reset in ~5 seconds
   [00:00:11.020,000] <wrn> watchdog_sample: Starting countdown...
   [00:00:12.020,000] <wrn> watchdog_sample: Reset in 4 seconds...
   [00:00:13.020,000] <wrn> watchdog_sample: Reset in 3 seconds...
   [00:00:14.020,000] <wrn> watchdog_sample: Reset in 2 seconds...
   [00:00:15.020,000] <wrn> watchdog_sample: Reset in 1 seconds...
   [00:00:15.500,000] <wrn> watchdog_sample: Watchdog callback triggered! System will reset soon...

   *** Booting Zephyr OS build v3.7.0 ***
   [00:00:00.010,000] <inf> watchdog_sample: Watchdog Sample Application

Expected Behavior
=================

1. The LED toggles every second to show the system is running
2. For the first 10 seconds, the watchdog is fed regularly
3. After 10 seconds, the application stops feeding the watchdog
4. Approximately 5 seconds later, the watchdog expires and resets the system
5. The system reboots and the cycle repeats

Watchdog Features
=================

The LPC54S018 WWDT (Windowed Watchdog Timer) provides:

- Configurable timeout period
- Optional windowed operation (not used in this sample)
- Callback notification before reset
- Debug halt support (watchdog pauses when debugger halts CPU)

References
**********

- :ref:`watchdog_api`
- `LPC54S018 Reference Manual`_ (Chapter on WWDT)

.. _LPC54S018 Reference Manual:
   https://www.nxp.com/docs/en/reference-manual/LPC54S01XJ2J4RM.pdf