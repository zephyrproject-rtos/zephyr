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
:zephyr_file:`samples/net/cloud/google_iot_mqtt`.

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
``samples/net/cloud/google_iot_mqtt/src/private_info/`` directory.
Be sure that they key type generated (RSA or ECDSA) matches your
config of either :code:`JWT_SIGN_RSA` or :code:`JWT_SIGN_ECDSA`.

Users will also be required to configure the following Kconfig options
based on their Google Cloud IOT project.  The following values come
from the Google Cloud Platform itself:

- PROJECT_ID: When you select your project at the top of the UI, it
  should have a "name", and there should be an ID field as well.  This
  seems to be two words and a number, separated by hyphens.
- REGION: The Region shows in the list of registries for your
  registry.  And example is "us-central1".
- REGISTRY_ID: Each registry has an id.  This is a string given when
  creating the registry.
- DEVICE_ID: A name given for each device.  When viewing the table of
  devices, this will be shown.

From these values, the config values can be set using the following
template:

.. code-block: kconfig

   CLOUD_CLIENT_ID="projects/PROJECT_ID/locations/REGION/registries/REGISTRY_ID/devices/DEVICE_ID"
   CLOUD_AUDIENCE="PROJECT_ID"
   CLOUD_SUBSCRIBE_CONFIG="/devices/DEVICE_ID/config"
   CLOUD_PUBLISH_TOPIC="/devices/DEVICE_ID/state"

See `Google Cloud MQTT Documentation
<https://cloud.google.com/iot/docs/how-tos/mqtt-bridge>`_.
