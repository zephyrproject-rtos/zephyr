.. zephyr:code-sample:: mqtt-sn-publisher
   :name: MQTT-SN publisher
   :relevant-api: mqtt_sn_socket

   Send MQTT-SN PUBLISH messages to an MQTT-SN gateway.

Overview
********

`MQTT <http://mqtt.org/>`_ (MQ Telemetry Transport) is a lightweight
publish/subscribe messaging protocol optimized for small sensors and
mobile devices.

MQTT-SN can be considered as a version of MQTT which is adapted to
the peculiarities of a wireless communication environment. While MQTT
requires a reliable TCP/IP transport, MQTT-SN is designed to be usable
on any datagram-based transport like UDP, ZigBee or even a plain UART
(with an additional framing protocol).

The Zephyr MQTT-SN Publisher sample application is an MQTT-SN v1.2
client that sends MQTT-SN PUBLISH messages to an MQTT-SN gateway.
It also SUBSCRIBEs to a topic.
See the `MQTT-SN v1.2 spec`_ for more information.

.. _MQTT-SN v1.2 spec: https://www.oasis-open.org/committees/download.php/66091/MQTT-SN_spec_v1.2.pdf

The source code of this sample application can be found at:
:zephyr_file:`samples/net/mqtt_sn_publisher`.

Requirements
************

- Linux machine
- MQTT-SN gateway, like Eclipse Paho
- Mosquitto server: any version that supports MQTT v3.1.1. This sample
  was tested with mosquitto 1.6.
- Mosquitto subscriber
- LAN for testing purposes (Ethernet)

Build and Running
*****************

Currently, this sample application only supports static IP addresses.
Open the :file:`prj.conf` file and set the IP addresses according
to the LAN environment.

You will also need to start an MQTT-SN gateway. With Paho, you can either
build it from source - see `PAHO MQTT-SN Gateway`_ - or run an unofficial
docker image, like `kyberpunk/paho`_.

.. _PAHO MQTT-SN Gateway: https://www.eclipse.org/paho/index.php?page=components/mqtt-sn-transparent-gateway/index.php
.. _kyberpunk/paho: https://hub.docker.com/r/kyberpunk/paho

On your Linux host computer, open 3 terminal windows. At first, start mosquitto:

.. code-block:: console

	$ sudo mosquitto -v -p 1883

Then, in another window, start the gateway, e.g. by using docker:

.. code-block:: console

	$ docker run -it -p 10000:10000 -p 10000:10000/udp --name paho -v $PWD/gateway.conf:/etc/paho/gateway.conf:ro kyberpunk/paho

Then, locate your zephyr directory and type:

.. zephyr-app-commands::
   :zephyr-app: samples/net/mqtt_sn_publisher
   :board: native_sim/native/64
   :goals: run
   :compact:

Optionally, use any MQTT explorer to connect to your broker.

Sample output
=============

This is the applications output:

.. code-block:: console

	WARNING: Using a test - not safe - entropy source
	*** Booting Zephyr OS build zephyr-v3.2.0-279-gc7fa387cea81  ***
	[00:00:00.000,000] <inf> net_config: Initializing network
	[00:00:00.000,000] <inf> net_config: IPv4 address: 172.18.0.20
	[00:00:00.000,000] <inf> mqtt_sn_publisher_sample: MQTT-SN sample
	[00:00:00.000,000] <inf> mqtt_sn_publisher_sample: Network connected
	[00:00:00.000,000] <inf> mqtt_sn_publisher_sample: Waiting for connection...
	[00:00:00.000,000] <inf> mqtt_sn_publisher_sample: Connecting client
	[00:00:00.510,000] <inf> net_mqtt_sn: Decoding message type: 5
	[00:00:00.510,000] <inf> net_mqtt_sn: Got message of type 5
	[00:00:00.510,000] <inf> net_mqtt_sn: MQTT_SN client connected
	[00:00:00.510,000] <inf> mqtt_sn_publisher_sample: MQTT-SN event EVT_CONNECTED
	[00:00:01.020,000] <inf> net_mqtt_sn: Decoding message type: 19
	[00:00:01.020,000] <inf> net_mqtt_sn: Got message of type 19
	[00:00:10.200,000] <inf> mqtt_sn_publisher_sample: Publishing timestamp
	[00:00:10.200,000] <inf> net_mqtt_sn: Registering topic
										2f 75 70 74 69 6d 65                             |/uptime
	[00:00:10.200,000] <inf> net_mqtt_sn: Can't publish; topic is not ready
	[00:00:10.710,000] <inf> net_mqtt_sn: Decoding message type: 11
	[00:00:10.710,000] <inf> net_mqtt_sn: Got message of type 11
	[00:00:10.710,000] <inf> net_mqtt_sn: Publishing to topic ID 14
	[00:00:20.400,000] <inf> mqtt_sn_publisher_sample: Publishing timestamp
	[00:00:20.400,000] <inf> net_mqtt_sn: Publishing to topic ID 14

This is the output from the MQTT-SN gateway:

.. code-block:: console

	20221024 140210.191   CONNECT           <---  ZEPHYR                              0C 04 04 01 00 3C 5A 45 50 48 59 52
	20221024 140210.192   CONNECT           ===>  ZEPHYR                              10 12 00 04 4D 51 54 54 04 02 00 3C 00 06 5A 45 50 48 59 52
	20221024 140210.192   CONNACK           <===  ZEPHYR                              20 02 00 00
	20221024 140210.192   CONNACK           --->  ZEPHYR                              03 05 00

	20221024 140210.643   SUBSCRIBE   0001  <---  ZEPHYR                              0C 12 00 00 01 2F 6E 75 6D 62 65 72
	20221024 140210.648   SUBSCRIBE   0001  ===>  ZEPHYR                              82 0C 00 01 00 07 2F 6E 75 6D 62 65 72 00
	20221024 140210.660   SUBACK      0001  <===  ZEPHYR                              90 03 00 01 00
	20221024 140210.661   SUBACK      0001  --->  ZEPHYR                              08 13 00 00 0D 00 01 00

	20221024 140220.338   REGISTER    0002  <---  ZEPHYR                              0D 0A 00 00 00 02 2F 75 70 74 69 6D 65
	20221024 140220.348   REGACK      0002  --->  ZEPHYR                              07 0B 00 0E 00 02 00

	20221024 140220.848   PUBLISH           <---  ZEPHYR                              0C 0C 00 00 0E 00 00 31 30 32 30 30
	20221024 140220.850   PUBLISH           ===>  ZEPHYR                              30 0E 00 07 2F 75 70 74 69 6D 65 31 30 32 30 30

	20221024 140230.539   PUBLISH           <---  ZEPHYR                              0C 0C 00 00 0E 00 00 32 30 34 30 30
	20221024 140230.542   PUBLISH           ===>  ZEPHYR                              30 0E 00 07 2F 75 70 74 69 6D 65 32 30 34 30 30
