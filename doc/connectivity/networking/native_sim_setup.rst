.. _networking_with_native_sim:

Networking with native_sim board
################################

.. contents::
    :local:
    :depth: 2

This page describes how to set up a virtual network between a (Linux) host
and a Zephyr application running in a :ref:`native_sim <native_sim>` board.

In this example, the :zephyr:code-sample:`sockets-echo-server` sample application from
the Zephyr source distribution is run in native_sim board. The Zephyr
native_sim board instance is connected to a Linux host using a tuntap device
which is modeled in Linux as an Ethernet network interface.

Prerequisites
*************

On the Linux Host, fetch the Zephyr ``net-tools`` project, which is located
in a separate Git repository:

.. code-block:: console

   git clone https://github.com/zephyrproject-rtos/net-tools


Basic Setup
***********

For the steps below, you will need three terminal windows:

* Terminal #1 is terminal window with net-tools being the current
  directory (``cd net-tools``)
* Terminal #2 is your usual Zephyr development terminal,
  with the Zephyr environment initialized.
* Terminal #3 is the console to the running Zephyr native_sim
  instance (optional).

Step 1 - Create Ethernet interface
==================================

Before starting native_sim with network emulation, a network interface
should be created.

In terminal #1, type:

.. code-block:: console

   ./net-setup.sh

You can tweak the behavior of the net-setup.sh script. See various options
by running ``net-setup.sh`` like this:

.. code-block:: console

   ./net-setup.sh --help


Step 2 - Start app in native_sim board
======================================

Build and start the ``echo_server`` sample application.

In terminal #2, type:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:


Step 3 - Connect to console (optional)
======================================

The console window should be launched automatically when the Zephyr instance is
started but if it does not show up, you can manually connect to the console.
The native_sim board will print a string like this when it starts:

.. code-block:: console

   UART connected to pseudotty: /dev/pts/5

You can manually connect to it like this:

.. code-block:: console

   screen /dev/pts/5
