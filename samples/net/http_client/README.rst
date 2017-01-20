HTTP Client
###########

Overview
********

This sample application shows how to create HTTP 1.1 requests to
an HTTP server and how to parse the incoming responses.
Supported HTTP 1.1 methods are: GET, HEAD, OPTIONS and POST.

The source code for this sample application can be found at:
:file:`samples/net/http_client`.

Requirements
************

- Freedom Board (FRDM-K64F)
- LAN for testing purposes (Ethernet)
- Terminal emulator software
- HTTP Server

Building and Running
********************

Open the project configuration file for your platform, for example:
:file:`prj_frdm_k64f.conf` is the configuration file for the
:ref:`frdm_k64f` board. For IPv4 networks, set the following variables:

.. code-block:: console

	CONFIG_NET_IPV4=y
	CONFIG_NET_IPV6=n

IPv6 is the preferred routing technology for this sample application,
if CONFIG_NET_IPV6=y is set, the value of CONFIG_NET_IPV4=y is ignored.

In this sample application, only static IP addresses are supported,
those addresses are specified in the project configuration file,
for example:

.. code-block:: console

	CONFIG_NET_SAMPLES_MY_IPV6_ADDR="2001:db8::1"
	CONFIG_NET_SAMPLES_PEER_IPV6_ADDR="2001:db8::2"

are the IPv6 addresses for the HTTP client running Zephyr and the
HTTP server, respectively.

Alternatively, the IP addresses may be specified in the
:file:`src/config.h` file.

Open the :file:`src/config.h` file and set the server port
to match the HTTP server setup, for example:

.. code-block:: c

   #define SERVER_PORT		80

assumes that the HTTP server is listening at the TCP port 80.

HTTP Server
===========

Setting up an HTTP server on your host computer is beyond the scope
of this document.
(We used `Apache 2 <http://httpd.apache.org/docs/2.4/getting-started.html>`_
for testing this sample application.

However, this application assumes that there is a server's
resource that can process an HTTP 1.1 POST request.

For example, assuming that the Apache 2 server with PHP support
is used, and that the client sends a POST request with
"Content-Type = application/x-www-form-urlencoded" the following
PHP script will echo the POST payload back to the client:

.. code-block:: html

	<html>
	<head>
		<title>HTTP Server POST test</title>
	</head>
	<body>
		<?php
			echo '<p>POST key/values:</p>';
			foreach ($_POST as $key => $value) {
				echo "<p> {$key} : {$value} </p>";
			}
		?>
	</body>
	</html>

In the machine hosting the HTTP server, this php script is at
:file:`/var/www/html/post_test.php`. However, for your test machine
this path can be different, but should be at your server's root folder.

HTTP Responses
==============

Server's responses are processed by the http_receive_cb routine
defined inside the :file:`src/http_client_rcv.c` file.

This sample application only prints the HTTP header fields via
the HTTP Parser Library, see :file:`include/net/http_parser.h`.
To process the HTTP response's body, use the HTTP Parser's callbacks
to determine where the body begins. Depending on the payload's size,
it may be necessary to traverse the network buffer's fragment chain.
See the :file:`src/http_client_rcv.c` file at line 70 for sample code
that shows how to walk the fragment chain.

FRDM K64F
=========

Open a terminal window and type:

.. code-block:: console

    $ make BOARD=frdm_k64f

The FRDM K64F board is detected as a USB storage device. The board
must be mounted (i.e. to /mnt) to 'flash' the binary:

.. code-block:: console

    $ cp outdir/frdm_k64f/zephyr.bin /mnt

On Linux, use the 'dmesg' program to find the right USB device for the
FRDM serial console. Assuming that this device is ttyACM0, open a
terminal window and type:

.. code-block:: console

    $ screen /dev/ttyACM0 115200

Once the binary is loaded into the FRDM board, press the RESET button.

Refer to the board documentation in Zephyr, :ref:`frdm_k64f`,
for more information about this board and how to access the FRDM
serial console under other operating systems.

Sample Output
=============

This sample application loops a specified number of times doing four
HTTP 1.1 requests and displays the header fields that were extracted
from the server's response. The four requests are:

- GET "/index.html"
- HEAD "/"
- OPTIONS "/"
- POST "/post_test.php"

The terminal window where screen is running will show something similar
to the following:

.. code-block:: console

	*******************************************
	HTTP Client: 2001:db8::1
	Connecting to: 2001:db8::2 port 80
	Hostname: 2001:db8::2
	HTTP Request: GET

	--------- HTTP response (headers) ---------
	Date: Thu, 02 Feb 2017 00:51:31 GMT
	Server: Apache/2.4.10 (Debian)
	Last-Modified: Sat, 28 Jan 2017 02:55:09 GMT
	ETag: "3c-5471eb5db3c73"
	Accept-Ranges: bytes
	Content-Length: 60
	Connection: close
	Content-Type: text/html

	HTTP server response status: OK
	HTTP parser status: success
	HTTP body: 60 bytes, expected: 60 bytes

	*******************************************
	HTTP Client: 2001:db8::1
	Connecting to: 2001:db8::2 port 80
	Hostname: 2001:db8::2
	HTTP Request: HEAD

	--------- HTTP response (headers) ---------
	Date: Thu, 02 Feb 2017 00:51:37 GMT
	Server: Apache/2.4.10 (Debian)
	Last-Modified: Sat, 28 Jan 2017 02:55:09 GMT
	ETag: "3c-5471eb5db3c73"
	Accept-Ranges: bytes
	Content-Length: 60
	Connection: close
	Content-Type: text/html

	HTTP server response status: OK
	HTTP parser status: success

	*******************************************
	HTTP Client: 2001:db8::1
	Connecting to: 2001:db8::2 port 80
	Hostname: 2001:db8::2
	HTTP Request: OPTIONS

	--------- HTTP response (headers) ---------
	Date: Thu, 02 Feb 2017 00:51:43 GMT
	Server: Apache/2.4.10 (Debian)
	Allow: GET,HEAD,POST,OPTIONS
	Content-Length: 0
	Connection: close
	Content-Type: text/html

	HTTP server response status: OK
	HTTP parser status: success

	*******************************************
	HTTP Client: 2001:db8::1
	Connecting to: 2001:db8::2 port 80
	Hostname: 2001:db8::2
	HTTP Request: POST

	--------- HTTP response (headers) ---------
	Date: Thu, 02 Feb 2017 00:51:49 GMT
	Server: Apache/2.4.10 (Debian)
	Vary: Accept-Encoding
	Content-Length: 231
	Connection: close
	Content-Type: text/html; charset=UTF-8

	HTTP server response status: OK
	HTTP parser status: success
