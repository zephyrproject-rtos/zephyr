.. _ttcn3_testing:

Protocol conformance testing with TTCN-3
########################################

.. contents::
    :local:
    :depth: 2

Zephyr's network protocols are covered from two directions. The tests under
:zephyr_file:`tests/net` exercise the implementation from the inside, in C,
built into the same image. Conformance suites written in TTCN-3 come at it from
the outside: they speak the protocol over a real network interface, and check
what Zephyr sends against what the standard requires.

The two catch different things. A test written against the implementation
tends to encode what the implementation does. A suite written against the
standard does not know what the implementation does, which is the point.

TTCN-3 is a language standardised by ETSI for writing tests. The suites here
are compiled with `Eclipse Titan`_, an open source TTCN-3 compiler, and are
kept in the ``net-tools`` repository under :file:`ttcn3`, alongside the other
host side tools used for network testing.

.. toctree::
   :maxdepth: 1

   usage.rst
   architecture.rst

How it fits together
********************

The Zephyr side of a conformance test is only the system under test: an
ordinary application, configured to enable the protocol being tested. Nothing
about the test is compiled into it, there is no control channel, and the suite
drives it entirely over the network.

Those applications, and the harness that runs a suite against them, live under
:zephyr_file:`tests/net/conformance`. Twister builds and starts the
application, and a small pytest harness builds the suite with Titan, runs it,
and turns Titan's verdict into a test result.

Each test skips itself when Titan, the third party TTCN-3 modules or the
network interface is missing, so the suites are harmless in a run that has not
been set up for them. See :ref:`ttcn3_running` for what a run needs, and
:ref:`ttcn3_architecture` for how the parts are put together.

.. _ttcn3_suites:

The suites
**********

Which interface a suite uses, and whether it has to be run as root, follow from
what it does: a suite that works below the IP layer reads frames from a packet
socket on a link of its own.

.. list-table::
   :header-rows: 1

   * - Suite
     - System under test
     - Interface
     - Runs as
   * - :zephyr_file:`mdns <tests/net/conformance/mdns/README.rst>`
     - :zephyr_file:`tests/net/conformance/mdns`
     - ``zeth``
     - any user
   * - :zephyr_file:`dnssd <tests/net/conformance/dnssd/README.rst>`
     - :zephyr_file:`tests/net/conformance/dnssd`
     - ``zeth``
     - any user
   * - :zephyr_file:`dns <tests/net/conformance/dns/README.rst>`
     - :zephyr_file:`tests/net/conformance/dns`
     - ``zeth``
     - any user
   * - :zephyr_file:`sntp <tests/net/conformance/sntp/README.rst>`
     - :zephyr_file:`tests/net/conformance/sntp`
     - ``zeth``
     - any user
   * - :zephyr_file:`coap <tests/net/conformance/coap/README.rst>`
     - :zephyr_file:`tests/net/conformance/coap`
     - ``zeth``
     - any user
   * - :zephyr_file:`dhcpv4 <tests/net/conformance/dhcpv4/README.rst>`
     - :zephyr_file:`tests/net/conformance/dhcpv4`
     - ``zeth``
     - root
   * - :zephyr_file:`dhcpv4_server <tests/net/conformance/dhcpv4_server/README.rst>`
     - :zephyr_file:`tests/net/conformance/dhcpv4_server`
     - ``zeth``
     - root
   * - :zephyr_file:`arp <tests/net/conformance/arp/README.rst>`
     - :zephyr_file:`tests/net/conformance/arp`
     - ``zethL2``
     - root
   * - :zephyr_file:`ndp <tests/net/conformance/ndp/README.rst>`
     - :zephyr_file:`tests/net/conformance/ndp`
     - ``zethL2``
     - root
   * - :zephyr_file:`tcp <tests/net/conformance/tcp/README.rst>`
     - :zephyr_file:`tests/net/conformance/tcp`
     - ``zethL2``
     - root

Adding a suite is described in :ref:`ttcn3_adding_a_suite`.

.. _ttcn3_known_gaps:

Known gaps
**********

Where a suite asserts behaviour that does not match the standard, it says so at
the point the assertion is made, so that the divergence is recorded rather than
frozen in silently. What follows is the other kind of gap: ground no suite
covers yet.

DNS-SD legacy unicast queries
=============================

The hostname side of the mDNS responder answers a legacy unicast query the way
:rfc:`6762` section 6.7 asks. The service discovery side does not: it builds
its own messages, sets the cache flush bit on the records that belong to one
instance, uses the lifetimes it would have used for a multicast answer, and
echoes neither the identifier nor the question. Fixing it means reworking name
compression offsets that are all computed from a fixed header size.

The ``dnssd`` suite records this rather than asserting the standard, in
``f_check_legacy_shape``, so that a test does not sit failing until somebody
gets to it. Each check there says what would have to change with it.

CoAP block transfer and observe
===============================

``TD_COAP_BLOCK_01`` and ``TD_COAP_OBS_01`` are not run. They address
``/large`` and ``/obs``, and the application provides only ``/test``; against
it the observe case waits for notifications that never arrive and the run does
not finish. Adding those two resources is the obvious next step.

Overlapping DNS queries
=======================

The resolver renews its source port before sending to a server that has nothing
outstanding, which with the default of one query at a time means every query.
Queries that overlap on one server still share a port, so the check in the
``dns`` suite would not catch a regression in that case. See :rfc:`5452`
section 9.2.

Other TTCN-3 suites
*******************

The Eclipse Titan project publishes protocol modules and test ports for a large
number of protocols, as separate repositories under
`gitlab.eclipse.org/eclipse/titan`_. The suites here build against those rather
than defining their own message formats.

Some complete suites exist there too. ``titan.misc`` contains a CoAP
conformance suite that can be run against the :zephyr:code-sample:`coap-server`
sample; see :ref:`coap_sock_interface` for that one.

An older TTCN-3 suite for TCP, written at Intel for the TCP rewrite, informed
what the ``tcp`` suite covers, but none of its code is used. It drove Zephyr
through a JSON control channel and asserted the stack's internal state names,
and the option that channel needed has been removed; see the 4.5 migration
guide.

.. _Eclipse Titan: https://projects.eclipse.org/projects/tools.titan
.. _gitlab.eclipse.org/eclipse/titan: https://gitlab.eclipse.org/eclipse/titan
