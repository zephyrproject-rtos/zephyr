.. _mqtt_sn_socket_interface:

MQTT-SN
#######

.. contents::
    :local:
    :depth: 2

Overview
********

MQTT-SN is a variant of the well-known MQTT protocol - see :ref:`mqtt_socket_interface`.

In contrast to MQTT, MQTT-SN does not require a TCP transport, but is designed to be used
over any message-based transport. Originally, it was mainly created with ZigBee in mind,
but others like Bluetooth, UDP or even a UART can be used just as well.

Zephyr provides an MQTT-SN client library built on top of BSD sockets API. The
library can be enabled with :kconfig:option:`CONFIG_MQTT_SN_LIB` Kconfig option
and is configurable at a per-client basis, with support for MQTT-SN version
1.2. The Zephyr MQTT-SN implementation can be used with any message-based transport,
but support for UDP is already built-in.

MQTT-SN clients require an MQTT-SN gateway to connect to. These gateways translate between
MQTT-SN and MQTT. The Eclipse Paho project offers an implementation of a MQTT-SN gateway, but
others are available too.
https://www.eclipse.org/paho/index.php?page=components/mqtt-sn-transparent-gateway/index.php

The MQTT-SN spec v1.2 can be found here:
https://www.oasis-open.org/committees/download.php/66091/MQTT-SN_spec_v1.2.pdf

Sample usage
************

To create an MQTT-SN client, a client context structure and buffers need to be
defined:

.. code-block:: c

   /* Buffers for MQTT client. */
   static uint8_t rx_buffer[256];
   static uint8_t tx_buffer[256];

   /* MQTT-SN client context */
   static struct mqtt_sn_client client;

Multiple MQTT-SN client instances can be created in the application and managed
independently. Additionally, a structure for the transport is needed as well.
The library already comes with an example implementation for UDP.

.. code-block:: c

   /* MQTT Broker address information. */
   static struct mqtt_sn_transport tp;

The MQTT-SN library will inform clients about certain events using a callback.

.. code-block:: c

   static void evt_cb(struct mqtt_sn_client *client,
                      const struct mqtt_sn_evt *evt)
   {
      switch(evt->type) {
      {
         /* Handle events here. */
      }
   }

For a list of possible events, see :ref:`mqtt_sn_api_reference`.

The client context structure needs to be initialized and set up before it can be
used. An example configuration for UDP transport is shown below:

.. code-block:: c

   struct mqtt_sn_data client_id = MQTT_SN_DATA_STRING_LITERAL("ZEPHYR");
   struct sockaddr_in gateway = {0};

   uint8_t tx_buf[256];
   uint8_t rx_buf[256];

   mqtt_sn_transport_udp_init(&tp, (struct sockaddr*)&gateway, sizeof((gateway)));

   mqtt_sn_client_init(&client, &client_id, &tp.tp, evt_cb, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));

After the configuration is set up, the MQTT-SN client can connect to the gateway.
While the MQTT-SN protocol offers functionality to discover gateways through an
advertisement mechanism, this is not implemented yet in the library.

Call the ``mqtt_sn_connect`` function, which will send a ``CONNECT`` message.
The application should periodically call the ``mqtt_sn_input`` function to process
the response received. The application does not have to call ``mqtt_sn_input`` if it
knows that no data has been received (e.g. when using Bluetooth). Note that
``mqtt_sn_input`` is a non-blocking function, if the transport struct contains a
``poll`` compatible function pointer.
If the connection was successful, ``MQTT_SN_EVT_CONNECTED`` will be notified to the
application through the callback function.

.. code-block:: c

	err = mqtt_sn_connect(&client, false, true);
	__ASSERT(err == 0, "mqtt_sn_connect() failed %d", err);

	while (1) {
		mqtt_sn_input(&client);
		if (connected) {
			mqtt_sn_publish(&client, MQTT_SN_QOS_0, &topic_p, false, &pubdata);
		}
		k_sleep(K_MSEC(500));
	}

In the above code snippet, the event handler function should set the ``connected``
flag upon a successful connection. If the connection fails at the MQTT level
or a timeout occurs, the connection will be aborted.

After the connection is established, an application needs to call ``mqtt_input``
function periodically to process incoming data. Connection upkeep, on the other hand,
is done automatically using a k_work item.
If a MQTT message is received, an MQTT callback function will be called and an
appropriate event notified.

The connection can be closed by calling the ``mqtt_sn_disconnect`` function. This
has no effect on the transport, however. If you want to close the transport (e.g.
the socket), call ``mqtt_sn_client_deinit``, which will deinit the transport as well.

Zephyr provides sample code utilizing the MQTT-SN client API. See
:zephyr:code-sample:`mqtt-sn-publisher` for more information.

Deviations from the standard
****************************

Certain parts of the protocol are not yet supported in the library.

* Pre-defined topic IDs
* QoS -1 - it's most useful with predefined topics
* Gateway discovery using ADVERTISE, SEARCHGW and GWINFO messages.
* Setting the will topic and message after the initial connect
* Forwarder Encapsulation

.. _mqtt_sn_api_reference:

API Reference
*************

.. doxygengroup:: mqtt_sn_socket
