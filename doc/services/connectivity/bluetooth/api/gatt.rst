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
:c:macro:`BT_GATT_ATTRIBUTE_DEFERRED`. Its :c:member:`bt_gatt_attr_deferred.read` and
:c:member:`bt_gatt_attr_deferred.write` methods receive a :c:struct:`bt_gatt_req`
and may return before the request has a response. The application copies the
request and responds to it later with :c:func:`bt_gatt_read_rsp`,
:c:func:`bt_gatt_write_rsp` or :c:func:`bt_gatt_error_rsp`, from any thread.
Errors are given as ``BT_GATT_ERR()`` values, the same as the synchronous
callbacks return them. The copy holds its connection without a reference, so
take one with :c:func:`bt_conn_ref` while it is kept.

An attribute uses either its synchronous ``read`` and ``write`` callbacks or
the deferred ones: every remote request of a kind that has a deferred
method goes to that method. The host only reads attributes it defines itself,
such as service declarations, so a service does not need both for one
attribute. A Client Characteristic Configuration descriptor is the exception:
the host reads it locally to tell whether a client is subscribed, so a
host-managed one keeps its synchronous ``read`` (:c:func:`bt_gatt_attr_read_ccc`)
and an application-managed one keeps a synchronous ``read`` that returns the
stored configuration.

These requests can be responded to later:

* Read
* Read Blob
* Read By Type
* Read Multiple
* Read Multiple Variable Length
* Write
* Prepare Write
* Execute Write

A Read Multiple request asks each deferred attribute it lists in turn, once
the previous one has a response, and an error for any of them fails the
request. An Execute Write request writes each prepared value the same way, in
the order the client prepared them (Core Specification 6.3 Vol 3 Part F
3.4.6.1), and an error for one of them is the response. It may be deferred only
when every attribute in the request is deferred; with a synchronous one
among them it is written synchronously, so no other bearer sees a half-applied
request (3.4.6.3, atomic Execute Write). Other bearers are served in between,
so the application keeps the request atomic when that matters. Write Command
and Signed Write Command have no response: the ``write`` method completes them
before it returns.

The deferred methods are called from the RX thread, like the synchronous
ones, and must return without waiting on other threads, semaphores or timers.
Responding to a request only records the response; the host then queues it
for sending from the RX thread, the same path a synchronous response takes.

A request has to be responded to within :c:macro:`BT_ATT_TIMEOUT_SEC` seconds,
or the client fails it. A client may only have one request outstanding per
bearer; when it sends another one before the deferred request has a response,
the host disconnects that bearer, which for the unenhanced bearer means the
connection. The host reserves one deferred-response slot for every bearer that
can exist at once, so every bearer can always defer its one in-flight request.
Each slot holds an ATT buffer until the response is sent, and a request that
finds no slot is rejected with Insufficient Resources.

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
