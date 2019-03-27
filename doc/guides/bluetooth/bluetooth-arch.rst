.. _bluetooth-arch:

Bluetooth Stack Architecture
############################

Overview
********

This page describes the software architecture of Zephyr's Bluetooth protocol
stack.

.. note::
   Zephyr supports mainly Bluetooth Low Energy (BLE), the low-power
   version of the Bluetooth specification. Zephyr also has limited support
   for portions of the BR/EDR Host. Throughout this architecture document we
   use BLE interchangably for Bluetooth except when noted.

.. _bluetooth-layers:

BLE Layers
==========

There are 3 main layers that together constitute a full Bluetooth Low Energy
protocol stack:

* **Host**: This layer sits right below the application, and is comprised of
  multiple (non real-time) network and transport protocols enabling
  applications to communicate with peer devices in a standard and interoperable
  way.
* **Controller**: The Controller implements the Link Layer (LE LL), the
  low-level, real-time protocol which provides, in conjunction with the Radio
  Hardware, standard interoperable over the air communication. The LL schedules
  packet reception and transmission, guarantees the delivery of data, and
  handles all the LL control procedures.
* **Radio Hardware**: Hardware implements the required analog and digital
  baseband functional blocks that permit the Link Layer firmware to send and
  receive in the 2.4GHz band of the spectrum.

.. _bluetooth-hci:

Host Controller Interface
=========================

The `Bluetooth Specification`_ describes the format in which a Host must
communicate with a Controller. This is called the Host Controller Interface
(HCI) protocol. HCI can be implemented over a range of different physical
transports like UART, SPI, or USB. This protocol defines the commands that a Host
can send to a Controller and the events that it can expect in return, and also
the format for user and protocol data that needs to go over the air. The HCI
ensures that different Host and Controller implementations can communicate
in a standard way making it possible to combine Hosts and Controllers from
different vendors.

.. _bluetooth-configs:

Configurations
==============

The three separate layers of the protocol and the standardized interface make
it possible to implement the Host and Controller on different platforms. The two
following configurations are commonly used:

* **Single-chip configuration**: In this configuration, a single microcontroller
  implements all three layers and the application itself. This can also be called a
  system-on-chip (SoC) implementation. In this case the BLE Host and the BLE
  Controller communicate directly through function calls and queues in RAM. The
  Bluetooth specification does not specify how HCI is implemented in this
  single-chip configuration and so how HCI commands, events, and data flows between
  the two can be implementation-specific. This configuration is well suited for
  those applications and designs that require a small footprint and the lowest
  possible power consumption, since everything runs on a single IC.
* **Dual-chip configuration**: This configuration uses two separate ICs,
  one running the Application and the Host, and a second one with the Controller
  and the Radio Hardware. This is sometimes also called a connectivity-chip
  configuration. This configuration allows for a wider variety of combinations of
  Hosts when using the Zephyr OS as a Controller. Since HCI ensures
  interoperability among Host and Controller implementations, including of course
  Zephyr's very own BLE Host and Controller, users of the Zephyr Controller can
  choose to use whatever Host running on any platform they prefer. For example,
  the host can be the Linux BLE Host stack (BlueZ) running on any processor
  capable of supporting Linux. The Host processor may of course also run Zephyr
  and the Zephyr OS BLE Host. Conversely, combining an IC running the Zephyr
  Host with an external Controller that does not run Zephyr is also supported.

.. _bluetooth-build-types:

Build Types
===========

The Zephyr software stack as an RTOS is highly configurable, and in particular,
the BLE subsystem can be configured in multiple ways during the build process to
include only the features and layers that are required to reduce RAM and ROM
footprint as well as power consumption. Here's a short list of the different
BLE-enabled builds that can be produced from the Zephyr project codebase:

* **Controller-only build**: When built as a BLE Controller, Zephyr includes
  the Link Layer and a special application. This application is different
  depending on the physical transport chosen for HCI:

  * :ref:`hci_uart <bluetooth-hci-uart-sample>`
  * :ref:`hci_usb <bluetooth-hci-usb-sample>`
  * :ref:`hci_spi <bluetooth-hci-spi-sample>`

  This application acts as a bridge between the UART, SPI or USB peripherals and
  the Controller subsystem, listening for HCI commands, sending application data
  and responding with events and received data.  A build of this type sets the
  following Kconfig option values:

  * :option:`CONFIG_BT` ``=y``
  * :option:`CONFIG_BT_HCI` ``=y``
  * :option:`CONFIG_BT_HCI_RAW` ``=y``
  * :option:`CONFIG_BT_CTLR` ``=y``
  * :option:`CONFIG_BT_LL_SW` ``=y`` (if using the open source Link Layer)

* **Host-only build**: A Zephyr OS Host build will contain the Application and
  the BLE Host, along with an HCI driver (UART or SPI) to interface with an
  external Controller chip.
  A build of this type sets the following Kconfig option values:

  * :option:`CONFIG_BT` ``=y``
  * :option:`CONFIG_BT_HCI` ``=y``
  * :option:`CONFIG_BT_CTLR` ``=n``

  All of the samples located in ``samples/bluetooth`` except for the ones
  used for Controller-only builds can be built as Host-only

* **Combined build**: This includes the Application, the Host and the
  Controller, and it is used exclusively for single-chip (SoC) configurations.
  A build of this type sets the following Kconfig option values:

  * :option:`CONFIG_BT` ``=y``
  * :option:`CONFIG_BT_HCI` ``=y``
  * :option:`CONFIG_BT_CTLR` ``=y``
  * :option:`CONFIG_BT_LL_SW` ``=y`` (if using the open source Link Layer)

  All of the samples located in ``samples/bluetooth`` except for the ones
  used for Controller-only builds can be built as Combined

The picture below shows the SoC or single-chip configuration when using a Zephyr
combined build (a build that includes both a BLE Host and a Controller in the
same firmware image that is programmed onto the chip):

.. figure:: img/ble_cfg_single.png
   :align: center
   :alt: BLE Combined build on a single chip

   A Combined build on a Single-Chip configuration

When using connectivity or dual-chip configurations, several Host and Controller
combinations are possible, some of which are depicted below:

.. figure:: img/ble_cfg_dual.png
   :align: center
   :alt: BLE dual-chip configuration builds

   Host-only and Controller-only builds on dual-chip configurations

When using a Zephyr Host (left side of image), two instances of Zephyr OS
must be built with different configurations, yielding two separate images that
must be programmed into each of the chips respectively. The Host build image
contains the application, the BLE Host and the selected HCI driver (UART or
SPI), while the Controller build runs either the
:ref:`hci_uart <bluetooth-hci-uart-sample>`, or the
:ref:`hci_spi <bluetooth-hci-spi-sample>` app to provide an interface to
the BLE Controller.

This configuration is not limited to using a Zephyr OS Host, as the right side
of the image shows. One can indeed take one of the many existing GNU/Linux
distributions, most of which include Linux's own BLE Host (BlueZ), to connect it
via UART or USB to one or more instances of the Zephyr OS Controller build.
BlueZ as a Host supports multiple Controllers simultaneously for applications
that require more than one BLE radio operating at the same time but sharing the
same Host stack.

Source tree layout
******************

The stack is split up as follows in the source tree:

``subsys/bluetooth/host``
  The host stack. This is where the HCI command and event handling
  as well as connection tracking happens. The implementation of the
  core protocols such as L2CAP, ATT, and SMP is also here.

``subsys/bluetooth/controller``
  Bluetooth Controller implementation. Implements the controller-side of
  HCI, the Link Layer as well as access to the radio transceiver.

``include/bluetooth/``
  Public API header files. These are the header files applications need
  to include in order to use Bluetooth functionality.

``drivers/bluetooth/``
  HCI transport drivers. Every HCI transport needs its own driver. For example,
  the two common types of UART transport protocols (3-Wire and 5-Wire)
  have their own drivers.

``samples/bluetooth/``
  Sample Bluetooth code. This is a good reference to get started with
  Bluetooth application development.

``tests/bluetooth/``
  Test applications. These applications are used to verify the
  functionality of the Bluetooth stack, but are not necessary the best
  source for sample code (see ``samples/bluetooth`` instead).

``doc/guides/bluetooth/``
  Extra documentation, such as PICS documents.

Host
****

The Bluetooth Host implements all the higher-level protocols and
profiles, and most importantly, provides a high-level API for
applications. The following diagram depicts the main protocol & profile
layers of the host.

.. figure:: img/ble_host_layers.png
   :align: center
   :alt: Bluetooth Host protocol & profile layers

   Bluetooth Host protocol & profile layers.

Lowest down in the host stack sits a so-called HCI driver, which is
responsible for abstracting away the details of the HCI transport. It
provides a basic API for delivering data from the controller to the
host, and vice-versa.

Perhaps the most important block above the HCI handling is the Generic
Access Profile (GAP). GAP simplifies Bluetooth LE access by defining
four distinct roles of BLE usage:

* Connection-oriented roles

  * Peripheral (e.g. a smart sensor, often with a limited user interface)

  * Central (typically a mobile phone or a PC)

* Connection-less roles

  * Broadcaster (sending out BLE advertisements, e.g. a smart beacon)

  * Observer (scanning for BLE advertisements)

Each role comes with its own build-time configuration option:
:option:`CONFIG_BT_PERIPHERAL`, :option:`CONFIG_BT_CENTRAL`,
:option:`CONFIG_BT_BROADCASTER` & :option:`CONFIG_BT_OBSERVER`. Of the
connection-oriented roles central implicitly enables observer role, and
peripheral implicitly enables broadcaster role. Usually the first step
when creating an application is to decide which roles are needed and go
from there. Bluetooth Mesh is a slightly special case, requiring at
least the observer and broadcaster roles, and possibly also the
Peripheral role. This will be described in more detail in a later
section.

Peripheral role
===============

Most Zephyr-based BLE devices will most likely be peripheral-role
devices. This means that they perform connectable advertising and expose
one or more GATT services. After registering services using the
:cpp:func:`bt_gatt_service_register` API the application will typically
start connectable advertising using the :cpp:func:`bt_le_adv_start` API.

There are several peripheral sample applications available in the tree,
such as :zephyr_file:`samples/bluetooth/peripheral_hr`.

Central role
============

Central role may not be as common for Zephyr-based devices as peripheral
role, but it is still a plausible one and equally well supported in
Zephyr. Rather than accepting connections from other devices a central
role device will scan for available peripheral device and choose one to
connect to. Once connected, a central will typically act as a GATT
client, first performing discovery of available services and then
accessing one or more supported services.

To initially discover a device to connect to the application will likely
use the :cpp:func:`bt_le_scan_start` API, wait for an appropriate device
to be found (using the scan callback), stop scanning using
:cpp:func:`bt_le_scan_stop` and then connect to the device using
:cpp:func:`bt_conn_create_le`. If the central wants to keep
automatically reconnecting to the peripheral it should use the
:cpp:func:`bt_le_set_auto_conn` API.

There are some sample applications for the central role available in the
tree, such as :zephyr_file:`samples/bluetooth/central_hr`.

Observer role
=============

An observer role device will use the :cpp:func:`bt_le_scan_start` API to
scan for device, but it will not connect to any of them. Instead it will
simply utilize the advertising data of found devices, combining it
optionally with the received signal strength (RSSI).

Broadcaster role
================

A broadcaster role device will use the :cpp:func:`bt_le_adv_start` API to
advertise specific advertising data, but the type of advertising will be
non-connectable, i.e. other device will not be able to connect to it.

Connections
===========

The Zephyr Bluetooth stack uses an abstraction called :c:type:`bt_conn`
to represent connections to other devices. The internals of this struct
are not exposed to the application, but a limited amount of information
(such as the remote address) can be acquired using the
:cpp:func:`bt_conn_get_info` API. Connection objects are reference
counted, and the application is expected to use the
:cpp:func:`bt_conn_ref` API whenever storing a connection pointer for a
longer period of time, since this ensures that the object remains valid
(even if the connection would get disconnected). Similarly the
:cpp:func:`bt_conn_unref` API is to be used when releasing a reference
to a connection.

An application may track connections by registering a
:c:type:`bt_conn_cb` struct using the :cpp:func:`bt_conn_cb_register`
API.  This struct lets the application define callbacks for connection &
disconnection events, as well as other events related to a connection
such as a change in the security level or the connection parameters.
When acting as a central the application will also get hold of the
connection object through the return value of the
:cpp:func:`bt_conn_create_le` API.

Security
========

To achieve a secure relationship between two Bluetooth devices a process
called pairing is used. This process can either be triggered implicitly
through the security properties of GATT services, or explicitly using
the :cpp:func:`bt_conn_security` API on a connection object.

To achieve a higher security level, and protect against
Man-In-The-Middle (MITM) attacks, it is recommended to use some
out-of-band channel during the pairing. If the devices have a sufficient
user interface this "channel" is the user itself. The capabilities of
the device are registered using the :cpp:func:`bt_conn_auth_cb_register`
API.  The :c:type:`bt_conn_auth_cb` struct that's passed to this API has
a set of optional callbacks that can be used during the pairing - if the
device lacks some feature the corresponding callback may be set to NULL.
For example, if the device does not have an input method but does have a
display, the ``passkey_entry`` and ``passkey_confirm`` callbacks would
be set to NULL, but the ``passkey_display`` would be set to a callback
capable of displaying a passkey to the user.

Depending on the local and remote security requirements & capabilities,
there are four possible security levels that can be reached:

    :cpp:enumerator:`BT_SECURITY_LOW`
        No encryption and no authentication.

    :cpp:enumerator:`BT_SECURITY_MEDIUM`
        Encryption but no authentication (no MITM protection).

    :cpp:enumerator:`BT_SECURITY_HIGH`
        Encryption and authentication using the legacy pairing method
        from Bluetooth 4.0 and 4.1.

    :cpp:enumerator:`BT_SECURITY_FIPS`
        Encryption and authentication using the LE Secure Connections
        feature available since Bluetooth 4.2.

.. note::
  Mesh has its own security solution through a process called
  provisioning. It follows a similar procedure as pairing, but is done
  using separate mesh-specific APIs.

L2CAP
=====

L2CAP layer enables connection-oriented channels which can be enable with the
configuration option: :option:`CONFIG_BT_L2CAP_DYNAMIC_CHANNEL`. This channels
support segmentation and reassembly transparently, they also support credit
based flow control making it suitable for data streams.

Channels instances are represented by the :cpp:class:`bt_l2cap_chan` struct which
contains the callbacks in the :cpp:class:`bt_l2cap_chan_ops` struct to inform
when the channel has been connected, disconnected or when the encryption has
changed.
In addition to that it also contains the ``recv`` callback which is called
whenever an incoming data has been received. Data received this way can be
marked as processed by returning 0 or using
:cpp:func:`bt_l2cap_chan_recv_complete` API if processing is asynchronous.

.. note::
  The ``recv`` callback is called directly from RX Thread thus it is not
  allowed to block.

For sending data the :cpp:func:`bt_l2cap_chan_send` API can be used noting that
it may block if no credits are available, and resuming as soon as more credits
are available.

Servers can be registered using :cpp:func:`bt_l2cap_server_register` API passing
the :cpp:class:`bt_l2cap_server` struct which informs what ``psm`` it should
listen to, the required security level ``sec_level``, and the callback
``accept`` which is called to authorize incoming connection requests and
allocate channel instances.

Client channels can be initiated with use of :cpp:func:`bt_l2cap_chan_connect`
API and can be disconnected with the :cpp:func:`bt_l2cap_chan_disconnect` API.
Note that the later can also disconnect channel instances created by servers.

GATT
====

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
  thus they are not allowed to block.

Attribute value changes can be notified using :cpp:func:`bt_gatt_notify` API,
alternatively there is :cpp:func:`bt_gatt_notify_cb` where is is possible to
pass a callback to be called when it is necessary to know the exact instant when
the data has been transmitted over the air. Indications are supported by
:cpp:func:`bt_gatt_indicate` API.

Client procedures can be enabled with the configuration option:
:option:`CONFIG_BT_GATT_CLIENT`

Discover procedures can be initiated with the use of
:cpp:func:`bt_gatt_discover` API which takes the
:cpp:class:`bt_gatt_dicover_params` struct which describes the type of
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

Mesh
====

Mesh is a little bit special when it comes to the needed GAP roles. By
default, mesh requires both observer and broadcaster role to be enabled.
If the optional GATT Proxy feature is desired, then peripheral role
should also be enabled.

Persistent storage
==================

The Bluetooth host stack uses the settings subsystem to implement
persistent storage to flash. This requires the presence of a flash
driver and a designated "storage" partition on flash. A typical set of
configuration options needed will look something like the following:

  .. code-block:: none

    CONFIG_BT_SETTINGS=y
    CONFIG_FLASH=y
    CONFIG_FLASH_PAGE_LAYOUT=y
    CONFIG_FLASH_MAP=y
    CONFIG_FCB=y
    CONFIG_SETTINGS=y
    CONFIG_SETTINGS_FCB=y

Once enabled, it is the responsibility of the application to call
settings_load() after having initialized Bluetooth (using the
bt_enable() API).

BLE Controller
**************

Standard
========

Split
=====



.. _Bluetooth Specification: https://www.bluetooth.com/specifications/bluetooth-core-specification
