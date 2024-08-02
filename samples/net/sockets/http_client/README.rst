.. zephyr:code-sample:: sockets-http-client
   :name: HTTP Client
   :relevant-api: bsd_sockets http_client tls_credentials secure_sockets_options

   Implement an HTTP(S) client that issues a variety of HTTP requests.

Overview
********

This sample application implements an HTTP(S) client that will do an HTTP
or HTTPS request and wait for the response from the HTTP server.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/http_client`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

You can use this application on a supported board, including
running it inside QEMU as described in :ref:`networking_with_qemu`.

Build the http-client sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/http_client
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Enabling TLS support
====================

Enable TLS support in the sample by building the project with the
``overlay-tls.conf`` overlay file enabled using these commands:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/http_client
   :board: qemu_x86
   :conf: "prj.conf overlay-tls.conf"
   :goals: build
   :compact:

An alternative way is to specify ``-DEXTRA_CONF_FILE=overlay-tls.conf`` when
running ``west build`` or ``cmake``.

The certificate and private key used by the sample can be found in the sample's
:zephyr_file:`samples/net/sockets/http_client/src/` directory.
The default certificates used by Socket HTTP Client and
``https-server.py`` program found in the
`net-tools <https://github.com/zephyrproject-rtos/net-tools>`_ project, enable
establishing a secure connection between the samples.


Running http-server in Linux Host
=================================

You can run this  ``http-client`` sample application in QEMU
and run the ``http-server.py`` (from net-tools) on a Linux host.

To use QEMU for testing, follow the :ref:`networking_with_qemu` guide.

In a terminal window:

.. code-block:: console

    $ ./http-server.py

Run ``http-client`` application in QEMU:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/http_client
   :host-os: unix
   :board: qemu_x86
   :conf: prj.conf
   :goals: run
   :compact:

Note that ``http-server.py`` must be running in the Linux host terminal window
before you start the http-client application in QEMU.
Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

You can verify TLS communication with a Linux host as well. Just use the
``https-server.py`` program in net-tools project.
