Sample TELNET console application
#################################

Overview
********

This application will setup IPv4/IPv6 addresses on the default
network interface. The telnet console service is started transparently
by the kernel, along with the shell and two shell modules: net and kernel.
Once up and running, you can connect to the target over the network,
using a telnet client.

Requirements
************

- net_tools:

    https://wiki.zephyrproject.org/view/Networking-with-Qemu

Building and Running
********************

QEMU x86
========

These are instructions for how to use this sample application using
QEMU on a Linux host connected to a network with DHCP service.

Follow the net-tools readme from:

    Run 'loop_socat' and 'loop-slip-tap' scripts from net-tools.

    https://gerrit.zephyrproject.org/r/gitweb?p=net-tools.git;a=blob;f=README

.. code-block:: console

    $ ./loop_socat.sh

In another window:

.. code-block:: console

    $ sudo ./loop-slip-tap.sh

Run Zephyr samples/net/telnet application in QEMU:

.. code-block:: console

    $ cd $ZEPHYR_BASE/samples/net/telnet
    $ make pristine && make run

Once started, you should see you IP address details for example:

.. code-block:: console

    [Setup] [INF] main: Starting Telnet sample
    [Setup] [INF] setup_ipv4: IPv4 address: 192.0.2.1
    [Setup] [INF] setup_ipv6: IPv6 address: 2001:db8::1

At this point, your QEMU guest is up and running. Connect to the telnet
console from your linux host this way:

.. code-block:: console

    $ telnet 192.0.2.1
    Telnet escape character is '^]'.
    Trying 192.0.2.1...
    Connected to 192.0.2.1.
    Escape character is '^]'.

Now type enter, the shell prompt will appear and you can enter commands,
for example ``help``.


FRDM_K64F
=========

These are instructions for how to use this sample application running on a
FRDM_K64F board. Unlike running it on QEMU, FRDM_K64F network configuration
for IPv4 will rely on DHCPv4. You cad modify the prj_frdm_k64f.conf to set
static IPv4 addresses if it is really needed.

Instructions to follow building, flashing and observe serial console logs:

    https://wiki.zephyrproject.org/view/NXP_FRDM-K64F#Quick_Start

Connect ethernet cable from FRDM_K64F board to a local network providing
IPv4 address configuration via DHCPv4. Creating your own dhcp server
on a local network is not in the scope of this README.

Build Zephyr samples/net/telnet application:

.. code-block:: console

    $ cd $ZEPHYR_BASE/samples/net/dhcpv4_client
    $ make pristine && make BOARD=frdm_k64f

Flash the resulting Zephyr binary following the FRDM-K64 board documentation
noted above.

From your host computer, open a serial console to your board:

.. code-block:: console

    $ sudo screen /dev/ttyACM0 115200

Plug the Ethernet cable to the FRDM_K64F board.
Reset the board, you should see first on the console:

.. code-block:: console

    [dev/eth_mcux] [INF] eth_0_init: Enabled 100M full-duplex mode.
    [dev/eth_mcux] [DBG] eth_0_init: MAC 00:04:9f:69:c7:36
    shell> [Setup] [INF] main: Starting Telnet sample
    [Setup] [INF] setup_dhcpv4: Running dhcpv4 client...
    [Setup] [INF] setup_ipv6: IPv6 address: 2001:db8::1

And if the DHCPv4 client succeeds, you will soon see something like:

.. code-block:: console

   [Setup] [INF] ipv4_addr_add_handler: IPv4 address: 192.168.0.21
   [Setup] [INF] ipv4_addr_add_handler: Lease time: 86400 seconds
   [Setup] [INF] ipv4_addr_add_handler: Subnet: 255.255.255.0
   [Setup] [INF] ipv4_addr_add_handler: Router: 192.168.0.1

The above result depends on your local network.
At this point you should be able to connect via telnet over the network.
On your linux host:

.. code-block:: console

    $ telnet 192.168.0.21
    Telnet escape character is '^]'.
    Trying 192.168.0.21...
    Connected to 192.168.0.1.
    Escape character is '^]'.

You are now connected, and as for the UART console, you can type in
your commands and get the output through your telnet client.
