.. zephyr:code-sample:: quic-service-echo
   :name: Echo server (QUIC)
   :relevant-api: quic

   Implements a simple IPv4/IPv6 echo server using QUIC API and socket service API.

Overview
********

The quic/echo_service sample application for Zephyr implements a QUIC protocol
echo server supporting both IPv4 and IPv6 and using a BSD Sockets compatible API.

The purpose of this sample is to show how to use QUIC and service APIs.
The socket service is a concept where many blocking sockets can be listened by
one thread, and which can then trigger a callback if there is activity in the set
of sockets. This saves memory as only one thread needs to be created in the
system.

The application supports IPv4 and IPv6, and uses QUIC protocol (:rfc:`9000`) to implement
the service. The source code for this sample application can be found at:
:zephyr_file:`samples/net/quic/echo_service`.

Requirements
************

- :ref:`networking_with_host`
- or, a board with hardware networking

Building and Running
********************

Build the Zephyr version of the quic/echo_service application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/quic/echo_service
   :board: <board_to_use>
   :goals: build
   :compact:

After the sample starts, it expects connections at 192.0.2.1, or 2001:db8::1
and port 4422.
The easiest way to connect is using helper echo-client script in net-tools project:

.. code-block:: console

    $ ./quic-echo-client.py --host 192.0.2.1 --max-size 1280 --packets 1
    $ ./quic-echo-client.py --host 2001:db8::1 --packets 10

After a connection is made, the application will echo back any line sent
to it. The quic-echo-client.py application will print statistics at the end.
