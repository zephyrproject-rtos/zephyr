.. _coaps-server-sample:

CoAP over DTLS sample server
############################

Overview
********
This sample code shows a CoAP over DTLS server using mbedTLS on top of Zephyr.

Building and Running
********************

Follow the steps for testing :ref:`networking_with_qemu`.

In the application directory type:

.. code-block:: console

   $ make run

In other terminal window, obtain the libcoap code from:

.. code-block:: console

   $ git clone --recursive -b dtls https://github.com/obgm/libcoap.git

and put it in a well known directory, in your Linux machine, this will be your
client. In order to compile libcoap you may need to install the following
libraries, for Ubuntu

.. code-block:: console

   $ sudo apt-get install libtool asciidoc

Move to that directory and compile the libcoap on your host machine

.. code-block:: console

   $ ./autogen.sh
   $ ./configure --disable-shared
   $ make all

Now you can run the client like

.. code-block:: console

   $ cd examples
   $ ./coap-client -m get coaps://[2001:db8::1]/test -u Client_identity -k passwd

You will get the following output:

.. code-block:: console

	v:1 t:CON c:GET i:7154 {} [ ]
	decrypt_verify(): found 24 bytes cleartext
	decrypt_verify(): found 123 bytes cleartext
	Type: 0
	Code: 1
	MID: 29012

From the app directory type the screen should display

.. code-block:: console

	*******
	type: 0 code 1 id 29012
	*******
 	mbedtls_ssl_read returned -0x7780

If the server does not receive the  messages, restart the app and try to connect
the client again.

References
**********

* https://tls.mbed.org/
* https://libcoap.net/
