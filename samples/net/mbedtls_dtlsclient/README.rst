.. _mbedtls-dtls-client-sample:

mbedTLS DTLS client
####################

Overview
********
This sample code shows a simple DTLS client using mbed TLS on top of Zephyr

Building and running
********************

Follow the steps for testing :ref:`networking_with_qemu`.

Obtain the mbed TLS code from:

https://tls.mbed.org/download/start/mbedtls-2.3.0-apache.tgz

and put it in a well known directory on your Linux machine, this will be your
server.

change to that directory and compile the mbedTLS on your host machine:

.. code-block:: console

   $ tar -xvzf mbedtls-2.3.0-apache.tgz
   $ cd mbedtls-2.3.0
   $ CFLAGS="-I$PWD/configs -DMBEDTLS_CONFIG_FILE='<config-thread.h>'" make

Assign the server IP address and start the DTLS server.

.. code-block:: console

   $ sudo ip addr add 192.0.2.2/24 dev tap0
   $ ./programs/ssl/ssl_server2 dtls=1 ecjpake_pw=passwd

.. code-block:: console

   . Seeding the random number generator... ok
   . Bind on udp://*:4433/ ... ok
   . Setting up the SSL/TLS structure... ok
   . Waiting for a remote connection ...

To stop the server use Ctrl-C and repeat steps described in f) every time
QEMU gets terminated, due the Netwrok interface (tap) being restarted.

From the application directory type

.. code-block:: console

   $ make run

This will result in QEMU running with the following output:

.. code-block:: console

	. Seeding the random number generator... ok
	. Setting up the DTLS structure... ok
	. Connecting to udp 192.0.2.2:4433... ok
	. Setting up ecjpake password ... ok
	. Performing the SSL/TLS handshake... ok
	> Write to server: ok
	. Closing the connection... done

On the server side you should see this

.. code-block:: console

	. Performing the SSL/TLS handshake... hello verification requested
	. Waiting for a remote connection ... ok
	. Performing the SSL/TLS handshake... ok
	[ Protocol is DTLSv1.2 ]
	[ Ciphersuite is TLS-ECJPAKE-WITH-AES-128-CCM-8 ]
	[ Record expansion is 29 ]
	[ Maximum fragment length is 16384 ]
	< Read from client: 18 bytes read

	GET / HTTP/1.0

	> Write to client: 143 bytes written in 1 fragments


	HTTP/1.0 200 OK
	Content-Type: text/html

	<h2>mbed TLS Test Server</h2>
	<p>Successful connection using: TLS-ECJPAKE-WITH-AES-128-CCM-8</p>

	. Closing the connection... done
	. Waiting for a remote connection ... ok
	. Performing the SSL/TLS handshake... failed
	! mbedtls_ssl_handshake returned -0x7900

	. Waiting for a remote connection ...

Disregard the last handshake failed message, due the closing connection.

If the server does not receive the  messages, use a network traffic analyzer,
like Wireshark.

Reset the board.

References
**********

- https://tls.mbed.org/
