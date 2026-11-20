.. _bluetooth_profiles:

Profiles
########

Profiles that are implemented on top of the GATT services. A profile role is a GATT client or
composes several services, and its implementation lives in ``subsys/bluetooth/profiles/``.

HID over GATT Profile
*********************

The HID over GATT Profile (HOGP) v1.2 defines three roles: HID Device, Boot Host and Report Host.
The HID Device is the :ref:`HID Service <bluetooth_services>` composed by the application.

Part of the Report Host role is implemented, see :kconfig:option:`CONFIG_BT_HOGP_REPORT_HOST`: it
discovers the HID Service of a HID Device, hands its HID Information and Report Map to the
application, subscribes to its Input Reports, and reads and writes its Reports. The parts that are
not supported yet are listed in the API documentation below. The Boot Host role is not supported.

.. doxygengroup:: bt_hogp
