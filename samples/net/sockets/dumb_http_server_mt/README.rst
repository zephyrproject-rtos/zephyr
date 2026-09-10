.. zephyr:code-sample:: socket-dumb-http-server-mt
   :name: Dumb HTTP server (multi-threaded)
   :relevant-api: bsd_sockets net_pkt thread_apis tls_credentials

   Implement a simple HTTP server supporting simultaneous connections using BSD sockets.

Overview
********

The ``sockets/dumb_http_server_mt`` sample application for Zephyr implements a
skeleton HTTP server using a BSD Sockets compatible API.
This sample has similar functionality as :zephyr:code-sample:`socket-dumb-http-server`
except it has support for multiple simultaneous connections, TLS and
IPv6. Also this sample application has no compatibility with POSIX.
This HTTP server example is very minimal and does not really parse an incoming
HTTP request, just reads and discards it, and always serves a single static
page.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/dumb_http_server_mt`.

Requirements
************

- :ref:`networking_with_host`
- or, a board with hardware networking

Building and Running
********************

Build the Zephyr version of the sockets/dumb_http_server_mt application like
this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/dumb_http_server_mt
   :board: <board_to_use>
   :goals: build
   :compact:

After the sample starts, it expects connections at 192.0.2.1 or 2001:db8::1,
port 8080. The easiest way to connect is by opening a following URL in a web
browser: http://192.0.2.1:8080/ or http://[2001:db8::1]:8080/
You should see a page with a sample content about Zephyr (captured at a
particular time from Zephyr's web site, note that it may differ from the
content on the live Zephyr site).
Alternatively, a tool like ``curl`` can be used:

.. code-block:: console

    $ curl http://192.0.2.1:8080/

Finally, you can run an HTTP profiling/load tool like Apache Bench
(``ab``) against the server::

    $ ab -n10 http://192.0.2.1:8080/

``-n`` parameter specifies the number of HTTP requests to issue against
a server.
