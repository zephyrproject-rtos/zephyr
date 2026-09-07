.. _net_conformance_mdns:

mDNS responder conformance test
###############################

Overview
********

This runs a TTCN-3 conformance suite against Zephyr's mDNS responder. The
application here is only the system under test: it enables the responder and
waits. The suite that queries it and checks the answers lives in the
``net-tools`` repository, under ``ttcn3/suites/mdns``, and is built with
`Eclipse Titan`_.

Nothing about the test is compiled into Zephyr. The suite talks to the
responder over the network exactly as any other host on the link would, which
is what makes it a conformance test rather than a unit test.

Requirements
************

* A Titan installation, with ``TTCN3_DIR`` pointing at it. Either
  ``sudo apt install --no-install-recommends eclipse-titan`` and
  ``export TTCN3_DIR=/usr``, or the source build that
  ``net-tools/docker/Dockerfile.ttcn3`` performs.
* The third party TTCN-3 modules, fetched once with
  ``net-tools/ttcn3/fetch-modules.sh``.
* A ``zeth`` interface facing the device, created with
  ``net-tools/net-setup.sh``.

The test skips itself, rather than failing, when any of these is missing, so
it is harmless in a run that is not set up for it.

Running
*******

.. code-block:: console

   cd $ZEPHYR_BASE/../tools/net-tools
   ./ttcn3/fetch-modules.sh
   sudo ./net-setup.sh --config zeth.conf start

   export TTCN3_DIR=/usr
   cd $ZEPHYR_BASE
   ./scripts/twister -p native_sim --enable-slow -T tests/net/conformance/mdns

Tear the interface down afterwards:

.. code-block:: console

   sudo ./net-setup.sh --config zeth.conf stop

What is covered
***************

Name resolution over IPv4 and IPv6, the shape of the records that come back,
and that the responder stays silent about names it does not own. See
:ref:`ttcn3_suites` for the current list, and :ref:`ttcn3_known_gaps` for the
divergences from RFC 6762 that the suite records.

.. _Eclipse Titan: https://projects.eclipse.org/projects/tools.titan
