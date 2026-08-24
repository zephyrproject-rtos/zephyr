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

Throughput is ``bytes / elapsed_time``, so the metric is only useful for
regression detection if ``elapsed_time`` is deterministic and host independent.
In icount mode the virtual CPU advances virtual time by ``2^shift`` ns per
executed instruction, so the reported throughput becomes a deterministic
function of *instructions per byte*: identical across runs and across machines,
and sensitive to real code regressions (more instructions per byte lowers the
number). Keeping all traffic on loopback means there is no external real-time
backend that would otherwise break icount determinism.

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
#. For each IP family, runs a sequence of transfers, each bracketed by starting
   the matching receiver, running a blocking upload, and stopping the receiver,
   and prints one ``ZPERF-RESULT <marker>_mbps=<value>`` line per transfer:

   .. list-table::
      :header-rows: 1

      * - Marker
        - Transfer
      * - ``udp4`` / ``udp6``
        - Baseline UDP, near-MTU payload (single, unfragmented datagram).
      * - ``udp4_frag`` / ``udp6_frag``
        - UDP with an oversized payload that forces **IP fragmentation** on TX
          and reassembly on RX. Only run when the matching
          :kconfig:option:`CONFIG_NET_IPV4_FRAGMENT` /
          :kconfig:option:`CONFIG_NET_IPV6_FRAGMENT` is enabled.
      * - ``tcp4`` / ``tcp6``
        - Baseline TCP (Nagle on).
      * - ``tcp4_nodelay`` / ``tcp6_nodelay``
        - TCP with ``TCP_NODELAY`` (Nagle off) to cover the alternate send path.

#. Prints a final ``ZPERF-DONE`` marker.

Both IP families are exercised so a regression in either code path is caught.
The IPv4 transfers use ``127.0.0.1`` and the IPv6 transfers use ``::1``; the
loopback driver assigns both addresses to the loopback interface. For each
family the receiver is bound to that loopback address (not just the interface's
default address) so the client can connect to it. The IPv6 runs are skipped
when :kconfig:option:`CONFIG_NET_IPV6` is disabled (and likewise for IPv4).

Every upload is sent with a non-default socket priority
(:c:enumerator:`NET_PRIORITY_VI`). Combined with more than one configured
traffic class (see below) this routes the traffic through the per-traffic-class
threads in ``net_tc.c`` instead of the caller's context, so that scheduling path
is exercised on every run.

The whole thing is configured by :file:`overlay-loopback-icount.conf`, which is
layered on top of the existing :file:`overlay-loopback.conf`.

Configuration knobs
===================

.. list-table::
   :header-rows: 1

   * - Kconfig option
     - Meaning
   * - :kconfig:option:`CONFIG_ZPERF_LOOPBACK_SELFTEST`
     - Enable the self-driven loopback runner.
   * - :kconfig:option:`CONFIG_ZPERF_LOOPBACK_SELFTEST_DURATION_MS`
     - Duration of each (UDP and TCP) upload run.
   * - :kconfig:option:`CONFIG_ZPERF_LOOPBACK_SELFTEST_PACKET_SIZE`
     - Payload size (bytes). The default is the IPv6 TCP MSS at a 1280 byte
       MTU so every write is one full-sized segment (see below).
   * - :kconfig:option:`CONFIG_ZPERF_LOOPBACK_SELFTEST_FRAG_PACKET_SIZE`
     - Payload size (bytes) for the extra fragmenting UDP runs. Must exceed the
       MTU (so the datagram fragments) and stay below
       :kconfig:option:`CONFIG_NET_ZPERF_MAX_PACKET_SIZE`.
   * - :kconfig:option:`CONFIG_ZPERF_LOOPBACK_SELFTEST_RATE_KBPS`
     - Target rate; ``0`` means send as fast as the (virtual) CPU allows.
   * - :kconfig:option:`CONFIG_ZPERF_LOOPBACK_SELFTEST_PORT`
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
  every write to a single full-sized segment for both families. The baseline UDP
  payload also stays under the MTU for both families, so those datagrams are not
  IP-fragmented. Because the net_buf data size (1100) is below the MTU, each
  frame still spans two buffer fragments, which deliberately exercises the
  fragment-chain walk.
- **IP fragmentation coverage.** :kconfig:option:`CONFIG_NET_IPV4_FRAGMENT` and
  :kconfig:option:`CONFIG_NET_IPV6_FRAGMENT` are enabled and the runner adds an
  extra UDP run per family whose payload
  (``CONFIG_ZPERF_LOOPBACK_SELFTEST_FRAG_PACKET_SIZE``, 2000 by default) exceeds
  the MTU. This drives the fragmentation path in ``ipv4.c`` / ``ipv6.c`` on TX
  and the reassembly path on RX (the ``*_frag`` markers).
  :kconfig:option:`CONFIG_NET_ZPERF_MAX_PACKET_SIZE` is raised above this payload.
- **Traffic classes and priority.** The overlay configures more than one TX/RX
  traffic class (:kconfig:option:`CONFIG_NET_TC_TX_COUNT` /
  :kconfig:option:`CONFIG_NET_TC_RX_COUNT` = 2) and enables
  :kconfig:option:`CONFIG_NET_CONTEXT_PRIORITY` (plus
  :kconfig:option:`CONFIG_NET_ALLOW_ANY_PRIORITY`). Every upload sets a
  non-default ``SO_PRIORITY``, so traffic is dispatched through the dedicated
  per-traffic-class threads rather than the caller's context. This exercises the
  ``net_tc.c`` scheduling path but adds context switches, which lowers the
  absolute numbers compared with a single-traffic-class configuration.
- **Unlimited-rate math.** In unlimited-rate (``rate = 0``) mode the runner
  picks a rate high enough that the per-packet pacing delay rounds down to zero.
  That synthetic rate is computed in 64-bit to avoid an intermediate overflow,
  so any realistic packet size (up to ~500 kB) is supported.
- **No SLIP/TAP.** The QEMU SLIP host networking backend is disabled; the run
  needs no host-side ``slip.sock``.
- **Log buffer size.** The result lines are printed with ``printk()``, which the
  log subsystem forwards through its deferred backend. At the default 1024 byte
  buffer the ``qemu_x86_64`` run overflows it during the first IPv4 transfers,
  and :kconfig:option:`CONFIG_LOG_MODE_OVERFLOW` then drops the three oldest
  messages - which silently swallows the ``udp4_frag`` result line, so that
  metric never reaches the report at all. The overlay raises
  :kconfig:option:`CONFIG_LOG_BUFFER_SIZE` to 4096. The measurement itself is
  unaffected; the 32-bit numbers are bit-identical with the larger buffer.

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

   ZPERF-RESULT udp4_mbps=13.099
   ZPERF-RESULT udp4_frag_mbps=20.249
   ZPERF-RESULT tcp4_mbps=5.970
   ZPERF-RESULT tcp4_nodelay_mbps=5.876
   ZPERF-RESULT udp6_mbps=13.310
   ZPERF-RESULT udp6_frag_mbps=20.059
   ZPERF-RESULT tcp6_mbps=5.979
   ZPERF-RESULT tcp6_nodelay_mbps=5.910
   ZPERF-DONE

The IPv4 and IPv6 TCP numbers are close because the default payload is the IPv6
TCP MSS; see the MSS caveat under `Implementation notes / gotchas`_. The
``*_frag`` (fragmented) UDP runs report a *higher* number than the baseline UDP
runs because the larger datagram amortizes the fixed per-datagram overhead over
more payload, even after accounting for the fragmentation work.

Run through twister
===================

The :file:`tests.yaml` scenario ``sample.net.zperf.loopback_icount`` uses the
``console`` harness and records the throughput values into the twister JSON
report:

.. code-block:: console

   ./scripts/twister -p qemu_x86 -s sample.net.zperf.loopback_icount \
       --outdir ../build/zperf_run

The scenario is allowed on both ``qemu_x86`` (32-bit) and ``qemu_x86_64``
(64-bit); the latter gives a 64-bit data point (for example the 64-bit checksum
fast path). Both can be measured in one run:

.. code-block:: console

   ./scripts/twister -T samples/net/zperf -s sample.net.zperf.loopback_icount \
       -p qemu_x86 -p qemu_x86_64 --outdir ../build/zperf_run

Restricting the scan with ``-T samples/net/zperf`` avoids walking the whole
tree for a single scenario.

The two platforms report different absolute numbers and are never comparable
with each other, so :file:`scripts/zperf_regression.py` keys every metric by
the twister platform name (``qemu_x86/atom``, ``qemu_x86_64/atom``) and reports
each one separately. Use ``--platform`` to restrict the report to one of them,
given either as the full ``board/soc`` name or as just the board.

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

Two runs can also be compared directly, without the intermediate baseline file:

.. code-block:: console

   samples/net/zperf/scripts/zperf_regression.py --base-dir .. \
       --twister-json ../build/zperf_cur/twister.json \
       --baseline-twister-json ../build/zperf_base/twister.json

The remaining options shape the report rather than the measurement:

.. list-table::
   :header-rows: 1

   * - Option
     - Meaning
   * - ``--threshold PCT``
     - Reporting noise floor, 1% by default. A metric that moved by less than
       this is described as unchanged rather than as an improvement or a
       regression. icount repeats exactly for a given binary, but two trees
       build two different binaries, and code layout alone can move the number
       without any change in the work performed. In practice that drift is
       small: comparing two trees 157 commits apart, across a change that only
       touched TCP, every UDP metric came back bit-identical on ``qemu_x86``
       and within 0.02% on ``qemu_x86_64``.
   * - ``--markdown PATH``
     - Write the comparison as a markdown table, one section per platform, for a
       GitHub Actions job summary or a pull request comment. A platform whose
       metrics all held still is folded into a collapsed ``<details>`` block
       whose summary line says so; one where something moved is left open.
   * - ``--annotate``
     - Emit one ``::notice::`` workflow command per platform, summarising the
       metrics that moved.
   * - ``--exit-zero``
     - Always exit successfully. Use when the comparison is advisory and must
       not fail a build. A missing or unbuildable baseline is reported the same
       way, as a comparison that could not be made.

.. note::

   For safety when the script is driven by automation, every file it reads or
   writes must resolve to a location under ``--base-dir`` (default: the current
   directory); paths that escape it (via ``..`` or an absolute path) are
   rejected. The examples pass ``--base-dir ..`` because they read the twister
   report from the sibling ``../build`` tree while writing outputs into the
   repository.

Continuous integration
======================

:file:`.github/workflows/net-perf.yml` runs this scenario on every pull request
that touches ``subsys/net/``, ``include/zephyr/net/``, ``lib/net_buf/``, the
loopback driver or this sample. It measures the pull request and the branch it
targets on the same runner, on both platforms, and writes the comparison to the
job summary.

Two details make the comparison mean what it should:

- The workflow runs after the pull request has been rebased onto its target
  branch, so the difference is what the series itself does and cannot pick up
  unrelated movement on the target branch.
- The target branch is measured with the pull request's copy of
  :file:`samples/net/zperf/`, so the runner, the overlay and the payload sizes
  are identical on both sides and only the code under test differs. A pull
  request that changes the sample therefore does not move the number by changing
  what is being measured.

The check is **advisory**. It runs with ``--exit-zero``, every measuring step is
``continue-on-error``, and it is not one of the required checks: a red or
surprising number is information for the reviewer, never a merge blocker. A
result that cannot be produced - because the target branch did not build, for
instance - is reported as a comparison that was not available, and the job still
passes.

Only the loopback path is measured, so a flat report is not a claim that a
change has no performance effect; driver, L2 and offload work will not show up
here at all.

Visualize the results
=====================

Pass ``--plot`` to write a bar chart of the metrics as a self-contained SVG
(no extra Python dependencies). Without a baseline it plots the current values;
combined with ``--baseline`` it draws grouped baseline-vs-current bars:

.. code-block:: console

   samples/net/zperf/scripts/zperf_regression.py --base-dir .. \
       --twister-json ../build/zperf_cur/twister.json \
       --baseline baseline.json --plot throughput.svg

With more than one platform in the report the platform name is appended to each
file name, so the example above writes :file:`throughput-qemu_x86_atom.svg` and
:file:`throughput-qemu_x86_64_atom.svg`.
