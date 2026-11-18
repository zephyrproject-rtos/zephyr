.. zephyr:code-sample:: ble_subrate_central
   :name: LE Connection Subrating Central
   :relevant-api: bt_conn bluetooth

   Connect to a peripheral and cycle the LE Connection Subrating factor.

Overview
********

Companion to the :zephyr:code-sample:`ble_subrate_peripheral` sample. It scans for and connects
to that peripheral (advertised name ``subrate_demo``) at a fixed connection
interval, then **automatically cycles** the subrate factor through active / idle
/ dormant tiers (``1 / 8 / 33``) on a 10-second dwell timer using
central-initiated Connection Subrate Updates (Core Spec, Vol 6, Part B, section
5.1.19).

It runs autonomously (no button) and logs over the on-board **VCOM UART** (USB
serial), so you can flash, reset, and capture the result, then measure the
central-side current at each factor tier. Factor 1 is the no-subrating baseline;
higher factors let both sides skip connection events. A ``alive, factor N``
heartbeat confirms the link survives each tier (factor 33 sits just under the
supervision-timeout bound). The LED blinks *tier-index* times (1/2/3) when a
factor is applied, then stays off so it does not perturb a current measurement.

Requirements
************

* Two boards running the Zephyr LL_SW controller with
  ``CONFIG_BT_CTLR_SUBRATING`` (e.g. two nRF54L15 DKs): one running this sample,
  the other running :zephyr:code-sample:`ble_subrate_peripheral`.

Building and Running
********************

.. code-block:: console

   west build -b nrf54l15dk/nrf54l15/cpuapp samples/bluetooth/subrate_central
   west flash

View the log on the DK's VCOM serial port at 115200 baud with any serial
terminal (``tio``, ``pyserial``, nRF Connect Serial Terminal). On the nRF54L15
DK the console is the second VCOM (vcom1), and VCOM must be enabled on the
on-board J-Link. After ``Connected``, the factor steps ``1 -> 8 -> 33``
automatically every 10 seconds; the ``Subrate factor now N`` line and the LED
blink count (1/2/3) confirm each tier, and ``alive, factor N`` confirms the link
survives it. Measure the board current at each tier to quantify the saving
(factor 1 is the baseline).
