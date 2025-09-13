.. zephyr:code-sample:: mqtt_offload_client
   :name: MQTT offload client with HL78xx

   Demonstrates using the HL78xx modem’s internal MQTT client (AT+KMQTT) through Zephyr’s socket API.

Overview
********

This sample shows how to publish and subscribe to MQTT topics using the **MQTT client stack running on the HL78xx modem itself**, instead of Zephyr’s built-in MQTT library.

Normally, Zephyr uses the modem only for IP connectivity, while the MQTT protocol runs on the host MCU.
In this sample, the **modem manages the entire MQTT session**, and Zephyr applications interact with it through Zephyr’s socket abstraction:

* Outgoing MQTT messages are sent with ``sendto()``.
* Incoming MQTT messages are delivered via ``recvfrom()``.
* The HL78xx notifies Zephyr about new data using URCs (``+KMQTT_DATA:``), which are mapped into socket events.

This allows applications to use the standard Zephyr socket APIs without caring that the MQTT stack is actually offloaded to the modem.

Notes
*****

This sample uses the devicetree alias ``modem`` to identify the modem instance to use.

Building and Running
********************

This application can be built and executed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/net/mqtt_offload_client
   :host-os: all
   :goals: build flash
   :compact:

To build for another board, change "qemu_x86" above to that board's name.

Sample Output
=============

.. code-block:: console

   **********************************************************
   ********* MQTT Modem Client Sample Application ***********
   **********************************************************
   [00:00:23.554,000] <dbg> hl78xx_socket: send_socket_data: Data to send:
                                           0c 0c 00 00 00 2f 75 70 74 69 6d 65 00 00 32 32 |...../uptime..22
                                           37 32 34                                         |724
   [00:00:24.217,000] <dbg> hl78xx_socket: prepare_mqtt_send_cmd: cmd=AT+KMQTTPUB=2,"/uptime",0,0,"3346"
   [00:00:29.058,000] <dbg> net_mqtt_sn: mqtt_offload_input: Received data
                                           30 13 00 09 22 2f 6e 75 6d 62 65 72 22 22 31 31 |0..."/number""11
                                           31 31 31 31 22                                   |1111"

After startup, the code performs:

#. Modem readiness check and power-on.
#. Network interface setup via Zephyr’s Connection Manager.
#. MQTT client configuration using HL78xx ``AT+KMQTT*`` commands.
#. Publishing a message to topic ``/uptime``.
#. Subscribing to topic ``/number`` and receiving messages.
#. Forwarding modem URCs into ``recvfrom()`` so the application sees standard socket data.
