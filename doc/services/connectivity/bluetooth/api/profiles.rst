.. _bluetooth_profiles:

Profiles
########

Profiles that are implemented on top of the GATT services. A profile role is a
GATT client or composes several services, and its implementation lives in
``subsys/bluetooth/profiles/``.

HID over GATT Profile Host
**************************

The Report Host role of the HID over GATT Profile (HOGP) discovers the HID
Service of a HID Device, reads its Report Map and HID Information, subscribes to
its Input Reports, and reads and writes its Reports. The Device side is the
:ref:`HID Service <bluetooth_services>` composed by the application.

.. doxygengroup:: bt_hogp_host
