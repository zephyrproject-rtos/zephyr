.. _net_app_api:

Network Application API
#######################

The Network Application (net-app) API allows applications to:

**Initialize**
  The application for networking use. This means, for example,
  that if the application needs to have an IPv4 address, and if DHCPv4 is
  enabled, then the net-app API will make sure that the device will get an
  IPv4 address before the application is started.

**Set**
  Various options for the networking subsystem. This means that if the
  user has set options like IP addresses, IEEE 802.15.4 channel etc. in the
  project configuration file, then those settings are applied to the system
  before the application starts.

**Create**
  A simple TCP/UDP server or client application. The net-app API
  has functions that make it easy to create a simple TCP or UDP based network
  application. The net-app API also provides transparent TLS and DTLS support
  for the application.

The net-app API functionality is enabled by :option:`CONFIG_NET_APP` option.
The current net-app API implementation is still experimental and may change and
improve in future releases.

Initialization
##############

The net-app API provides a :cpp:func:`net_app_init()` function that can
configure the networking subsystem for the application. The following
configuration options control this configuration:

:option:`CONFIG_NET_CONFIG_AUTO_INIT`
  automatically configures the system according to other configuration options.
  The user does not need to call :cpp:func:`net_app_init()` in this case as that
  function will be automatically called when the system boots. This option is
  enabled by default.

:option:`CONFIG_NET_CONFIG_INIT_TIMEOUT`
  specifies how long to wait for the network configuration during the system
  boot. For example, if DHCPv4 is enabled, and if the IPv4 address discovery
  takes too long or the DHCPv4 server is not found, the system will resume
  booting after this number of seconds.

:option:`CONFIG_NET_CONFIG_NEED_IPV6`
  specifies that the application needs IPv6 connectivity. The
  :cpp:func:`net_app_init()` function will wait until it is able to setup an
  IPv6 address for the system before continuing. This means that the IPv6
  duplicate address detection (DAD) has finished and the system has properly
  setup the IPv6 address.

:option:`CONFIG_NET_CONFIG_NEED_IPV6_ROUTER`
  specifies that the application needs IPv6 router connectivity; i.e., it needs
  access to external networks (such as the Internet). The
  :cpp:func:`net_app_init()` function will wait until it receives a router
  advertisement (RA) message from the IPv6 router before continuing.

:option:`CONFIG_NET_CONFIG_NEED_IPV4`
  specifies that the application needs IPv4 connectivity. The
  :cpp:func:`net_app_init()` function will wait, unless a static IP address is
  configured, until it is able to setup an IPv4 address for the network
  subsystem.

Setup
#####

Various system level network configuration options can be added to the project
configuration file. These settings are enabled by the
:option:`CONFIG_NET_CONFIG_SETTINGS` configuration option. This option is disabled
by default, and other net-app options may also be disabled by default if
generic support for the networking feature is disabled. For example, the IPv6
net-app options are only available if generic IPv6 support is enabled.

:option:`CONFIG_NET_CONFIG_MY_IPV6_ADDR`
  This option sets a static IPv6 address for the system. This is typically only
  useful in device testing as normally the system should use SLAAC (IPv6
  Stateless Address Auto Configuration), which is enabled by default in the
  system. The system can be configured to use multiple IPv6 addresses; this is
  controlled by the :option:`CONFIG_NET_IF_UNICAST_IPV6_ADDR_COUNT`
  configuration option.

:option:`CONFIG_NET_CONFIG_PEER_IPV6_ADDR`
  This option specifies what is the peer device IPv6 address. This is only
  useful when testing client/server type applications. This peer address is
  typically used as a parameter when calling :cpp:func:`net_app_connect()`.

:option:`CONFIG_NET_CONFIG_MY_IPV4_ADDR`
  This option sets a static IPv4 address for the system. This is typically
  useful only in device testing as normally the system should use DHCPv4 to
  discover the IPv4 address.

:option:`CONFIG_NET_CONFIG_PEER_IPV4_ADDR`
  This option specifies what is the peer device IPv4 address. This is only
  useful when testing client/server type applications. This peer address is
  typically used as a parameter when connecting to other device.

The following options are only available if IEEE 802.15.4 wireless network
technology support is enabled.

:option:`CONFIG_NET_CONFIG_IEEE802154_DEV_NAME`
  This option specifies the name of the IEEE 802.15.4 device.

:option:`CONFIG_NET_CONFIG_IEEE802154_PAN_ID`
  This option specifies the used PAN identifier.
  Note that the PAN id can be changed at runtime if needed.

:option:`CONFIG_NET_CONFIG_IEEE802154_CHANNEL`
  This option specifies the used radio channel.
  Note that the used channel can be changed at runtime if needed.

:option:`CONFIG_NET_CONFIG_IEEE802154_RADIO_TX_POWER`
  This option specifies the initial radio TX power level. The TX power level can
  be changed at runtime if needed.

:option:`CONFIG_NET_CONFIG_IEEE802154_SECURITY_KEY`
  This option specifies the initially used security key. The security key can be
  changed at runtime if needed.

:option:`CONFIG_NET_CONFIG_IEEE802154_SECURITY_KEY_MODE`
  This option specifies the initially used security key mode. The security key
  mode can be changed at runtime if needed.

:option:`CONFIG_NET_CONFIG_IEEE802154_SECURITY_LEVEL`
  This option specifies the initially used security level. The used security
  level can be changed at runtime if needed.

Client / Server Applications
############################

The net-app API provides functions that enable the application to create
client / server applications easily. If needed, the applications can
have the communication secured by TLS (for TCP connections) or DTLS (for
UDP connections) automatically.

A simple **TCP server** application would make the following net-app API
function calls:

* :cpp:func:`net_app_init_tcp_server()` to configure a local address and TCP
  port.

* :cpp:func:`net_app_set_cb()` to configure callback functions to invoke in
  response to events, such as data reception.

* :cpp:func:`net_app_server_tls()` will optionally setup the system for secured
  connections. To enable the TLS server, also call the
  :cpp:func:`net_app_server_tls_enable()` function.

* :cpp:func:`net_app_listen()` will start listening for new client connections.

Creating a **UDP server** is also very easy:

* :cpp:func:`net_app_init_udp_server()` to configure a local address and UDP
  port.

* :cpp:func:`net_app_set_cb()` to configure callback functions to invoke in
  response to events, such as data reception.

* :cpp:func:`net_app_server_tls()` will optionally setup the system for secured
  connections. To enable the DTLS server, also call the
  :cpp:func:`net_app_server_tls_enable()` function.

* :cpp:func:`net_app_listen()` will start listening for new client connections.

If the server wants to stop listening for connections, it can call
:cpp:func:`net_app_release()`. After this, if the application wants to start
listening for incoming connections again, it must call the server
initialization functions.

For TLS/DTLS connections, the server can be disabled by a call to
:cpp:func:`net_app_server_tls_disable()`. There are separate enable/disable
functions for TLS support because we need a separate crypto thread for calling
mbedtls crypto API functions. The enable/disable TLS functions will
either create the TLS thread or kill it.

A simple **TCP client** application would make the following net-app API
function calls:

* :cpp:func:`net_app_init_tcp_client()` to configure a local address, peer
  address and TCP port. If the DNS resolver support is enabled in the
  project configuration file, then the peer address can be given as a hostname,
  and the API tries to resolve it to IP address before connecting.

* :cpp:func:`net_app_set_cb()` to configure callback functions to invoke in
  response to events, such as data reception.

* :cpp:func:`net_app_client_tls()` will optionally setup the system for secured
  connections. The TLS crypto thread will be automatically created when the
  application calls :cpp:func:`net_app_connect()` function.

* :cpp:func:`net_app_connect()` will initiate a new connection to the peer host.

Creating a **UDP client** is also very easy:

* :cpp:func:`net_app_init_udp_client()` to configure a local address, peer
  address and UDP port. If peer name is a hostname, then it will be
  automatically resolved to IP address if DNS resolver is enabled.

* :cpp:func:`net_app_set_cb()` to configure callback functions to invoke in
  response to events, such as data reception.

* :cpp:func:`net_app_client_tls()` will optionally setup the system for secured
  connections. The DTLS crypto thread will be automatically created when the
  application calls :cpp:func:`net_app_connect()` function.

* :cpp:func:`net_app_connect()` will initiate a new connection to the peer host.
  As the UDP is connectionless protocol, this function is very simple and it
  will just call the connected callback if that is defined.

As both the ``echo_server`` and ``echo_client`` applications use net-app API
functions, please see those applications for more detailed usage examples.

The `net-tools`_ project has information how to test the system if TLS and
DTLS support is enabled. See the **README** file in that project for more
information.

.. _`net-tools`: https://github.com/zephyrproject-rtos/net-tools
