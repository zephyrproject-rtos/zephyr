.. _net_stats-sample:

Network Statistics Sample Application
#####################################

Overview
********

This sample shows how to query (and display) network statistics from a user
application.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/stats`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

A good way to run this sample application is with QEMU or native_posix board
as described in :ref:`networking_with_host`.

Follow these steps to build the network statistics sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/net/stats
   :board: <board to use>
   :conf: prj.conf
   :goals: build
   :compact:

If everything is configured correctly, the application will periodically print
current network statistics to the console.

.. code-block:: console

    Global network statistics
    IPv6 recv      27	sent	8	drop	0	forwarded	0
    IPv6 ND recv   2	sent	5	drop	2
    IPv6 MLD recv  0	sent	3	drop	0
    IPv4 recv      20	sent	0	drop	20	forwarded	0
    IP vhlerr      0	hblener	0	lblener	0
    IP fragerr     0	chkerr	0	protoer	0
    ICMP recv      15	sent	3	drop	13
    ICMP typeer    0	chkerr	0
    UDP recv       0	sent	0	drop	30
    UDP chkerr     0
    TCP bytes recv 0	sent	0
    TCP seg recv   0	sent	0	drop	0
    TCP seg resent 0	chkerr	0	ackerr	0
    TCP seg rsterr 0	rst	0	re-xmit	0
    TCP conn drop  0	connrst	0
    Bytes received 7056
    Bytes sent     564
    Processing err 1
