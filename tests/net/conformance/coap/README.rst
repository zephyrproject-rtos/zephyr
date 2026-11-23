.. _net_conformance_coap:

CoAP server conformance test
############################

Overview
********

This runs the ETSI derived CoAP conformance test cases against Zephyr's CoAP
server. The test cases are not written here: they are the suite the Eclipse
Titan project publishes in ``titan.misc``, pinned and built by the harness in
the ``net-tools`` repository under ``ttcn3/suites/coap``, which contributes the
configuration that points them at Zephyr.

The application here is the system under test: a CoAP server exposing the
three resources the test cases address. ``/test`` answers GET, POST, PUT and
DELETE with Content, Created, Changed and Deleted. ``/large`` is bigger than
one block, so a GET of it is a block-wise transfer. ``/obs`` can be observed
and changes every couple of seconds.

Requirements
************

The same as :ref:`net_conformance_mdns`, plus ``expect``: this suite creates
parallel test components, so it is run through Titan's main controller rather
than as a single process. The test skips itself with a reason when anything is
missing.

Running
*******

.. code-block:: console

   export TTCN3_DIR=/usr
   ./scripts/twister -p native_sim --enable-slow -T tests/net/conformance/coap

What is covered
***************

``TD_COAP_CORE_01`` to ``TD_COAP_CORE_08``: the four methods on a resource,
each over a confirmable and a non-confirmable request, checking the response
code, the response type, and that the token and message identifier are echoed.

``TD_COAP_BLOCK_01``: a GET of ``/large`` in blocks of the size the client asks
for, each response carrying the Block2 option with the right number and more
bit, until the last.

``TD_COAP_OBS_01``: a GET of ``/obs`` with the Observe option registers an
observer, which the response says with an Observe option of its own; the
changes that follow arrive as confirmable notifications, and a reset in place
of an acknowledgment ends the observation. The registration uses an empty
token, which the server takes as the token it is.
