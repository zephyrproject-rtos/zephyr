.. zephyr:code-sample:: ble_peripheral_gatt_async
   :name: Peripheral GATT Async
   :relevant-api: bt_gatt bluetooth

   Answer a GATT read or write after the attribute callback has returned.

Overview
********

This sample advertises one vendor service with a single one-octet
characteristic, declared with :c:macro:`BT_GATT_CHARACTERISTIC_ASYNC` so that
it gets the asynchronous callbacks enabled by
:kconfig:option:`CONFIG_BT_GATT_ASYNC_ATTR`. A request that can be deferred is
not answered by the callback: the callback copies the :c:struct:`bt_gatt_req`
it is handed, schedules a work item and returns 0. The work item runs 100 ms
later and answers the request with :c:func:`bt_gatt_read_rsp` or
:c:func:`bt_gatt_write_rsp`. An application needs this when the value lives
somewhere the callback cannot reach, for example behind another link. A read
at a non-zero offset is refused by the callback itself, with
``BT_ATT_ERR_ATTRIBUTE_NOT_LONG``: the characteristic holds a single octet.

A Write Command or Signed Write Command has no response, so it cannot be
deferred: ``BT_GATT_WRITE_FLAG_CMD`` is set in the flags and the callback does
the write before it returns.

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support

Building and Running
********************

Build and flash the sample as follows, replacing ``<board>`` with your target board:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/peripheral_gatt_async
   :board: <board>
   :goals: build flash
   :compact:

Connect from a Bluetooth LE host and read or write the characteristic, for
example with BlueZ:

.. code-block:: console

   $ bluetoothctl
   [bluetooth]# connect <address of the board>
   [bluetooth]# menu gatt
   [bluetooth]# select-attribute 3a1c0b01-7f2d-4e51-9c6a-1d8b4f0e2a73
   [bluetooth]# read
   [bluetooth]# write 0x2a

The board prints ``Read deferred`` or ``Write of 0x2a deferred`` when the
callback returns, and ``Answered from the work queue`` when the response is
sent.

See :zephyr:code-sample-category:`bluetooth` samples for details.
