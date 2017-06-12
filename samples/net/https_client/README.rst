.. _https-client-sample:

HTTPS Client
############

Overview
********

This sample application shows how to create encrypted HTTP 1.1 requests
to an HTTPS server and how to parse the incoming responses.
Supported HTTP 1.1 methods are: GET, HEAD, OPTIONS and POST.

The source code for this sample application can be found at:
:file:`samples/net/https_client`.

Requirements
************

- :ref:`networking_with_qemu`
- Terminal emulator software
- HTTPS Server
- DNS server (optional)

Building and Running
********************

Open the project configuration file for your platform, for example:
:file:`prj_qemu_x86.conf` is the configuration file for QEMU.

To use QEMU for testing, follow the :ref:`networking_with_qemu` guide.

For IPv4 networks, set the following variables:

.. code-block:: console

	CONFIG_NET_IPV4=y
	CONFIG_NET_IPV6=n

IPv6 is the preferred routing technology for this sample application,

In this sample application, both static IP addresses and DHCPv4 are supported.
Static IP addresses are specified in the project configuration file,
for example:

.. code-block:: console

	CONFIG_NET_APP_MY_IPV6_ADDR="2001:db8::1"
	CONFIG_NET_APP_PEER_IPV6_ADDR="2001:db8::2"

are the IPv6 addresses for the HTTPS client running Zephyr and the
HTTPS server, respectively. The application also supports DNS resolving so the
peer address is resolved automatically if host name is given, so you
can also write the HTTPS server name like this:

.. code-block:: console

	CONFIG_NET_APP_MY_IPV6_ADDR="2001:db8::1"
	CONFIG_NET_APP_PEER_IPV6_ADDR="6.zephyr.test"

Open the :file:`src/config.h` file and set the server port
to match the HTTP server setup, for example:

.. code-block:: c

   #define SERVER_PORT		443

assumes that the HTTPS server is listening at the TCP port 443.
If the default example HTTPS server is used (described in the next section),
then the default port is 4443.

HTTPS Server
============

Sample code for a very simple HTTPS server can be downloaded from the
zephyrproject-rtos/net-tools project area:
https://github.com/zephyrproject-rtos/net-tools

Open a terminal window and type:

.. code-block:: console

   $ cd net-tools
   $ ./https-server.sh


DNS setup
=========

The net-tools project provides a simple DNS resolver. You can activate
it like this if you want to test DNS resolving with the HTTP client.

Open a terminal window and type:

.. code-block:: console

    $ cd net-tools
    $ ./dnsmasq.sh


Sample Output
=============

This sample application loops a specified number of times doing several
HTTP 1.1 requests and printing some output. The requests are:

- GET "/index.html"
- HEAD "/"
- OPTIONS "/index.html"
- POST "/post_test.php"

The terminal window where QEMU is running will show something similar
to the following:

.. code-block:: console

   [https-client] [INF] response: Received 364 bytes piece of data
   [https-client] [INF] response: HTTP server response status: OK
   [https-client] [INF] response: HTTP body: 178 bytes, expected: 178 bytes
   [https-client] [INF] response: HTTP server response status: OK
   [https-client] [INF] response: HTTP server response status: Unsupported method ('OPTIONS')
   [https-client] [INF] response: Received 163 bytes piece of data
   [https-client] [INF] response: HTTP server response status: OK
   [https-client] [INF] response: HTTP body: 24 bytes, expected: 24 bytes
