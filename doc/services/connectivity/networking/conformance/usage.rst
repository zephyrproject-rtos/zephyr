.. _ttcn3_running:

Running the conformance suites
##############################

.. contents::
    :local:
    :depth: 2

A run goes like this. Twister builds the system under test and starts it, the
harness builds the TTCN-3 suite with Titan, the suite talks to the running
application over a tap interface, and Titan's verdict becomes the test result.

What a run needs
****************

* **A Titan installation**, with ``TTCN3_DIR`` pointing at it. See
  :ref:`ttcn3_installing_titan`.
* **The third party TTCN-3 modules**, fetched once with
  :file:`ttcn3/fetch-modules.sh`. They are cloned at pinned commits and are not
  part of the ``net-tools`` repository.
* **A checkout of net-tools**, which a west workspace already has: it is in
  :file:`west.yml` under the ``tools`` group, which is not filtered out.
* **make**, because that is how Titan builds a suite.
* **The** ``zeth`` **interface**, or ``zethL2`` for the one suite that works
  below the IP layer. See :ref:`ttcn3_interfaces`.
* **Root**, for the two suites that cannot avoid a privileged port or a
  packet socket.
* **expect**, for the one suite that runs through Titan's main controller.

The harness looks for net-tools in ``NET_TOOLS_BASE`` if that is set, otherwise
at :file:`../tools/net-tools/ttcn3` relative to ``ZEPHYR_BASE`` and then one
directory further up. All of this is checked before anything is built, and a
missing piece skips the test with a reason rather than failing it.

.. _ttcn3_installing_titan:

Installing Titan
****************

Most distributions package a Titan, and that is what continuous integration
installs:

.. code-block:: console

   sudo apt install --no-install-recommends eclipse-titan expect
   export TTCN3_DIR=/usr

The packaged version trails the protocol modules the suites build against, so
if a suite fails to compile, build a current Titan from source instead.

Building Titan from source
==========================

:file:`net-tools/docker/Dockerfile.ttcn3` builds Titan into :file:`/opt/titan`
and is the reference for doing it by hand, either as a container or as a recipe
to follow. It is a local option, not what continuous integration uses.

Two things a hand built Titan has to get right. Titan is configured through a
:file:`Makefile.personal` in its source tree rather than a ``configure`` script,
and ``TTCN3_DIR`` there is the install prefix. And ``make install`` has to be
serial: parts of the runtime include headers that another part generates, and a
parallel make loses that race.

.. _ttcn3_interfaces:

Setting up the network interfaces
*********************************

The suites use two tap interfaces, because a suite working below the IP layer
cannot share a link with a host that answers for itself.

``zeth``, the shared interface
==============================

Used by the ``mdns``, ``dns``, ``coap`` and ``dhcpv4`` suites, where the tester
is just another host on the link:

.. code-block:: console

   cd $ZEPHYR_BASE/../tools/net-tools
   sudo ./net-setup.sh --config zeth.conf start

The host holds ``192.0.2.2/24`` and ``2001:db8::2``; Zephyr answers on
``192.0.2.1`` and ``2001:db8::1``. Tear it down with ``stop`` in place of
``start``.

``zethL2``, the address-less interface
======================================

Used by the ``arp`` and ``tcp`` suites:

.. code-block:: console

   sudo ./net-setup.sh --config zeth-l2.conf --iface zethL2 start

This interface is deliberately given no IP address. Linux answers address
resolution and neighbour discovery for any address it holds on any interface
unless it is told otherwise, and an answer from the host would be
indistinguishable from an answer from Zephyr. The tester speaks raw frames, so
it needs no address of its own.

The configuration sets three sysctls to stop the host joining in:
``arp_ignore=8`` so it answers address resolution for no local address at all,
``arp_announce=2`` so it never answers with an address this interface does not
hold, and ``disable_ipv6=1`` so there are no neighbour advertisements or router
solicitations.

The name ``zethL2`` is not freely choosable; see :ref:`ttcn3_test_network`.

Running the suites with Twister
*******************************

Fetch the third party modules once. The script is safe to re-run:

.. code-block:: console

   cd $ZEPHYR_BASE/../tools/net-tools
   ./ttcn3/fetch-modules.sh

Then run the tests:

.. code-block:: console

   export TTCN3_DIR=/usr
   cd $ZEPHYR_BASE
   ./scripts/twister -p native_sim --enable-slow -T tests/net/conformance

``--enable-slow`` is required: the suites mark themselves slow, because a full
run takes tens of minutes. ``native_sim`` is the only platform they allow.

A single suite is selected by its test identifier, which is
``net.conformance.<suite>``:

.. code-block:: console

   ./scripts/twister -p native_sim --enable-slow -T tests/net/conformance \
       -s net.conformance.mdns

They also carry the ``net`` and ``conformance`` tags, so ``--tag conformance``
picks up all six.

Running as root
===============

Two suites have to be run as root. DHCP is defined on ports 67 and 68 and
there is no way to move it elsewhere, so the tester cannot avoid binding a
privileged port; and reading frames off a link needs a packet socket. Those
tests skip themselves when they are not run with enough privilege.

Use ``sudo -E`` so that ``TTCN3_DIR`` and the rest of the environment survive.
A run is either wholly privileged or wholly not — see :ref:`ttcn3_runner` for
why the two cannot be mixed.

Why a run is serial
===================

Every system under test answers to the same address on the same interface, so
only one conformance test can be running at a time. They take an exclusive lock
on the interface and wait for each other, which means a run of the whole
directory is serial however many jobs Twister is given.

.. _ttcn3_running_by_hand:

Running a suite by hand
***********************

Twister is convenient but slow to go round. While writing or debugging a suite,
run the two halves yourself.

Start the system under test and leave it running:

.. code-block:: console

   cd $ZEPHYR_BASE
   west build -p -b native_sim -d ../build/mdns tests/net/conformance/mdns
   ../build/mdns/zephyr/zephyr.exe

Build and run the suite against it:

.. code-block:: console

   cd $ZEPHYR_BASE/../tools/net-tools/ttcn3
   ./build.sh mdns
   cd suites/mdns/build
   ./mdns ../mdns.cfg

For a suite whose test cases create parallel test components, ``coap`` today,
start it through the main controller instead:

.. code-block:: console

   ttcn3_start ./coap ../coap.cfg

A single test case is run by naming it:

.. code-block:: console

   ./mdns ../mdns.cfg MDNS_Suite.tc_a_query

Addresses, the interface and the timeouts all come from the
``[MODULE_PARAMETERS]`` section of the suite's configuration file, so a run can
be moved to a different link by editing one file rather than the suite.

One thing the harness does that you have to do yourself: put Titan's library
directory on ``LD_LIBRARY_PATH``.

.. _ttcn3_verdicts:

Reading the result
******************

A Titan run ends with a count of each verdict and a verdict for the run:

.. code-block:: console

   Verdict statistics: 0 none (0.00 %), 7 pass (100.00 %), 0 inconc (0.00 %), 0 fail (0.00 %), 0 error (0.00 %).
   Test execution summary: 7 test cases were executed. Overall verdict: pass

``inconc`` means a test case could not reach a conclusion, usually because
something it depended on did not happen. It is not a pass. ``error`` means the
suite itself failed, rather than the system under test.

The evidence is in two places. Titan writes a log per suite into the build
directory, named from the ``LogFile`` setting in the configuration file. Twister
writes :file:`twister_harness.log` under its output directory, which carries the
whole suite output at ``INFO``.

When a suite is skipped
***********************

Everything a suite needs is checked before anything is built, and a missing
piece skips the test rather than failing it. The reasons, in the order they are
checked:

.. list-table::
   :header-rows: 1

   * - Reason
     - What to do
   * - ``TTCN3_DIR is unset``
     - Install Titan and export ``TTCN3_DIR``; see :ref:`ttcn3_installing_titan`
   * - ``make is not installed``
     - Install make; Titan builds a suite with a generated makefile
   * - ``no TTCN-3 suites under ...``
     - net-tools was not found; set ``NET_TOOLS_BASE``
   * - ``... has no <suite> suite``
     - The net-tools checkout predates the suite; update it
   * - ``third party modules are missing``
     - Run :file:`ttcn3/fetch-modules.sh`
   * - ``the <iface> interface does not exist``
     - Create it with :file:`net-setup.sh`; see :ref:`ttcn3_interfaces`
   * - ``ttcn3_start is not in TTCN3_DIR/bin``
     - Install ``expect`` and a Titan that ships the main controller
   * - ``has to be run as root``
     - Re-run under ``sudo -E``, or use the script

Troubleshooting
***************

The suite does not compile
==========================

Almost always a packaged Titan that trails the protocol modules the suite
builds against. Build Titan from source.

The suite sees nothing and times out
====================================

Check that the application and the suite are on the same interface: a suite
working below IP wants ``zethL2``, and the application has to be built with a
``host-interface`` property naming the same interface, which the test sets in
:file:`boards/native_sim.overlay`. If the host is answering on the link instead
of Zephyr, the ``zethL2`` sysctls did not take; confirm with
``sysctl net.ipv4.conf.zethL2.arp_ignore``.

A run hangs or is cut off
=========================

Four timeouts nest around a run, and which one fires says where the problem is:
30 seconds for the application's ready line, 1800 seconds for the suite build,
600 seconds for the suite run, and 900 seconds for the Twister test as a whole.
A suite that overruns is killed along with its whole process group, so no main
controller is left behind.

Leftover state
==============

Tear the interfaces down with :file:`net-setup.sh` and ``stop``. The lock the
tests take is a file named :file:`zephyr-net-conformance-<euid>.lock` in the
temporary directory; it is released when the process exits, so a stale one is
harmless.
