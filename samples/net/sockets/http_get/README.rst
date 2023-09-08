.. zephyr:code-sample:: sockets-http-get
   :name: HTTP GET using plain sockets
   :relevant-api: bsd_sockets tls_credentials secure_sockets_options

   Implement an HTTP(S) client using plain BSD sockets.

Overview
********

The sockets/http_get sample application for Zephyr implements a simple
HTTP GET client using a BSD Sockets compatible API. The purpose of this
sample is to show how it's possible to develop a sockets application
portable to both POSIX and Zephyr. As such, it is kept minimal and
supports only IPv4.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/http_get`.

Requirements
************

- :ref:`networking_with_host`
- or, a board with hardware networking
- NAT/routing should be set up to allow connections to the Internet
- DNS server should be available on the host to resolve domain names

Building and Running
********************

Build the Zephyr version of the application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/http_get
   :board: <board_to_use>
   :goals: build
   :compact:

After the sample starts, it issues HTTP GET request to "google.com:80"
and dumps the response. You can edit the source code to issue a request
to any other site on the Internet (or on the local network, in which
case no NAT/routing setup is needed).
Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Enabling TLS support
=================================

Enable TLS support in the sample by building the project with the
``overlay-tls.conf`` overlay file enabled, for example, using these commands:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/http_get
   :board: qemu_x86
   :conf: "prj.conf overlay-tls.conf"
   :goals: build
   :compact:

An alternative way is to specify ``-DEXTRA_CONF_FILE=overlay-tls.conf`` when
running ``west build`` or ``cmake``.

For boards that support TLS offloading (e.g. TI's cc3220sf_launchxl), use
``overlay-tls-offload.conf`` instead of ``overlay-tls.conf``.

The certificate used by the sample can be found in the sample's ``src``
directory. The certificate was selected to enable access to the default website
configured in the sample (https://google.com). To access a different web page
over TLS, provide an appropriate certificate to authenticate to that web server.

Note, that TLS support in the sample depends on non-posix, TLS socket
functionality. Therefore, it is only possible to run TLS in this sample
on Zephyr.

Running application on POSIX Host
=================================

The same application source code can be built for a POSIX system, e.g.
Linux. (Note: if you look at the source, you will see that the code is
the same except the header files are different for Zephyr vs POSIX.)

To build for a host POSIX OS:

.. code-block:: console

    $ make -f Makefile.posix

To run:

.. code-block:: console

    $ ./http_get

As can be seen, the behavior of the application is the same as the Zephyr
version.
