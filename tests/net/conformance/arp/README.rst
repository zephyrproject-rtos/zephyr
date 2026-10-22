.. _net_conformance_arp:

Address resolution conformance test
###################################

Overview
********

This runs a TTCN-3 conformance suite against Zephyr's address resolution,
RFC 826. It is the first conformance test to work below the IP layer: the
suite reads and writes ethernet frames on the link rather than using sockets.
It lives in the ``net-tools`` repository under ``ttcn3/suites/arp``.

The application here is the system under test. It answers address resolution
for its own address without any help, and it sends a datagram to a peer on the
link over and over so that it also has to ask for somebody else's.

A link of its own
*****************

This test uses a second interface, ``zethL2``, which is given no IP address.
Linux answers address resolution for any address it holds on any interface
unless told otherwise, and an answer from the host would be indistinguishable
from an answer from Zephyr. With no address, and with ``arp_ignore`` set to
refuse every local address, nothing on the link answers but the suite.

Requirements
************

A Titan installation with ``TTCN3_DIR`` set, the third party modules fetched
with ``net-tools/ttcn3/fetch-modules.sh``, the ``zethL2`` interface, and root:
reading frames off a link needs a packet socket. The test skips itself with a
reason when any of these is missing.

.. code-block:: console

   cd $ZEPHYR_BASE/../tools/net-tools
   sudo ./net-setup.sh --config zeth-l2.conf --iface zethL2 start

Running
*******

.. code-block:: console

   export TTCN3_DIR=/usr
   sudo -E ./scripts/twister -p native_sim --enable-slow \
     -T tests/net/conformance/arp

What is covered
***************

That a request for its own address is answered, whether it was broadcast or
addressed to it directly; that the answer carries the right addresses in the
right places; that a request for an address it does not hold is not answered;
that a reply is not answered; and that it asks before it sends to an address
it has no entry for.
