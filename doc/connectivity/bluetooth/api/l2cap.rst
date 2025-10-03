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

Creating a simple L2CAP server
-----------------------------

The documentation above describes the APIs but does not show a minimal flow to create a working L2CAP server.  The steps are:

1. Register a server using :c:func:`bt_l2cap_server_register` and provide an ``accept`` callback.
2. Allocate a channel instance (for example :c:type:`bt_l2cap_le_chan` or a :c:array:`bt_l2cap_chan` array sized by ``CONFIG_BT_MAX_CONN``) whose ``chan.ops`` points to a :c:type:`bt_l2cap_chan_ops` struct. This instance is assigned to the ``*chan`` parameter of the ``accept`` callback and becomes the channel used for the connection.
3. In the ``accept`` callback assign the channel instance to ``*chan`` to accept the incoming connection.
4. Implement the callbacks in :c:type:`bt_l2cap_chan_ops` such as ``recv``, ``connected`` and ``disconnected`` to handle data and connection events.

A minimal skeleton example follows (fill in allocation and error handling as needed):

.. code-block:: c

    /* Minimal L2CAP server example (skeleton) */

    #include <zephyr/bluetooth/bluetooth.h>
    #include <zephyr/bluetooth/l2cap.h>
    #include <zephyr/sys/printk.h>

    static int my_chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf);
    static void my_chan_connected(struct bt_l2cap_chan *chan);
    static void my_chan_disconnected(struct bt_l2cap_chan *chan);

    /* Define channel operations */
    static struct bt_l2cap_chan_ops my_chan_ops = {
        .recv       = my_chan_recv,
        .connected  = my_chan_connected,
        .disconnected = my_chan_disconnected,
    };

    /* Channel instance (one per connection). Allocate dynamically if you need many connections. */
    static struct bt_l2cap_le_chan my_chan = {
        .chan.ops = &my_chan_ops,
    };

    /* Accept callback: called on an incoming connection request. */
    static int my_server_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
    {
        printk("L2CAP accept from %p\n", conn);

        /* Assign our channel instance (or dynamically allocate a new one) */
        *chan = &my_chan.chan;

        /* return 0 to accept, or negative errno to reject */
        return 0;
    }

    static void my_chan_connected(struct bt_l2cap_chan *chan)
    {
        printk("L2CAP channel connected\n");
    }

    static void my_chan_disconnected(struct bt_l2cap_chan *chan)
    {
        printk("L2CAP channel disconnected\n");
    }

    static int my_chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
    {
        printk("L2CAP received %u bytes\n", buf->len);
            /* Process data here; returning 0 tells the stack to free the buffer.
            * If you need to process asynchronously, return -EINPROGRESS and later
            * call bt_l2cap_chan_recv_complete(chan, buf) when done (then you must
            * manage buf ownership yourself).
            */
        return 0;
    }

    static struct bt_l2cap_server my_server = {
        .psm = BT_L2CAP_PSM_DYNAMIC, /* or a fixed PSM value */
        .sec_level = BT_SECURITY_LOW,
        .accept = my_server_accept,
    };

    /* Call this after Bluetooth is enabled to register the server */
    static int register_my_server(void)
    {
        int err = bt_l2cap_server_register(&my_server);
        if (err) {
            printk("Failed to register L2CAP server: %d\n", err);
            return err;
        }
        printk("L2CAP server registered\n");
        return 0;
    }

A minimal skeleton example follows. See the complete sample at
:zephyr_file:`samples/bluetooth/l2cap_server_simple`.

.. literalinclude:: ../../../../samples/bluetooth/l2cap_server_simple/src/main.c
   :language: c
   :linenos:
   :lines: 1-100

Notes
~~~~~
* For multiple concurrent connections allocate channel instances dynamically (k_malloc or pool) instead of reusing a single ``my_chan``.
* See the test reference implementation for a fuller example: ``tests/bsim/bluetooth/host/l2cap/credits/src/main.c``.

Client channels can be initiated with use of :c:func:`bt_l2cap_chan_connect`
API and can be disconnected with the :c:func:`bt_l2cap_chan_disconnect` API.
Note that the latter can also disconnect channel instances created by servers.

API Reference
*************

.. doxygengroup:: bt_l2cap
