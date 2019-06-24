.. _sockets-echo-server-sample:

Socket Echo Server
##################

Overview
********

The echo-server sample application for Zephyr implements a UDP/TCP server
that complements the echo-client sample application: the echo-server listens
for incoming IPv4 or IPv6 packets (sent by the echo client) and simply sends
them back.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/echo_server`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

There are multiple ways to use this application. One of the most common
usage scenario is to run echo-server application inside QEMU. This is
described in :ref:`networking_with_qemu`.

There are configuration files for different boards and setups in the
echo-server directory:

- :file:`prj.conf`
  Generic config file, normally you should use this.

- :file:`overlay-ot.conf`
  This overlay config enables support for OpenThread.

- :file:`overlay-802154.conf`
  This overlay config enables support for native IEEE 802.15.4 connectivity.
  Note, that by default IEEE 802.15.4 L2 uses unacknowledged communication. To
  improve connection reliability, acknowledgments can be enabled with shell
  command: ``ieee802154 ack set``.

- :file:`overlay-bt.conf`
  This overlay config enables support for Bluetooth IPSP connectivity.

- :file:`overlay-qemu_802154.conf`
  This overlay config enables support for two QEMU's when simulating
  IEEE 802.15.4 network that are connected together.

- :file:`overlay-ppp.conf`
  This overlay config enables support for PPP (Point-to-Point Protocol).

- :file:`overlay-tls.conf`
  This overlay config enables support for TLS.

Build echo-server sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Example building for the nRF52840_pca10056 with OpenThread support:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :host-os: unix
   :board: nrf52840_pca10056
   :conf: "prj.conf overlay-ot.conf"
   :goals: run
   :compact:

Enabling TLS support
====================

Enable TLS support in the sample by building the project with the
``overlay-tls.conf`` overlay file enabled, for example, using these commands:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_server
   :board: qemu_x86
   :conf: "prj.conf overlay-tls.conf"
   :goals: build
   :compact:

An alternative way is to specify ``-DOVERLAY_CONFIG=overlay-tls.conf`` when
running ``west build`` or ``cmake``.

The certificate used by the sample can be found in the sample's ``src``
directory. The default certificates used by Socket Echo Server and
:ref:`sockets-echo-client-sample` enable establishing a secure connection
between the samples.

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

You can verify TLS communication with a Linux host as well. See
https://github.com/zephyrproject-rtos/net-tools documentation for information
on how to test TLS with Linux host samples.

See the :ref:`sockets-echo-client-sample` documentation for an alternate
way of running, with the echo-server on the Linux host and the echo-client
in QEMU.
