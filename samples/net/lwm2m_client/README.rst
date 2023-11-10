.. zephyr:code-sample:: lwm2m-client
   :name: LwM2M client
   :relevant-api: lwm2m_api

   Implement a LwM2M client that connects to a LwM2M server.

Overview
********

Lightweight Machine to Machine (LwM2M) is an application layer protocol
based on CoAP/UDP, and is designed to expose various resources for reading,
writing and executing via an LwM2M server in a very lightweight environment.

This LwM2M client sample application for Zephyr implements the LwM2M library
and establishes a connection to an LwM2M server using the
`Open Mobile Alliance Lightweight Machine to Machine Technical Specification`_
(Section 5.3: Client Registration Interface).

.. _Open Mobile Alliance Lightweight Machine to Machine Technical Specification:
    http://www.openmobilealliance.org/release/LightweightM2M/V1_0-20170208-A/OMA-TS-LightweightM2M-V1_0-20170208-A.pdf

The source code for this sample application can be found at:
:zephyr_file:`samples/net/lwm2m_client`.

Requirements
************

- :ref:`networking_with_eth_qemu`, :ref:`networking_with_qemu` or :ref:`networking_with_native_sim`
- Linux machine
- Leshan Demo Server (https://eclipse.org/leshan/)

Building and Running
********************

There are configuration files for various setups in the
samples/net/lwm2m_client directory:

.. list-table::

    * - :file:`prj.conf`
      - This is the standard default config.

    * - :file:`overlay-bootstrap.conf`
      - This overlay config can be added to enable LWM2M Bootstrap support.

    * - :file:`overlay-ot.conf`
      - This overlay config can be added for OpenThread support.

    * - :file:`overlay-dtls.conf`
      - This overlay config can be added for DTLS support via MBEDTLS.

    * - :file:`overlay-bt.conf`
      - This overlay config can be added to enable Bluetooth networking support.

    * - :file:`overlay-queue.conf`
      - This overlay config can be added to enable LWM2M Queue Mode support.

    * - :file:`overlay-tickless.conf`
      - This overlay config can be used to stop LwM2M engine for periodically interrupting socket polls. It can have significant effect on power usage on certain devices.

Build the lwm2m-client sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/lwm2m_client
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

The easiest way to setup this sample application is to build and run it
as a native_sim application or as a QEMU target using the default configuration :file:`prj.conf`.
This requires a small amount of setup described in :ref:`networking_with_eth_qemu`, :ref:`networking_with_qemu` and :ref:`networking_with_native_sim`.

Download and run the latest build of the Leshan Demo Server:

.. code-block:: console

    $ wget https://ci.eclipse.org/leshan/job/leshan/lastSuccessfulBuild/artifact/leshan-server-demo.jar
    $ java -jar ./leshan-server-demo.jar -wp 8080

You can now open a web browser to: http://localhost:8080 This is where you
can watch and manage connected LwM2M devices.

Build the lwm2m-client sample application for QEMU like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/lwm2m_client
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

The sample will start and automatically connect to the Leshan Demo Server with
an IPv6 client endpoint "qemu_x86".

To change the sample to use IPv4, disable IPv6 by changing these two
configurations in ``prj.conf``::

    CONFIG_NET_IPV6=n
    CONFIG_NET_CONFIG_NEED_IPV6=n

DTLS Support
============

To build the lwm2m-client sample for QEMU with DTLS support do the following:

.. zephyr-app-commands::
   :zephyr-app: samples/net/lwm2m_client
   :host-os: unix
   :board: qemu_x86
   :conf: "prj.conf overlay-dtls.conf"
   :goals: run
   :compact:

Setup DTLS security in Leshan Demo Server:

1. Open up the Leshan Demo Server web UI
#. Click on "Security"
#. Click on "Add new client security configuration"
#. Enter the following data:

    * Client endpoint: qemu_x86
    * Security mode: Pre-Shared Key
    * Identity: Client_identity
    * Key: 000102030405060708090a0b0c0d0e0f

#. Start the Zephyr sample

Bootstrap Support
=================

In order to run Bootstrap procedure with the sample, you need to download and
run the Leshan Demo Bootstrap Server:

.. code-block:: console

    $ wget https://ci.eclipse.org/leshan/job/leshan/lastSuccessfulBuild/artifact/leshan-bsserver-demo.jar
    $ java -jar ./leshan-bsserver-demo.jar -wp 8888 -lp 5783 -slp 5784


You can now open a web browser to: http://localhost:8888 The Demo Bootstrap
Server web UI will open, this is where you can configure your device for
bootstrap.

Configure the lwm2m-client sample in the Demo Bootstrap Server:

1. Click on "Add new client bootstrap configuration"
#. Enter the following data:

    * Client endpoint: qemu_x86

#. In the ``LWM2M Server`` tab, enter the following data:

    * LWM2M Server URL: coap://[2001:db8::2]:5683 (or coap://192.0.2.2:5683 if IPv4 is used)
    * Security mode: No Security

#. The ``LWM2M Bootstrap Server`` tab can be left intact in the default
   configuration (No Security).

To build the lwm2m-client sample for QEMU with Bootstrap enabled do the
following:

.. zephyr-app-commands::
   :zephyr-app: samples/net/lwm2m_client
   :host-os: unix
   :board: qemu_x86
   :conf: "prj.conf overlay-bootstrap.conf"
   :goals: run
   :compact:

The sample will start and automatically connect to the Leshan Demo Bootstrap
Server to obtain the LwM2M Server information. After that, the sample will
automatically connect to the Leshan Demo Sever, as it was indicated in the
Bootstrap Server configuration.

It is possible to combine overlay files, to enable DTLS and Bootstrap for
instance. In that case, the user should make sure to update the port number in
the overlay file for Bootstrap over DTLS (5784 in case of Leshan Demo Bootstrap
Server) and to configure correct security mode in the ``LWM2M Bootstrap Server``
tab in the web UI (Pre-shared Key).

Bluetooth Support
=================

To build the lwm2m-client sample for hardware requiring Bluetooth for
networking (IPSP node connected via 6lowpan) do the following:

.. zephyr-app-commands::
   :zephyr-app: samples/net/lwm2m_client
   :host-os: unix
   :board: <board to use>
   :conf: "prj.conf overlay-bt.conf"
   :goals: build
   :compact:

The overlay-\*.conf files can also be combined.  For example, you could build a
DTLS-enabled LwM2M client sample for BLENano2 hardware by using the following
commands (requires Bluetooth for networking):

.. zephyr-app-commands::
   :zephyr-app: samples/net/lwm2m_client
   :host-os: unix
   :board: nrf52_blenano2
   :conf: "prj.conf overlay-bt.conf overlay-dtls.conf"
   :goals: build
   :compact:

OpenThread Support
==================

To build the lwm2m-client sample for hardware requiring OpenThread for
networking do the following:

.. zephyr-app-commands::
   :zephyr-app: samples/net/lwm2m_client
   :host-os: unix
   :board: <board to use>
   :conf: "prj.conf overlay-ot.conf"
   :goals: build
   :compact:

Note: If not provisioned (fully erased before flash), device will form
new OpenThread network and promote itself to leader (Current role: leader).
To join into already existing OT network, either enable CONFIG_OPENTHREAD_JOINER=y
and CONFIG_OPENTHREAD_JOINER_AUTOSTART=y and send join request from other
already joined device with joiner capabilities, or provision it manually
from console:

.. code-block:: console

   ot thread stop
   ot channel <channel>
   ot networkname <network name>
   ot masterkey <key>
   ot panid <panid>
   ot extpanid <extpanid>
   ot thread start

You could get all parameters for existing OT network from your OTBR with
the following command:

.. code-block:: console

    wpanctl get Thread:ActiveDataset

Queue Mode Support
==================

To build the lwm2m-client sample with LWM2M Queue Mode support do the following:

.. zephyr-app-commands::
   :zephyr-app: samples/net/lwm2m_client
   :host-os: unix
   :board: <board to use>
   :conf: "prj.conf overlay-queue.conf"
   :goals: build
   :compact:

With Queue Mode enabled, the LWM2M client will register with "UDP with Queue
Mode" binding. The LWM2M engine will notify the application with
``LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF`` event when the RX window
is closed so it can e. g. turn the radio off. The next RX window will be open
with consecutive ``LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE`` event.

WNC-M14A2A LTE-M Modem Support
==============================

To build the lwm2m-client sample for use with the WNC-M14A2A LTE-M modem
shield do the following:

.. zephyr-app-commands::
   :zephyr-app: samples/net/lwm2m_client
   :host-os: unix
   :board: <board to use>
   :conf: "prj.conf overlay-wncm14a2a.conf"
   :goals: build
   :compact:

Sample output without DTLS enabled
==================================

The following is sample output from the QEMU console.  First, LwM2M engine is
initialized.  Then, several LwM2M Smart Objects register themselves with the
engine.  The sample app then sets some client values so that they can be seen
in the Leshan Demo Server interface, and finally, the registration request is
sent to the server where the endpoint is initialized.

.. code-block:: console

    To exit from QEMU enter: 'CTRL+a, x'
    [QEMU] CPU: qemu32,+nx,+pae
    qemu-system-i386: warning: Unknown firmware file in legacy mode: genroms/multiboot.bin

    shell> [lib/lwm2m_engine] [DBG] lwm2m_engine_init: LWM2M engine thread started
    [lwm2m_obj_security] [DBG] security_create: Create LWM2M security instance: 0
    [lwm2m_obj_server] [DBG] server_create: Create LWM2M server instance: 0
    [lwm2m_obj_device] [DBG] device_create: Create LWM2M device instance: 0
    [lwm2m_obj_firmware] [DBG] firmware_create: Create LWM2M firmware instance: 0
    [lwm2m-client] [INF] main: Run LWM2M client
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/0, value:0x0001c99e, len:6
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/1, value:0x0001c9ab, len:23
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/2, value:0x0001c9c9, len:9
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/3, value:0x0001c9d9, len:3
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/9, value:0x0041a3a4, len:1
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/10, value:0x0041a3b4, len:4
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/17, value:0x0001c9fc, len:16
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/18, value:0x0001ca14, len:5
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/20, value:0x0041a3a4, len:1
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/21, value:0x0041a3b4, len:4
    [lib/lwm2m_engine] [DBG] lwm2m_engine_create_obj_inst: path:3303/0
    [ipso_temp_sensor] [DBG] temp_sensor_create: Create IPSO Temperature Sensor instance: 0
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3303/0/5700, value:0x0041a3b8, len:8
    [lib/lwm2m_rd_client] [INF] lwm2m_rd_client_start: LWM2M Client: qemu_x86
    [lib/lwm2m_rd_client] [INF] sm_do_init: RD Client started with endpoint 'qemu_x86' and client lifetime 0
    [lib/lwm2m_rd_client] [DBG] sm_send_registration: registration sent [2001:db8::2]
    [lib/lwm2m_engine] [DBG] lwm2m_udp_receive: checking for reply from [2001:db8::2]
    [lib/lwm2m_rd_client] [DBG] do_registration_reply_cb: Registration callback (code:2.1)
    [lwm2m-client] [DBG] rd_client_event: Registration complete
    [lib/lwm2m_rd_client] [INF] do_registration_reply_cb: Registration Done (EP='EZd501ZF26')
    [lib/lwm2m_engine] [DBG] lwm2m_udp_receive: reply 0x004001ec handled and removed
