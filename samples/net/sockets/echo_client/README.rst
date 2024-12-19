.. zephyr:code-sample:: sockets-echo-client
   :name: Echo client (advanced)
   :relevant-api: bsd_sockets tls_credentials

   Implement a client that sends IP packets, waits for data to be sent back, and verifies it.

Overview
********

The echo-client sample application for Zephyr implements a UDP/TCP client
that will send IPv4 or IPv6 packets, wait for the data to be sent back,
and then verify it matches the data that was sent.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/echo_client`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

There are multiple ways to use this application. One of the most common
usage scenario is to run echo-client application inside QEMU. This is
described in :ref:`networking_with_qemu`.

There are configuration files for different boards and setups in the
echo-client directory:

- :file:`prj.conf`
  Generic config file, normally you should use this.

- :file:`overlay-ot.conf`
  This overlay config enables support for OpenThread.

- :file:`overlay-802154.conf`
  This overlay config enables support for native IEEE 802.15.4 connectivity.
  Note, that by default IEEE 802.15.4 L2 uses unacknowledged communication. To
  improve connection reliability, acknowledgments can be enabled with shell
  command: ``ieee802154 ack set``.

- :file:`overlay-qemu_802154.conf`
  This overlay config enables support for two QEMU's when simulating
  IEEE 802.15.4 network that are connected together.

- :file:`overlay-tls.conf`
  This overlay config enables support for TLS.

Build echo-client sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_client
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Example building for the nrf52840dk/nrf52840 with OpenThread support:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_client
   :host-os: unix
   :board: nrf52840dk/nrf52840
   :conf: "prj.conf overlay-ot.conf"
   :goals: run
   :compact:

Example building for the IEEE 802.15.4 RF2XX transceiver:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_client
   :host-os: unix
   :board: [samr21_xpro | sam4s_xplained | sam_v71_xult/samv71q21]
   :gen-args: -DEXTRA_CONF_FILE=overlay-802154.conf
   :goals: build flash
   :compact:

In a terminal window you can check if communication is happen:

.. code-block:: console

    $ minicom -D /dev/ttyACM1



Enabling TLS support
====================

Enable TLS support in the sample by building the project with the
``overlay-tls.conf`` overlay file enabled, for example, using these commands:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_client
   :board: qemu_x86
   :conf: "prj.conf overlay-tls.conf"
   :goals: build
   :compact:

An alternative way is to specify ``-DEXTRA_CONF_FILE=overlay-tls.conf`` when
running ``west build`` or ``cmake``.

The certificate and private key used by the sample can be found in the sample's
``src`` directory. The default certificates used by Socket Echo Client and
:zephyr:code-sample:`sockets-echo-server` enable establishing a secure connection
between the samples.

SOCKS5 proxy support
====================

It is also possible to connect to the echo-server through a SOCKS5 proxy.
To enable it, use ``-DEXTRA_CONF_FILE=overlay-socks5.conf`` when running ``west
build`` or  ``cmake``.

By default, to make the testing easier, the proxy is expected to run on the
same host as the echo-server in Linux host.

To start a proxy server, for example a builtin SOCKS server support in ssh
can be used (-D option). Use the following command to run it on your host
with the default port:

For IPv4 proxy server:

.. code-block:: console

        $ ssh -N -D 0.0.0.0:1080 localhost

For IPv6 proxy server:

.. code-block:: console

        $ ssh -N -D [::]:1080 localhost

Run both commands if you are testing IPv4 and IPv6.

To connect to a proxy server that is not running under the same IP as the
echo-server or uses a different port number, modify the following values
in echo_client/src/tcp.c.

.. code-block:: c

        #define SOCKS5_PROXY_V4_ADDR IPV4_ADDR
        #define SOCKS5_PROXY_V6_ADDR IPV6_ADDR
        #define SOCKS5_PROXY_PORT    1080

Running echo-server in Linux Host
=================================

There is one useful testing scenario that can be used with Linux host.
Here echo-client is run in QEMU and echo-server is run in Linux host.

To use QEMU for testing, follow the :ref:`networking_with_qemu` guide.

In a terminal window:

.. code-block:: console

    $ sudo ./echo-server -i tap0

Run echo-client application in QEMU:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_client
   :host-os: unix
   :board: qemu_x86
   :conf: "prj.conf overlay-linux.conf"
   :goals: run
   :compact:

Note that echo-server must be running in the Linux host terminal window
before you start the echo-client application in QEMU.
Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

You can verify TLS communication with a Linux host as well. See
https://github.com/zephyrproject-rtos/net-tools documentation for information
on how to test TLS with Linux host samples.

See the :zephyr:code-sample:`sockets-echo-server` documentation for an alternate
way of running, with the echo-client on the Linux host and the echo-server
in QEMU.

OpenThread RCP+Zephyr HOST (SPINEL connection via UART)
=======================================================

Prerequisites:
--------------

- Build ``echo-server`` for HOST PC (x86_64)
  (https://github.com/zephyrproject-rtos/net-tools) SHA1:1c4fdba

.. code-block:: console

    $ make echo-server

- Program nRF RCP from Nordic nrf SDK (v2.7.0):

.. code-block:: console

   (v2.7.0) ~/ncs$ west build -p always -b nrf21540dk/nrf52840 -S logging nrf/samples/openthread/coprocessor


- Build mimxrt1020_evk HOST (Zephyr):

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/echo_client
   :board: mimxrt1020_evk
   :conf: "prj.conf overlay-ot-rcp-host-uart.conf"
   :goals: build
   :compact:

And flash

.. code-block:: console

    $ west flash -r pyocd -i 0226000047784e4500439004d9170013e56100009796990


- Connect the nRF RCP with IMXRT1020 (HOST) via UART

.. code-block:: c

	/*
	 * imxrt1020_evk -> HOST
	 * nRF21540-DK   -> RCP (nrf/samples/openthread/coprocessor)
	 * LPUART2 used for communication:
	 *  nRF21540 (P6) P0.08 RXD -> IMXRT1020-EVK (J17) D1 (GPIO B1 08) (TXD)
	 *  nRF21540 (P6) P0.07 CTS -> IMXRT1020-EVK (J19) D8 (GPIO B1 07) (RTS)
	 *  nRF21540 (P6) P0.06 TXD -> IMXRT1020-EVK (J17) D0 (GPIO B1 09) (RXD)
	 *  nRF21540 (P6) P0.05 RTS -> IMXRT1020-EVK (J17) D7 (GPIO B1 06) (CTS)
	 */


- Install the OTBR (OpenThread Border Router) docker container on your HOST PC (x86_64)
  Follow steps from https://docs.nordicsemi.com/bundle/ncs-2.5.1/page/nrf/protocols/thread/tools.html#running_otbr_using_docker

**Most notable ones:**

  1. Create ``otbr0`` network bridge to have access to OT network from HOST
     Linux PC

  .. code-block:: console

    sudo docker network create --ipv6 --subnet fd11:db8:1::/64 -o com.docker.network.bridge.name=otbr0 otbr


  2. Pull docker container for OTBR:

  .. code-block:: console

    docker pull nrfconnect/otbr:84c6aff


  3. Start the docker image:

  .. code-block:: console

    sudo modprobe ip6table_filter
    sudo docker run -it --rm --privileged --name otbr --network otbr -p 8080:80 --sysctl "net.ipv6.conf.all.disable_ipv6=0 net.ipv4.conf.all.forwarding=1 net.ipv6.conf.all.forwarding=1" --volume /dev/ttyACM5:/dev/radio nrfconnect/otbr:84c6aff --radio-url spinel+hdlc+uart:///dev/radio?uart-baudrate=1000000


  4. Add proper routing (``fd11:22::/64`` are the IPv6 addresses - On-Mesh - which allow accessing the OT devices) on HOST PC (x86_64)

  .. code-block:: console

    sudo ip -6 route add fd11:22::/64 dev otbr0 via fd11:db8:1::2


  And the output for on-OT address:

  .. code-block:: console

    ip route get fd11:22:0:0:5188:1678:d0c0:6893
    fd11:22::5188:1678:d0c0:6893 from :: via fd11:db8:1::2 dev otbr0 src fd11:db8:1::1 metric 1024 pref medium


  5. Start the console to the docker image:

  .. code-block:: console

    sudo docker exec -it otbr /bin/bash


  Test with e.g.

  .. code-block:: console

    ot-ctl router table
    ot-ctl ipaddr



Configure OTBR
--------------

On the HOST PC's webbrowser: http://localhost:8080/

Go to ``Form`` and leave default values - e.g:

  * Network Key:	``00112233445566778899aabbccddeeff``
  * On-Mesh Prefix:	``fd11:22::``
  * Channel:	``15``


to "FORM" the OT network.

*Note:*
The "On-Mesh Prefix" shall match the one setup in ``otbr0`` routing.


Configure RCP (nRF21540-DK) + OT HOST (mimxrt1020)
--------------------------------------------------

.. code-block:: console

   ot factoryreset
   ot dataset networkkey 00112233445566778899aabbccddeeff
   ot ifconfig up


In the HOST PC www webpage interface please:
Commission -> Joiner PSKd* set to ``J01NME`` -> START COMMISSION

.. code-block:: console

   ot joiner start J01NME
   ot thread start


The ``ot ipaddr`` shall show IPv6 address starting from ``fd11:22:0:0:``.
This one can be accessed from HOST's PC network (via e.g.
``ping -6 fd11:22:0:0:e8bf:266b:63ca:eff4``).

Start ``echo-server`` on HOST PC (x86-64)
-----------------------------------------

.. code-block:: console

   ./echo-server -i otbr0
