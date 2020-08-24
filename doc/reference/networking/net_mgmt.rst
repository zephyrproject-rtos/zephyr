.. _net_mgmt_interface:

Network Management
##################

.. contents::
    :local:
    :depth: 2

Overview
********

The Network Management APIs allow applications, as well as network
layer code itself, to call defined network routines at any level in
the IP stack, or receive notifications on relevant network events. For
example, by using these APIs, code can request a scan be done on a
Wi-Fi- or Bluetooth-based network interface, or request notification if
a network interface IP address changes.

The Network Management API implementation is designed to save memory
by eliminating code at build time for management routines that are not
used. Distinct and statically defined APIs for network management
procedures are not used.  Instead, defined procedure handlers are
registered by using a :c:macro:`NET_MGMT_REGISTER_REQUEST_HANDLER`
macro. Procedure requests are done through a single :c:func:`net_mgmt` API
that invokes the registered handler for the corresponding request.

The current implementation is experimental and may change and improve
in future releases.

Requesting a defined procedure
******************************

All network management requests are of the form
``net_mgmt(mgmt_request, ...)``. The ``mgmt_request`` parameter is a bit
mask that tells which stack layer is targeted, if a ``net_if`` object is
implied, and the specific management procedure being requested. The
available procedure requests depend on what has been implemented in
the stack.

To avoid extra cost, all :c:func:`net_mgmt` calls are direct. Though this
may change in a future release, it will not affect the users of this
function.

Listening to network events
***************************

You can receive notifications on network events by registering a
callback function and specifying a set of events used to filter when
your callback is invoked. The callback  will have to be unique for a
pair of layer and code, whereas on the command part it will be a mask of
events.

Two functions are available, :c:func:`net_mgmt_add_event_callback` for
registering the callback function, and
:c:func:`net_mgmt_del_event_callback`
for unregistering a callback. A helper function,
:c:func:`net_mgmt_init_event_callback`, can
be used to ease the initialization of the callback structure.

When an event occurs that matches a callback's event set, the
associated callback function is invoked with the actual event
code. This makes it possible for different events to be handled by the
same callback function, if desired.

.. warning::

   Event set filtering allows false positives for events that have the same
   layer and layer code.  A callback handler function **must** check
   the event code (passed as an argument) against the specific network
   events it will handle, **regardless** of how many events were in the
   set passed to :c:func:`net_mgmt_init_event_callback`.

   (False positives can occur for events which have the same layer and
   layer code.)

An example follows.

.. code-block:: c

	/*
	 * Set of events to handle.
	 * See e.g. include/net/net_event.h for some NET_EVENT_xxx values.
	 */
	#define EVENT_SET (NET_EVENT_xxx | NET_EVENT_yyy)

	struct net_mgmt_event_callback callback;

	void callback_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
			      struct net_if *iface)
	{
		if (mgmt_event == NET_EVENT_xxx) {
			/* Handle NET_EVENT_xxx */
		} else if (mgmt_event == NET_EVENT_yyy) {
			/* Handle NET_EVENT_yyy */
		} else {
			/* Spurious (false positive) invocation. */
		}
	}

	void register_cb(void)
	{
		net_mgmt_init_event_callback(&callback, callback_handler, EVENT_SET);
		net_mgmt_add_event_callback(&callback);
	}

See :zephyr_file:`include/net/net_event.h` for available generic core events that
can be listened to.


Defining a network management procedure
***************************************

You can provide additional management procedures specific to your
stack implementation by defining a handler and registering it with an
associated mgmt_request code.

Management request code are defined in relevant places depending on
the targeted layer or eventually, if l2 is the layer, on the
technology as well. For instance, all IP layer management request code
will be found in the :zephyr_file:`include/net/net_event.h` header file. But in case
of an L2 technology, let's say Ethernet, these would be found in
:zephyr_file:`include/net/ethernet.h`

You define your handler modeled with this signature:

.. code-block:: c

   static int your_handler(uint32_t mgmt_event, struct net_if *iface,
                           void *data, size_t len);

and then register it with an associated mgmt_request code:

.. code-block:: c

   NET_MGMT_REGISTER_REQUEST_HANDLER(<mgmt_request code>, your_handler);

This new management procedure could then be called by using:

.. code-block:: c

   net_mgmt(<mgmt_request code>, ...);


Signaling a network event
*************************

You can signal a specific network event using the :c:func:`net_mgmt_notify`
function and provide the network event code. See
:zephyr_file:`include/net/net_mgmt.h` for details. As for the management request
code, event code can be also found on specific L2 technology mgmt headers,
for example :zephyr_file:`include/net/ieee802154_mgmt.h` would be the right place if
802.15.4 L2 is the technology one wants to listen to events.

API Reference
*************

.. doxygengroup:: net_mgmt
   :project: Zephyr
