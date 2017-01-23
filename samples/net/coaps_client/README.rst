CoAP over DTLS sample client
############################

Overview
========
This sample code shows a CoAP over DTLS client using mbedTLS on top of Zephyr.

Building and Running
====================

Follow the steps for testing :ref:`networking with Qemu <networking_with_qemu>`.

Run the server application at samples/net/coaps_server, with the following
command:

.. code-block:: console

	make server

In other terminal window, run this client application at samples/net/coaps_client:

.. code-block:: console

	make client

You will get the following output:

.. code-block:: console

	reply: 60 45 00 01 ff 54 79 70 65 3a 20 30 0a 43 6f 64 65 3a 20 31 0a 4d
	49 44 3a 20 31 0a 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
	00 00 00 00 00 (123 bytes)

From the server application directory the screen should display

.. code-block:: console

	*******
	type: 0 code 1 id 1
	*******
	connection was closed gracefully
	done

If the server does not receive the  messages, restart the app and try to connect
the client again.

References
==========

* https://wiki.zephyrproject.org/view/Networking-with-Qemu
