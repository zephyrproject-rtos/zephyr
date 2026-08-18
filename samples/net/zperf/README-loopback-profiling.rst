:orphan:

.. _zperf-loopback-profiling:

Per-function profiling of the zperf loopback run
################################################

Overview
********

:ref:`zperf-loopback-icount` measures the networking stack's throughput as a
deterministic, host independent number. That number says whether the stack got
slower, not where the cycles went.

This describes how to break the same run down per function. The guest runs
under a QEMU TCG plugin that counts executed instructions per translation
block; the blocks are then mapped back onto the functions in the build's ELF
file. The result is a table of instructions attributed to each function, one
transfer type at a time.

Under ``-icount shift=N`` one guest instruction is 2\ :sup:`N` ns of virtual
time, so throughput is a closed form function of the instructions spent per
payload byte:

.. code-block:: none

   Mbps = 8000 / (2**shift * ipb)          at shift=5:  Mbps = 250 / ipb

A per-function breakdown of ``ipb`` is therefore a decomposition of the
reported throughput rather than a correlated proxy for it. That is what makes
the last column of the report, the throughput this transfer would reach if a
given function cost nothing, meaningful.

.. note::

   The plugin is not part of Zephyr. QEMU's plugin interface header
   :file:`include/qemu/qemu-plugin.h` is ``GPL-2.0-or-later``, so a plugin
   built against it is a derivative work of QEMU, while this tree carries only
   ``Apache-2.0`` and ``CC-BY-4.0`` (see :file:`LICENSES/`). Only the
   Apache-2.0 driver, the setup recipe and this document are in the tree; the
   plugin is fetched from QEMU and built outside the repository. Nothing GPL
   licensed is redistributed with Zephyr, and neither the Zephyr build nor its
   CI downloads or builds any of it.

Relation to the in-guest perf tool
**********************************

Zephyr has a second, unrelated profiler. Pick whichever fits the question:

* :ref:`profiling-perf` samples the guest's own call stacks from a timer
  interrupt, so it gives call graphs and works on real hardware, but it
  perturbs what it measures, needs a shell and only reports what it managed to
  sample. Its output is post-processed with
  :zephyr_file:`scripts/profiling/stackcollapse.py` into FlameGraph format.
* This workflow counts every instruction from outside the guest, so it is
  exact, reproducible and adds no guest overhead, but it is flat: a TCG plugin
  sees translation blocks, not stack frames, so there are no call graphs and
  only emulated targets are supported.

Prerequisites
*************

Build the plugins once. The QEMU tag has to match the emulator that will load
them, because the loader checks the plugin API version; check yours with
``qemu-system-i386 --version``.

.. code-block:: console

   samples/net/zperf/scripts/qemu_plugin_setup.sh ../tools/qemu-plugins

This downloads two plugins from QEMU, raises the report limit in one of them
and compiles them with :command:`gcc`. It needs :command:`curl`,
:command:`gcc` and the glib development files; all of QEMU does *not* have to
be built, because a plugin resolves QEMU's symbols at load time.

.. note::

   Released QEMU versions print only the twenty hottest blocks: the limit is a
   hardcoded constant in ``contrib/plugins/hotblocks.c`` and the plugin parses
   no argument for it. The setup script raises it. If a report is ever
   truncated anyway, :file:`scripts/zperf_profile.py` detects the mismatch
   between the block count in the report header and the number of rows, and
   warns instead of quietly understating the cost.

Usage
*****

Profile every transfer, boot cost removed:

.. code-block:: console

   samples/net/zperf/scripts/zperf_profile.py run --base-dir .. \
       --plugin ../tools/qemu-plugins/lib/libhotblocks.so \
       --outdir ../build/zprof --save ../build/zprof/profile.json

For each transfer this builds the sample with
:kconfig:option:`CONFIG_ZPERF_LOOPBACK_SELFTEST_ONLY` set to that transfer and
:kconfig:option:`CONFIG_ZPERF_LOOPBACK_SELFTEST_HALT_ON_DONE` enabled, then runs
it twice: once to completion, and once stopped as it reaches :c:func:`main` to
measure the boot cost. The second profile is subtracted from the first.

Restrict the work with ``--transfers``, for example ``--transfers tcp4 udp4``.

To re-report on an already collected run, or to use a plugin report produced by
hand:

.. code-block:: console

   samples/net/zperf/scripts/zperf_profile.py report --base-dir .. \
       --report ../build/zprof/tcp4.report \
       --boot-report ../build/zprof/tcp4.boot.report \
       --console ../build/zprof/tcp4.console \
       --elf ../build/zprof/tcp4/zephyr/zephyr.elf --label tcp4 --top 25

To compare two commits, save a profile on each and diff them. The comparison is
on instructions per byte, so it is unaffected by how long each run happened to
last:

.. code-block:: console

   samples/net/zperf/scripts/zperf_profile.py diff --base-dir .. \
       --baseline ../build/zprof-base/profile.json \
       --current ../build/zprof/profile.json --tolerance 1

:file:`diff` exits non-zero when a transfer needs more than ``--tolerance``
percent more instructions per byte than the baseline, which makes it usable as
a follow-up to the throughput gate in
:file:`samples/net/zperf/scripts/zperf_regression.py`: that one answers whether
throughput dropped, this one answers which function caused it.

Why one transfer per run
========================

The selftest normally runs eight transfers in one boot. A profile collected
over all eight blends UDP, TCP, fragmentation and both IP families into an
average that is attributable to none of them, so
:kconfig:option:`CONFIG_ZPERF_LOOPBACK_SELFTEST_ONLY` restricts a build to one.
Absolute numbers therefore differ slightly from a full eight transfer run,
where earlier transfers warm caches and allocator state; compare profiles taken
with the same setting.

Interpreting the output
***********************

The header states how the run adds up (values are illustrative)::

   === tcp4 (qemu_x86) ===
   instructions      31,889,702   payload      791,780 B   ipb   40.276   boot subtracted 6,840,845
   throughput   measured    6.325 Mbps   from profile    6.207 Mbps   cpu-bound  98.1%
   attribution  exact 96.38%   inferred  3.62%   unattributed  0.00%

``from profile`` is the throughput implied by the instruction count. Because
``sleep=off`` lets halted time advance virtual time without executing
instructions, the ratio to the measured value is the fraction of the run that
was CPU bound; the remainder is time the guest spent waiting, which for TCP is
mostly waiting for acknowledgements. A ratio above 100% means the accounting is
wrong, not that the stack is fast.

``unattributed`` should be essentially zero once the boot cost is subtracted.
Before subtraction it is dominated by the BIOS the guest boots through.

Then follows the per-function table, and a roll-up by subsystem group.

Two limitations are worth keeping in mind when reading the table:

* The sample is built with :kconfig:option:`CONFIG_SPEED_OPTIMIZATIONS`, so
  small static helpers are inlined into their callers and are charged to them.
  Lowering the optimisation level to separate them would change the code being
  measured, so it is not done.
* A translation block is attributed to the function containing its first
  instruction. Blocks end at branches, so they rarely straddle two functions,
  but hand written assembly without symbol sizes is attributed up to the next
  entry point; the ``inferred`` share in the header says how much of the run
  that covers.

Correctness of the measurement
******************************

The plugin's callbacks run on the host and execute no guest instructions, so
attaching it does not change what the guest does. This is worth re-checking
after a QEMU or plugin upgrade: run the same build with and without
``-plugin``, and every ``ZPERF-RESULT`` line must be identical.

The instruction counts are reproducible but not bit-identical: repeated runs of
the same binary agree to within about 0.03% of the total. The residual sits
entirely in the logging subsystem (``cbprintf_package_convert``, the
``mpsc_pbuf`` ring and the log message handlers), because the deferred log
thread is scheduled independently of the traffic and does a slightly different
amount of work per wakeup. The console output itself is identical. That
variation is two orders of magnitude below the share of any function worth
optimising, but it does mean a reported difference of a few hundredths of a
percent is noise rather than a finding.

The boot subtraction uses the same binary stopped at :c:func:`main` rather than
a separate boot-only build, so the instruction stream it covers is identical to
the full run's prefix and the subtraction is exact. :file:`zperf_profile.py`
verifies this: if any block ran more often in the shorter run, the two runs did
not share a prefix and it refuses to report a result.

Two checks confirm that the attribution itself is meaningful rather than merely
self-consistent. Both were run on ``udp4`` on ``qemu_x86``.

Halving :kconfig:option:`CONFIG_ZPERF_LOOPBACK_SELFTEST_PACKET_SIZE` from 1220
to 610 bytes doubles the number of packets carrying the same payload. Costs
that scale with bytes must then stay flat per byte, and costs that scale with
packets must double. Measured ratios of instructions per byte, 610 B against
1220 B:

.. list-table::
   :header-rows: 1

   * - Function
     - Ratio
     - Scales with
   * - ``memcpy``
     - 1.02
     - bytes
   * - ``calc_chksum``
     - 1.15
     - bytes, plus a fixed per-packet part
   * - ``net_pkt_get_contiguous_len``
     - 2.00
     - packets
   * - ``z_impl_k_mutex_lock`` / ``_unlock``
     - 2.00
     - packets
   * - ``rb_remove`` / ``rb_insert``
     - 2.00
     - packets
   * - ``zvfs_poll_internal``
     - 2.00
     - packets

Setting :kconfig:option:`CONFIG_NET_UDP_CHECKSUM` to ``n`` removes the receive
side checksum verification, one of the two passes the loopback path makes over
each datagram. ``calc_chksum`` loses 0.95 instructions per payload byte, which
is one pass, and throughput rises from 13.353 to 14.396 Mbps.
