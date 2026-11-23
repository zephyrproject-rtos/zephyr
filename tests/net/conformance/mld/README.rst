.. _net_conformance_mld:

MLD listener conformance test
#############################

Overview
********

This runs a TTCN-3 conformance suite against Zephyr as a Multicast Listener
Discovery listener, RFC 3810 and RFC 2710. The suite plays the multicast
router on the link: it watches what the node reports of its own accord and
asks it questions, reading and writing frames on the link rather than using
sockets. It lives in the ``net-tools`` repository under ``ttcn3/suites/mld``.

The application here is the system under test. Everything the suite looks at
is what the stack does when a multicast address is listened to or left and
when a querier asks, so all the application does is join one address, keep it
for a while, leave it, wait, and start again, for as long as it runs. A test
that wants to see a join or a leave waits for the next one.

It is the IPv6 twin of the IGMP host test: the suite works below IP,
reads and writes frames on ``zethL2``, and builds its own ethernet, IPv6 and
MLD headers.

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
     -T tests/net/conformance/mld

What is covered
***************

That a join is reported at once to the MLDv2 routers, from a link-local
address, with hop limit 1, the Router Alert option in a Hop-by-Hop Options
header and a change to EXCLUDE mode for the address, and that the report is
repeated within the Unsolicited Report Interval as many times as the
Robustness Variable says; that a General Query is answered within its Maximum
Response Delay with the current state of the address and never with the
all-nodes address; that a Multicast Address Specific Query is answered for
that address alone, and not at all for an address the node does not listen to;
that a query without the Router Alert option, with a hop limit other than 1 or
from an address that is not link-local is ignored; that a leave is reported as
a change to INCLUDE mode; and that an MLDv1 querier moves the node to MLDv1,
where it reports and leaves in version 1 and stops answering for an address
another listener has reported.

The last two change the node's compatibility mode for minutes, and the
application is started once for the whole run, so they run last, in that
order.
