.. zephyr:code-sample:: bluetooth_hap_ha
   :name: Hearing Access Profile (HAP) Hearing Aid (HA)
   :relevant-api: bt_has

   Advertise and await connection from a Hearing Aid Unicast Client or Remote Controller.

Overview
********

Application demonstrating the LE Audio Hearing Aid sample functionality.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth Low Energy 5.2 support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target
board (e.g. :zephyr:board:`nrf5340dk`):

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/hap_ha
   :board: <board>
   :goals: build flash
   :compact:

After flashing, the sample starts advertising as ``Hearing Aid sample`` and
awaits a connection from a Hearing Aid Unicast Client (HAUC) or Hearing Aid
Remote Controller (HARC).

See :zephyr:code-sample-category:`bluetooth` samples for details.
