.. _bt_l2cap_br:

Bluetooth Logical Link Control and Adaptation Protocol (L2CAP) for BR/EDR
#########################################################################

L2CAP BR/EDR provides support for Bluetooth Classic L2CAP (Logical Link Control and Adaptation
Protocol) features including ECHO request/response and connectionless data channels.

ECHO Request/Response
*********************

The L2CAP ECHO feature allows testing the connection by sending ECHO requests and receiving ECHO
responses.
Applications can register callbacks to monitor ECHO packets and send ECHO data.
The feature is enabled through the configuration option: :kconfig:option:`CONFIG_BT_CLASSIC`.

Registering ECHO Callbacks
==========================

To monitor ECHO request/response packets, register a :c:struct:`bt_l2cap_br_echo_cb` callback
structure:

.. code-block:: c

   static void echo_req_cb(struct bt_conn *conn, uint8_t identifier, struct net_buf *buf)
   {
       /* Handle ECHO request */
   }

   static void echo_rsp_cb(struct bt_conn *conn, struct net_buf *buf)
   {
       /* Handle ECHO response */
   }

   static struct bt_l2cap_br_echo_cb echo_cb = {
       .req = echo_req_cb,
       .rsp = echo_rsp_cb,
   };

   bt_l2cap_br_echo_cb_register(&echo_cb);

Sending ECHO Request
====================

To send an ECHO request, allocate a buffer with :c:macro:`BT_L2CAP_BR_ECHO_REQ_RESERVE`
bytes reserved for the L2CAP header:

.. code-block:: c

   struct net_buf *buf;

   buf = net_buf_alloc(&pool, K_FOREVER);
   net_buf_reserve(buf, BT_L2CAP_BR_ECHO_REQ_RESERVE);
   net_buf_add_mem(buf, data, data_len);

   bt_l2cap_br_echo_req(conn, buf);

Sending ECHO Response
=====================

To send an ECHO response (typically in response to a received ECHO request), allocate a buffer with
:c:macro:`BT_L2CAP_BR_ECHO_RSP_RESERVE` bytes reserved for the L2CAP header:

.. code-block:: c

   struct net_buf *buf;

   buf = net_buf_alloc(&pool, K_FOREVER);
   net_buf_reserve(buf, BT_L2CAP_BR_ECHO_RSP_RESERVE);
   net_buf_add_mem(buf, data, data_len);

   bt_l2cap_br_echo_rsp(conn, buf);

The identifier parameter must match the identifier from the received ECHO request to properly
correlate the response with the request.

Connectionless Data Channel
***************************

The connectionless data channel allows sending and receiving data to/from a specific PSM
(Protocol/Service Multiplexer) without establishing a connection-oriented L2CAP channel.
The feature is enabled through the configuration option: :kconfig:option:`CONFIG_BT_L2CAP_CONNLESS`.

Registering Connectionless Callbacks
====================================

To receive connectionless data, register a :c:struct:`bt_l2cap_br_connless_cb` callback structure:

.. code-block:: c

   static void connless_recv_cb(struct bt_conn *conn, uint16_t psm, struct net_buf *buf)
   {
       /* Handle received connectionless data */
   }

   static struct bt_l2cap_br_connless_cb connless_cb = {
       .psm = MY_PSM,  /* Or 0 to receive all */
       .sec_level = BT_SECURITY_L1,
       .recv = connless_recv_cb,
   };

   bt_l2cap_br_connless_register(&connless_cb);

Sending Connectionless Data
===========================

To send connectionless data, allocate a buffer with :c:macro:`BT_L2CAP_CONNLESS_RESERVE`
bytes reserved:

.. code-block:: c

   struct net_buf *buf;

   buf = net_buf_alloc(&pool, K_FOREVER);
   net_buf_reserve(buf, BT_L2CAP_CONNLESS_RESERVE);
   net_buf_add_mem(buf, data, data_len);

   bt_l2cap_br_connless_send(conn, psm, buf);

API Reference
*************

.. doxygengroup:: bt_l2cap_br
