.. _google-iot-mqtt-sample:

Google IOT MQTT Sample
######################

Overview
********

This sample application demonstrates a "full stack" application.  This
currently is able to

- Acquire a DHCPv4 lease.
- Connect to an SNTP server and acquire current time
- Establish a TLS connection with the Google IOT Cloud servers
- Publish data to the Google IOT Cloud
- Send/Receive keep alive / pings from cloud server

The source code for this sample application can be found at:
:zephyr_file:`samples/net/google_iot_mqtt`.

Requirements
************
- Entropy source
- Google IOT Cloud account
- Google IOT Cloud credentials and required information
- Network connectivity

Building and Running
********************
This application has been built and tested on the NXP FRDMK64F.  RSA or
ECDSA certs/keys are required to authenticate to the Google IOT Cloud.
The application includes a key creation script.

Run the ``create_keys.py`` script in the
``samples/net/google_iot_mqtt/src/private_info/`` directory.

Users will also be required to configure the following Kconfig options
based on their Google Cloud IOT project:

- CLOUD_CLIENT_ID - created from <telemetry>/<region>/registries/<registry name>/devices/<device name>
- CLOUD_AUDIENCE - created from telemetry without projects on front
- CLOUD_SUBSCRIBE_CONFIG - created from /devices/<device name>/config
- CLOUD_PUBLISH_TOPIC - created from /devices/<device name>/state

See `Google Cloud MQTT Documentation
<https://cloud.google.com/iot/docs/how-tos/mqtt-bridge>`_.
