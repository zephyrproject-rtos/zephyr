.. zephyr:code-sample:: ice-lite
   :name: ICE-lite agent
   :relevant-api: ice_lite

   Answer the connectivity checks of a full ICE agent and echo its data.

Overview
********

The sample opens a UDP socket and runs an ICE-lite agent (:rfc:`8445`) on it.
It prints the credentials of the agent, answers the connectivity checks of a
full ICE agent and, once that agent has nominated a pair, sends back every
datagram it receives from it.

See :ref:`ice_lite_interface` for the library.

Requirements
************

* A network in which the peer can reach the device directly.
* A full ICE agent as the peer.
* An entropy source for the credentials.

Building and Running
********************

On the :zephyr:board:`native_sim` board the device has the address 192.0.2.1
on the TAP interface that ``net-setup.sh`` creates, see
:ref:`networking_with_native_sim`, and the peer on the host is 192.0.2.2:

.. zephyr-app-commands::
   :zephyr-app: samples/net/ice_lite
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:

Without any network setup, the sample uses the sockets of the host instead and
has the address of the host:

.. zephyr-app-commands::
   :zephyr-app: samples/net/ice_lite
   :host-os: unix
   :board: native_sim
   :gen-args: -DEXTRA_CONF_FILE=nsos.conf -DCONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME=y
   :goals: run
   :compact:

For a board that is connected to a network, set its address with
:kconfig:option:`CONFIG_NET_CONFIG_MY_IPV4_ADDR`, or take it from DHCP with
:kconfig:option:`CONFIG_NET_DHCPV4`, as the other network samples do:

.. zephyr-app-commands::
   :zephyr-app: samples/net/ice_lite
   :board: <board to use>
   :goals: build flash
   :compact:

The sample has no signaling channel: the peer is given the address of the
device, the port ``CONFIG_NET_SAMPLE_ICE_LITE_PORT`` and the credentials the
sample prints. This script does it with the `aioice`_ Python package:

.. code-block:: python

   import asyncio
   import sys

   import aioice


   async def main(host, port, ufrag, pwd):
       conn = aioice.Connection(ice_controlling=True, use_ipv6=False)
       await conn.gather_candidates()
       conn.remote_is_lite = True
       conn.remote_username = ufrag
       conn.remote_password = pwd
       await conn.add_remote_candidate(
           aioice.Candidate(
               foundation="1",
               component=1,
               transport="udp",
               priority=2130706431,
               host=host,
               port=port,
               type="host",
           )
       )
       await conn.add_remote_candidate(None)
       await conn.connect()
       await conn.send(b"hello over ice")
       print(await conn.recv())
       await conn.close()


   asyncio.run(main(sys.argv[1], int(sys.argv[2]), sys.argv[3], sys.argv[4]))

.. code-block:: console

   $ python3 peer.py 192.0.2.1 5000 7bxM2VIq h+Hk7IuxdmIe7tXQW3oxvAmE
   b'hello over ice'

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v4.5.0 ***
   [00:00:00.000,000] <inf> net_ice_lite_sample: Waiting for network...
   [00:00:00.000,000] <inf> net_ice_lite_sample: ice-lite, port 5000, priority 2130706431
   [00:00:00.000,000] <inf> net_ice_lite_sample: ice-ufrag: 7bxM2VIq
   [00:00:00.000,000] <inf> net_ice_lite_sample: ice-pwd: h+Hk7IuxdmIe7tXQW3oxvAmE
   [00:00:03.220,006] <inf> net_ice_lite_sample: Peer nominated 192.0.2.2 port 54534
   [00:00:03.230,008] <inf> net_ice_lite_sample: Echoing 14 bytes

.. _aioice: https://github.com/aiortc/aioice
