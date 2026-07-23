:orphan:

.. _zperf-loopback-icount:

zperf deterministic loopback throughput (qemu_x86 + icount)
###########################################################

Overview
********

This describes a self-contained way to use the zperf sample as a **network
throughput regression check** that is hardware agnostic and, most importantly,
independent of the speed of the host running the emulator.

The measurement runs entirely inside a ``qemu_x86`` guest:

- All traffic goes over the in-guest loopback interface, so there is no external
  (real-time) network backend in the measurement path.
- QEMU is run in **icount mode**, so the guest's notion of time is derived from
  the number of executed instructions rather than from host wall-clock time.
- The sample drives itself (no interactive shell): it starts a UDP and a TCP
  receiver on loopback, uploads to ``127.0.0.1``, and prints the results.

Why not native_sim or plain qemu_x86
====================================

Throughput is ``bytes / elapsed_time``, so the metric is only meaningful if
``elapsed_time`` is deterministic and host independent.

``native_sim``
   Runs native host code with an event-driven simulated clock
   (:kconfig:option:`CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME`). It is either
   slowed to wall clock (host-speed dependent) or runs "as fast as possible",
   where CPU work costs zero simulated time - so the throughput number is
   meaningless and insensitive to CPU-cost regressions. In addition, the zperf
   uploaders take a ``k_busy_wait()`` path on the POSIX architecture, which caps
   the rate artificially.

plain ``qemu_x86``
   By default icount is disabled when networking and the shell are enabled, and
   the throughput would then track host wall-clock time.

``qemu_x86`` + icount + loopback
   In icount mode the virtual CPU advances virtual time by ``2^shift`` ns per
   executed instruction. Throughput therefore becomes a deterministic function
   of *instructions per byte*: identical across runs and across machines, and
   sensitive to real code regressions (more instructions per byte lowers the
   reported throughput). Because all traffic stays on loopback, there is no
   external real-time backend that would break icount determinism.

.. note::

   The absolute Mbps value is a *virtual-time proxy* (a stand-in for
   instructions-per-byte), not a real line rate. Only compare values captured
   with the same icount shift, packet size, and tick rate.

How it works
************

The runner lives in :file:`src/main.c`, guarded by
:kconfig:option:`CONFIG_ZPERF_LOOPBACK_SELFTEST`. When enabled the sample:

#. Brings the network up itself (automatic init is disabled so the transfer does
   not race the bring-up).
#. Starts a UDP receiver, runs a blocking UDP upload, stops the receiver, and
   prints ``ZPERF-RESULT udp4_mbps=<value>``.
#. Repeats the same for TCP and prints ``ZPERF-RESULT tcp4_mbps=<value>``.
#. Repeats both over IPv6, printing ``ZPERF-RESULT udp6_mbps=<value>`` and
   ``ZPERF-RESULT tcp6_mbps=<value>``.
#. Prints a final ``ZPERF-DONE`` marker.

Both IP families are exercised so a regression in either code path is caught.
The IPv4 transfers use ``127.0.0.1`` and the IPv6 transfers use ``::1``; the
loopback driver assigns both addresses to the loopback interface. For each
family the receiver is bound to that loopback address (not just the interface's
default address) so the client can connect to it. The IPv6 runs are skipped
when :kconfig:option:`CONFIG_NET_IPV6` is disabled (and likewise for IPv4).

The whole thing is configured by :file:`overlay-loopback-icount.conf`, which is
layered on top of the existing :file:`overlay-loopback.conf`.

Configuration knobs
===================

.. list-table::
   :header-rows: 1

   * - Kconfig option
     - Default
     - Meaning
   * - :kconfig:option:`CONFIG_ZPERF_LOOPBACK_SELFTEST`
     - ``n``
     - Enable the self-driven loopback runner.
   * - ``CONFIG_ZPERF_LOOPBACK_SELFTEST_DURATION_MS``
     - ``1000``
     - Duration of each (UDP and TCP) upload run.
   * - ``CONFIG_ZPERF_LOOPBACK_SELFTEST_PACKET_SIZE``
     - ``1220``
     - Payload size (bytes). The default is the IPv6 TCP MSS at a 1280 byte
       MTU so every write is one full-sized segment (see below).
   * - ``CONFIG_ZPERF_LOOPBACK_SELFTEST_RATE_KBPS``
     - ``0``
     - Target rate; ``0`` means send as fast as the (virtual) CPU allows.
   * - ``CONFIG_ZPERF_LOOPBACK_SELFTEST_PORT``
     - ``5001``
     - Server port used by both the receiver and the client.

Implementation notes / gotchas
==============================

A few non-obvious points are baked into :file:`overlay-loopback-icount.conf`:

- **Stack sizes.** The loopback driver delivers every packet synchronously in
  the sender's context, so the entire receive path (and the blocking client
  upload) nests on the caller's stack. The main and networking thread stacks are
  enlarged; otherwise the guest overflows its stack and faults at boot.
- **Tick rate.** The tick rate is raised to 10 kHz. At the default 100 Hz,
  ``USECS_PER_TICK >= 1000`` compiles in the UDP uploader's clock-compensation
  path, which divides by the per-packet duration. With the unlimited rate that
  duration is zero, so a >1 kHz tick both avoids the division-by-zero and gives
  fine-grained pacing.
- **Near-MTU packet size and the MSS caveat.** The overlay raises the loopback
  MTU (:kconfig:option:`CONFIG_NET_LOOPBACK_MTU`) to 1280 and lifts zperf's own
  packet ceiling (:kconfig:option:`CONFIG_NET_ZPERF_MAX_PACKET_SIZE`) above the
  payload, so the default 1220 byte payload produces near-MTU frames. Large
  frames make the per-byte work (checksums, buffer/fragment walking) dominate
  the fixed per-call overhead, which is what a throughput regression check
  should be sensitive to. The exact value 1220 matters for a *fair* TCP
  comparison across families: the zperf TCP uploader sends the whole payload
  per ``send()`` and lets TCP segment it at the MSS. At a 1280 byte MTU the
  IPv4 MSS is ``1280 - 40`` = 1240 but the IPv6 MSS is only ``1280 - 60`` =
  1220 (the IPv6 header is 20 bytes larger). A payload of, say, 1232 fits the
  IPv4 MSS in one segment but exceeds the IPv6 MSS, so every IPv6 write is split
  into a full 1220 byte segment plus a 12 byte runt - roughly doubling the IPv6
  segment/ACK count and nearly halving IPv6 TCP throughput. Using 1220 keeps
  every write to a single full-sized segment for both families. The payload also
  stays under the MTU for UDP in both families, so no IP fragmentation is needed
  (IP fragmentation is disabled in this config). Because the net_buf data size
  (1100) is below the MTU, each frame still spans two buffer fragments, which
  deliberately exercises the fragment-chain walk.
- **Unlimited-rate math.** In unlimited-rate (``rate = 0``) mode the runner
  picks a rate high enough that the per-packet pacing delay rounds down to zero.
  That synthetic rate is computed in 64-bit to avoid an intermediate overflow,
  so any realistic packet size (up to ~500 kB) is supported.
- **No SLIP/TAP.** The QEMU SLIP host networking backend is disabled; the run
  needs no host-side ``slip.sock``.

Usage
*****

Build and run once
==================

.. code-block:: console

   west build -p -b qemu_x86 -d ../build/zperf_loop samples/net/zperf -- \
       -DEXTRA_CONF_FILE="overlay-loopback.conf;overlay-loopback-icount.conf"
   west build -t run -d ../build/zperf_loop

Expected output (values are deterministic and repeat exactly across runs on a
given build; the exact numbers depend on the toolchain, packet size, icount
shift and tick rate, so treat them as an example)::

   ZPERF-RESULT udp4_mbps=13.251
   ZPERF-RESULT tcp4_mbps=6.003
   ZPERF-RESULT udp6_mbps=13.463
   ZPERF-RESULT tcp6_mbps=6.050
   ZPERF-DONE

The IPv4 and IPv6 TCP numbers are close because the default payload is the IPv6
TCP MSS; see the MSS caveat under `Implementation notes / gotchas`_.

Run through twister
===================

The :file:`tests.yaml` scenario ``sample.net.zperf.loopback_icount`` uses the
``console`` harness and records the throughput values into the twister JSON
report:

.. code-block:: console

   ./scripts/twister -p qemu_x86 -s sample.net.zperf.loopback_icount \
       --outdir ../build/zperf_run

The scenario is allowed on both ``qemu_x86`` (32-bit) and ``qemu_x86_64``
(64-bit); run the latter for a 64-bit data point (for example to exercise the
64-bit checksum fast path):

.. code-block:: console

   ./scripts/twister -p qemu_x86_64 -s sample.net.zperf.loopback_icount \
       --outdir ../build/zperf_run64

.. note::

   Unlike the other zperf scenarios, this one is tagged ``net-perf`` instead of
   ``net``. The ``qemu_x86_64`` board sets ``ignore_tags: [net, ...]`` in its
   board YAML, so any ``net``-tagged suite is statically filtered there. Using a
   dedicated ``net-perf`` tag lets the performance check run on ``qemu_x86_64``
   while the functional ``net`` zperf scenarios remain 32-bit only, as intended
   upstream. Baselines are per-platform: a 32-bit and a 64-bit run produce
   different absolute numbers, so keep separate baseline files for each.

Baseline and regression gate
============================

The helper :file:`scripts/zperf_regression.py` reads the recordings from a
twister run. Capture a baseline once on the reference commit:

.. code-block:: console

   ./scripts/twister -p qemu_x86 -s sample.net.zperf.loopback_icount \
       --outdir ../build/zperf_base
   samples/net/zperf/scripts/zperf_regression.py --base-dir .. \
       --twister-json ../build/zperf_base/twister.json \
       --save baseline.json

On a later commit, re-run and gate on a maximum allowed drop (in percent):

.. code-block:: console

   ./scripts/twister -p qemu_x86 -s sample.net.zperf.loopback_icount \
       --outdir ../build/zperf_cur
   samples/net/zperf/scripts/zperf_regression.py --base-dir .. \
       --twister-json ../build/zperf_cur/twister.json \
       --baseline baseline.json --tolerance 5

The script exits non-zero if any recorded metric dropped by more than the
tolerance, which makes it suitable as a CI regression check.

.. note::

   For safety when the script is driven by automation, every file it reads or
   writes must resolve to a location under ``--base-dir`` (default: the current
   directory); paths that escape it (via ``..`` or an absolute path) are
   rejected. The examples pass ``--base-dir ..`` because they read the twister
   report from the sibling ``../build`` tree while writing outputs into the
   repository.
