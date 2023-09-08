.. zephyr:code-sample:: socket-dumb-http-server
   :name: Dumb HTTP server
   :relevant-api: bsd_sockets net_pkt

   Implement a simple, portable, HTTP server using BSD sockets.

Overview
********

The sockets/dumb_http_server sample application for Zephyr implements a
skeleton HTTP server using a BSD Sockets compatible API. The purpose of
this sample is to show how it's possible to develop a sockets application
portable to both POSIX and Zephyr. As such, this HTTP server example is
kept very minimal and does not really parse an incoming HTTP request,
just reads and discards it, and always serve a single static page. Even
with such simplification, it is useful as an example of a socket
application which can be accessed via a conventional web browser, or to
perform performance/regression testing using existing HTTP diagnostic
tools.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/dumb_http_server`.

Requirements
************

- :ref:`networking_with_host`
- or, a board with hardware networking

Building and Running
********************

Build the Zephyr version of the sockets/echo application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/dumb_http_server
   :board: <board_to_use>
   :goals: build
   :compact:

After the sample starts, it expects connections at 192.0.2.1, port 8080.
The easiest way to connect is by opening a following URL in a web
browser: http://192.0.2.1:8080/ . You should see a page with a sample
content about Zephyr (captured at a particular time from Zephyr's web
site, note that it may differ from the content on the live Zephyr site).
Alternatively, a tool like ``curl`` can be used:

.. code-block:: console

    $ curl http://192.0.2.1:8080/

Finally, you can run an HTTP profiling/load tool like Apache Bench
(``ab``) against the server::

    $ ab -n10 http://192.0.2.1:8080/

``-n`` parameter specifies the number of HTTP requests to issue against
a server.

Running application on POSIX Host
=================================

The same application source code can be built for a POSIX system, e.g.
Linux. (Note: if you look at the source, you will see that the code is
the same except the header files are different for Zephyr vs POSIX.)

To build for a host POSIX OS:

.. code-block:: console

    $ make -f Makefile.posix

To run:

.. code-block:: console

    $ ./socket_dumb_http

To test, connect to http://127.0.0.1:8080/ , and follow the steps in the
previous section.

As can be seen, the behavior of the application is the same as the Zephyr
version.
