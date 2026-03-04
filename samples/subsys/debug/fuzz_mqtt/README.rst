.. zephyr:code-sample:: fuzzing
   :name: Fuzzing
   :relevant-api: mqtt_socket

   Fuzz the Zephyr MQTT decoder using LLVM libFuzzer.

Overview
********

This sample implements a libFuzzer harness that targets the Zephyr MQTT
decoder (``subsys/net/lib/mqtt``).  The objective is to discover parsing
bugs, assertion failures, and undefined behaviour in the MQTT packet
decoding logic by feeding coverage-guided, mutated byte streams directly
into the MQTT state machine.

Both MQTT 5.0 and MQTT 3.1.0 protocol variants are exercised.  A custom
in-memory transport replaces real TCP sockets so that libFuzzer can
inject arbitrary bytes without any network stack involvement.

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

The harness is guided by a seed corpus of ~1,345 entries
(``corpus/``) and an MQTT 5.0 mutation dictionary (``mqtt5.dict``)
that provides structured byte sequences for packet types, property IDs,
reason codes, and common field values.

Building and Running
********************

Set up the environment
======================

.. code-block:: console

   $ export ZEPHYR_BASE=/path/to/zephyr
   $ export ZEPHYR_TOOLCHAIN_VARIANT=llvm
   $ export PATH="/usr/lib/llvm-18/bin:${PATH}"

Build
=====

.. code-block:: console

   $ west build -p always -b native_sim/native/64 \
         samples/subsys/debug/fuzz \
         -- -DCONFIG_COVERAGE=y \
            -DCONFIG_LOG=n \
            -DCONFIG_BOOT_BANNER=n

The ``-DCONFIG_COVERAGE=y`` flag instruments the binary with
``-fprofile-instr-generate`` and ``-fcoverage-mapping`` so that raw
profile data (``fuzz-<pid>.profraw``) is written when the fuzzer exits.

Run the fuzzer
==============

.. code-block:: console

   $ ./build/zephyr/zephyr.exe \
         samples/subsys/debug/fuzz/corpus \
         -dict=samples/subsys/debug/fuzz/mqtt5.dict \
         -max_len=4096 \
         -verbosity=0 \
         -print_final_stats=1

Example output
==============

.. code-block:: console

   INFO: Running with entropic power schedule (0xFF, 100).
   INFO: Seed: 3271501816
   INFO: Loaded 1 modules   (8765 inline 8-bit counters): 8765 [0x...)
   INFO: Loaded 1 PC tables (8765 PCs): 8765 [0x...)
   INFO: 1345 files loaded, total size: 246732 bytes
   #1345   INITED cov: 724 ft: 2653 corp: 412/75Kb exec/s: 0 rss: 52Mb
   #1408   NEW    cov: 725 ft: 2657 corp: 413/75Kb ...
   ...
   stat::number_of_executed_units: 386801
   stat::average_exec_per_sec:     38291
   stat::new_units_added:          847
   stat::slowest_unit_time_sec:    0
   stat::peak_rss_mb:              56

Collecting Coverage
*******************

After the fuzzer exits (or is interrupted with ``Ctrl-C``), a raw
profile file ``fuzz-<pid>.profraw`` is written to the working directory.
Follow these steps to produce an HTML coverage report.

Step 1 — Merge raw profile data
================================

.. code-block:: console

   $ mkdir -p coverage
   $ mv fuzz-*.profraw coverage/
   $ llvm-profdata merge -sparse coverage/fuzz-*.profraw \
         -o coverage/fuzz.profdata

Step 2 — Export to LCOV format
================================

.. code-block:: console

   $ mkdir -p coverage_report
   $ llvm-cov export build/zephyr/zephyr.exe \
         -instr-profile=coverage/fuzz.profdata \
         --format=lcov \
         > coverage_report/lcov_raw.info

Step 3 — Filter to MQTT sources
================================

.. code-block:: console

   $ lcov --extract coverage_report/lcov_raw.info \
         '*/subsys/net/lib/mqtt/*' \
         '*/samples/subsys/debug/fuzz/src/*' \
         -o coverage_report/lcov_mqtt.info

Step 4 — Generate HTML report
================================

.. code-block:: console

   $ genhtml coverage_report/lcov_mqtt.info \
         --output-directory coverage_report/html
   $ xdg-open coverage_report/html/index.html

Current Coverage Status
***********************

The figures below reflect the most recent fuzzing run
(report date: 2026-03-03).

Overall
=======

.. list-table::
   :header-rows: 1
   :widths: 20 15 15 15

   * - Metric
     - Hit
     - Total
     - Coverage
   * - Lines
     - 1,239
     - 2,131
     - **58.1 %**
   * - Functions
     - 99
     - 162
     - **61.1 %**
   * - Branches
     - 381
     - 881
     - **43.2 %**

Per-file breakdown
==================

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
     - 87.1 %
     - 55.6 %
     - 42.7 %
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
=============================

* ``mqtt.c`` and ``mqtt_encoder.c`` coverage is low (~32 %) because the
  harness only drives the *receive* path.  Adding fuzzing modes that
  call ``mqtt_publish()``, ``mqtt_subscribe()``, and ``mqtt_ping()``
  would close this gap.
* ``mqtt_transport_socket_tcp.c`` is intentionally uncovered; a
  separate network-level fuzzer would be needed to exercise it.
* Branch coverage across all files sits at **43.2 %**; adding
  corpus seeds that trigger error-return paths (malformed remaining
  lengths, unknown property IDs, reason-code edge cases) would improve
  this metric.
