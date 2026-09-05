.. _net_conformance_ndp:

IPv6 neighbour discovery conformance test
#########################################

Overview
********

This runs a TTCN-3 conformance suite against Zephyr's IPv6 neighbour
discovery. The application here is the system under test: it answers for its
own addresses, and sends a datagram to a peer every couple of seconds so that
there is an address for it to resolve. The suite acts as another node on the
link, and lives in the ``net-tools`` repository under ``ttcn3/suites/ndp``.

It is the same shape as :ref:`net_conformance_arp` one layer up: the suite
works below IP, reads and writes frames on ``zethL2``, and builds its own
ethernet, IPv6 and ICMPv6 headers.

Only what the stack does in answer to the suite is covered. What it does when
it starts, the duplicate address detection for its own addresses and the router
solicitations that follow, is over before a suite can attach, so the test that
watches an address being configured makes that happen by advertising a prefix.

Requirements
************

The same as :ref:`net_conformance_arp`: a Titan installation with ``TTCN3_DIR``
set, the third party modules fetched with ``net-tools/ttcn3/fetch-modules.sh``,
the address-less ``zethL2`` interface, and root. The test skips itself with a
reason when any of them is missing.

Running
*******

.. code-block:: console

   export TTCN3_DIR=/usr
   sudo -E ./scripts/twister -p native_sim --enable-slow \
        -T tests/net/conformance/ndp

What is covered
***************

That a solicitation for an address it holds is answered, to the address that
asked, with the override bit set and a target link layer address; that a
solicitation for an address it does not hold is not; that a solicitation from
the unspecified address, which is another node checking an address is free, is
answered to every node on the link and not marked as solicited; that a
solicitation which arrived with a hop limit below 255 came through a router and
is discarded; that one from the unspecified address carrying a source link
layer address contradicts itself and is discarded; that an address it cannot
resolve is asked about before anything is sent to it, and that answering lets
the datagram through; and that a prefix advertised by a router is one it gives
itself an address from, checking first that the address is free.

The interface is given room for a third address and its group, and the
neighbour cache is left at its default size. Sending a solicitation takes a
cache entry even when the address asked about is the interface's own, so a
small cache stops duplicate address detection from being sent at all.

See :ref:`ttcn3_known_gaps` for what is deliberately not tested and why.
