.. _sockets-echo-client-sample:

Socket Echo Client
##################

Overview
********

The echo-client sample application for Zephyr implements a UDP/TCP client
that will send IPv4 or IPv6 packets, wait for the data to be sent back,
and then verify it matches the data that was sent.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/echo_client`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

There are multiple ways to use this application. One of the most common
usage scenario is to run echo-client application inside QEMU. This is
described in :ref:`networking_with_qemu`.

There are configuration files for different boards and setups in the
echo-client directory:

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

- :file:`overlay-tls.conf`
  This overlay config enables support for TLS.

Build echo-client sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_client
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Example building for the nRF52840_pca10056 with OpenThread support:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_client
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
   :zephyr-app: samples/net/sockets/echo_client
   :board: qemu_x86
   :conf: "prj.conf overlay-tls.conf"
   :goals: build
   :compact:

An alternative way is to specify ``-DOVERLAY_CONFIG=overlay-tls.conf`` when
running ``west build`` or ``cmake``.

The certificate and private key used by the sample can be found in the sample's
``src`` directory. The default certificates used by Socket Echo Client and
:ref:`sockets-echo-server-sample` enable establishing a secure connection
between the samples.

SOCKS5 proxy support
====================

It is also possible to connect to the echo-server through a SOCKS5 proxy.
To enable it, use ``-DOVERLAY_CONFIG=overlay-socks5.conf`` when running ``west
build`` or  ``cmake``.

By default, to make the testing easier, the proxy is expected to run on the
same host as the echo-server in Linux host.

To start a proxy server, for example a builtin SOCKS server support in ssh
can be used (-D option). Use the following command to run it on your host
with the default port:

For IPv4 proxy server:

.. code-block: console

        $ ssh -N -D 0.0.0.0:1080 localhost

For IPv6 proxy server:

.. code-block: console

        $ ssh -N -D [::]:1080 localhost

Run both commands if you are testing IPv4 and IPv6.

To connect to a proxy server that is not running under the same IP as the
echo-server or uses a different port number, modify the following values
in echo_client/src/tcp.c.

.. code-block:: c

        #define SOCKS5_PROXY_V4_ADDR IPV4_ADDR
        #define SOCKS5_PROXY_V6_ADDR IPV6_ADDR
        #define SOCKS5_PROXY_PORT    1080

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
   :conf: "prj.conf overlay-linux.conf"
   :goals: run
   :compact:

Note that echo-server must be running in the Linux host terminal window
before you start the echo-client application in QEMU.
Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

You can verify TLS communication with a Linux host as well. See
https://github.com/zephyrproject-rtos/net-tools documentation for information
on how to test TLS with Linux host samples.

See the :ref:`sockets-echo-server-sample` documentation for an alternate
way of running, with the echo-client on the Linux host and the echo-server
in QEMU.
