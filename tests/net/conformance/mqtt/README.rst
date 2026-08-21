.. _net_conformance_mqtt:

MQTT client conformance test
############################

Overview
********

This runs a TTCN-3 conformance suite against Zephyr's MQTT client. The
application here is the system under test: an ordinary publisher that connects
to the broker, publishes to one topic, keeps the connection alive in between,
and starts again when the connection goes. The suite acts as the broker, and
lives in the ``net-tools`` repository under ``ttcn3/suites/mqtt``.

Starting again is what lets the test cases stand alone. Each takes the
connection the client makes to it, drives the part of the exchange it is about
and closes; the client notices and connects afresh for the next.

This is the first suite to work over a connection rather than over datagrams.
The shared TTCN-3 layer grew ``f_tcp_listen``, ``f_tcp_accept`` and ``f_send``
for it, and the port is told how to find the length of an MQTT message so that
it hands whole messages up rather than whatever arrived together.

Requirements
************

The same as :ref:`net_conformance_mdns`: a Titan installation with
``TTCN3_DIR`` set, the third party modules fetched with
``net-tools/ttcn3/fetch-modules.sh``, and a ``zeth`` interface. The broker port
is not a privileged one, so no root is needed. The test skips itself with a
reason when a requirement is missing.

Running
*******

.. code-block:: console

   export TTCN3_DIR=/usr
   ./scripts/twister -p native_sim --enable-slow -T tests/net/conformance/mqtt

What is covered
***************

That the connect names the protocol, its level, the client and a keep alive;
that nothing is sent until the broker has accepted the connection; that a
publish carries the topic, the quality of service and an identifier to
acknowledge; that an acknowledged message is not sent again; that identifiers
are not reused while a message is outstanding; that an idle connection is kept
alive with a ping; and that a refused connection is not used.

Re-sending a message that was never acknowledged is not covered. MQTT 3.1.1
asks for that only when a client reconnects to a session it left behind, and
this client connects cleanly every time; while a connection is up, whether to
re-send is left to whatever is doing the publishing.

The keep alive is short so that the ping test does not wait a minute, and the
publish interval is longer still, so that the connection is genuinely idle
sometimes and a ping falls due.

The suite covers MQTT 3.1.1. Zephyr also implements MQTT 5.0
(:kconfig:option:`CONFIG_MQTT_VERSION_5_0`), for which the Titan project
publishes no protocol module; see :ref:`ttcn3_known_gaps`.
