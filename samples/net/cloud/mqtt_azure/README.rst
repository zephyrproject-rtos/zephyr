.. zephyr:code-sample:: mqtt-azure
   :name: Microsoft Azure IoT Hub MQTT
   :relevant-api: bsd_sockets mqtt_socket tls_credentials random_api

   Connect to Azure IoT Hub and publish messages using MQTT.

Overview
********

This sample application demonstrates how an MQTT client
can publish messages to an Azure Cloud IoT hub based on MQTT protocol.

- Acquire a DHCPv4 lease
- Establish a TLS connection with Azure Cloud IoT hub
- Publish data to the Azure cloud
- SOCKS5 supported
- DNS supported

The source code of this sample application can be found at:
:zephyr_file:`samples/net/cloud/mqtt_azure`.

Requirements
************

- Azure Cloud account
- Azure IOT Cloud credentials and required information
- Freedom Board (FRDM-K64F)
- Network connectivity

Building and Running
********************

This application has been built and tested on the NXP FRDMK64F.
Certs are required to authenticate to the Azure Cloud IoT hub.
Current certs in :zephyr_file:`samples/net/cloud/mqtt_azure/src/digicert.cer` are
copied from `<https://github.com/Azure/azure-iot-sdk-c/blob/master/certs/certs.c>`_

Configure the following Kconfig options based on your Azure Cloud IoT Hub
in your own overlay config file:

- ``SAMPLE_CLOUD_AZURE_USERNAME`` - Username field use::

    {iothubhostname}/{device_id}/?api-version=2018-06-30,

  where ``{iothubhostname}`` is the full CName of the IoT hub.

- ``SAMPLE_CLOUD_AZURE_PASSWORD``    - Password field, use an SAS token.
- ``SAMPLE_CLOUD_AZURE_CLIENT_ID``   - ClientId field, use the deviceId.
- ``SAMPLE_CLOUD_AZURE_HOSTNAME``    - IoT hub hostname
- ``SAMPLE_CLOUD_AZURE_SERVER_ADDR`` - IP address of the Azure MQTT broker
- ``SAMPLE_CLOUD_AZURE_SERVER_PORT`` - Port number of the Azure MQTT broker

You'll also need to set these Kconfig options if you're running
the sample behind a proxy:

- ``SAMPLE_SOCKS_ADDR`` - IP address of SOCKS5 Proxy server
- ``SAMPLE_SOCKS_PORT`` - Port number of SOCKS5 Proxy server

On your Linux host computer, open a terminal window, locate the source code
of this sample application (i.e., :zephyr_file:`samples/net/cloud/mqtt_azure`) and type:

.. zephyr-app-commands::
   :zephyr-app: samples/net/cloud/mqtt_azure
   :board: frdm_k64f
   :conf: "prj.conf <overlay.conf>"
   :goals: build flash
   :compact:

Also this application can be tested with QEMU. This is described in
:ref:`networking_with_qemu`. Set up Zephyr and NAT/masquerading on host
to access Internet and use :file:`overlay-qemu_x86.conf`.
DHCP support is not enabled with QEMU. It uses static IP addresses.

Sample overlay file
===================

This is the overlay template for Azure IoT hub and other details:

.. code-block:: cfg

	CONFIG_SAMPLE_CLOUD_AZURE_USERNAME="<username>"
	CONFIG_SAMPLE_CLOUD_AZURE_PASSWORD="<SAS token>"
	CONFIG_SAMPLE_CLOUD_AZURE_CLIENT_ID="<device id>"
	CONFIG_SAMPLE_CLOUD_AZURE_HOSTNAME="<IoT hub hostname>"
	CONFIG_SAMPLE_SOCKS_ADDR="<proxy addr>"
	CONFIG_SAMPLE_SOCKS_PORT=<proxy port>
	CONFIG_SAMPLE_CLOUD_AZURE_SERVER_ADDR="<server ip addr, if DNS disabled set this>"
	CONFIG_SAMPLE_CLOUD_AZURE_SERVER_PORT=<server port, if DNS disabled set this>

Sample output
=============

This is the output from the FRDM UART console, with:

.. code-block:: console

	[00:00:03.001,000] <inf> eth_mcux: Enabled 100M full-duplex mode.
	[00:00:03.010,000] <dbg> mqtt_azure.main: Waiting for network to setup...
	[00:00:03.115,000] <inf> net_dhcpv4: Received: 10.0.0.2
	[00:00:03.124,000] <inf> net_config: IPv4 address: 10.0.0.2
	[00:00:03.132,000] <inf> net_config: Lease time: 43200 seconds
	[00:00:03.140,000] <inf> net_config: Subnet: 255.255.255.0
	[00:00:03.149,000] <inf> net_config: Router: 10.0.0.10
	[00:00:06.157,000] <dbg> mqtt_azure.try_to_connect: attempting to connect...
	[00:00:06.167,000] <dbg> net_sock_tls.tls_alloc: (0x200024f8): Allocated TLS context, 0x20001110
	[00:00:19.412,000] <dbg> mqtt_azure.mqtt_event_handler: MQTT client connected!
	[00:00:19.424,000] <dbg> mqtt_azure.publish_message: mqtt_publish OK
	[00:00:19.830,000] <dbg> mqtt_azure.mqtt_event_handler: PUBACK packet id: 63387
	[00:00:31.842,000] <dbg> mqtt_azure.publish_message: mqtt_publish OK
	[00:00:51.852,000] <dbg> mqtt_azure.publish_message: mqtt_publish OK
	[00:00:51.861,000] <dbg> mqtt_azure.mqtt_event_handler: PUBACK packet id: 38106

You can also check events or messages information on Azure Portal.

Cloud to device communication
=============================

Goto IoT devices section in Azure Portal. Click on the device from
IoT devices. If you have configured multiple devices, select correct device.
Goto Message to Device section. Enter text in Message Body section.
Click on Send Message.

See `Azure Cloud MQTT Documentation
<https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-mqtt-support>`_.
