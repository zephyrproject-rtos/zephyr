.. _net_conformance_dnssd:

DNS service discovery conformance test
######################################

Overview
********

This runs a TTCN-3 conformance suite against Zephyr's DNS service discovery.
The application here is the system under test: it registers one service, listens
on the port it advertises, and waits. The suite acts as a client looking for
services, and lives in the ``net-tools`` repository under ``ttcn3/suites/dnssd``.

The listening socket matters even though nothing ever connects to it. The
responder checks that a service is bound before advertising it, so a
registration whose port nothing listens on is silently never mentioned.

Requirements
************

The same as :ref:`net_conformance_mdns`: a Titan installation with
``TTCN3_DIR`` set, the third party modules fetched with
``net-tools/ttcn3/fetch-modules.sh``, and a ``zeth`` interface. The test skips
itself with a reason when any of them is missing.

Running
*******

.. code-block:: console

   export TTCN3_DIR=/usr
   ./scripts/twister -p native_sim --enable-slow -T tests/net/conformance/dnssd

What is covered
***************

The three questions a client asks, in order: what service types are on the link,
what instances of a type are on the link, and where an instance is. The last
answer has to carry the service record and the address with it, so that naming
an instance was enough and no second lookup is needed. A query for a service
type nothing offers is not answered.

The last test records how an answer to a legacy unicast query differs from what
:rfc:`6762` section 6.7 asks, rather than asserting the standard and leaving a
test that fails until somebody gets to it. See :ref:`ttcn3_known_gaps`.

Queries go to the multicast DNS group address, so anything else on the link
that speaks multicast DNS answers them too. The suite skips those answers
rather than failing on them, which is what lets it run on a machine with a
service discovery daemon of its own.
