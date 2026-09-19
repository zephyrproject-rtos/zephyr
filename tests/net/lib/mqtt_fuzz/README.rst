.. Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
.. SPDX-License-Identifier: Apache-2.0

MQTT libFuzzer harness
***********************

This is a test, not a sample: it is driven by twister
(``tests.yaml``) and the ``fuzz_scripts/`` helpers, and is not
registered in the samples index.

Overview
========

This harness implements a libFuzzer target that exercises the Zephyr MQTT
decoder (``subsys/net/lib/mqtt``).  The objective is to discover parsing
bugs, assertion failures, and undefined behaviour in the MQTT packet
decoding logic by feeding coverage-guided, mutated byte streams directly
into the MQTT state machine.

Both MQTT 5.0 and MQTT 3.1.0 protocol variants are exercised.  A custom
in-memory transport replaces real TCP sockets so that libFuzzer can
inject arbitrary bytes without any network stack involvement.

.. note::

   ``fuzz_one()`` (and everything it calls, including the MQTT library
   itself) must run in thread context, never from ``fuzz_isr()``.  The
   MQTT library takes a mutex, and ``k_mutex_lock()`` asserts that it is
   not called from an ISR.  ``fuzz_isr()`` only signals a semaphore; a
   main-thread loop takes the semaphore and calls ``fuzz_one()``.  Do not
   reintroduce direct calls into the MQTT library from the ISR.

Fuzzing Modes
=============

The first byte of every fuzz input selects one of four operating modes
via its two least-significant bits:

.. list-table::
   :header-rows: 1
   :widths: 10 20 70

   * - Bits ``[1:0]``
     - Mode
     - Description
   * - ``0x00``
     - Pre-connected, MQTT 5.0
     - Client starts in the pre-connected state; fuzz data is fed as the
       server response (exercises CONNACK parsing).
   * - ``0x01``
     - Connected, MQTT 5.0
     - A synthetic CONNACK is injected first to bring the client to the
       connected state; fuzz data is then fed as subsequent server
       packets in a multi-packet loop.
   * - ``0x02``
     - Pre-connected, MQTT 3.1.0
     - Same as ``0x00`` but using the MQTT 3.1.0 protocol branch.
   * - ``0x03``
     - Connected, MQTT 3.1.0
     - Same as ``0x01`` but using the MQTT 3.1.0 protocol branch.

The harness is guided by an MQTT 5.0 mutation dictionary
(``mqtt5.dict``) that provides structured byte sequences for packet
types, property IDs, reason codes, and common field values.  The seed
corpus itself is **not** part of this repository: it is treated as
external, unversioned state passed in via ``$MQTT_FUZZ_CORPUS`` (see
below), since a growing binary corpus does not belong in source
control.

How this is tested
===================

Following the pattern used by ``tests/bsim``, this test is split into
two layers:

* **twister** owns building and *bounded* execution.  ``tests.yaml``
  defines ``net.mqtt.fuzz.smoke`` (a fast PR-gating boot check),
  ``net.mqtt.fuzz.campaign`` (a ``slow: true``, sanitizer-enabled, 1-hour
  bounded libFuzzer run via a pytest harness), and
  ``net.mqtt.fuzz.coverage.build`` (a build-only ``CONFIG_COVERAGE=y``
  variant so that config cannot bitrot).
* **fuzz_scripts/** (plus ``compile.sh``, ``fuzz_common.source``, and
  ``ci.fuzz.sh`` in this directory) own *indefinite* execution outside
  twister: long-running campaigns, corpus merging, and crash triage.
  ``ci.fuzz.sh`` is the single entry point a CI job needs to know about;
  any bound on how long it runs is imposed externally (e.g. via
  ``timeout(1)``), never by the fuzzer itself.

Building and Running
=====================

Set up the environment
-----------------------

.. code-block:: console

   $ export ZEPHYR_BASE=/path/to/zephyr
   $ export ZEPHYR_TOOLCHAIN_VARIANT=llvm
   $ export PATH="/usr/lib/llvm-18/bin:${PATH}"
   $ export MQTT_FUZZ_CORPUS=/path/to/external/corpus

Build
-----

.. code-block:: console

   $ west build -p always -b native_sim/native/64 \
         tests/net/lib/mqtt_fuzz \
         -- -DCONFIG_COVERAGE=y \
            -DCONFIG_BOOT_BANNER=n

The ``-DCONFIG_COVERAGE=y`` flag instruments the binary with ``--coverage``
so that gcov counter data (``*.gcda``) is written into the build tree when
the fuzzer exits.  ``compile.sh`` wraps this build (with ASAN/UBSAN/ASSERT
by default, or ``--coverage`` for this variant) and is what
``ci.fuzz.sh`` and the twister campaign scenario use.

Logging is disabled in ``prj.conf`` (``CONFIG_LOG=n``) rather than on the
command line.  Because almost every fuzz case is malformed by design, the
MQTT receive path would otherwise log a warning or error per packet and
drown the console in millions of lines.

Run the fuzzer
---------------

.. code-block:: console

   $ ./build/zephyr/zephyr.exe \
         "${MQTT_FUZZ_CORPUS}" \
         -dict=tests/net/lib/mqtt_fuzz/mqtt5.dict \
         -max_len=4096 \
         -verbosity=0 \
         -print_final_stats=1

For an indefinite campaign with the same flags used in CI, prefer
``fuzz_scripts/mqtt_campaign.sh`` over invoking the binary by hand.

A run may end with libFuzzer reporting ``fuzz target exited`` and writing
a ``crash-*`` artifact from inside ``nsi_exit()``.  This is not
necessarily an MQTT defect: the native simulator's ``exit()`` and a real
crash both terminate the process the same way, and libFuzzer cannot tell
a deliberate exit from a crash, so it conservatively saves whatever input
was in flight.  Such artifacts replay cleanly (exit 0); this is exactly
what ``fuzz_scripts/mqtt_triage.sh`` checks for before reporting a
finding.  Pass ``-fork=1`` (as the campaign scripts do) so that one
worker's exit does not end the whole campaign.

Collecting Coverage
====================

After the fuzzer exits (or is interrupted with ``Ctrl-C``), gcov counter
files (``*.gcda``) are written next to the object files in the build tree.
Follow these steps to produce an HTML coverage report.

Step 1 — Wrap llvm-cov as a gcov tool
--------------------------------------

``lcov`` expects a GCC-compatible ``gcov``; ``llvm-cov gcov`` provides that
but needs a wrapper because the sub-command cannot be passed via
``--gcov-tool``.

.. code-block:: console

   $ printf '#!/bin/bash\nexec llvm-cov gcov "$@"\n' > llvm-gcov.sh
   $ chmod +x llvm-gcov.sh

Step 2 — Capture the counter data
------------------------------------

.. code-block:: console

   $ mkdir -p coverage_report
   $ lcov --capture --directory build \
         --gcov-tool ./llvm-gcov.sh \
         --rc lcov_branch_coverage=1 \
         --ignore-errors source,graph \
         -o coverage_report/lcov_raw.info

Step 3 — Filter to MQTT sources
----------------------------------

.. code-block:: console

   $ lcov --extract coverage_report/lcov_raw.info \
         '*/subsys/net/lib/mqtt/*' \
         '*/tests/net/lib/mqtt_fuzz/src/*' \
         --rc lcov_branch_coverage=1 \
         -o coverage_report/lcov_mqtt.info

Step 4 — Generate HTML report
--------------------------------

.. code-block:: console

   $ genhtml coverage_report/lcov_mqtt.info \
         --output-directory coverage_report/html
   $ xdg-open coverage_report/html/index.html

Historical Coverage Numbers
============================

The figures below are from a fuzzing run recorded on 2026-08-10, kept for
reference only.  They were measured on a plain ``CONFIG_COVERAGE=y``
build **without** ``CONFIG_ASAN``/``CONFIG_UBSAN``/``CONFIG_ASSERT``
enabled, so they say nothing about sanitizer findings, and they predate
the twister/``fuzz_scripts`` split described above — treat them as a
rough historical baseline, not a current target.

Overall
-------

.. list-table::
   :header-rows: 1
   :widths: 20 15 15 15

   * - Metric
     - Hit
     - Total
     - Coverage
   * - Lines
     - 1,238
     - 2,124
     - **58.3 %**
   * - Functions
     - 99
     - 155
     - **63.9 %**
   * - Branches
     - 382
     - 841
     - **45.4 %**

Per-file breakdown
-------------------

.. list-table::
   :header-rows: 1
   :widths: 40 14 14 14 14

   * - File
     - Lines
     - Functions
     - Branches
     - Notes
   * - ``mqtt_decoder.c``
     - 98.0 %
     - 100.0 %
     - 92.7 %
     - Primary target; near-complete coverage.
   * - ``mqtt_rx.c``
     - 97.3 %
     - 100.0 %
     - 85.9 %
     - Receive path fully exercised.
   * - ``mqtt_internal.h``
     - 100.0 %
     - 100.0 %
     - N/A
     - Small inline helpers; fully covered.
   * - ``src/main.c`` (harness)
     - 91.0 %
     - 90.9 %
     - 85.7 %
     - Harness itself well covered.
   * - ``mqtt_transport.c``
     - 83.3 %
     - 80.0 %
     - N/A
     - Custom transport dispatch covered.
   * - ``mqtt_os.h``
     - 68.4 %
     - 80.0 %
     - 0.0 %
     - OS abstraction layer.
   * - ``mqtt.c``
     - 31.7 %
     - 41.9 %
     - 20.0 %
     - Higher-level API (publish, subscribe,
       ping, abort) not called by harness.
   * - ``mqtt_encoder.c``
     - 32.6 %
     - 46.4 %
     - 20.9 %
     - Only connect/disconnect encoders
       exercised; publish/subscribe/auth
       encoders untouched.
   * - ``mqtt_transport_socket_tcp.c``
     - 0.0 %
     - 0.0 %
     - 0.0 %
     - Real TCP transport; never invoked
       because a custom transport is used.

Coverage gaps and next steps
------------------------------

* ``mqtt.c`` and ``mqtt_encoder.c`` coverage is low (~32 %) because the
  harness only drives the *receive* path.  Adding fuzzing modes that
  call ``mqtt_publish()``, ``mqtt_subscribe()``, and ``mqtt_ping()``
  would close this gap.
* ``mqtt_transport_socket_tcp.c`` is intentionally uncovered; a
  separate network-level fuzzer would be needed to exercise it.
* Branch coverage across all files sits at **45.4 %**; adding
  corpus seeds that trigger error-return paths (malformed remaining
  lengths, unknown property IDs, reason-code edge cases) would improve
  this metric.
