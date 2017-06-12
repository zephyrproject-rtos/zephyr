.. _mbedtsl-delsserver-sample:

mbedTLS DTLS sample server
############################

Overview
********
This sample code shows a simple DTLS server using mbedTLS on top of Zephyr.

Building and Running
********************

Follow the steps for testing :ref:`networking_with_qemu`.

In the application directory type:

.. code-block:: console

   $make run

.. code-block:: console

   . Seeding the random number generator... ok
   . Setting up the DTLS structure... ok
   . Setting connection
   ok
   . Setting up ecjpake password ... ok
   . Performing the TLS handshake...

In other terminal window, obtain the mbed TLS code from:

	https://tls.mbed.org/download/start/mbedtls-2.3.0-apache.tgz

and put it in a well known directory, in your Linux machine, this will be your
client.

Move to that directory and compile the mbedTLS on your host machine

.. code-block:: console

   tar -xvzf mbedtls-2.3.0-apache.tgz
   cd mbedtls-2.3.0
   CFLAGS="-I$PWD/configs -DMBEDTLS_CONFIG_FILE='<config-thread.h>'" make

   ./programs/ssl/ssl_client2 server_addr=192.0.2.1 dtls=1 ecjpake_pw=passwd

You will get the following output:

.. code-block:: console

   . Seeding the random number generator... ok
   . Connecting to udp/192.0.2.1/4433... ok
   . Setting up the SSL/TLS structure... ok
   . Performing the SSL/TLS handshake... ok
   [ Protocol is DTLSv1.2 ]
   [ Ciphersuite is TLS-ECJPAKE-WITH-AES-128-CCM-8 ]
   [ Record expansion is 29 ]
   [ Maximum fragment length is 16384 ]
   > Write to server: 34 bytes written in 1 fragments

   GET / HTTP/1.0
   Extra-header:


   < Read from server: 34 bytes read

   GET / HTTP/1.0
   Extra-header:

   . Closing the connection... done

From the app directory type the screen should display

.. code-block:: console

   . Performing the TLS handshake... hello verification requested
   . Setting up ecjpake password ... ok
   . Performing the TLS handshake... ok
   < Read from client: 34 bytes read

   GET / HTTP/1.0
   Extra-header:


   > Write to client: 34 bytes written

   GET / HTTP/1.0
   Extra-header:

   < Read from client: connection was closed gracefully
   . Closing the connection... done
   . Setting up ecjpake password ... ok
   . Performing the TLS handshake...

If trying to use IPv6 edit the file prj_qemu_x86.conf and change the line from
CONFIG_NET_IPV6=n to CONFIG_NET_IPV6=y

And run the client on mbedTLS as

.. code-block:: console

   ./programs/ssl/ssl_client2 server_addr=2001:db8::1 dtls=1 ecjpake_pw=passwd

If the server does not receive the  messages, restart the app and try to connect
the client again.

References
**********

* https://tls.mbed.org/
