.. _bt_l2cap:

Logical Link Control and Adaptation Protocol (L2CAP)
####################################################

L2CAP layer enables connection-oriented channels which can be enabled with the
configuration option: :kconfig:option:`CONFIG_BT_L2CAP_DYNAMIC_CHANNEL`. These channels
support segmentation and reassembly transparently, they also support credit
based flow control making it suitable for data streams.

The user can also define fixed channels using the :c:macro:`BT_L2CAP_FIXED_CHANNEL_DEFINE`
macro. Fixed channels are initialized upon connection, and do not support segmentation. An example
of how to define a fixed channel is shown below.

.. code-block:: c

   static struct bt_l2cap_chan fixed_chan[CONFIG_BT_MAX_CONN];

   /* Callbacks are assumed to be defined prior. */
   static struct bt_l2cap_chan_ops ops = {
       .recv = recv_cb,
       .sent = sent_cb,
       .connected = connected_cb,
       .disconnected = disconnected_cb,
   };

   static int l2cap_fixed_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
   {
       uint8_t conn_index = bt_conn_index(conn);

       *chan = &fixed_chan[conn_index];

       **chan = (struct bt_l2cap_chan){
           .ops = &ops,
       };

       return 0;
   }

   BT_L2CAP_FIXED_CHANNEL_DEFINE(fixed_channel) = {
       .cid = 0x0010,
       .accept = l2cap_fixed_accept,
   };

Channels instances are represented by the :c:struct:`bt_l2cap_chan` struct which
contains the callbacks in the :c:struct:`bt_l2cap_chan_ops` struct to inform
when the channel has been connected, disconnected or when the encryption has
changed.
In addition to that it also contains the ``recv`` callback which is called
whenever an incoming data has been received. Data received this way can be
marked as processed by returning 0 or using
:c:func:`bt_l2cap_chan_recv_complete` API if processing is asynchronous.

.. note::
  The ``recv`` callback is called directly from RX Thread thus it is not
  recommended to block for long periods of time.

For sending data the :c:func:`bt_l2cap_chan_send` API can be used noting that
it may block if no credits are available, and resuming as soon as more credits
are available.

Servers can be registered using :c:func:`bt_l2cap_server_register` API passing
the :c:struct:`bt_l2cap_server` struct which informs what ``psm`` it should
listen to, the required security level ``sec_level``, and the callback
``accept`` which is called to authorize incoming connection requests and
allocate channel instances.

Client channels can be initiated with use of :c:func:`bt_l2cap_chan_connect`
API and can be disconnected with the :c:func:`bt_l2cap_chan_disconnect` API.
Note that the later can also disconnect channel instances created by servers.

API Reference
*************

.. doxygengroup:: bt_l2cap
