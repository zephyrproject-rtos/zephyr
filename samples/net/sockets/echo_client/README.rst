.. _sockets-echo-client-sample:

Echo Client
###########

Overview
********

The echo-client sample application for Zephyr implements a UDP/TCP client
that will send IPv4 or IPv6 packets, wait for the data to be sent back,
and then verify it matches the data that was sent.

This sample is a port of the :ref:`echo-client-sample`. It has been rewritten
to use socket API instead of native net-app API.

The source code for this sample application can be found at:
:file:`samples/net/sockets/echo_client`.

Requirements
************

- :ref:`networking_with_qemu`

Building and Running
********************

There are multiple ways to use this application. One of the most common
usage scenario is to run echo-client application inside QEMU. This is
described in :ref:`networking_with_qemu`.

Build echo-client sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_client
   :board: <board_to_use>
   :goals: build
   :compact:

``board_to_use`` defaults to ``qemu_x86``. In this case, you can run the
application in QEMU using ``make run``. If you used another BOARD, you
will need to consult its documentation for application deployment
instructions. You can read about Zephyr support for specific boards in
the documentation at :ref:`boards`.

Running echo-server in Linux Host
=================================

There is one useful testing scenario that can be used with Linux host.
Here echo-client is run in QEMU and echo-server is run in Linux host.

To use QEMU for testing, follow the :ref:`networking_with_qemu` guide.

In a terminal window:

.. code-block:: console

    $ sudo ./echo-server -i tap0

Run echo-client application in QEMU:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_client
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Note that echo-server must be running in the Linux host terminal window
before you start the echo-client application in QEMU.

See the :ref:`sockets-echo-server-sample` documentation for an alternate
way of running, with the echo-client on the Linux host and the echo-server
in QEMU.
