.. zephyr:code-sample:: sockets-net-mgmt
   :name: Network management socket
   :relevant-api: bsd_sockets net_if

   Listen to network management events using a network management socket.

Overview
********

The net-mgmt-socket sample application for Zephyr implements a listener
for network management events that the networking subsystem is sending.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/net_mgmt`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

There are multiple ways to use this application. One of the most common
usage scenario is to run echo-server application inside QEMU. This is
described in :ref:`networking_with_qemu`.

Build net-mgmt socket sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/net_mgmt
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Example building for the native_sim board:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/net_mgmt
   :host-os: unix
   :board: native_sim
   :conf: prj.conf
   :goals: run
   :compact:
