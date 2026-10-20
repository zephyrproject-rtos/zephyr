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

With :kconfig:option:`CONFIG_BT_GATT_DEFERRED_RSP` an attribute whose value the
application cannot produce right away, for example because it lives behind
another link, can be declared with :c:macro:`BT_GATT_CHARACTERISTIC_DEFERRED` or
:c:macro:`BT_GATT_ATTRIBUTE_DEFERRED`. Its :c:member:`bt_gatt_deferred_attr.read` and
:c:member:`bt_gatt_deferred_attr.write` methods receive an id for the request
and may return before the request has a response. The application keeps the
id and responds to the request later with :c:func:`bt_gatt_deferred_read_rsp`,
:c:func:`bt_gatt_deferred_write_rsp` or :c:func:`bt_gatt_deferred_error_rsp`, from any thread.
Errors are given as :c:macro:`BT_GATT_ERR` values, the same as
:c:member:`bt_gatt_attr.read` and :c:member:`bt_gatt_attr.write` return them.

An attribute uses either :c:member:`bt_gatt_attr.read` and
:c:member:`bt_gatt_attr.write` or the deferred methods. One that uses the
deferred methods has no :c:member:`bt_gatt_attr.read`, so it cannot be read
locally, except a Client Characteristic Configuration descriptor: the host
reads it locally to check subscriptions, so it keeps its
:c:member:`bt_gatt_attr.read`.

These requests can be responded to later:

* Read
* Read Blob
* Read By Type
* Read Multiple
* Read Multiple Variable Length
* Write
* Prepare Write
* Execute Write

A Read Multiple request asks each deferred attribute it lists in turn, once the
previous one has a response, and an error for any of them fails the request. An
Execute Write request writes its prepared values the same way, in the order the
client prepared them (Core Specification 6.3 Vol 3 Part F 3.4.6.3), and an error
for one of them is the response. It is only deferred when every value in it is
for a deferred attribute (see :c:type:`bt_gatt_deferred_attr_write_func_t`), and
then other bearers are served between its values, so the application keeps the
request atomic when that matters. A Write Command has no response:
:c:member:`bt_gatt_deferred_attr.write` completes it before it returns.

Responding to a request only records the response; the host then queues it
for sending from the RX thread, the same path any other response takes.

A request has to be responded to within :c:macro:`BT_ATT_TIMEOUT_SEC` seconds,
or the client fails it and stops using the bearer. A client may only have one request outstanding
per bearer; when it sends another one before the deferred request has a response,
the host disconnects that bearer, which for the unenhanced bearer means the
ACL connection. The host reserves one deferred-response slot for every bearer that
can exist at once, so every bearer can always defer its one in-flight request.
Each slot holds an ATT buffer until the response is sent.

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
