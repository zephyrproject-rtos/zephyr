.. zephyr:code-sample:: online-connectivity-check
   :name: Online Connectivity Checker for Connection Manager
   :relevant-api: conn_mgr http_client tls_credentials secure_sockets_options

   Verify system network connectivity using connection manager's connectivity
   checker and a user-specified network/host.

Overview
********

This sample application enables the online connectivity checker in network
:ref:`connection manager <conn_mgr_overview>`. The checker tries to connect to
a user-specified host either by sending an ICMPv4/6 Echo-request ping message,
or by doing a HTTP(S) GET request. If the connection can be established,
a ``NET_EVENT_CONNECTIVITY_ONLINE`` is generated and the application can monitor
this event and then act on it. If the connectivity to user-specified host is lost,
then a ``NET_EVENT_CONNECTIVITY_OFFLINE`` event is generated.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/connectivity_check`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

You can use this application on a supported board, including
running it inside QEMU as described in :ref:`networking_with_qemu`
or with ``native_sim`` board.

Build the sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/connectivity_check
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

As the connectivity check is meant to connect to some outside network
with public IP addresses, it might be necessary to setup network settings
on the host side if you run the sample in an emulated environment.

It is possible to see how the checker works by utilizing a Docker container
from the `net-tools <https://github.com/zephyrproject-rtos/net-tools>`_ project.

Running connectivity_check against Docker Container
===================================================

The Docker container support is provided to make it easier to test the sample in QEMU or
``native_sim`` board. You can of course use any board with network capabilities to run
the sample. In that case you might need to adjust the configuration options to suit
your network environment.

The Docker tests by default use ``native_sim`` board. The ``connectivity_check`` sample
application is run in Linux host and it connects to a Docker container which contains
needed server components like HTTP and HTTPS servers, a DNS server and a DHCP server.

Get the ``net-tools`` project and build the net-tools container.

In a terminal window:

.. code-block:: console

    $ cd tools/net-tools
    $ cd docker
    $ docker build -t net-tools .

Run the test script which compiles the connectivity_check application for ``native_sim``
board, then starts the Docker container, and finally starts the ``zephyr.exe``
application process which uses the network services provided by the container.

.. code-block:: console

    $ cd $ZEPHYR_BASE
    $ scripts/net/run-sample-tests.sh samples/net/connectivity_check

Note that ``run-sample-tests.sh`` script can be stopped manually by pressing
:kbd:`CTRL+A` :kbd:`x`. The application will run for a couple of minutes and try to
verify the online connectivity.

Example of the sample application log output is here. The HTTP GET request output is
printed by the simple HTTP server running in the Docker container. The Docker container
also runs the DHCPv4 server, and the DNS server returns proper IP addresses when the
application queries the IP address of the ``online.zephyr.test`` host.

.. code-block:: console

    *** Booting Zephyr OS build v3.6.0-1029-g5020f1fa8ff1 ***
    [00:00:00.000,000] <dbg> conn_mgr: conn_mgr_mon_initial_state: (): Iface 0x808e968 UP
    [00:00:00.000,000] <dbg> conn_mgr: conn_mgr_mon_thread_fn: (): Connection Manager started
    [00:00:00.000,000] <inf> net_config: Initializing network
    [00:00:00.000,000] <dbg> conn_mgr: conn_mgr_ipv4_events_handler: (net_mgmt): IPv4 event 0xe0040001 received on iface 1 (0x808e968)
    [00:00:00.000,000] <dbg> conn_mgr: conn_mgr_ipv4_events_handler: (net_mgmt): Iface index 0
    [00:00:00.000,000] <inf> net_config: IPv4 address: 192.0.2.1
    [00:00:00.000,000] <inf> net_config: Running dhcpv4 client...
    [00:00:00.000,000] <dbg> conn_mgr: conn_mgr_ipv4_events_handler: (net_mgmt): IPv4 event 0xe0040007 received on iface 1 (0x808e968)
    [00:00:00.000,000] <dbg> conn_mgr: conn_mgr_ipv6_events_handler: (net_mgmt): IPv6 event 0xe0600003 received on iface 1 (0x808e968)
    [00:00:00.000,000] <dbg> conn_mgr: conn_mgr_ipv6_events_handler: (net_mgmt): Iface index 0
    [00:00:00.000,000] <dbg> conn_mgr: conn_mgr_ipv6_events_handler: (net_mgmt): IPv6 event 0xe0600007 received on iface 1 (0x808e968)
    [00:00:00.000,000] <dbg> conn_mgr: conn_mgr_ipv6_events_handler: (net_mgmt): Iface index 0
    [00:00:00.000,000] <dbg> conn_mgr: conn_mgr_ipv6_events_handler: (net_mgmt): IPv6 event 0xe0600001 received on iface 1 (0x808e968)
    [00:00:00.000,000] <dbg> conn_mgr: conn_mgr_ipv6_events_handler: (net_mgmt): Iface index 0
    [00:00:00.110,000] <inf> net_config: IPv6 address: 2001:db8::1
    [00:00:00.110,000] <dbg> conn_mgr: conn_mgr_ipv6_events_handler: (net_mgmt): IPv6 event 0xe060000d received on iface 1 (0x808e968)
    [00:00:00.110,000] <dbg> conn_mgr: conn_mgr_ipv6_events_handler: (net_mgmt): Iface index 0
    [00:00:00.110,000] <inf> net_config: IPv6 address: 2001:db8::1
    [00:00:00.110,000] <dbg> conn_mgr: conn_mgr_ipv6_events_handler: (net_mgmt): IPv6 event 0xe060000d received on iface 1 (0x808e968)
    [00:00:00.110,000] <dbg> conn_mgr: conn_mgr_ipv6_events_handler: (net_mgmt): Iface index 0
    [00:00:00.110,000] <dbg> conn_mgr: conn_mgr_set_online_check_strategy: (main): Setting online connectivity check strategy to http
    [00:00:00.110,000] <dbg> conn_mgr: resolve_host: (conn_mgr_monitor): Resolving online.zephyr.test
    [00:00:00.240,000] <dbg> conn_mgr: do_online_http_check: (conn_mgr_monitor): Connecting to 192.0.2.2:8000
    [00:00:00.300,000] <dbg> conn_mgr: do_online_http_check: (conn_mgr_monitor): Sending HTTP Get request to 192.0.2.2:8000 (1)
    ::ffff:192.0.2.1 - - [15/Mar/2024 12:28:35] "GET http://online.zephyr.test:8000/testing HTTP/1.1" 200 -
    [00:00:00.360,000] <dbg> conn_mgr: response_cb: (conn_mgr_monitor): All the data received (161 bytes)
    [00:00:00.360,000] <inf> conn_mgr: Response status OK
    [00:00:00.360,000] <dbg> conn_mgr: exec_http_query: (conn_mgr_monitor): Sending Online Connectivity event for interface 1
    [00:00:00.360,000] <dbg> net_trickle: net_trickle_create: (conn_mgr_monitor): Imin 60000 Imax 1 k 1 Imax_abs 120000
    [00:00:00.360,000] <dbg> net_trickle: get_t: (conn_mgr_monitor): [53107, 106214)
    [00:00:00.360,000] <dbg> net_trickle: setup_new_interval: (conn_mgr_monitor): new interval at 360 ends 106575 t 54923 I 106215
    [00:00:00.360,000] <dbg> net_trickle: net_trickle_start: (conn_mgr_monitor): start 360 end 106575 in [53107 , 106215)
    [00:00:00.360,000] <inf> net_connectivity_check_sample: Connected online on interface eth0 (0x808e968)
    [00:00:10.140,000] <inf> net_dhcpv4: Received: 192.0.2.17
    [00:00:10.140,000] <inf> net_config: IPv4 address: 192.0.2.17
    [00:00:10.140,000] <inf> net_config: Lease time: 3600 seconds
    [00:00:10.140,000] <inf> net_config: Subnet: 255.255.255.0
    [00:00:10.140,000] <inf> net_config: Router: 192.0.2.2
    [00:00:10.140,000] <dbg> conn_mgr: conn_mgr_ipv4_events_handler: (net_mgmt): IPv4 event 0xe0040001 received on iface 1 (0x808e968)
    [00:00:10.140,000] <dbg> conn_mgr: conn_mgr_ipv4_events_handler: (net_mgmt): Iface index 0
    [00:00:55.300,000] <dbg> net_trickle: inteval_timeout: (sysworkq): Trickle timeout at 55300
    [00:00:55.300,000] <dbg> net_trickle: inteval_timeout: (sysworkq): TX ok 1 c(0) < k(1)
    [00:00:55.300,000] <dbg> conn_mgr: verifier_cb: (sysworkq): Verifier 0x8090c6c callback called (suppress 1)
    [00:00:55.300,000] <dbg> conn_mgr: verifier_cb: (sysworkq): No data transfer seen, not considered online.
    [00:00:55.300,000] <inf> net_connectivity_check_sample: Network no longer online
    [00:00:55.300,000] <dbg> conn_mgr: do_online_http_check: (conn_mgr_monitor): Connecting to 192.0.2.2:8000
    [00:00:55.320,000] <dbg> conn_mgr: do_online_http_check: (conn_mgr_monitor): Sending HTTP Get request to 192.0.2.2:8000 (1)
    ::ffff:192.0.2.17 - - [15/Mar/2024 12:29:30] "GET http://online.zephyr.test:8000/testing HTTP/1.1" 200 -
    [00:00:55.380,000] <dbg> conn_mgr: response_cb: (conn_mgr_monitor): All the data received (161 bytes)
    [00:00:55.380,000] <inf> conn_mgr: Response status OK
    [00:00:55.380,000] <dbg> conn_mgr: exec_http_query: (conn_mgr_monitor): Sending Online Connectivity event for interface 1
    [00:00:55.380,000] <dbg> net_trickle: net_trickle_create: (conn_mgr_monitor): Imin 60000 Imax 1 k 1 Imax_abs 120000
    [00:00:55.380,000] <dbg> net_trickle: get_t: (conn_mgr_monitor): [52021, 104042)
    [00:00:55.380,000] <dbg> net_trickle: setup_new_interval: (conn_mgr_monitor): new interval at 55380 ends 159423 t 70291 I 104043
    [00:00:55.380,000] <dbg> net_trickle: net_trickle_start: (conn_mgr_monitor): start 55380 end 159423 in [52021 , 104043)
    [00:00:55.380,000] <inf> net_connectivity_check_sample: Connected online on interface eth0 (0x808e968)

Note that the Trickle algorithm values seen above, are only to be used in this
sample application as they cause the launch the online check once / minute which
is too often for any real life application.
