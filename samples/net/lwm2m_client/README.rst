.. _lwm2m-client-sample:

LwM2M client
############

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
:file:`samples/net/lwm2m_client`.

Requirements
************

- :ref:`networking_with_qemu`
- Linux machine
- Leshan Demo Server (https://eclipse.org/leshan/)

Building and Running
********************

There are configuration files for various setups in the
samples/net/lwm2m_client directory:

- :file:`prj.conf`
  This is the standard default config.

- :file:`prj_dtls.conf`
  This config is the same as the standard but enables DTLS support via MBEDTLS.

Build the lwm2m-client sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/lwm2m_client
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

The easiest way to setup this sample application is to build and run it
via QEMU using the default configuration :file:`prj.conf`.
This requires a small amount of setup described in :ref:`networking_with_qemu`.

Download and run the latest build of the Leshan Demo Server:

.. code-block:: console

    $ wget https://hudson.eclipse.org/leshan/job/leshan/lastSuccessfulBuild/artifact/leshan-server-demo.jar
    $ java -jar ./leshan-server-demo.jar -wp 8080

You can now open a web browser to: http://localhost:8080 This is where you
can watch and manage connected LwM2M devices.

Build the lwm2m-client sample application for QEMU like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/lwm2m_client
   :board: qemu_x86
   :goals: run
   :compact:

The sample will start and automatically connect to the Leshan Demo Server with
an IPv6 client endpoint "qemu_x86".

To change the sample to use IPv4, disable IPv6 by changing these two
configurations in ``prj.conf``::

    CONFIG_NET_IPV6=n
    CONFIG_NET_APP_NEED_IPV6=n

DTLS Support
============

To build the lwm2m-client sample for QEMU with DTLS support do the following:

.. zephyr-app-commands::
   :zephyr-app: samples/net/lwm2m_client
   :board: qemu_x86
   :conf: prj_dtls.conf
   :goals: run
   :compact:

Setup DTLS security in Leshan Demo Server:

- Open up the Leshan Demo Server web UI
- Click on "Security"
- Click on "Add new client security configuration"
- Enter the following data:
    Client endpoint: qemu_x86
    Security mode: Pre-Shared Key
    Identity: Client_identity
    Key: 000102030405060708090a0b0c0d0e0f
- Start the Zephyr sample

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
