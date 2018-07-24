.. _echo-client-sample:

Echo Client
###########

Overview
********

The echo-client sample application for Zephyr implements a UDP/TCP client
that will send IPv4 or IPv6 packets, wait for the data to be sent back,
and then verify it matches the data that was sent.

The source code for this sample application can be found at:
:file:`samples/net/echo_client`.

Requirements
************

- :ref:`networking_with_qemu`

Building and Running
********************

There are multiple ways to use this application. One of the most common
usage scenario is to run echo-client application inside QEMU. This is
described in :ref:`networking_with_qemu`.

There are configuration files for different boards and setups in the
echo-client directory:

- :file:`prj.conf`
  Generic config file, normally you should use this.

- :file:`overlay-frdm_k64f_cc2520.conf`
  This overlay config enables support for IEEE 802.15.4 CC2520 and frdm_k64f

- :file:`overlay-frdm_k64f_mcr20a.conf`
  This overlay config enables support for IEEE 802.15.4 mcr20a and frdm_k64f

- :file:`overlay-ot.conf`
  This overlay config enables support for OpenThread

- :file:`overlay-enc28j60.conf`
  This overlay config enables support for enc28j60 ethernet board. This
  add-on board can be used for example with Arduino 101 board.

- :file:`overlay-cc2520.conf`
  This overlay config enables support for IEEE 802.15.4 cc2520 chip.

- :file:`overlay-bt.conf`
  This overlay config enables support for Bluetooth IPSP connectivity.

- :file:`overlay-qemu_802154.conf`
  This overlay config enables support for two QEMU's when simulating
  IEEE 802.15.4 network that are connected together.

- :file:`overlay-tls.conf`
  This overlay config enables support for TLS.

Build echo-client sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/echo_client
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Example building for the FRDM-K64F with TI CC2520 support:

.. zephyr-app-commands::
   :zephyr-app: samples/net/echo_client
   :host-os: unix
   :board: frdm_k64f
   :conf: "prj.conf overlay-frdm_k64f_cc2520.conf"
   :goals: run
   :compact:

Cmake can select the default configuration file based on the BOARD you've
specified automatically so you might not always need to mention it.

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
   :zephyr-app: samples/net/echo_client
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:
