.. zephyr:code-sample:: ship-mode
   :name: Ship Mode Sample
   :relevant-api: regulator_parent_interface

   Use the Regulator driver to enter Ship Mode.

Overview
********

This sample demonstrates how to enter Ship Mode using a regulator parent.

Some devices do NOT establish debug communication after entering Ship Mode.
In such case, device may become hard or impossible to recover if it enters
ship mode immediately after boot/reset. A boot delay of two seconds is added
to prevent device deadlocking. Thus, after a reset, there are two seconds
to start debug session (or flash other HEX through debug interface).


Building and Running
********************

The application will work correctly if the target supports Ship Mode.
Building for another board requires providing an overlay that defines
the ``regulator-parent`` alias pointing to the regulator parent node.
Build and flash as follows, replacing ``nrf54lv10dk/nrf54lv10a/cpuapp`` with your board:

.. zephyr-app-commands::
   :zephyr-app: samples/regulator/ship_mode
   :board: nrf54lv10dk/nrf54lv10a/cpuapp
   :goals: build flash
   :compact:


Sample Output
=============

.. code-block:: console

  ***** delaying boot 2000ms (per build configuration) *****
  *** Booting Zephyr OS build v4.5.0-2192-gf6468d80e69d (delayed boot 2000ms) ***
  [00:00:01.997,323] <inf> ship_mode: regulator@120000 entering ship mode
