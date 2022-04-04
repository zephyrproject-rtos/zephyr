.. _bluetooth_mesh_heartbeat:

Heartbeat
#########

The Heartbeat feature provides functionality for monitoring Bluetooth mesh nodes
and determining the distance between nodes.

The Heartbeat feature is configured through the :ref:`bluetooth_mesh_models_cfg_srv` model.

Heartbeat messages
******************

Heartbeat messages are sent as transport control packets through the network,
and are only encrypted with a network key. Heartbeat messages contain the
original Time To Live (TTL) value used to send the message and a bitfield of
the active features on the node. Through this, a receiving node can determine
how many relays the message had to go through to arrive at the receiver, and
what features the node supports.

Available Heartbeat feature flags:

- :c:macro:`BT_MESH_FEAT_RELAY`
- :c:macro:`BT_MESH_FEAT_PROXY`
- :c:macro:`BT_MESH_FEAT_FRIEND`
- :c:macro:`BT_MESH_FEAT_LOW_POWER`

Heartbeat publication
*********************

Heartbeat publication is controlled through the Configuration models, and can
be triggered in two ways:

Periodic publication
   The node publishes a new Heartbeat message at regular intervals. The
   publication can be configured to stop after a certain number of messages, or
   continue indefinitely.

Triggered publication
   The node publishes a new Heartbeat message every time a feature changes. The
   set of features that can trigger the publication is configurable.

The two publication types can be combined.

Heartbeat subscription
**********************

A node can be configured to subscribe to Heartbeat messages from one node at
the time. To receive a Heartbeat message, both the source and destination must
match the configured subscription parameters.

Heartbeat subscription is always time limited, and throughout the subscription
period, the node keeps track of the number of received Heartbeats as well as
the minimum and maximum received hop count.

All Heartbeats received with the configured subscription parameters are passed
to the :cpp:member:`bt_mesh_hb_cb::recv` event handler.

When the Heartbeat subscription period ends, the
:cpp:member:`bt_mesh_hb_cb::sub_end` callback gets called.

API reference
**************

.. doxygengroup:: bt_mesh_heartbeat
