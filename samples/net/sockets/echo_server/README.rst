.. _sockets-echo-server-sample:

Echo Server
###########

Overview
********

The echo-server sample application for Zephyr implements a UDP/TCP server
that complements the echo-client sample application: the echo-server listens
for incoming IPv4 or IPv6 packets (sent by the echo client) and simply sends
them back.

This sample is a port of the :ref:`echo-server-sample`. It has been rewritten
to use socket API instead of native net-app API.

The source code for this sample application can be found at:
:file:`samples/net/sockets/echo_server`.

Requirements
************

- :ref:`networking_with_qemu`

Building and Running
********************

There are multiple ways to use this application. One of the most common
usage scenario is to run echo-server application inside QEMU. This is
described in :ref:`networking_with_qemu`.

Build echo-server sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :board: <board_to_use>
   :goals: build
   :compact:

``board_to_use`` defaults to ``qemu_x86``. In this case, you can run the
application in QEMU using ``make run``. If you used another BOARD, you
will need to consult its documentation for application deployment
instructions. You can read about Zephyr support for specific boards in
the documentation at :ref:`boards`.

Running echo-client in Linux Host
=================================

There is one useful testing scenario that can be used with Linux host.
Here echo-server is run in QEMU and echo-client is run in Linux host.

To use QEMU for testing, follow the :ref:`networking_with_qemu` guide.

Run echo-server application in QEMU:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

In a terminal window:

.. code-block:: console

    $ sudo ./echo-client -i tap0 2001:db8::1

Note that echo-server must be running in QEMU before you start the
echo-client application in host terminal window.

See the :ref:`sockets-echo-client-sample` documentation for an alternate
way of running, with the echo-server on the Linux host and the echo-client
in QEMU.
