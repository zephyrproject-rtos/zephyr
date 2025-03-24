.. zephyr:code-sample:: wireguard-vpn
   :name: Wireguard VPN
   :relevant-api: bsd_sockets

   Implement a echo-server application that sends received packets back to
   the sender over Wireguard VPN connection.

Wireguard VPN
*************

The Wireguard VPN demo application can be compiled and run like this for ``native_sim`` target:

.. zephyr-app-commands::
   :zephyr-app: samples/net/wireguard
   :board: native_sim
   :conf: <config file to use>
   :goals: build
   :compact:

For Linux GNOME desktop, it can be useful to add
``-DCONFIG_NATIVE_UART_AUTOATTACH_DEFAULT_CMD="\"gnome-terminal -- screen %s\""``
to command line compilation.

You host setup the Wireguard VPN can be done like this:

.. code-block:: console

    $ cd tools/net-setup
    $ ./net-setup.sh -c wireguard-vpn.conf

The example configuration will create this kind of configuration:

* In host side:

  * Normal ``zeth`` interface is created. It has ``192.0.2.2/24``
    address. All the network traffic to Zephyr will go through this
    interface.

  * VPN interface ``zwg0`` interface is the Wireguard tunnel endpoint.
    It has ``198.51.100.2`` IPv4 and ``2001:db8:100::2`` IPv6 addresses.
    All VPN traffic will go through this interface.

* The network interfaces and addresses in Zephyr (when using ``native_sim``
  board):

  * The ``eth0`` is the main interface, it has ``192.0.2.1/24``
    address and it has connection to the host.

  * The ``wg0`` is a virtual interface. Application should send data via
    it in order to get network traffic tunneled via Wireguard VPN.
    It has ``198.51.100.1/24`` IPv4 and ``2001:db8:100::1/64`` addresses.

  * The ``wg_ctrl`` is a VPN control interface. All the VPN virtual interfaces
    are attached to it. It does not have an IP address.

Example network interfaces in the host side:

.. code-block:: console

  zeth: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500
        inet 192.0.2.2  netmask 255.255.255.0  broadcast 0.0.0.0
        inet6 2001:db8::2  prefixlen 64  scopeid 0x0<global>
        inet6 fe80::200:5eff:fe00:53ff  prefixlen 64  scopeid 0x20<link>
        ether 00:00:5e:00:53:ff  txqueuelen 1000  (Ethernet)
        RX packets 16  bytes 2356 (2.3 KB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 27  bytes 2688 (2.6 KB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

  zwg0: flags=209<UP,POINTOPOINT,RUNNING,NOARP>  mtu 1420
        inet 198.51.100.2  netmask 255.255.255.255  destination 198.51.100.1
        inet6 2001:db8:100::2  prefixlen 128  scopeid 0x0<global>
        unspec 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  txqueuelen 1000  (UNSPEC)
        RX packets 3  bytes 364 (364.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 4  bytes 452 (452.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

and these are the routes setup in the host:

.. code-block:: console

  192.0.2.0/24 dev zeth proto kernel scope link src 192.0.2.2 linkdown
  198.51.100.1 dev zwg0 proto kernel scope link src 198.51.100.2
  2001:db8::/64 dev zeth proto kernel metric 256 linkdown pref medium
  2001:db8::/64 dev zeth metric 1024 linkdown pref medium
  2001:db8:100::2 dev zwg0 proto kernel metric 256 pref medium
  2001:db8:100::/64 dev zwg0 metric 1024 pref medium

.. warning::
   The IPv4 and IPv6 addresses used in the demo are not to be used
   in any live network and are only meant for these network samples.
   These IPv4 and IPv6 addresses are meant for documentation use only and
   are not routable.
   Do not use the private key found in the sample Wireguard VPN config files
   in any real network devices. You should always generate new private key
   when needed. See `WireGuard Quick Start <https://www.wireguard.com/quickstart/>`_
   how to generate keys.
