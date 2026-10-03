.. _bt_sco:


Synchronous Connection-Oriented (SCO)
#####################################

SCO provides support for Bluetooth Classic Synchronous Connection-Oriented logical transport,
which is primarily used for voice/audio data transmission such as telephony audio in HFP
(Hands-Free Profile) scenarios. The SCO stream interface allows applications to send and receive
audio data over an established SCO connection using the Voice over HCI path.
The feature is enabled through the configuration option: :kconfig:option:`CONFIG_BT_VOICE_OVER_HCI`.

SCO Stream Lifecycle
********************

The SCO stream follows a specific lifecycle: register callbacks, connect the stream to an SCO
connection, send/receive data, and disconnect the stream when SCO conn is disconnected.

Registering Stream Callbacks
============================

To handle data reception and transmission events, register a :c:struct:`bt_sco_stream_ops`
callback structure with a :c:struct:`bt_sco_stream`:

.. code-block:: c

   static void sco_recv_cb(struct bt_sco_stream *stream, uint8_t flag, struct net_buf *buf)
   {
       /* Handling RX data */
   }

   static void sco_sent_cb(struct bt_sco_stream *stream)
   {
       /* TX done */
   }

   static const struct bt_sco_stream_ops stream_ops = {
       .recv = sco_recv_cb,
       .sent = sco_sent_cb,
   };

   static struct bt_sco_stream sco_stream;

   bt_sco_stream_cb_register(&sco_stream, &stream_ops);

Connecting an SCO Stream
========================

When an SCO connection is established (e.g., via the :c:member:`bt_hfp_hf_cb.sco_connected`
callback), bind a stream to the SCO connection before any data operations:

.. code-block:: c

   static void sco_connected_cb(struct bt_hfp_hf *hf, struct bt_conn *sco_conn)
   {
       bt_sco_stream_connect(sco_conn, &sco_stream);
   }

Sending SCO Data
================

To send data over a connected SCO stream, allocate a buffer with at least
:c:macro:`BT_SCO_CHAN_SEND_RESERVE` bytes reserved for the HCI SCO header. The user_data_size
of the allocated net buffer must be at least :kconfig:option:`CONFIG_BT_CONN_TX_USER_DATA_SIZE`
bytes. Also, the total length of TX audio data should be no more than the SCO MTU which can be
achieved by :c:func:`bt_conn_get_info`.

.. code-block:: c

   struct net_buf *buf;

   buf = net_buf_alloc(&pool, K_FOREVER);
   net_buf_reserve(buf, BT_SCO_CHAN_SEND_RESERVE);
   net_buf_add_mem(buf, audio_data, audio_data_len);

   bt_sco_stream_send(&sco_stream, buf);

The total buffer size including the header can be calculated using the :c:macro:`BT_SCO_SDU_SIZE`
helper macro.

Disconnecting an SCO Stream
============================

When the SCO connection is closed (e.g., via the :c:member:`bt_hfp_hf_cb.sco_disconnected`
callback), disconnect the stream to release associated resources. The stream must be disconnected
to properly release the SCO connection:

.. code-block:: c

   static void sco_disconnected_cb(struct bt_conn *sco_conn, uint8_t reason)
   {
       bt_sco_stream_disconnect(&sco_stream);
   }

Also the registered stream callbacks can be unregistered:

.. code-block:: c

   bt_sco_stream_cb_unregister(&sco_stream);


API Reference
*************

.. doxygengroup:: bt_sco
