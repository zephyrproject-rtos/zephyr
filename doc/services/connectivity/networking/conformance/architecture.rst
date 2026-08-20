.. _ttcn3_architecture:

How the conformance tests are put together
##########################################

.. contents::
    :local:
    :depth: 2

There are four moving parts, spread over two repositories, and they meet only
on the wire: a Zephyr application, a Twister and pytest harness, a shell
wrapper around Eclipse Titan, and the TTCN-3 suite itself.

The pieces
**********

.. graphviz::
   :caption: What builds what, and where the two halves meet
   :alt: Diagram showing Twister and the pytest harness driving both a Zephyr
       application and a Titan built TTCN-3 suite, which meet only at a tap
       interface.

   digraph ttcn3_pieces {
       rankdir=TB;
       node [shape=box, style=filled, fillcolor="#e8e8e8", fontname="sans-serif"];
       edge [arrowsize=0.8];

       twister [label="Twister", fillcolor="#cce5ff"];
       harness [label="pytest harness\n(ttcn3_runner.py)", fillcolor="#cce5ff"];

       subgraph cluster_zephyr {
           label="zephyr";
           style=dashed;
           fontname="sans-serif";
           sut [label="System under test\n(native_sim)"];
       }

       subgraph cluster_nettools {
           label="net-tools";
           style=dashed;
           fontname="sans-serif";
           build [label="build.sh\n+ Eclipse Titan"];
           suite [label="TTCN-3 suite\nexecutable"];
       }

       tap [label="zeth\ntap interface", shape=ellipse, fillcolor="#ffe0b2"];

       twister -> harness [label="harness: pytest"];
       harness -> sut [label="start, read ready line"];
       harness -> build [label="build"];
       build -> suite;
       harness -> suite [label="run, read verdict"];
       sut -> tap [dir=both, label="the protocol"];
       suite -> tap [dir=both];
   }

Twister
   Builds the system under test, starts it, and runs the pytest harness against
   it. It contributes the test identifier, the platform restriction and the
   900 second budget for the whole thing.

pytest harness
   :zephyr_file:`tests/net/conformance/ttcn3_runner.py`, shared by all six
   tests. It decides whether the suite can run at all, takes the interface
   lock, builds the suite, runs it and reads the verdict.

System under test
   An ordinary Zephyr application with the protocol enabled. Nothing about the
   test is compiled into it.

build.sh and Eclipse Titan
   The host side build. Titan compiles TTCN-3 to C++ and generates a makefile;
   :file:`build.sh` arranges the sources so that it can.

TTCN-3 suite executable
   The test itself, driving the protocol from the outside.

Tap interface
   The only thing the two halves share. There is no control channel, no shared
   memory and no test hook in the Zephyr image.

The Zephyr side
***************

The system under test
=====================

Each application is deliberately ordinary: enable the protocol, do whatever
keeps traffic flowing that the suite needs to observe, and print a distinctive
ready line. That line is the only contract between the application and the
harness — it is how the harness knows the stack is up before the suite starts
sending.

A suite directory holds:

.. code-block:: none

   tests/net/conformance/<suite>/
       CMakeLists.txt
       prj.conf
       README.rst
       tests.yaml
       src/main.c
       pytest/pytest.ini
       pytest/conftest.py
       pytest/test_<suite>_conformance.py

Twister integration
===================

Every :file:`tests.yaml` shares the same block:

.. code-block:: yaml

   common:
     harness: pytest
     slow: true
     timeout: 900
     platform_allow:
       - native_sim
     integration_platforms:
       - native_sim

``harness: pytest`` hands the run to the harness rather than reading console
output for a ztest summary. ``slow: true`` keeps the suites out of an ordinary
Twister run, since a full pass takes tens of minutes. The 900 second timeout
has to cover building the suite as well as running it. ``native_sim`` is the
only platform because the tap driver is what puts the system under test on a
real link.

The test itself is three lines: request the lock, wait for the ready line, run
the suite.

.. code-block:: python

   def test_mdns_conformance(network_lock, dut, suite_binary):
       dut.readlines_until(regex='mDNS responder ready', timeout=30.0)
       run_suite(suite_binary, SUITE)

The ``network_lock`` argument comes first on purpose; see below.
:file:`conftest.py` is identical in all six directories and does one thing: put
the shared runner on ``sys.path`` and re-export the lock fixture.

.. _ttcn3_runner:

The harness
***********

.. mermaid::
   :caption: One conformance test, from lock to verdict
   :alt: Sequence diagram showing the harness taking the interface lock before
       starting the system under test, then building and running the suite and
       parsing its verdict before releasing the lock.

   sequenceDiagram
       participant T as Twister
       participant H as Harness<br/>(ttcn3_runner.py)
       participant Z as System under test
       participant B as build.sh + Titan
       participant S as TTCN-3 suite

       T->>H: start test (900 s budget)
       H->>H: flock(LOCK_EX) on the interface
       Note over H,Z: the lock is taken before the DUT fixture,<br/>so nothing starts while another test runs
       H->>Z: start
       Z-->>H: ready line (30 s)
       H->>B: build the suite (1800 s)
       B-->>H: executable
       H->>S: run against the running application (600 s)
       S-->>H: verdict statistics, overall verdict
       H->>H: release the lock
       H-->>T: pass or fail

Everything below lives in
:zephyr_file:`tests/net/conformance/ttcn3_runner.py`.

Holding the interface
=====================

Twister runs each test in its own pytest process, so excluding one test from
another has to work between processes. The harness takes an exclusive
``flock`` on a lock file in a session scoped fixture.

The ordering matters as much as the lock. The fixture is requested *before* the
``dut`` fixture, so the system under test is not even started while another
conformance test holds the interface — two applications answering to
``192.0.2.1`` at once would confuse both suites.

Suite traits
============

Three facts about a suite are read from :file:`suites/<name>/build.conf`:

``MODE=parallel``
   The test cases create parallel test components, so the suite is run through
   Titan's main controller rather than as a single executable, and ``expect``
   has to be installed.

The same file is sourced as a shell fragment by :file:`build.sh`, which is why
it is written as shell assignments; the harness only matches substrings in it.

Building and running the suite
==============================

Building is :file:`build.sh <suite>` with a 1800 second budget, producing
:file:`suites/<suite>/build/<suite>`.

Running has to cope with Titan being installed two different ways. A
distribution package puts its libraries in :file:`{TTCN3_DIR}/lib/titan`, a
source build in :file:`{TTCN3_DIR}/lib`; the harness picks whichever exists and
prepends it to ``LD_LIBRARY_PATH``, and prepends :file:`{TTCN3_DIR}/bin` to
``PATH``. A parallel suite is started with ``ttcn3_start``, any other suite
directly.

The suite is started in a new session, so that a suite which overruns can be
killed along with everything it started — a main controller left running would
hold the interface for the next test.

Turning a verdict into a result
===============================

Two regular expressions match the lines Titan prints at the end of a run, and
three things are asserted: that a verdict was printed at all, that at least one
test case ran, and that the overall verdict is ``pass``. A suite that produced
no output, or that ran nothing because every case was filtered out, fails
rather than quietly passing. See :ref:`ttcn3_verdicts` for what the verdicts
mean.

The host side
*************

Everything below is in the ``net-tools`` repository, under :file:`ttcn3`.

Layout
======

.. code-block:: none

   ttcn3/
       common/            shared TTCN-3 modules and the ethernet test port
       modules/           third party modules, cloned, not in git
       modules.txt        which modules, at which commit
       fetch-modules.sh   clone or check out the pinned commits
       build.sh           build one suite
       suites/<name>/     the suite, its sources.txt and its .cfg

How a suite is built
====================

:file:`build.sh` wipes the suite's build directory and rebuilds it flat,
symlinking every source it needs side by side in one directory.

The flatness is not tidiness, it is a workaround. The makefile Titan generates
builds a dependency rule with ``sed``, using the target stem as the pattern,
and that breaks as soon as a source is named through a path containing a
slash. Every source therefore has to be reachable by its bare name.

The link order is the suite's own sources, then :file:`common`, then the module
sources named in :file:`common/sources.txt`, then those in the suite's own
:file:`sources.txt`. Then ``ttcn3_makefilegen`` generates the makefile — with
``-s`` for a single mode suite, without it for a parallel one — and ``make``
builds it.

The shared TTCN-3 layer
=======================

``Zephyr_SUT`` holds the module parameters every suite shares. A suite never
writes an address or a timeout into itself; it takes them from here, so that a
run can be moved to a different link by editing one configuration file.

.. list-table::
   :header-rows: 1

   * - Parameter
     - Default
     - Meaning
   * - ``tsp_sut_ipv4``
     - ``192.0.2.1``
     - Where Zephyr answers
   * - ``tsp_sut_ipv6``
     - ``2001:db8::1``
     - Where Zephyr answers, IPv6
   * - ``tsp_tester_ipv4``
     - ``192.0.2.2``
     - Where the suite answers
   * - ``tsp_tester_ipv6``
     - ``2001:db8::2``
     - Where the suite answers, IPv6
   * - ``tsp_tester_interface``
     - ``zeth``
     - Needed for link local multicast
   * - ``tsp_sut_hostname``
     - ``zephyr``
     - The name the responder owns
   * - ``tsp_response_timeout``
     - ``5.0``
     - How long an answer may take
   * - ``tsp_silence_timeout``
     - ``2.0``
     - How long silence is watched for

``Zephyr_Transport`` provides the ``Zephyr_Tester`` component, with an
``IPL4asp`` port and helpers to listen, open, send, receive with a guard timer,
and assert that nothing arrives. A socket based suite extends that component
rather than opening sockets itself.

Third party modules
===================

The suites build against the protocol modules and test ports the Titan project
publishes, rather than defining their own message formats. Eleven repositories
are used, each pinned to a commit in :file:`modules.txt` and cloned on demand by
:file:`fetch-modules.sh`. They are not vendored, and the clones are ignored by
git, so a suite that passes today still builds tomorrow. Moving a pin is a one
line edit and a re-run.

The upstream modules are EPL-2.0 while everything written here is Apache-2.0.
Both are OSI approved, which is what Zephyr asks of tooling that never becomes
part of a Zephyr image; see :ref:`external-contributions`.

.. _ttcn3_adding_a_suite:

Adding a suite
**************

A suite has two halves, one in each repository, and one decision to make before
either.

Choosing the shape
==================

**Single or parallel.** Single mode unless the test cases create parallel test
components. Parallel costs Titan's main controller and a dependency on
``expect``, and it makes the suite harder to run by hand.

The host side
=============

Create :file:`suites/<name>` containing the TTCN-3 source, a
:file:`sources.txt` naming the module sources the suite needs, and a
:file:`<name>.cfg` with the module parameters and the list of test cases to
execute. A suite that only runs test cases from a third party module needs no
source of its own — ``coap`` is the worked example.

Take addresses and timeouts from ``Zephyr_SUT``. Add any new upstream module to
:file:`modules.txt` with a pinned commit. Add a :file:`build.conf` if the suite
is parallel, privileged or works below IP.

The Zephyr side
===============

Create an application under :zephyr_file:`tests/net/conformance` with the
layout shown above. The :file:`src/main.c` enables the protocol, generates
whatever traffic the suite needs to observe, and prints a distinctive ready
line. :file:`tests.yaml` copies the common block and names the test
``net.conformance.<name>``.

Of the three pytest files, :file:`pytest.ini` and :file:`conftest.py` are
copied verbatim; :file:`test_<name>_conformance.py` differs only in the suite
name and the ready line it waits for.

Recording what does not match
=============================

Where a suite asserts behaviour that does not match the standard, say so at the
point the assertion is made, so that the divergence is recorded rather than
frozen in silently. Ground that no suite covers at all belongs in
:ref:`ttcn3_known_gaps`, and a new suite is a good moment to add a row to
:ref:`ttcn3_suites`.

Trying it out
=============

Run the two halves by hand first, as in :ref:`ttcn3_running_by_hand`, and only
go through Twister once the suite passes. Expect the first build to be slow,
and remember that a missing prerequisite skips the test rather than failing it
— a suite that seems to pass instantly probably never ran.
