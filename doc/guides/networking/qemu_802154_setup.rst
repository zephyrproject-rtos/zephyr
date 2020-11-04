.. _networking_with_ieee802154_qemu:

Networking with QEMU and IEEE 802.15.4
######################################

.. contents::
    :local:
    :depth: 2

This page describes how to set up a virtual network between two QEMUs that
are connected together via UART and are running IEEE 802.15.4 link layer
between them. Note that this only works in Linux host.

Basic Setup
***********

For the steps below, you will need two terminal windows:

* Terminal #1 is terminal window with ``echo-server`` Zephyr sample application.
* Terminal #2 is terminal window with ``echo-client`` Zephyr sample application.

Step 1 - Compile and start echo-server
======================================

In terminal #1, type:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :host-os: unix
   :board: qemu_x86
   :build-dir: server
   :gen-args: -DOVERLAY_CONFIG=overlay-qemu_802154.conf
   :goals: server
   :compact:

Step 2 - Compile and start echo-client
======================================

In terminal #2, type:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_client
   :host-os: unix
   :board: qemu_x86
   :build-dir: client
   :gen-args: -DOVERLAY_CONFIG=overlay-qemu_802154.conf
   :goals: client
   :compact:

You should see data passed between the two QEMUs.
Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
