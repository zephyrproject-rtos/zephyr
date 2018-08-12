.. _rpl-border-router-sample:

RPL Border Router
#################

Overview
********

The RPL border router sample application for Zephyr provides a HTTP(S) server
and net shell for management purposes. Typically border router would be used to
connect to IEEE 802.15.4 network but Bluetooth IPSP network functionality is
also possible.

The source code for this sample application can be found at:
:file:`samples/net/rpl_border_router`.

Requirements
************

- Real device like Freedom Board (FRDM-K64F) with MCR20A IEEE 802.15.4 support.
- Linux machine with web browser and the screen terminal emulator (optional).
- Ethernet access for management purposes (optional).

Note that there is no support for running an RPL border router in QEMU, as the
border router requires access to a real radio network technology such as
IEEE 802.15.4, which is not available in QEMU.

For testing purposes you can compile the RPL border router for QEMU and do some
testing with the web UI. But with QEMU, it is not possible to connect to RPL
network and get information about the RPL nodes.

Building and Running
********************

By default, the integrated HTTP server is configured to listen at port 80.
If you want to modify the HTTP server config options, please check
the configuration settings in :file:`samples/net/rpl_border_router/src/main.c`
file and also in the :file:`samples/net/rpl_border_router/src/config.h` file.

This sample code supports both static and dynamic (DHCPv4) IP addresses that
can be defined in the project configuration file:

.. code-block:: console

	CONFIG_NET_CONFIG_MY_IPV6_ADDR="2001:db8::1"
	CONFIG_NET_CONFIG_MY_IPV4_ADDR="192.0.2.1"

Note that the IPv4 address is only used for connection to the integrated web
server that provides an admin web page for management purposes. The web server
can also be connected using IPv6 address. If you do not have a web management
network interface for your host computer, then IPv4 support can be disabled
in the configuration file by setting :option:`CONFIG_NET_IPV4` to "n".
The RPL router uses only IPv6 when routing the network traffic.

Note that the default project configuration file
:file:`samples/net/rpl_border_router/prj.conf` does not currently provide
a working system as there are no boards that would provide suitable network
interface support. The prj.conf file is provided only to compile test the
border router sample application.

It is possible to use the border router application with these boards and
add-on cards:

* `FRDM-K64F with Freescale CR20A card <http://www.nxp.com/products/developer-resources/hardware-development-tools/freedom-development-boards/freedom-development-board-for-mcr20a-wireless-transceiver:FRDM-CR20A>`_

You can build the application like this for CR20A:

.. zephyr-app-commands::
   :zephyr-app: samples/net/rpl_border_router
   :board: frdm_k64f
   :conf: prj_frdm_k64f_mcr20a.conf
   :goals: build flash
   :compact:

By default, the RPL border router application enables net shell support and
provides some useful commands for debugging and viewing the network status.

The **br repair** command will cause the RPL network to re-configure itself.

.. code-block:: console

	shell> br repair
	[rpl-br/shell] [INF] br_repair: Starting global repair...

The **net rpl** command first prints out static compile time configuration
settings. Then it prints information about runtime configuration of the system.

.. code-block:: console

	shell> net rpl
	RPL Configuration
	=================
	RPL mode                     : mesh
	Used objective function      : MRHOF
	Used routing metric          : none
	Mode of operation (MOP)      : Storing, no mcast (MOP2)
	Send probes to nodes         : disabled
	Max instances                : 1
	Max DAG / instance           : 2
	Min hop rank increment       : 256
	Initial link metric          : 2
	RPL preference value         : 0
	DAG grounded by default      : no
	Default instance id          : 30 (0x1e)
	Insert Hop-by-hop option     : yes
	Specify DAG when sending DAO : yes
	DIO min interval             : 12 (4096 ms)
	DIO doublings interval       : 8
	DIO redundancy value         : 10
	DAO sending timer value      : 4 sec
	DAO max retransmissions      : 4
	Node expecting DAO ack       : yes
	Send DIS periodically        : yes
	DIS interval                 : 60 sec
	Default route lifetime unit  : 65535 sec
	Default route lifetime       : 255

	Runtime status
	==============
	Default instance (id 30) : 0xa80081e0 (active)
	Instance DAGs   :
	[ 1]* fde3:2cda:3eea:4d14::1 prefix fde3:2cda:3eea:4d14::/64 rank 256/65535 ver 255 flags GJ parent 0x00000000

	No parents found.

The **net nbr** command prints information about currently found IPv6 neighbor
nodes. In this example there are two leaf nodes that are part of this RPL
network.

.. code-block:: console

	shell> net nbr
	     Neighbor   Flags   Interface  State        Remain  Link                    Address
	[ 1] 0xa80065e0 1/0/1/0 0xa8007140 reachable      2920  00:12:4B:00:00:00:00:01 fe80::212:4b00:0:1
	[ 2] 0xa8006660 1/0/1/0 0xa8007140 stale             0  00:12:4B:00:00:00:00:03 fe80::212:4b00:0:3

The **nbr route** command prints information about currently found IPv6 routes.
In this example all the nodes are directly connected to this RPL border router
root node.

.. code-block:: console

	shell> net route
	IPv6 routes for interface 0xa8007140
	====================================
	IPv6 prefix : fde3:2cda:3eea:4d14::212:4b00:0:3/128
	        neighbor  : 0xa80065e0
	        link addr : 00:12:4B:00:00:00:00:03
	IPv6 prefix : fde3:2cda:3eea:4d14::212:4b00:0:1/128
	        neighbor  : 0xa8006660
	        link addr : 00:12:4B:00:00:00:00:01

The IEEE 802.15.4 shell support is enabled by default, so the **ieee15_4**
command can be used to change the IEEE 802.15.4 network parameters such as
used channel or PAN id, if needed.

.. code-block:: console

	shell> ieee15_4 set_chan 15
	Channel 15 set

The border router sample application provides integrated HTTP(S) server.
Currently the admin support is very rudimentary but you can try it by connecting
to http://192.0.2.1 or http://[2001:db8::1] using web browser.
