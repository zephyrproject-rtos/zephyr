.. _net_conformance_sntp:

SNTP client conformance test
############################

Overview
********

This runs a TTCN-3 conformance suite against Zephyr's SNTP client. The
application here is the system under test: an ordinary client that asks the
server for the time, sets the system clock from the answer, and asks again half
a second later. The suite acts as the time server, and lives in the
``net-tools`` repository under ``ttcn3/suites/sntp``.

Setting the clock is what makes the interesting half of the client testable
from outside. ``sntp_query()`` stamps every request with the current real
time, so the request that follows a reply says whether that reply was accepted:
it carries the offered time if it was, and the old time if it was not. No
control channel into the device is needed to see it.

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
   ./scripts/twister -p native_sim --enable-slow -T tests/net/conformance/sntp

What is covered
***************

The shape of the request, field by field, against the table in :rfc:`4330`
section 4; that the transmit timestamp changes from one request to the next, so
that a reply cannot be replayed against a later request; that an unanswered
request is retransmitted rather than left until the next query; and that a
correct reply sets the clock.

Then the replies a client has to refuse: one that does not carry back the
transmit timestamp of the request it claims to answer, one that is mode 3
rather than mode 4, a kiss-o'-death packet with a stratum of 0, one whose
transmit timestamp is 0, and one that is shorter than a message can be.

The suite listens on port 12123 rather than 123, so it needs no privileges and
cannot collide with a time daemon running on the host. Zephyr accepts a port
there because the address is given to ``sntp_simple_addr()`` with the port
already filled in.

See :ref:`ttcn3_known_gaps` for what is deliberately not tested and why.
