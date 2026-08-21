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
     - Cases
   * - ``mdns``
     - :zephyr_file:`tests/net/conformance/mdns`
     - ``zeth``
     - any user
     - 8
   * - ``dns``
     - :zephyr_file:`tests/net/conformance/dns`
     - ``zeth``
     - any user
     - 7
   * - ``sntp``
     - :zephyr_file:`tests/net/conformance/sntp`
     - ``zeth``
     - any user
     - 9
   * - ``coap``
     - :zephyr_file:`tests/net/conformance/coap`
     - ``zeth``
     - any user
     - 8
   * - ``dhcpv4``
     - :zephyr_file:`tests/net/conformance/dhcpv4`
     - ``zeth``
     - root
     - 3
   * - ``arp``
     - :zephyr_file:`tests/net/conformance/arp`
     - ``zethL2``
     - root
     - 5
   * - ``tcp``
     - :zephyr_file:`tests/net/conformance/tcp`
     - ``zethL2``
     - root
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

DNS
===

The application resolves ``conformance.test`` over and over, one lookup at a
time, against the tester acting as its only configured server. That server is
at port 15353 rather than 53 so the suite needs no privilege and cannot collide
with a resolver running on the host. The resolver does not retransmit, so an
unanswered query simply fails and the next one follows, which lets a test case
start at any point and still finish quickly.

* ``tc_query_is_well_formed``
* ``tc_query_id_varies``
* ``tc_query_source_port_varies``
* ``tc_unanswered_query_does_not_wedge``
* ``tc_answer_is_followed_by_more_queries``
* ``tc_malformed_answers_are_survived``
* ``tc_answer_with_wrong_id_is_ignored``

SNTP
====

The application is an ordinary client: it asks the tester for the time, sets
the system clock from the answer, and asks again half a second later. That
last part is what makes the client testable from outside, because a request
carries the time the clock holds when it is sent. Whether a reply was believed
is visible in the request that follows it.

The suite listens on port 12123 rather than 123, so it needs no privileges and
cannot collide with a time daemon on the host.

* ``tc_request_is_well_formed``
* ``tc_transmit_timestamp_varies``
* ``tc_unanswered_request_is_repeated``
* ``tc_reply_sets_the_clock``
* ``tc_originate_timestamp_must_be_echoed``
* ``tc_reply_must_be_mode_server``
* ``tc_kiss_of_death_is_not_a_time_source``
* ``tc_zero_transmit_timestamp_is_rejected``
* ``tc_truncated_reply_is_rejected``

CoAP
====

This suite contributes no TTCN-3 source of its own. It runs the ETSI derived
test cases that the Titan project publishes in ``titan.misc``, pointed at
Zephyr by a configuration file. The application exposes a single ``/test``
resource answering GET, POST, PUT and DELETE.

It is the one suite whose test cases create parallel test components, so it is
run through Titan's main controller and needs ``expect`` installed.

* ``tc_client_TD_COAP_CORE_01`` through ``tc_client_TD_COAP_CORE_08``

DHCPv4
======

The application starts the client and is then left alone; the suite is the
server. The order of the test cases matters, so ``tc_exchange_completes`` is
last: a client that has been given an address stops asking. The suite waits up
to 75 seconds for a message, which outlasts the client's four second backoff as
it doubles.

* ``tc_unanswered_discover_is_repeated``
* ``tc_discover_is_well_formed``
* ``tc_exchange_completes``

ARP
===

The first suite to work below the IP layer, so it reads and writes frames on
``zethL2`` and has to be run as root. The application sends a datagram to a peer
every two seconds, which means an uncached destination keeps producing address
resolution requests for the suite to look at.

* ``tc_it_asks_before_it_sends``
* ``tc_request_is_answered``
* ``tc_unicast_request_is_answered``
* ``tc_request_for_another_address_is_ignored``
* ``tc_reply_is_not_answered``

TCP
===

The application is an echo server on port 4242 plus a loop that connects out to
the peer on port 4243 every three seconds, so the suite can both drive a
connection and watch Zephyr open one. Nothing test specific is compiled in and
no Kconfig option is changed for the test, so the TCP being exercised is the
one that ships.

* ``tc_outbound_connect_is_well_formed``
* ``tc_handshake``
* ``tc_initial_sequence_number_varies``
* ``tc_data_is_acknowledged_and_echoed``
* ``tc_closed_port_is_reset``
* ``tc_close_is_completed``
* ``tc_undersized_data_offset_is_dropped``
* ``tc_sequence_number_wraps``

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

Continuous integration
**********************

These suites are not part of the ordinary Twister run: they need a Titan
installation, a network interface facing the device, and a checkout of the
suites, none of which a normal build has. They are also slow, and they cannot
run at the same time as each other.

They run nightly instead, from
:zephyr_file:`.github/workflows/net_conformance.yml`, which can also be started
by hand from the Actions tab. The job installs the packaged Titan and sets
``TTCN3_DIR=/usr``, brings up both interfaces in a container holding
``NET_ADMIN``, and runs the whole directory as root so that no suite is
skipped. The Twister report and the harness logs are kept as artifacts.

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
