.. _bt_gatt:


Generic Attribute Profile (GATT)
################################

GATT layer manages the service database providing APIs for service registration
and attribute declaration.

Services can be registered using :cpp:func:`bt_gatt_service_register` API
which takes the :cpp:class:`bt_gatt_service` struct that provides the list of
attributes the service contains. The helper macro :cpp:func:`BT_GATT_SERVICE`
can be used to declare a service.

Attributes can be declared using the :cpp:class:`bt_gatt_attr` struct or using
one of the helper macros:

    :cpp:func:`BT_GATT_PRIMARY_SERVICE`
        Declares a Primary Service.

    :cpp:func:`BT_GATT_SECONDARY_SERVICE`
        Declares a Secondary Service.

    :cpp:func:`BT_GATT_INCLUDE_SERVICE`
        Declares a Include Service.

    :cpp:func:`BT_GATT_CHARACTERISTIC`
        Declares a Characteristic.

    :cpp:func:`BT_GATT_DESCRIPTOR`
        Declares a Descriptor.

    :cpp:func:`BT_GATT_ATTRIBUTE`
        Declares an Attribute.

    :cpp:func:`BT_GATT_CCC`
        Declares a Client Characteristic Configuration.

    :cpp:func:`BT_GATT_CEP`
        Declares a Characteristic Extended Properties.

    :cpp:func:`BT_GATT_CUD`
        Declares a Characteristic User Format.

Each attribute contain a ``uuid``, which describes their type, a ``read``
callback, a ``write`` callback and a set of permission. Both read and write
callbacks can be set to NULL if the attribute permission don't allow their
respective operations.

.. note::
  Attribute ``read`` and ``write`` callbacks are called directly from RX Thread
  thus it is not recommended to block for long periods of time in them.

Attribute value changes can be notified using :cpp:func:`bt_gatt_notify` API,
alternatively there is :cpp:func:`bt_gatt_notify_cb` where is is possible to
pass a callback to be called when it is necessary to know the exact instant when
the data has been transmitted over the air. Indications are supported by
:cpp:func:`bt_gatt_indicate` API.

Client procedures can be enabled with the configuration option:
:option:`CONFIG_BT_GATT_CLIENT`

Discover procedures can be initiated with the use of
:cpp:func:`bt_gatt_discover` API which takes the
:cpp:class:`bt_gatt_discover_params` struct which describes the type of
discovery. The parameters also serves as a filter when setting the ``uuid``
field only attributes which matches will be discovered, in contrast setting it
to NULL allows all attributes to be discovered.

.. note::
  Caching discovered attributes is not supported.

Read procedures are supported by :cpp:func:`bt_gatt_read` API which takes the
:cpp:class:`bt_gatt_read_params` struct as parameters. In the parameters one or
more attributes can be set, though setting multiple handles requires the option:
:option:`CONFIG_BT_GATT_READ_MULTIPLE`

Write procedures are supported by :cpp:func:`bt_gatt_write` API and takes
:cpp:class:`bt_gatt_write_params` struct as parameters. In case the write
operation don't require a response :cpp:func:`bt_gatt_write_without_response`
or :cpp:func:`bt_gatt_write_without_response_cb` APIs can be used, with the
later working similarly to ``bt_gatt_notify_cb``.

Subscriptions to notification and indication can be initiated with use of
:cpp:func`bt_gatt_subscribe` API which takes
:cpp:class:`bt_gatt_subscribe_params` as parameters. Multiple subscriptions to
the same attribute are supported so there could be multiple ``notify`` callback
being triggered for the same attribute. Subscriptions can be removed with use of
:cpp:func:`bt_gatt_unsubscribe()` API.

.. note::
  When subscriptions are removed ``notify`` callback is called with the data
  set to NULL.

API Reference
*************

.. doxygengroup:: bt_gatt
   :project: Zephyr

GATT Server
===========

.. doxygengroup:: bt_gatt_server
   :project: Zephyr

GATT Client
===========

.. doxygengroup:: bt_gatt_client
   :project: Zephyr

