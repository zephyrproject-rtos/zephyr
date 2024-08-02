.. zephyr:code-sample:: sockets-service-echo
   :name: Echo server (service)
   :relevant-api: bsd_sockets

   Implements a simple IPv4/IPv6 TCP echo server using BSD sockets and socket service API.

Overview
********

The sockets/echo_service sample application for Zephyr implements a TCP echo
server supporting both IPv4 and IPv6 and using a BSD Sockets compatible API.

The purpose of this sample is to show how to use socket service API.
The socket service is a concept where many blocking sockets can be listened by
one thread, and which can then trigger a callback if there is activity in the set
of sockets. This saves memory as only one thread needs to be created in the
system.

The application supports IPv4 and IPv6, and both UDP and TCP are also supported.
The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/echo_service`.

Requirements
************

- :ref:`networking_with_host`
- or, a board with hardware networking

Building and Running
********************

Build the Zephyr version of the sockets/echo_service application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_service
   :board: <board_to_use>
   :goals: build
   :compact:

After the sample starts, it expects connections at 192.0.2.1, or 2001:db8::1
and port 4242.
The easiest way to connect is:

.. code-block:: console

    $ telnet 192.0.2.1 4242

After a connection is made, the application will echo back any line sent
to it. The application implements a single-threaded server using blocking
sockets, and currently is only implemented to serve only one client connection
at time. After the current client disconnects, the next connection can proceed.
