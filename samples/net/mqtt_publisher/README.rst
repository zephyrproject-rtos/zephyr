.. _mqtt-publisher-sample:

MQTT Publisher
##############

Overview
********

`MQTT <http://mqtt.org/>`_ (MQ Telemetry Transport) is a lightweight
publish/subscribe messaging protocol optimized for small sensors and
mobile devices.

The Zephyr MQTT Publisher sample application is a MQTT v3.1.1
client that sends MQTT PUBLISH messages to a MQTT broker.
See the `MQTT V3.1.1 spec`_ for more information.

.. _MQTT V3.1.1 spec: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html

The source code of this sample application can be found at:
:file:`samples/net/mqtt_publisher`.

Requirements
************

- Linux machine
- Freedom Board (FRDM-K64F)
- Mosquitto server: any version that supports MQTT v3.1.1. This sample
  was tested with mosquitto 1.3.4.
- Mosquitto subscriber
- LAN for testing purposes (Ethernet)

Build and Running
*****************

Currently, this sample application only supports static IP addresses.
Open the :file:`src/config.h` file and set the IP addresses according
to the LAN environment.
Alternatively, set the IP addresses in the :file:`prj_frdm_k64f.conf` file.

This file :file:`src/config.h` also contains some variables that may be changed:

MQTT broker TCP port:

.. code-block:: c

	#define SERVER_PORT		1883

Application sleep time:

.. code-block:: c

	#define APP_SLEEP_MSECS		500

Application RX and TX timeout:

.. code-block:: c

	#define APP_TX_RX_TIMEOUT       300

Max number of connection tries:

.. code-block:: c

	#define APP_CONNECT_TRIES	10

Max number of MQTT PUBLISH iterations:

.. code-block:: c

	#define APP_MAX_ITERATIONS	5

MQTT Client Identifier:

.. code-block:: c

	#define MQTT_CLIENTID		"zephyr_publisher"

This sample application supports the IBM Bluemix Watson topic format that can
be enabled by changing the default value of APP_BLUEMIX_TOPIC from 0 to 1:

.. code-block:: c

	#define APP_BLUEMIX_TOPIC	1

The Bluemix topic may include some parameters like device type, device
identifier, event type and message format. This application uses the
following macros to specify those values:

.. code-block:: c

	#define BLUEMIX_DEVTYPE		"sensor"
	#define BLUEMIX_DEVID		"carbon"
	#define BLUEMIX_EVENT		"status"
	#define BLUEMIX_FORMAT		"json"

On your Linux host computer, open a terminal window, locate the source code
of this sample application (i.e. :file:`samples/net/mqtt_publisher`) and type:

.. code-block:: console

	make BOARD=frdm_k64f

Open another terminal window and type:

.. code-block:: console

	sudo mosquitto -v -p 1883

Open another terminal window and type:

.. code-block:: console

	mosquito_sub -t sensors

Sample output
=============

This is the output from the FRDM UART console, with:

.. code-block:: c

	#define APP_MAX_ITERATIONS     5

.. code-block:: console

	[dev/eth_mcux] [INF] eth_0_init: Enabled 100M full-duplex mode.
	[dev/eth_mcux] [DBG] eth_0_init: MAC 00:04:9f:3e:1a:0a
	[publisher:233] network_setup: 0 <OK>
	[publisher:258] mqtt_init: 0 <OK>
	[connect_cb:81] user_data: CONNECTED
	[try_to_connect:212] mqtt_tx_connect: 0 <OK>
	[publisher:276] try_to_connect: 0 <OK>
	[publisher:285] mqtt_tx_pingreq: 0 <OK>
	[publisher:290] mqtt_tx_publish: 0 <OK>
	[publish_cb:149] <MQTT_PUBACK> packet id: 1888, user_data: PUBLISH
	[publisher:295] mqtt_tx_publish: 0 <OK>
	[publish_cb:149] <MQTT_PUBREC> packet id: 16356, user_data: PUBLISH
	[publish_cb:149] <MQTT_PUBCOMP> packet id: 16356, user_data: PUBLISH
	[publisher:300] mqtt_tx_publish: 0 <OK>
	[publisher:285] mqtt_tx_pingreq: 0 <OK>
	[publisher:290] mqtt_tx_publish: 0 <OK>
	[publish_cb:149] <MQTT_PUBACK> packet id: 45861, user_data: PUBLISH
	[publisher:295] mqtt_tx_publish: 0 <OK>
	[publish_cb:149] <MQTT_PUBREC> packet id: 53870, user_data: PUBLISH
	[publish_cb:149] <MQTT_PUBCOMP> packet id: 53870, user_data: PUBLISH
	[publisher:300] mqtt_tx_publish: 0 <OK>
	[publisher:285] mqtt_tx_pingreq: 0 <OK>
	[publisher:290] mqtt_tx_publish: 0 <OK>
	[publish_cb:149] <MQTT_PUBACK> packet id: 60144, user_data: PUBLISH
	[publisher:295] mqtt_tx_publish: 0 <OK>
	[publish_cb:149] <MQTT_PUBREC> packet id: 6561, user_data: PUBLISH
	[publish_cb:149] <MQTT_PUBCOMP> packet id: 6561, user_data: PUBLISH
	[publisher:300] mqtt_tx_publish: 0 <OK>
	[publisher:285] mqtt_tx_pingreq: 0 <OK>
	[publisher:290] mqtt_tx_publish: 0 <OK>
	[publish_cb:149] <MQTT_PUBACK> packet id: 38355, user_data: PUBLISH
	[publisher:295] mqtt_tx_publish: 0 <OK>
	[publish_cb:149] <MQTT_PUBREC> packet id: 60656, user_data: PUBLISH
	[publish_cb:149] <MQTT_PUBCOMP> packet id: 60656, user_data: PUBLISH
	[publisher:300] mqtt_tx_publish: 0 <OK>
	[publisher:285] mqtt_tx_pingreq: 0 <OK>
	[publisher:290] mqtt_tx_publish: 0 <OK>
	[publish_cb:149] <MQTT_PUBACK> packet id: 28420, user_data: PUBLISH
	[publisher:295] mqtt_tx_publish: 0 <OK>
	[publish_cb:149] <MQTT_PUBREC> packet id: 49829, user_data: PUBLISH
	[publish_cb:149] <MQTT_PUBCOMP> packet id: 49829, user_data: PUBLISH
	[publisher:300] mqtt_tx_publish: 0 <OK>
	[disconnect_cb:101] user_data: DISCONNECTED
	[publisher:304] mqtt_tx_disconnect: 0 <OK>

	Bye!

The line:

.. code-block:: console

	[try_to_connect:220] mqtt_connect: -5 <ERROR>

means that an error was detected and a new connect message will be sent.

The MQTT API is asynchronous, so messages are displayed as the callbacks are
executed.

This is the information that the subscriber will receive:

.. code-block:: console

	mosquitto_sub -t sensors
	DOORS:OPEN_QoS0
	DOORS:OPEN_QoS1
	DOORS:OPEN_QoS2
	DOORS:OPEN_QoS0
	DOORS:OPEN_QoS1
	DOORS:OPEN_QoS2
	DOORS:OPEN_QoS0
	DOORS:OPEN_QoS1
	DOORS:OPEN_QoS2
	DOORS:OPEN_QoS0
	DOORS:OPEN_QoS1
	DOORS:OPEN_QoS2
	DOORS:OPEN_QoS0
	DOORS:OPEN_QoS1
	DOORS:OPEN_QoS2

This is the output from the MQTT broker:

.. code-block:: console

	sudo mosquitto -v
	1485663791: mosquitto version 1.3.4 (build date 2014-08-17 00:14:52-0300) starting
	1485663791: Using default config.
	1485663791: Opening ipv4 listen socket on port 1883.
	1485663791: Opening ipv6 listen socket on port 1883.
	1485663797: New connection from 192.168.1.101 on port 1883.
	1485663797: New client connected from 192.168.1.101 as zephyr_publisher (c1, k0).
	1485663797: Sending CONNACK to zephyr_publisher (0)
	1485663798: Received PINGREQ from zephyr_publisher
	1485663798: Sending PINGRESP to zephyr_publisher
	1485663798: Received PUBLISH from zephyr_publisher (d0, q0, r0, m0, 'sensors', ... (15 bytes))
	1485663799: Received PUBLISH from zephyr_publisher (d0, q1, r0, m1888, 'sensors', ... (15 bytes))
	1485663799: Sending PUBACK to zephyr_publisher (Mid: 1888)
	1485663799: Received PUBLISH from zephyr_publisher (d0, q2, r0, m16356, 'sensors', ... (15 bytes))
	1485663799: Sending PUBREC to zephyr_publisher (Mid: 16356)
	1485663799: Received PUBREL from zephyr_publisher (Mid: 16356)
	1485663799: Sending PUBCOMP to zephyr_publisher (Mid: 16356)
	1485663800: Received PINGREQ from zephyr_publisher
	1485663800: Sending PINGRESP to zephyr_publisher
	1485663800: Received PUBLISH from zephyr_publisher (d0, q0, r0, m0, 'sensors', ... (15 bytes))
	1485663801: Received PUBLISH from zephyr_publisher (d0, q1, r0, m45861, 'sensors', ... (15 bytes))
	1485663801: Sending PUBACK to zephyr_publisher (Mid: 45861)
	1485663801: Received PUBLISH from zephyr_publisher (d0, q2, r0, m53870, 'sensors', ... (15 bytes))
	1485663801: Sending PUBREC to zephyr_publisher (Mid: 53870)
	1485663801: Received PUBREL from zephyr_publisher (Mid: 53870)
	1485663801: Sending PUBCOMP to zephyr_publisher (Mid: 53870)
	1485663802: Received PINGREQ from zephyr_publisher
	1485663802: Sending PINGRESP to zephyr_publisher
	1485663802: Received PUBLISH from zephyr_publisher (d0, q0, r0, m0, 'sensors', ... (15 bytes))
	1485663803: Received PUBLISH from zephyr_publisher (d0, q1, r0, m60144, 'sensors', ... (15 bytes))
	1485663803: Sending PUBACK to zephyr_publisher (Mid: 60144)
	1485663803: Received PUBLISH from zephyr_publisher (d0, q2, r0, m6561, 'sensors', ... (15 bytes))
	1485663803: Sending PUBREC to zephyr_publisher (Mid: 6561)
	1485663803: Received PUBREL from zephyr_publisher (Mid: 6561)
	1485663803: Sending PUBCOMP to zephyr_publisher (Mid: 6561)
	1485663804: Received PINGREQ from zephyr_publisher
	1485663804: Sending PINGRESP to zephyr_publisher
	1485663804: Received PUBLISH from zephyr_publisher (d0, q0, r0, m0, 'sensors', ... (15 bytes))
	1485663805: Received PUBLISH from zephyr_publisher (d0, q1, r0, m38355, 'sensors', ... (15 bytes))
	1485663805: Sending PUBACK to zephyr_publisher (Mid: 38355)
	1485663805: Received PUBLISH from zephyr_publisher (d0, q2, r0, m60656, 'sensors', ... (15 bytes))
	1485663805: Sending PUBREC to zephyr_publisher (Mid: 60656)
	1485663805: Received PUBREL from zephyr_publisher (Mid: 60656)
	1485663805: Sending PUBCOMP to zephyr_publisher (Mid: 60656)
	1485663806: Received PINGREQ from zephyr_publisher
	1485663806: Sending PINGRESP to zephyr_publisher
	1485663806: Received PUBLISH from zephyr_publisher (d0, q0, r0, m0, 'sensors', ... (15 bytes))
	1485663807: Received PUBLISH from zephyr_publisher (d0, q1, r0, m28420, 'sensors', ... (15 bytes))
	1485663807: Sending PUBACK to zephyr_publisher (Mid: 28420)
	1485663807: Received PUBLISH from zephyr_publisher (d0, q2, r0, m49829, 'sensors', ... (15 bytes))
	1485663807: Sending PUBREC to zephyr_publisher (Mid: 49829)
	1485663807: Received PUBREL from zephyr_publisher (Mid: 49829)
	1485663807: Sending PUBCOMP to zephyr_publisher (Mid: 49829)
	1485663808: Received DISCONNECT from zephyr_publisher
