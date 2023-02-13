.. _net-promiscuous-mode-sample:

Promiscuous Mode Sample Application
###################################

Overview
********

This application will enable promiscuous mode for every network
interface in the system. It will then start to listen for incoming
network packets and show information about them.

The application will also provide a shell so that user can enable
or disable promiscuous mode at runtime. The commands are called
``promisc on`` and ``promisc off``.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

There are multiple ways to use this application. In this example QEMU
is used:

.. zephyr-app-commands::
   :zephyr-app: samples/net/promiscuous_mode
   :board: qemu_x86
   :conf: <config file to use>
   :goals: build
   :compact:
