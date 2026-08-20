.. _net_conformance_tcp:

TCP conformance test
####################

Overview
********

This runs a TTCN-3 conformance suite against Zephyr's TCP, RFC 9293. The suite
builds every header itself, down to the ethernet one, and reads frames off the
link; it lives in the ``net-tools`` repository under ``ttcn3/suites/tcp``.

The application here is the system under test, and it is an ordinary one: a
server that accepts a connection and echoes what it is sent, so the suite can
open one and drive it, and a client that connects out over and over, so the
suite can look at what Zephyr sends when it is the one starting. Nothing about
the test is compiled into it and there is no control channel, so the TCP being
exercised is the one that ships.

Requirements
************

The same as :ref:`net_conformance_arp`: a Titan installation, the third party
modules, the address-less ``zethL2`` interface, and root. The test skips itself
with a reason when any of these is missing.

Running
*******

.. code-block:: console

   export TTCN3_DIR=/usr
   sudo -E ./scripts/twister -p native_sim --enable-slow \
     -T tests/net/conformance/tcp

What is covered
***************

The three way handshake and its sequence accounting; that the initial sequence
number is neither fixed nor counting up; that data is acknowledged exactly and
echoed back; that a connection to a port nothing listens on is refused with a
reset rather than by silence; that a close is acknowledged and answered with a
close; that a segment claiming a header shorter than a header is dropped; that
sequence numbers wrapping does not break the accounting; and the shape of the
connection Zephyr opens when it is the one connecting.

Relation to the older TTCN-3 TCP suite
**************************************

Intel published a TCP suite for the TCP rewrite, in the ``net-test-suites``
repository. Its scenarios informed the list above, but none of its code is used
here: forty-one of its forty-three test cases drove Zephyr through a JSON
control channel and asserted the stack's internal state names, which tests the
implementation rather than the protocol, and needed a build whose TCP had been
deliberately weakened to be testable.

That channel is removed earlier in the same series that adds these tests; see
the migration guide for the option it went with.
