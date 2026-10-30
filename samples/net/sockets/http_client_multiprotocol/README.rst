.. zephyr:code-sample:: sockets-http-client-multiprotocol
   :name: Multiprotocol HTTP Client
   :relevant-api: http_client_multiprotocol tls_credentials

   Fetch and upload resources over HTTP/2, with and without TLS.

Overview
********

This sample uses the :ref:`multiprotocol HTTP client <http_client_multiprotocol_interface>`
over HTTP/2, in two parts:

* A synchronous session, with a GET request and a PUT request whose body is
  streamed through the payload callback.
* An asynchronous session, with three GET requests sent back to back and their
  responses delivered through the callback, interleaved on one connection.

Built with the TLS overlay, it does the same over HTTPS. The server has to
accept PUT requests for the upload to be answered with success.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/http_client_multiprotocol`.

Requirements
************

- :ref:`networking_with_host`
- An HTTP/2 server. In the clear, the server has to accept HTTP/2 with prior
  knowledge on port 8080; over TLS, it has to offer ``h2`` through ALPN on
  port 4443 with the ``https-server.pem`` certificate from the `net-tools`_
  project, which the sample trusts for the host name ``localhost``.

Building and Running
********************

Build and run the sample on a supported board, or on ``native_sim`` as
described in :ref:`networking_with_native_sim`:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/http_client_multiprotocol
   :board: native_sim
   :goals: run
   :compact:

Add the ``overlay-tls.conf`` overlay for HTTPS:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/http_client_multiprotocol
   :board: native_sim
   :conf: "prj.conf overlay-tls.conf"
   :goals: run
   :compact:

The server address is taken from the ``CONFIG_NET_CONFIG_PEER_IPV4_ADDR``
option in :file:`prj.conf`.

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
