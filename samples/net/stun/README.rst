.. zephyr:code-sample:: stun-client
   :name: STUN client
   :relevant-api: stun_client stun

   Ask a STUN server for the public address of the device.

Overview
********

The sample asks a STUN server (:rfc:`8489`) which address it sees the device
at, and prints the answer: the public address of the device, the one it has on
the outside of the NAT of its network. One call does it,
:c:func:`stun_client_simple`.

The sample also enables the ``net stun`` commands of the network shell, with
which other servers can be asked while it runs.

See :ref:`stun_interface` for the library, and for what an application does to
learn the address of a socket of its own.

Requirements
************

* A network through which the STUN server can be reached. The device takes its
  address and its DNS servers from DHCP.
* A STUN server. The default is a public one; ``CONFIG_NET_SAMPLE_STUN_SERVER``
  and ``CONFIG_NET_SAMPLE_STUN_SERVER_PORT`` name another. The address that is
  printed is the public one only if the server is on the other side of the NAT:
  a server in the local network sees the local address.
* An entropy source: the transaction id of a STUN request has to be
  unpredictable, and the client takes it from the cryptographically secure
  random number generator.

Building and Running
********************

The sample runs on the :zephyr:board:`native_sim` board without any network
setup when it uses the sockets of the host:

.. zephyr-app-commands::
   :zephyr-app: samples/net/stun
   :host-os: unix
   :board: native_sim
   :gen-args: -DEXTRA_CONF_FILE=overlay-nsos.conf
   :goals: run
   :compact:

For a board that is connected to a network:

.. zephyr-app-commands::
   :zephyr-app: samples/net/stun
   :board: <board to use>
   :goals: build flash
   :compact:

To ask another server, here one in the local network that is reached over
IPv6:

.. zephyr-app-commands::
   :zephyr-app: samples/net/stun
   :board: <board to use>
   :gen-args: -DCONFIG_NET_SAMPLE_STUN_SERVER=\"2001:db8::2\"
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v4.5.0 ***
   [00:00:00.010,000] <inf> net_samples_common: Waiting for network...
   [00:00:03.120,000] <inf> net_samples_common: Network connectivity established and IP address assigned
   [00:00:03.120,000] <inf> net_stun_sample: Asking stun.cloudflare.com port 3478
   [00:00:03.190,000] <inf> net_stun_sample: Public address: 203.0.113.7

The shell asks any other server, and shows the port of the request as well:

.. code-block:: console

   uart:~$ net stun server stun.example.org
   STUN server: stun.example.org port 3478
   uart:~$ net stun query
   Mapped address: 203.0.113.7:54321

If no answer comes, the sample says so after the time
``CONFIG_NET_SAMPLE_STUN_TIMEOUT_MS`` allows, 5 seconds by default. A firewall
that drops UDP is the usual reason.
