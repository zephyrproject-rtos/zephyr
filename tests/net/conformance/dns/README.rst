.. _net_conformance_dns:

DNS resolver conformance test
#############################

Overview
********

This runs a TTCN-3 conformance suite against Zephyr's DNS resolver. The
application here is the system under test: it asks for the same name over and
over, so that the suite can collect as many queries as a test case needs
without having to trigger anything. The suite acts as the DNS server, and
lives in the ``net-tools`` repository under ``ttcn3/suites/dns``.

The resolver does not retransmit, so a query that goes unanswered simply fails
on the device and the next one follows.

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
   ./scripts/twister -p native_sim --enable-slow -T tests/net/conformance/dns

What is covered
***************

The shape of the queries the resolver puts on the wire, that the query
identifier is unpredictable rather than fixed or counting up, that an
unanswered query does not wedge the resolver, that an answer carrying somebody
else's identifier is ignored, and that a range of malformed answers are
survived.

The suite listens on port 15353 rather than 53, so it needs no privileges and
cannot collide with a resolver running on the host.

See :ref:`ttcn3_known_gaps` for what is deliberately not tested and why.
