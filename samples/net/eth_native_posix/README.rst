.. _eth-native-posix-sample:

Native Posix Ethernet
#####################

Overview
********

The eth_native_posix sample application for Zephyr creates a **zeth** network
interface to the host system. One can communicate with Zephyr via this network
interface.

The source code for this sample application can be found at:
:file:`samples/net/eth_native_posix`.

Building And Running
********************

To build the eth_native_posix sample application, follow the steps
below.

.. zephyr-app-commands::
   :zephyr-app: samples/net/eth_native_posix
   :host-os: unix
   :board: native_posix
   :conf: <config file to use>
   :goals: build
   :compact:

Normally one needs extra privileges to create and configure the TAP device in
the host system. Use sudo to execute the Zephyr process with admin privileges,
like this:

.. code-block:: console

    sudo --preserve-env=ZEPHYR_BASE make run

If the ``sudo --preserve-env=ZEPHYR_BASE`` gives an error,
just use ``sudo --preserve-env`` instead:

If you want to connect two Zephyr instances together, you can do it like this:

Create two Zephyr config files prj1.conf and prj2.conf. You can use
:file:`samples/net/eth_native_posix/prj.conf` as a base.

Set prj1.conf IP address configuration like this:

.. code-block:: console

    CONFIG_NET_CONFIG_MY_IPV6_ADDR="2001:db8:100::1"
    CONFIG_NET_CONFIG_PEER_IPV6_ADDR="2001:db8:100::2"
    CONFIG_NET_CONFIG_MY_IPV4_ADDR="198.51.100.1"
    CONFIG_NET_CONFIG_PEER_IPV4_ADDR="198.51.100.2"
    CONFIG_NET_CONFIG_MY_IPV4_GW="203.0.113.1"
    CONFIG_ETH_NATIVE_POSIX_RANDOM_MAC=n
    CONFIG_ETH_NATIVE_POSIX_MAC_ADDR="00:00:5e:00:53:64"
    CONFIG_ETH_NATIVE_POSIX_SETUP_SCRIPT="echo"
    CONFIG_ETH_NATIVE_POSIX_DRV_NAME="zeth.1"

Set prj2.conf IP address configuration like this:

.. code-block:: console

    CONFIG_NET_CONFIG_MY_IPV6_ADDR="2001:db8:200::1"
    CONFIG_NET_CONFIG_PEER_IPV6_ADDR="2001:db8:200::2"
    CONFIG_NET_CONFIG_MY_IPV4_ADDR="203.0.113.1"
    CONFIG_NET_CONFIG_PEER_IPV4_ADDR="203.0.113.2"
    CONFIG_NET_CONFIG_MY_IPV4_GW="198.51.100.1"
    CONFIG_ETH_NATIVE_POSIX_RANDOM_MAC=n
    CONFIG_ETH_NATIVE_POSIX_MAC_ADDR="00:00:5e:00:53:c8"
    CONFIG_ETH_NATIVE_POSIX_SETUP_SCRIPT="echo"
    CONFIG_ETH_NATIVE_POSIX_DRV_NAME="zeth.2"

Then compile and run two Zephyr instances
(if ``sudo --preserve-env=ZEPHYR_BASE`` gives an error,
just use ``sudo --preserve-env`` instead):

.. code-block:: console

    mkdir -p build1/native_posix
    cmake -DCONF_FILE=prj1.conf -DBOARD=native_posix -Bbuild1/native_posix -H.
    make -s -C build1/native_posix
    sudo --preserve-env=ZEPHYR_BASE make -s -C build1/native_posix run

.. code-block:: console

    mkdir -p build2/native_posix
    cmake -DCONF_FILE=prj2.conf -DBOARD=native_posix -Bbuild2/native_posix -H.
    make -s -C build2/native_posix
    sudo --preserve-env=ZEPHYR_BASE make -s -C build2/native_posix run

Bridge the two Zephyr instances together:

.. code-block:: console

    sudo brctl addbr zeth-br
    sudo brctl addif zeth-br zeth.1
    sudo brctl addif zeth-br zeth.2
    sudo ifconfig zeth-br up

After this, you are able to ping device 1 from device 2 in net-shell:

.. code-block:: console

    # In device 1
    net ping 2001:db8:200::1
    net ping 203.0.113.1

.. code-block:: console

    # In device 2
    net ping 2001:db8:100::1
    net ping 198.51.100.1

Note that in this setup you cannot access these two Zephyr devices from
your host. If you want to do that, then you could create a new network
interface with proper IP addresses and add that interface to the Zephyr
bridge.
