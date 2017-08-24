.. _lwm2m-client-sample:

LwM2M client
############

Overview
********

Lightweight Machine to Machine (LwM2M) is an application layer protocol
based on CoAP/UDP, and is designed to expose various resources for reading,
writing and executing via an LwM2M server in a very lightweight environment.

This LwM2M client sample application for Zephyr implements the LwM2M library
and establishes both IPv4 and IPv6 connections to an LwM2M server using
the `Open Mobile Alliance Lightweight Machine to Machine Technical
Specification`_ (Section 5.3: Client Registration Inferface).

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

There are configuration files for various hardware setups in the
samples/net/lwm2m_client directory:

- :file:`prj_frdm_k64f.conf`
  Use this for FRDM-K64F board with built-in ethernet.

- :file:`prj_qemu_x86.conf`
  Use this for x86 QEMU.

Build the lwm2m-client sample application like this:

.. code-block:: console

    $ cd $ZEPHYR_BASE/samples/net/lwm2m_client
    $ make pristine && make CONF_FILE=<your desired conf file> \
      BOARD=<board to use>

The easiest way to setup this sample application is to build and run it
via QEMU using the configuration :file:`prj_qemu_x86.conf`.
This requires a small amount of setup described in :ref:`networking_with_qemu`.

Download and run the latest build of the Leshan Demo Server:

.. code-block:: console

    $ wget https://hudson.eclipse.org/leshan/job/leshan/lastSuccessfulBuild/artifact/leshan-server-demo.jar
    $ java -jar ./leshan-server-demo.jar -wp 8080

You can now open a web browser to: http://localhost:8080 This is where you
can watch and manage connected LwM2M devices.

Build the lwm2m-client sample application for QEMU like this:

.. code-block:: console

    $ cd $ZEPHYR_BASE/samples/net/lwm2m_client
    $ make pristine && make run

The sample will start and automatically connect to the Leshan Demo Server with
both an IPv4 client endpoint (qemu_x86-ipv4-########) and an IPV6 client
endpoint (qemu_x86-ipv6-########).  The "########" portion is a randomly
generated number so you can have multiple clients running at the same time.

Sample output
=============

The following is sample output from the QEMU console.  First, LwM2M engine is
initialized.  Then, several LwM2M Smart Objects register themselves with the
engine.  The sample app then sets some client values so that they can be seen
in the Leshan Demo Server interface, and finally, registration requests are
sent to the server where the endpoints are initialized.

.. code-block:: console

    To exit from QEMU enter: 'CTRL+a, x'
    [QEMU] CPU: qemu32
    qemu-system-i386: warning: Unknown firmware file in legacy mode:
    genroms/multiboot.bin

    [lib/lwm2m_engine] [DBG] lwm2m_engine_init: LWM2M engine thread started
    [lwm2m_obj_security] [DBG] security_create: Create LWM2M security instance: 0
    [lwm2m_obj_server] [DBG] server_create: Create LWM2M server instance: 0
    [lwm2m_obj_device] [DBG] device_create: Create LWM2M device instance: 0
    [lib/lwm2m_rd_client] [DBG] lwm2m_rd_client_init: LWM2M RD client thread started
    [lwm2m_obj_firmware] [DBG] firmware_create: Create LWM2M firmware instance: 0
    shell> [lwm2m-client] [INF] main: Run LWM2M client
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/0, value:0x00018e31, len:6
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/1, value:0x00018e3e, len:23
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/2, value:0x00018e5c, len:9
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/3, value:0x00018e6c, len:3
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/9, value:0x00429394, len:1
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/10, value:0x004293a4, len:4
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/17, value:0x00018e8f, len:16
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/18, value:0x00018ea7, len:5
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/20, value:0x00429394, len:1
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3/0/21, value:0x004293a4, len:4
    [lib/lwm2m_engine] [DBG] lwm2m_engine_create_obj_inst: path:3303/0
    [ipso_temp_sensor] [DBG] temp_sensor_create: Create IPSO Temperature Sensor instance: 0
    [lib/lwm2m_engine] [DBG] lwm2m_engine_set: path:3303/0/5700, value:0x004293a8, len:8
    [lib/lwm2m_rd_client] [INF] lwm2m_rd_client_start: LWM2M Client: qemu_x86-ipv6-4139873732
    [lwm2m-client] [INF] main: IPv6 setup complete.
    [lib/lwm2m_rd_client] [INF] lwm2m_rd_client_start: LWM2M Client: qemu_x86-ipv4-4143279384
    [lwm2m-client] [INF] main: IPv4 setup complete.
    [lib/lwm2m_rd_client] [DBG] sm_do_init: RD Client started with endpoint 'qemu_x86-ipv6-4139873732' and client lifetime 0
    [lib/lwm2m_rd_client] [DBG] sm_do_init: RD Client started with endpoint 'qemu_x86-ipv4-4143279384' and client lifetime 0
    [lib/lwm2m_rd_client] [DBG] sm_send_registration: registration sent [2001:db8::2]
    [lib/lwm2m_rd_client] [DBG] sm_send_registration: registration sent [192.0.2.2]
    [lib/lwm2m_engine] [DBG] lwm2m_udp_receive: checking for reply from [2001:db8::2]
    [lib/lwm2m_rd_client] [DBG] do_registration_reply_cb: Registration callback (code:2.1)
    [lib/lwm2m_rd_client] [INF] do_registration_reply_cb: Registration Done (EP='KUNmxEfMl1')
    [lib/lwm2m_engine] [DBG] lwm2m_udp_receive: reply 0x004097c0 handled and removed
    [lib/lwm2m_engine] [DBG] lwm2m_udp_receive: checking for reply from [192.0.2.2]
    [lib/lwm2m_rd_client] [DBG] do_registration_reply_cb: Registration callback (code:2.1)
    [lib/lwm2m_rd_client] [INF] do_registration_reply_cb: Registration Done (EP='LAN9BHobOp')
    [lib/lwm2m_engine] [DBG] lwm2m_udp_receive: reply 0x004097d8 handled and removed
