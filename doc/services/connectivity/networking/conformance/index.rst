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

Each suite drives one protocol against an application that enables it, over the
interface that application appears on.

.. list-table::
   :header-rows: 1

   * - Suite
     - System under test
     - Interface
     - Runs as
     - Cases
   * - ``mdns``
     - :zephyr_file:`tests/net/conformance/mdns`
     - ``zeth``
     - any user
     - 8

Adding a suite is described in :ref:`ttcn3_adding_a_suite`.

mDNS
====

The application does nothing but enable the responder and wait. The suite
sends queries from the tester and checks the answers: the records they carry,
the name compression, the cache flush bit, and the silence that should follow a
query for a name the responder does not own.

* ``tc_a_query``
* ``tc_aaaa_query``
* ``tc_aaaa_query_over_ipv6``
* ``tc_unknown_name_is_ignored``
* ``tc_unknown_type_is_ignored``
* ``tc_multiple_answers_share_the_name``
* ``tc_legacy_query_is_answered_conventionally``
* ``tc_answer_count_matches``

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
its own messages, still sets the cache flush bit, uses its own long time to
live, and echoes neither the identifier nor the question. Fixing it means
reworking name compression offsets that are all computed from a fixed header
size. No suite covers it.

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
