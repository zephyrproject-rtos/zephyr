.. _sockets-websocket-client-sample:

Socket Websocket Client
#######################

Overview
********

This sample application implements a Websocket client that will do an HTTP
or HTTPS handshake request to HTTP server, then start to send data and wait for
the responses from the Websocket server.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/websocket_client`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

You can use this application on a supported board, including
running it inside QEMU as described in :ref:`networking_with_qemu`.

Build websocket-client sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/websocket_client
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Enabling TLS support
====================

Enable TLS support in the sample by building the project with the
``overlay-tls.conf`` overlay file enabled using these commands:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/websocket_client
   :board: qemu_x86
   :conf: "prj.conf overlay-tls.conf"
   :goals: build
   :compact:

An alternative way is to specify ``-DOVERLAY_CONFIG=overlay-tls.conf`` when
running ``west build`` or ``cmake``.

The certificate and private key used by the sample can be found in the sample's
:zephyr_file:`samples/net/sockets/websocket_client/src/` directory.


Running websocket-server in Linux Host
======================================

You can run this ``websocket-client`` sample application in QEMU
and run the ``zephyr-websocket-server.py`` (from net-tools) on a Linux host.
Other alternative is to install `websocketd <http://websocketd.com/>`_ and
use that.

To use QEMU for testing, follow the :ref:`networking_with_qemu` guide.

In a terminal window you can do either:

.. code-block:: console

    $ ./zephyr-websocket-server.py

or

.. code-block:: console

    $ websocketd --port=9001 cat

Run ``websocket-client`` application in QEMU:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/websocket_client
   :host-os: unix
   :board: qemu_x86
   :conf: prj.conf
   :goals: run
   :compact:

Note that ``zephyr-websocket-server.py`` or ``websocketd`` must be running in
the Linux host terminal window before you start the ``websocket-client``
application in QEMU. Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Current version of ``zephyr-websocket-server.py`` found in
`net-tools <https://github.com/zephyrproject-rtos/net-tools>`_ project, does
not support TLS.
