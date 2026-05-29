.. zephyr:code-sample:: ble_peripheral_ht
   :name: Health Thermometer (Peripheral)
   :relevant-api: bt_bas bluetooth

   Expose a Health Thermometer (HT) GATT Service generating dummy temperature values.

Overview
********

Similar to the :zephyr:code-sample:`ble_peripheral` sample, except that this
application specifically exposes the HT (Health Thermometer) GATT Service.

On boards with a ``dht0`` Devicetree alias node, this sample uses this sensor to
return ambient temperature values. On Nordic nRF devices, it uses the built-in
TEMP peripheral to return die temperature values.  On other boards, it will
generate dummy temperature values.


Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_ht
   :board: <board>
   :goals: build flash
   :compact:

After flashing, use a Bluetooth scanner app (e.g. nRF Connect) to connect to the device
and enable indications on the Temperature Measurement characteristic to receive temperature
readings via the Health Thermometer (HT) service.
