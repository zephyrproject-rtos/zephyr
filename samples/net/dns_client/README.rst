DNS Client Application
######################

Overview
********

The DNS resolver or DNS client sample application implements a basic
DNS resolver according to RFC 1035. Supported DNS answers are:
IPv4/IPv6 addresses and CNAME.

If a CNAME is received, the DNS resolver will create another DNS query.
The number of additional queries is controlled by the
DNS_RESOLVER_ADDITIONAL_QUERIES Kconfig variable.

For more information about DNS configuration variables, see:
:file:`subsys/net/lib/dns/Kconfig`. The DNS resolver API can be found at
:file:`include/net/dns_client.h`. The sample code can be found at:
:file:`samples/net/dns_client`.

The return codes used by this sample application are described by the following table.

=======		=====	================================================
Macro		Value	Description
-------		-----	------------------------------------------------
-EINVAL		-22	if an invalid parameter was detected
-EIO		-5	network error
-ENOMEM		-12	memory error, perhaps related to network buffers
=======		=====	================================================

A return code of 0 must be interpreted as success.

Requirements
************

- net_tools:

    https://wiki.zephyrproject.org/view/Networking-with-Qemu

- screen terminal emulator or equivalent.

- For the Arduino 101 board, the ENC28J60 Ethernet module is required.

- dnsmasq application. The dnsmasq version used in this sample is:

.. code-block:: console

    dnsmasq -v
    Dnsmasq version 2.76  Copyright (c) 2000-2016 Simon Kelley

Wiring
******

The ENC28J60 module is an Ethernet device with SPI interface.
The following pins must be connected from the ENC28J60 device to the
Arduino 101 board:

===========	===================================
Arduino 101	ENC28J60 (pin numbers on the board)
-----------	-----------------------------------
D13		SCK  (1)
D12		SO   (3)
D11		SI   (2)
D10		CS   (7)
D04		INT  (5)
3.3V		VCC  (10)
GDN		GND  (9)
===========	===================================


Building and Running
********************

Read the :file:`samples/net/dns_client/src/config.h` file.
Change the IP addresses and DNS server port according to the
LAN environment.

Network Configuration
=====================

For example, if your LAN is 192.168.0.0/16, the IPv4 addresses must be
similar to:

.. code-block:: c

    #define LOCAL_ADDR		{ { { 192, 168, 1, 101 } } }
    #define REMOTE_ADDR		{ { { 192, 168, 1, 1 } } }

where REMOTE_ADDR is the address of the DNS server.

The DNS server port must be specified also, for example:

.. code-block:: c

   #define PEER_PORT		5353

assumes that the DNS server is listening at UDP port 5353.

DNS server
==========

The dnsmasq tool may be used for testing purposes. Open a terminal
window and type:

.. code-block:: console

    $ dnsmasq -p 5353 -d

NOTE: some systems may require root privileges to run dnsmaq, use sudo or su.

If dnsmasq fails to start with an error like this:

.. code-block:: console

    dnsmasq: failed to create listening socket for port 5353: Address already in use

Open a terminal window and type:

.. code-block:: console

    $ killall -s KILL dnsmasq

Try to launch the dnsmasq application again.


QEMU x86
========

Open a terminal window and type:

.. code-block:: console

    $ make


Run 'loop_socat.sh' and 'loop-slip-tap.sh' as indicated at:

    https://gerrit.zephyrproject.org/r/gitweb?p=net-tools.git;a=blob;f=README

Set the IPv4 ip address:

.. code-block:: console

    $ ip addr add 192.0.2.2/24 dev tap0

Open a terminal where the project was build (i.e. :file:`samples/net/dns_client`) and type:

.. code-block:: console

    $ make run

FRDM K64F
=========

Open a terminal window and type:

.. code-block:: console

    $ make BOARD=frdm_k64f


The FRDM K64F board is detected as a USB storage device. The board
must be mounted (i.e. to /mnt) to 'flash' the binary:

.. code-block:: console

    $ cp outdir/frdm_k64f/zephyr.bin /mnt


See https://developer.mbed.org/platforms/frdm-k64f/ for more information
about this board.

Open a terminal window and type:

.. code-block:: console

    $ screen /dev/ttyACM0 115200


Use 'dmesg' to find the right USB device.

Once the binary is loaded into the FRDM board, press the RESET button.

Arduino 101
===========

Open a terminal window and type:

.. code-block:: console

	$ make BOARD=arduino_101

To load the binary in the development board follow the steps
in :ref:`arduino_101`.

Open a terminal window and type:

.. code-block:: console

    $ screen /dev/ttyUSB0 115200

Use 'dmesg' to find the right USB device.

Once the binary is loaded into the Arduino 101 board, press the RESET button.

Sample Output
=============

IPv4 (CONFIG_NET_IPV6=n, CONFIG_NET_IPV4=y)

.. literalinclude:: sample_output_IPv4.txt


IPv6 (CONFIG_NET_IPV6=y, CONFIG_NET_IPV4=n)

.. literalinclude:: sample_output_IPv6.txt


Note: IPv6 addresses obtained via dnsmasq and :file:`/etc/hosts`.

Known Issues
============

- The above sample contains a rc: -22 (-EINVAL). This is the expected behavior
  for that domain name.

- If a lot of rc: -5 (-EIO) errors are flooding the screen, increment
  APP_SLEEP_MSECS to 500 or even 1000. See :file:`samples/net/dns_client/src/config.h`.

- IPv4: there is still an issue not yet isolated that causes the application
  to fail during the first two queries. The issue lies between L2 (ARP) and
  UDP and it only appears during application startup.

