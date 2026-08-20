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

The application here is the system under test: a CoAP server exposing one
resource, ``/test``, answering GET, POST, PUT and DELETE with Content, Created,
Changed and Deleted. That is what the core test cases address.

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

``TD_COAP_BLOCK_01`` and ``TD_COAP_OBS_01`` are not run. They address a
``/large`` and an ``/obs`` resource, which this system under test does not
provide. See :ref:`ttcn3_known_gaps`.
