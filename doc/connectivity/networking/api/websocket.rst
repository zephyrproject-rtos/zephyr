.. _websocket_interface:

Websocket Client API
####################

.. contents::
    :local:
    :depth: 2

Overview
********

The Websocket client library allows Zephyr to connect to a Websocket server.
The Websocket client API can be used directly by application to establish
a Websocket connection to server, or it can be used as a transport for other
network protocols like MQTT.

See this
`Websocket Wikipedia article <https://en.wikipedia.org/wiki/WebSocket>`_
for a detailed overview of how Websocket works.

For more information about the protocol itself, see
`IETF RFC6455 The WebSocket Protocol <https://tools.ietf.org/html/rfc6455>`_.

Websocket Transport
*******************

The Websocket API allows it to be used as a transport for other high level
protocols like MQTT. The Zephyr MQTT client library can be configured to use
Websocket transport by enabling :kconfig:option:`CONFIG_MQTT_LIB_WEBSOCKET` and
:kconfig:option:`CONFIG_WEBSOCKET_CLIENT` Kconfig options.

First a socket needs to be created and connected to the Websocket server:

.. code-block:: c

    sock = socket(family, SOCK_STREAM, IPPROTO_TCP);
    ...
    ret = connect(sock, addr, addr_len);
    ...

The Websocket transport socket is then created like this:

.. code-block:: c

    ws_sock = websocket_connect(sock, &config, timeout, user_data);

The Websocket socket can then be used to send or receive data, and the
Websocket client API will encapsulate the sent or received data to/from
Websocket packet payload. Both the :c:func:`websocket_xxx()` API or normal
BSD socket API functions can be used to send and receive application data.

.. code-block:: c

    ret = websocket_send_msg(ws_sock, buf_to_send, buf_len,
                             WEBSOCKET_OPCODE_DATA_BINARY, true, true,
			     K_FOREVER);
    ...
    ret = send(ws_sock, buf_to_send, buf_len, 0);

If normal BSD socket functions are used, then currently only TEXT data
is supported. In order to send BINARY data, the :c:func:`websocket_send_msg()`
must be used.

When done, the Websocket transport socket must be closed.

.. code-block:: c

    ret = close(ws_sock);
    or
    ret = websocket_disconnect(ws_sock);


API Reference
*************

.. doxygengroup:: websocket
