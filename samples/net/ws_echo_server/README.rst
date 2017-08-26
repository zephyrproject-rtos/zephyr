.. _websocket-server-sample:

Websocket Server
################

Overview
********

The websocket-server sample application for Zephyr implements a websocket
server. The websocket-server listens for incoming IPv4 or IPv6 HTTP(S)
requests.

The source code for this sample application can be found at:
:file:`samples/net/ws_server`.

Requirements
************

- :ref:`networking_with_qemu`

Building and Running
********************

There are multiple ways to use this application. One of the most common
usage scenario is to run websocket-server application inside QEMU. This is
described in :ref:`networking_with_qemu`.

Build echo-server sample application like this:

.. code-block:: console

    $ cd $ZEPHYR_BASE/samples/net/websocket
    $ make pristine && make run

The default make BOARD configuration for this sample is ``qemu_x86``.
