.. zephyr:code-sample:: ble_subrate_peripheral
   :name: LE Connection Subrating Peripheral
   :relevant-api: bt_conn bluetooth

   Accept a connection and apply the LE Connection Subrating factor set by the central.

Overview
********

Companion to the :zephyr:code-sample:`ble_subrate_central` sample. It advertises as
``subrate_demo`` and accepts a connection from the central, which drives the
subrate factor (Core Spec, Vol 6, Part B, section 5.1.19). The peripheral applies
each negotiated factor and skips connection events accordingly; it logs the
applied factor and blinks the LED, which is otherwise off so it does not perturb
a current measurement (the peripheral saves power too).

Requirements
************

* Two boards running the Zephyr LL_SW controller with
  ``CONFIG_BT_CTLR_SUBRATING`` (e.g. two nRF54L15 DKs): one running this sample,
  the other running :zephyr:code-sample:`ble_subrate_central`.

Building and Running
********************

.. code-block:: console

   west build -b nrf54l15dk/nrf54l15/cpuapp samples/bluetooth/subrate_peripheral
   west flash

The central drives the factor; view this side's VCOM serial port at 115200 baud
(any serial terminal) for each applied factor (``Subrate factor now N``) and the
``alive, factor N`` heartbeat confirming the link survives each tier.
