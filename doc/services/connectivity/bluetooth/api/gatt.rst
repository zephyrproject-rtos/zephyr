.. _bt_gatt:


Generic Attribute Profile (GATT)
################################

The GATT layer manages the service database providing APIs for service
registration and attribute declaration.

The GATT Client initiates commands and requests towards the GATT Server, and can
receive responses, indications and notifications sent by the server. It is
enabled through the configuration option:
:kconfig:option:`CONFIG_BT_GATT_CLIENT`

The GATT Server accepts incoming commands and requests from the GATT Client, and
sends responses, indications and notifications to the client.

Services can be registered using the :c:func:`bt_gatt_service_register` API
which takes the :c:struct:`bt_gatt_service` struct that provides the list of
attributes the service contains. The helper macro :c:macro:`BT_GATT_SERVICE()`
can be used to declare a service.

Attributes can be declared using the :c:struct:`bt_gatt_attr` struct or using
one of the helper macros:

    :c:macro:`BT_GATT_PRIMARY_SERVICE`
        Declares a Primary Service.

    :c:macro:`BT_GATT_SECONDARY_SERVICE`
        Declares a Secondary Service.

    :c:macro:`BT_GATT_INCLUDE_SERVICE`
        Declares a Include Service.

    :c:macro:`BT_GATT_CHARACTERISTIC`
        Declares a Characteristic.

    :c:macro:`BT_GATT_DESCRIPTOR`
        Declares a Descriptor.

    :c:macro:`BT_GATT_ATTRIBUTE`
        Declares an Attribute.

    :c:macro:`BT_GATT_CCC`
        Declares a Client Characteristic Configuration.

    :c:macro:`BT_GATT_CEP`
        Declares a Characteristic Extended Properties.

    :c:macro:`BT_GATT_CUD`
        Declares a Characteristic User Format.

Each attribute contain a ``uuid``, which describes their type, a ``read``
callback, a ``write`` callback and a set of permission. Both read and write
callbacks can be set to NULL if the attribute permission don't allow their
respective operations.

.. note::
   32-bit UUIDs are not supported in GATT. All 32-bit UUIDs shall be converted
   to 128-bit UUIDs when the UUID is contained in an ATT PDU.

.. note::
  Attribute ``read`` and ``write`` callbacks are called directly from RX Thread
  thus it is not recommended to block for long periods of time in them.

With :kconfig:option:`CONFIG_BT_GATT_ASYNC_ATTR` an attribute can instead
serve remote reads and writes asynchronously, through the ``read`` and
``write`` methods of :c:struct:`bt_gatt_attr_async` that
:c:macro:`BT_GATT_ATTRIBUTE_ASYNC` and :c:macro:`BT_GATT_CHARACTERISTIC_ASYNC`
declare. They receive a :c:struct:`bt_gatt_req` for the request being served and
may answer it after returning, with :c:func:`bt_gatt_rsp_read`,
:c:func:`bt_gatt_rsp_write` or :c:func:`bt_gatt_rsp_error`. This is for values
the application cannot produce right away, for example because they live behind
another link. A kept copy of the request holds its connection without a
reference, so take one with :c:func:`bt_conn_ref` while it is kept.

Read and write requests can be deferred, Read Multiple Request and Read
Multiple Variable Length Request included. Write Command and Signed Write
Command have no response and cannot be deferred, and neither can Execute Write,
so a write longer than one ATT PDU (ATT_MTU - 3, 20 octets with the default
MTU) has to complete before the callback returns.

The response has to be sent within the ATT transaction timeout (30 seconds), or
the remote drops the connection. A client that sends another request before that
one is answered gets the connection dropped too, which takes down the ACL link,
so every bearer on it, not only the one the request arrived on.

For a Read Multiple [Variable Length] Request the callback for the next value
may run inside :c:func:`bt_gatt_rsp_read`, so the asynchronous ``read`` method
of one attribute can run while another is answering. An error for any of the
handles fails the whole request.

Attributes the host reads itself, the declaration attributes of a service
among them, keep their synchronous ``read``: there is no connection behind
them, so no asynchronous method is called.

Attribute value changes can be notified using :c:func:`bt_gatt_notify` API,
alternatively there is :c:func:`bt_gatt_notify_cb` where it is possible to
pass a callback to be called when it is necessary to know the exact instant when
the data has been transmitted over the air. Indications are supported by
:c:func:`bt_gatt_indicate` API.

Discover procedures can be initiated with the use of
:c:func:`bt_gatt_discover` API which takes the
:c:struct:`bt_gatt_discover_params` struct which describes the type of
discovery. The parameters also serves as a filter when setting the ``uuid``
field only attributes which matches will be discovered, in contrast setting it
to NULL allows all attributes to be discovered.

.. note::
  Caching discovered attributes is not supported.

Read procedures are supported by :c:func:`bt_gatt_read` API which takes the
:c:struct:`bt_gatt_read_params` struct as parameters. In the parameters one or
more attributes can be set, though setting multiple handles requires the option:
:kconfig:option:`CONFIG_BT_GATT_READ_MULTIPLE`

Write procedures are supported by :c:func:`bt_gatt_write` API and takes
:c:struct:`bt_gatt_write_params` struct as parameters. In case the write
operation don't require a response :c:func:`bt_gatt_write_without_response`
or :c:func:`bt_gatt_write_without_response_cb` APIs can be used, with the
later working similarly to :c:func:`bt_gatt_notify_cb`.

Subscriptions to notification and indication can be initiated with use of
:c:func:`bt_gatt_subscribe` API which takes
:c:struct:`bt_gatt_subscribe_params` as parameters. Multiple subscriptions to
the same attribute are supported so there could be multiple ``notify`` callback
being triggered for the same attribute. Subscriptions can be removed with use of
:c:func:`bt_gatt_unsubscribe` API.

.. note::
  When subscriptions are removed ``notify`` callback is called with the data
  set to NULL.

API Reference
*************

.. doxygengroup:: bt_gatt

GATT Server
===========

.. doxygengroup:: bt_gatt_server

GATT Client
===========

.. doxygengroup:: bt_gatt_client
