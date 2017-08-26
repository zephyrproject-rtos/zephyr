.. _websocket-server-sample:

Websocket Server
################

Overview
********

The websocket-server sample application for Zephyr implements a websocket
server. The websocket-server listens for incoming IPv4 or IPv6 HTTP(S)
requests and sends back the same data.

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

Build websocket-server sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/ws_echo_server
   :board: qemu_x86
   :goals: run
   :compact:

The default make BOARD configuration for this sample is ``qemu_x86``.

Connect to the websocket server from your browser using these URLs
http://[2001:db8::1] or http://192.0.2.1 as configured in the project's
``prj.conf`` file.
