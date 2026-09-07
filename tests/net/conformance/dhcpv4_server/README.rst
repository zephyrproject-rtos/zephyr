.. _net_conformance_dhcpv4_server:

DHCPv4 server conformance test
##############################

Overview
********

This runs a TTCN-3 conformance suite against Zephyr's DHCPv4 server. The
application here is the system under test: it starts the server on the default
interface and then does nothing, because everything the suite looks at is the
server answering. The suite acts as the client, and lives in the ``net-tools``
repository under ``ttcn3/suites/dhcpv4_server``.

It is the mirror image of :ref:`net_conformance_dhcpv4`, which drives the
client with the tester acting as the server.

Requirements
************

The same as :ref:`net_conformance_mdns`, and root as well: the tester binds the
port a DHCP client binds, and :rfc:`2131` fixes both ports below 1024 so
neither can be moved. The test skips itself with a reason when any of them is
missing.

Running
*******

.. code-block:: console

   export TTCN3_DIR=/usr
   sudo -E ./scripts/twister -p native_sim --enable-slow \
        -T tests/net/conformance/dhcpv4_server

What is covered
***************

That a Discover is answered with an Offer of a free address from the pool,
carrying what a client needs to accept it; that the Request accepting the offer
is acknowledged for the same address; that one client asking twice is offered
the same address and a second client a different one; that a client returning
with an address it already had is acknowledged; that a request for an address
on another network is refused, once; that a released address is no longer the
client's to keep; that a declined address is not handed out again; and that an
Inform is answered without an address or a lease.

Every test case appears as a client of its own so that none depends on what an
earlier one left behind. The server keeps its bindings for the life of the run,
so the pool is larger than the default to leave room for them all.

See :ref:`ttcn3_known_gaps` for what is deliberately not tested and why.
