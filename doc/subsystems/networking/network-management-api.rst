.. _network_management_api:

Network Management API
######################

The Network Management APIs allow applications, as well as network
layer code itself, to call defined network routines at any level in
the IP stack, or receive notifications on relevant network events. For
example, by using these APIs, code can request a scan be done on a
WiFi- or Bluetooth-based network interface, or request notification if
a network interface IP address changes.

The Network Management API implementation is designed to save memory
by eliminating code at build time for management routines that are not
used. Distinct and statically defined APIs for network management
procedures are not used.  Instead, defined procedure handlers are
registered by using a `NET_MGMT_REGISTER_REQUEST_HANDLER`
macro. Procedure requests are done through a single `net_mgmt()` API
that invokes the registered handler for the corresponding request.

The current implementation is experimental and may change and improve
in future releases.

Requesting a defined procedure
******************************

All network management requests are of the form
`net_mgmt(mgmt_request, ...)`. The `mgmt_request` parameter is a bit
mask that tells which stack layer is targeted, if a `net_if` object is
implied, and the specific management procedure being requested. The
available procedure requests depend on what has been implemented in
the stack.

To avoid extra cost, all `net_mgmt()` calls are direct. Though this
may change in a future release, it will not affect the users of this
function.

Listening to network event
**************************

You can receive notifications on network events by registering a
callback function and specifying an event mask used to match one or
more events that are relevant.

Two functions are available, `net_mgmt_add_event_callback()` for
registering the callback function, and `net_mgmt_del_event_callback()`
for unregistering. A helper function, `net_mgmt_init_event_cb()`, can
be used to ease the initialization of the callback structure.

When an event is raised that matches a registered event mask, the
associated callback function is invoked with the actual event
code. This makes it possible for different events to be handled by the
same callback function, if desired.

See an example of registering callback functions and using the network
management API in `test/net/mgmt/src/mgmt.c`.

Defining a network management procedure
***************************************

You can provide additional management procedures specific to your
stack implementation by defining a handler and registering it with an
associated mgmt_request code.

Management request code are defined in relevant places depending on
the targeted layer or eventually, if l2 is the layer, on the
technology as well. For instance, all IP layer management request code
will be found in the `include/net/net_mgmt.h` header file. But in case
of an L2 technology, let's say Ethernet, these would be found in
`include/net/ethernet.h`

You define your handler modeled with this signature:

.. code-block:: c

   static int your_handler(u32_t mgmt_event, struct net_if *iface,
                           void *data, size_t len);

and then register it with an associated mgmt_request code:

.. code-block:: c

   NET_MGMT_REGISTER_REQUEST_HANDLER(<mgmt_request code>, your_handler);

This new management procedure could then be called by using:

.. code-block:: c

   net_mgmt(<mgmt_request code>, ...);


Signaling a network event
*************************

You can signal a specific network event using the `net_mgmt_notify()`
function and provide the network event code. See
`include/net/net_mgmt.h` for details. As for the management request
code, event code can be also found on specific L2 technology headers,
for example `include/net/ieee802154.h` would be the right place if
802.15.4 L2 is the technology one wants to listen to events.
