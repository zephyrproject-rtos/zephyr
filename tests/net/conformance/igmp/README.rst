.. _net_conformance_igmp:

IGMP host conformance test
##########################

Overview
********

This runs a TTCN-3 conformance suite against Zephyr as an IGMP host, RFC 3376
and RFC 2236. The suite plays the multicast router on the link: it watches what
the host reports of its own accord and asks it questions, reading and writing
frames on the link rather than using sockets. It lives in the ``net-tools``
repository under ``ttcn3/suites/igmp``.

The application here is the system under test. Everything the suite looks at
is what the stack does when a group is joined or left and when a querier asks,
so all the application does is join one group, keep it for a while, leave it,
wait, and start again, for as long as it runs. A test that wants to see a join
or a leave waits for the next one.

It is the same shape as :ref:`net_conformance_arp` one layer up: the suite
works below IP, reads and writes frames on ``zethL2``, and builds its own
ethernet, IPv4 and IGMP headers.

Requirements
************

The same as :ref:`net_conformance_arp`: a Titan installation with ``TTCN3_DIR``
set, the third party modules fetched with ``net-tools/ttcn3/fetch-modules.sh``,
the ``zethL2`` interface, and root, because reading frames off a link needs a
packet socket. The test skips itself with a reason when any of these is
missing.

.. code-block:: console

   cd $ZEPHYR_BASE/../tools/net-tools
   sudo ./net-setup.sh --config zeth-l2.conf --iface zethL2 start

Running
*******

.. code-block:: console

   export TTCN3_DIR=/usr
   sudo -E ./scripts/twister -p native_sim --enable-slow \
     -T tests/net/conformance/igmp

What is covered
***************

That a join is reported at once to the IGMPv3 routers, with TTL 1, the Router
Alert option and a change to EXCLUDE mode for the group, and that the report is
repeated within the Unsolicited Report Interval as many times as the Robustness
Variable says; that a General Query is answered within its Max Resp Time with
the current state of the joined group and never with the all systems group;
that a Group-Specific Query is answered for that group alone, and not at all
for a group the host has not joined; that a query without the Router Alert
option is ignored; that a leave is reported as a change to INCLUDE mode; that
an IGMPv2 querier moves the host to IGMPv2, where it reports and leaves in
version 2 and stops answering for a group another member has reported; and
that an IGMPv1 querier moves it to IGMPv1.

The last three change the host's compatibility mode for minutes, and the
application is started once for the whole run, so they run last, in that
order.
