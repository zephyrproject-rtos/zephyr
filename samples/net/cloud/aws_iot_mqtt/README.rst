.. zephyr:code-sample:: aws-iot-mqtt
   :name: AWS IoT Core MQTT
   :relevant-api: bsd_sockets mqtt_socket dns_resolve tls_credentials json sntp random_api

   Connect to AWS IoT Core and publish messages using MQTT.

Overview
********

This sample application demonstrates the implementation of an MQTT client that
can publish messages to AWS IoT Core using the MQTT protocol. Key features include:

- Acquiring a DHCPv4 lease
- Connecting to an SNTP server to obtain the current time
- Establishing a TLS 1.2 connection with AWS IoT Core servers
- Subscribing to a topic on AWS IoT Core
- Publishing data to AWS IoT Core
- Passing the AWS Device Qualification Program (DQP) test suite: `Device Qualification Program (DQP) <https://aws.amazon.com/partners/programs/dqp/>`_
- Sending and receiving keep-alive pings
- Retrying connections using an exponential backoff algorithm

Requirements
************

- An entropy source
- An AWS account with access to AWS IoT Core
- AWS credentials and necessary information
- Network connectivity

Building and Running
********************

This application has been built and tested on the ST NUCLEO-F429ZI board and
QEMU x86 target. A valid certificate and private key are required to
authenticate to the AWS IoT Core. The sample includes a script to convert
the certificate and private key in order to embed them in the application.

Register a *thing* in AWS IoT Core and download the certificate and private key.
Copy these files to the :zephyr_file:`samples/net/cloud/aws_iot_mqtt/src/creds`
directory. Run the :zephyr_file:`samples/net/cloud/aws_iot_mqtt/src/creds/convert_keys.py`
script, which will generate files ``ca.c``, ``cert.c`` and ``key.c``.

To configure the sample, set the following Kconfig options based on your AWS IoT
Core region, thing, and device advisor configuration:

- :kconfig:option:`CONFIG_AWS_ENDPOINT`: The AWS IoT Core broker endpoint, found in the AWS IoT Core
  console. This will be specific if running a test suite using device advisor.
- :kconfig:option:`CONFIG_AWS_THING_NAME`: The name of the thing created in AWS IoT Core. Associated
  with the certificate it will be used as the client id. We will use
  ``zephyr_sample`` in this example.
- :kconfig:option:`CONFIG_AWS_SUBSCRIBE_TOPIC`: The topic to subscribe to.
- :kconfig:option:`CONFIG_AWS_PUBLISH_TOPIC`: The topic to publish to.
- :kconfig:option:`CONFIG_AWS_QOS`: The QoS level for subscriptions and publications.
- :kconfig:option:`CONFIG_AWS_EXPONENTIAL_BACKOFF`: Enable the exponential backoff algorithm.

Refer to the `AWS IoT Core Documentation <https://docs.aws.amazon.com/iot/index.html>`_
for more information.

Additionnaly, it is possible to tune the firmware to pass the AWS DQP test
suite, to do set Kconfig option :kconfig:option:`CONFIG_AWS_TEST_SUITE_DQP` to ``y``.

More information about the AWS device advisor can be found here:
`AWS IoT Core Device Advisor <https://aws.amazon.com/iot-core/device-advisor/>`_.

MQTT test client
================

Access the MQTT test client in the AWS IoT Core console, subscribe to the
``zephyr_sample/data`` topic, and publish a payload to the ``zephyr_sample/downlink``
topic. The device console will display the payload received by your device, and
the AWS console will show the JSON message sent by the device under the
``zephyr_sample/data`` topic.

Sample output
=============

This is the output from the ST-Link UART on the NUCLEO-F429ZI board.

.. code-block:: console

  *** Booting Zephyr OS build zephyr-v3.3.0 ***
  [00:00:01.626,000] <inf> aws: starting DHCPv4
  [00:00:01.969,000] <dbg> aws: sntp_sync_time: Acquired time from NTP server: 1683472436
  [00:00:01.977,000] <inf> aws: Resolved: 52.212.60.110:8883
  [00:00:03.327,000] <dbg> aws: mqtt_event_cb: MQTT event: CONNACK [0] result: 0
  [00:00:03.327,000] <inf> aws: Subscribing to 1 topic(s)
  [00:00:03.390,000] <dbg> aws: mqtt_event_cb: MQTT event: SUBACK [7] result: 0
  [00:00:03.390,000] <inf> aws: PUBLISHED on topic "zephyr_sample/data" [ id: 1 qos: 0 ], payload: 13 B
  [00:00:03.390,000] <dbg> aws: publish_message: Published payload:
                                7b 22 63 6f 75 6e 74 65  72 22 3a 30 7d          |{"counte r":0}
  [00:00:11.856,000] <dbg> aws: mqtt_event_cb: MQTT event: PUBLISH [2] result: 0
  [00:00:11.856,000] <inf> aws: RECEIVED on topic "zephyr_sample/downlink" [ id: 13 qos: 0 ] payload: 45 / 4096 B
  [00:00:11.856,000] <dbg> aws: handle_published_message: Received payload:
                                7b 0a 20 20 22 6d 65 73  73 61 67 65 22 3a 20 22 |{.  "mes sage": "
                                48 65 6c 6c 6f 20 66 72  6f 6d 20 41 57 53 20 49 |Hello fr om AWS I
                                6f 54 20 63 6f 6e 73 6f  6c 65 22 0a 7d          |oT conso le".}
  [00:00:11.857,000] <inf> aws: PUBLISHED on topic "zephyr_sample/data" [ id: 2 qos: 0 ], payload: 13 B
  [00:00:11.857,000] <dbg> aws: publish_message: Published payload:
                                7b 22 63 6f 75 6e 74 65  72 22 3a 31 7d          |{"counte r":1}
  [00:01:11.755,000] <dbg> aws: mqtt_event_cb: MQTT event: 9 result: 0
  [00:02:11.755,000] <dbg> aws: mqtt_event_cb: MQTT event: 9 result: 0

Run in QEMU x86
===============

The sample can be run in QEMU x86. To do so, you will need to configure
NAT/MASQUERADE on your host machine. Refer to the Zephyr documentation
:ref:`networking_with_qemu`. for more information.
