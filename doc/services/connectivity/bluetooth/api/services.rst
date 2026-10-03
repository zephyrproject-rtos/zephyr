.. _bluetooth_services:

Bluetooth standard services
###########################

Battery Service
***************

.. doxygengroup:: bt_bas

Current Time Service
********************

.. doxygengroup:: bt_cts

Elapsed Time Service
********************

.. doxygengroup:: bt_ets

Heart Rate Service
******************

.. doxygengroup:: bt_hrs

HID Service
***********

The HID Service carries the Reports and the Report Map of a HID device. The HID Device role of the
HID over GATT Profile (HOGP) composes it with the Battery Service and with the Device Information
Service, including the PnP ID characteristic, over a bondable and encrypted link. Composing the
services and requesting security are the responsibility of the application; the service itself only
requires that the link is encrypted.

.. doxygengroup:: bt_hids

Immediate Alert Service
***********************

.. doxygengroup:: bt_ias

Object Transfer Service
***********************

.. doxygengroup:: bt_ots
