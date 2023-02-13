.. _networking_api_usage:

Network Connectivity API
########################

Applications should use the BSD socket API defined in
:zephyr_file:`include/zephyr/net/socket.h` to create a connection, send or receive data,
and close a connection. The same API can be used when working with UDP or
TCP data. See :ref:`BSD socket API <bsd_sockets_interface>` for more details.

See :ref:`sockets-echo-server-sample` and :ref:`sockets-echo-client-sample`
applications how to create a simple server or client BSD socket based
application.

The legacy connectivity API in :zephyr_file:`include/zephyr/net/net_context.h` should not be
used by applications.
