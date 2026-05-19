.. zephyr:code-sample:: quic-client-echo
   :name: Echo client (QUIC)
   :relevant-api: quic

   Implements a simple IPv4/IPv6 echo client using QUIC API and socket service API.

Overview
********

The quic/echo_client sample application for Zephyr implements a QUIC protocol
echo client supporting both IPv4 and IPv6 and using a BSD Sockets compatible API.

The purpose of this sample is to show how to use QUIC and service APIs.
The socket service is a concept where many blocking sockets can be listened by
one thread, and which can then trigger a callback if there is activity in the set
of sockets. This saves memory as only one thread needs to be created in the
system.

The application supports IPv4 and IPv6, and uses QUIC protocol (:rfc:`9000`) to implement
the service. The source code for this sample application can be found at:
:zephyr_file:`samples/net/quic/echo_client`.

Requirements
************

- :ref:`networking_with_host`
- or, a board with hardware networking

Building and Running
********************

Build the Zephyr version of the quic/echo_client application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/quic/echo_client
   :board: <board_to_use>
   :goals: build
   :compact:

After the sample starts, it tries to connect to 192.0.2.2, or 2001:db8::2
and port 4422.
The easiest way to listen these connections is using helper echo-server script in net-tools project:

.. code-block:: console

    $ ./quic-echo-server.py --host 192.0.2.2 --certificate quic-ca-cert.pem --private-key quic-private-key.pem --max-stream-data 8192
    $ ./quic-echo-server.py --host 2001:db8::1 --certificate quic-ca-cert.pem --private-key quic-private-key.pem

After a connection is made, the echo-client application will check that it received back
the data it sent to the echo-server application. The quic-echo-server.py application will
print statistics at the end.
