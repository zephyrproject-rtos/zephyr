.. _mqtt_socket_interface:

MQTT
####

.. contents::
    :local:
    :depth: 2

Overview
********

MQTT (Message Queuing Telemetry Transport) is an application layer protocol
which works on top of the TCP/IP stack. It is a lightweight
publish/subscribe messaging transport for machine-to-machine communication.
For more information about the protocol itself, see http://mqtt.org/.

Zephyr provides an MQTT client library built on top of BSD sockets API. The
library is configurable at a per-client basis, with support for MQTT versions
3.1.0 and 3.1.1. The Zephyr MQTT implementation can be used with either plain
sockets communicating over TCP, or with secure sockets communicating over
TLS. See :ref:`bsd_sockets_interface` for more information about Zephyr sockets.

MQTT clients require an MQTT server to connect to. Such a server, called an MQTT Broker,
is responsible for managing client subscriptions and distributing messages
published by clients. There are many implementations of MQTT brokers, one of them
being Eclipse Mosquitto. See https://mosquitto.org/ for more information about
the Eclipse Mosquitto project.

Sample usage
************

To create an MQTT client, a client context structure and buffers need to be
defined:

.. code-block:: c

   /* Buffers for MQTT client. */
   static uint8_t rx_buffer[256];
   static uint8_t tx_buffer[256];

   /* MQTT client context */
   static struct mqtt_client client_ctx;

Multiple MQTT client instances can be created in the application and managed
independently. Additionally, a structure for MQTT Broker address information
is needed. This structure must be accessible throughout the lifespan
of the MQTT client and can be shared among MQTT clients:

.. code-block:: c

   /* MQTT Broker address information. */
   static struct sockaddr_storage broker;

An MQTT client library will notify MQTT events to the application through a
callback function created to handle respective events:

.. code-block:: c

   void mqtt_evt_handler(struct mqtt_client *client,
                         const struct mqtt_evt *evt)
   {
      switch (evt->type) {
         /* Handle events here. */
      }
   }

For a list of possible events, see :ref:`mqtt_api_reference`.

The client context structure needs to be initialized and set up before it can be
used. An example configuration for TCP transport is shown below:

.. code-block:: c

   mqtt_client_init(&client_ctx);

   /* MQTT client configuration */
   client_ctx.broker = &broker;
   client_ctx.evt_cb = mqtt_evt_handler;
   client_ctx.client_id.utf8 = (uint8_t *)"zephyr_mqtt_client";
   client_ctx.client_id.size = sizeof("zephyr_mqtt_client") - 1;
   client_ctx.password = NULL;
   client_ctx.user_name = NULL;
   client_ctx.protocol_version = MQTT_VERSION_3_1_1;
   client_ctx.transport.type = MQTT_TRANSPORT_NON_SECURE;

   /* MQTT buffers configuration */
   client_ctx.rx_buf = rx_buffer;
   client_ctx.rx_buf_size = sizeof(rx_buffer);
   client_ctx.tx_buf = tx_buffer;
   client_ctx.tx_buf_size = sizeof(tx_buffer);

After the configuration is set up, the MQTT client can connect to the MQTT broker.
Call the ``mqtt_connect`` function, which will create the appropriate socket,
establish a TCP/TLS connection, and send an ``MQTT CONNECT`` message.
When notified, the application should call the ``mqtt_input`` function to process
the response received. Note, that ``mqtt_input`` is a non-blocking function,
therefore the application should use socket ``poll`` to wait for the response.
If the connection was successful, ``MQTT_EVT_CONNACK`` will be notified to the
application through the callback function.

.. code-block:: c

   rc = mqtt_connect(&client_ctx);
   if (rc != 0) {
      return rc;
   }

   fds[0].fd = client_ctx.transport.tcp.sock;
   fds[0].events = ZSOCK_POLLIN;
   poll(fds, 1, K_MSEC(5000));

   mqtt_input(&client_ctx);

   if (!connected) {
      mqtt_abort(&client_ctx);
   }

In the above code snippet, the MQTT callback function should set the ``connected``
flag upon a successful connection. If the connection fails at the MQTT level
or a timeout occurs, the connection will be aborted, and the underlying socket
closed.

After the connection is established, an application needs to call ``mqtt_input``
and ``mqtt_live`` functions periodically to process incoming data and upkeep
the connection. If an MQTT message is received, an MQTT callback function will
be called and an appropriate event notified.

The connection can be closed by calling the ``mqtt_disconnect`` function.

Zephyr provides sample code utilizing the MQTT client API. See
:ref:`mqtt-publisher-sample` for more information.

Using MQTT with TLS
*******************

The Zephyr MQTT library can be used with TLS transport for secure communication
by selecting a secure transport type (``MQTT_TRANSPORT_SECURE``) and some
additional configuration information:

.. code-block:: c

   client_ctx.transport.type = MQTT_TRANSPORT_SECURE;

   struct mqtt_sec_config *tls_config = &client_ctx.transport.tls.config;

   tls_config->peer_verify = TLS_PEER_VERIFY_REQUIRED;
   tls_config->cipher_list = NULL;
   tls_config->sec_tag_list = m_sec_tags;
   tls_config->sec_tag_count = ARRAY_SIZE(m_sec_tags);
   tls_config->hostname = MQTT_BROKER_HOSTNAME;

In this sample code, the ``m_sec_tags`` array holds a list of tags, referencing TLS
credentials that the MQTT library should use for authentication. We do not specify
``cipher_list``, to allow the use of all cipher suites available in the system.
We set ``hostname`` field to broker hostname, which is required for server
authentication. Finally, we enforce peer certificate verification by setting
the ``peer_verify`` field.

Note, that TLS credentials referenced by the ``m_sec_tags`` array must be
registered in the system first. For more information on how to do that, refer
to :ref:`secure sockets documentation <secure_sockets_interface>`.

An example of how to use TLS with MQTT is also present in
:ref:`mqtt-publisher-sample`.

.. _mqtt_api_reference:

API Reference
*************

.. doxygengroup:: mqtt_socket
