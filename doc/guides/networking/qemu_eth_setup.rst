.. _networking_with_eth_qemu:

Networking with QEMU Ethernet
#############################

.. contents::
    :local:
    :depth: 2

This page describes how to set up a virtual network between a (Linux) host
and a Zephyr application running in QEMU.

In this example, the :ref:`sockets-echo-server-sample` sample application from
the Zephyr source distribution is run in QEMU. The Zephyr instance is
connected to a Linux host using a tuntap device which is modeled in Linux as
an Ethernet network interface.

Prerequisites
*************

On the Linux Host, fetch the Zephyr ``net-tools`` project, which is located
in a separate Git repository:

.. code-block:: console

   git clone https://github.com/zephyrproject-rtos/net-tools


Basic Setup
***********

For the steps below, you will need two terminal windows:

* Terminal #1 is terminal window with net-tools being the current
  directory (``cd net-tools``)
* Terminal #2 is your usual Zephyr development terminal,
  with the Zephyr environment initialized.

When configuring the Zephyr instance, you must select the correct Ethernet
driver for QEMU connectivity:

* For ``qemu_x86``, select ``Intel(R) PRO/1000 Gigabit Ethernet driver``
  Ethernet driver. Driver is called ``e1000`` in Zephyr source tree.
* For ``qemu_cortex_m3``, select ``TI Stellaris MCU family ethernet driver``
  Ethernet driver. Driver is called ``stellaris`` in Zephyr source tree.
* For ``mps2_an385``, select ``SMSC911x/9220 Ethernet driver`` Ethernet driver.
  Driver is called ``smsc911x`` in Zephyr source tree.

Step 1 - Create Ethernet interface
==================================

Before starting QEMU with network connectivity, a network interface
should be created in the host system.

In terminal #1, type:

.. code-block:: console

   ./net-setup.sh

You can tweak the behavior of the ``net-setup.sh`` script. See various options
by running ``net-setup.sh`` like this:

.. code-block:: console

   ./net-setup.sh --help


Step 2 - Start app in QEMU board
================================

Build and start the :ref:`sockets-echo-server-sample` sample application.
In this example, the qemu_x86 board is used.

In terminal #2, type:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :host-os: unix
   :board: qemu_x86
   :gen-args: -DOVERLAY_CONFIG=overlay-e1000.conf
   :goals: run
   :compact:

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
