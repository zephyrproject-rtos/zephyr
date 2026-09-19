.. SPDX-License-Identifier: Apache-2.0

PMU overflow sampling (PC profiler)
###################################

This directory implements a **sampling profiler** on top of the architecture-neutral
PMU API: periodic **program-counter** samples are taken when a programmable PMU
counter **overflows**, using the **PMU overflow interrupt** (ARMv8-A:
``PMINTENSET_EL1`` / GIC).

Benchmark-style **perf start** / **perf report** (frozen counter reads) remain
separate; sampling answers “where was the CPU?” over time.

Architecture and files
**********************

1. **PMU overflow support (ARM64)**

   - ``arch/arm64/core/pmu.c``: ``pmu_counter_write32()`` (preload counter for a
     periodic overflow) and ``pmu_counter_overflow_interrupt_set()`` (enable or
     disable overflow signaling via PMINTENSET_EL1 / PMINTENCLR_EL1).
   - ``include/zephyr/pmu.h``: Declarations are guarded by
     ``CONFIG_PROFILING_PMU_SAMPLING``.

2. **Subsystem** (this directory)

   - ``pmu_sampling.c``: Per-CPU ring buffers of ``struct pmu_sample_record``
     (``tstamp``, ``pc``, ``tid``, ``cpu``, ``event``); lock-free producer in the
     overflow ISR, ``lost`` counter when a ring overruns. APIs:
     ``pmu_sampling_init()``, ``pmu_sampling_start()`` / ``pmu_sampling_start_cfg()``,
     ``pmu_sampling_stop()``, ``pmu_sampling_clear()``, ``pmu_sampling_count()``,
     ``pmu_sampling_get_stats()``, ``pmu_sampling_copy()``,
     ``pmu_sampling_period_from_hz()``, ``pmu_sampling_export_zperf()`` /
     ``pmu_sampling_export_zperf_size()``.
   - ``pmu_sampling_arm64.c``: ``IRQ_CONNECT`` and ISR: read **ELR_EL1** as an
     approximate interrupted PC, push a sample, clear the overflow flag, reload
     the counter with ``-reload_period``.

3. **Kconfig** (``Kconfig`` in this directory, sourced from ``subsys/profiling``)

   - ``PROFILING_PMU_SAMPLING``: Master switch (depends on ``PROFILING``,
     ``ARM64``, ``ARM64_PMUV3``, ``GEN_SW_ISR_TABLE``).
   - ``PROFILING_PMU_SAMPLE_BUFFER_LEN``: Per-CPU ring capacity (default 512).
   - ``PROFILING_PMU_ZPERF_MAX``: Max size for shell ``perf record export`` (default 32768).
   - ``ARM64_PMU_OVERFLOW_IRQ``: Fallback GIC **INTID** when devicetree does not
     supply an ``arm,armv8-pmu`` node (default ``-1`` = require DT).

4. **Devicetree**

   - Binding: ``dts/bindings/arm/arm,armv8-pmu.yaml`` — ``compatible:
     arm,armv8-pmu`` and required ``interrupts`` (no MMIO; the node describes the
     overflow IRQ only).
   - Example (disabled): ``dts/arm64/qemu/qemu-virt-arm64.dtsi`` adds a ``pmu``
     node (PPI 23) with a comment that QEMU does not model this path usefully.

   **IRQ selection** (runtime init):

   - If an **enabled** ``arm,armv8-pmu`` node exists → use ``DT_IRQN(...)``.
   - Else if ``CONFIG_ARM64_PMU_OVERFLOW_IRQ >= 0`` → use that INTID.
   - Else → ``pmu_sampling_init()`` fails (logged).

5. **Shell** (``subsys/shell/modules/perf_service.c`` when
   ``CONFIG_PROFILING_PMU_SAMPLING``) — see `Shell usage`_.

6. **Host decode**

   - ``scripts/profiling/zephyr-perf-decode.py`` reads a binary zperf file and prints TSV (or ``--folded`` lines for quick aggregation). Resolve PCs with
     ``addr2line`` / ``llvm-symbolizer`` on the matching ``zephyr.elf``.

7. **Public API**

   - ``include/zephyr/profiling/pmu_sampling.h`` — same functionality without the shell.

Shell usage
***********

These commands require ``CONFIG_PERF_SHELL=y`` and ``CONFIG_PROFILING_PMU_SAMPLING=y``.
**Frozen-counter** profiling (``perf start`` / ``perf report``) and **sampling**
(``perf record``) are mutually exclusive: switching modes stops the other path.

Top-level ``perf`` (counting / listing)
========================================

.. code-block:: text

   perf list                    # Names accepted by perf start -e
   perf status                  # Sampling + lost samples + ring fill; counting session state
   perf start [-e <ev>|help]    # Arm PMU for manual counter reads (see perf list)
   perf stop                    # Freeze counters
   perf report                  # Print PMCCNTR and counter 0

``perf record`` (PMU overflow sampling)
==========================================

**Start** — choose **either** a sample rate (**``-f``**) **or** a raw PMU reload
**period** (event counts between overflows), not both. If you omit both, the shell
defaults to **~1000 Hz** using ``pmu_cpu_freq_mhz()`` (requires a non-zero CPU MHz).

.. code-block:: text

   perf record start [-e <name|0xNN>] [-f <Hz>] [<period>] [-t <sec>] [-o <path>]

**Flags**

.. list-table::
   :widths: 18 62
   :header-rows: 1

   * - Flag
     - Meaning
   * - ``-e <ev>``
     - PMU event: alias from ``perf list`` (e.g. ``cycles``, ``instructions``) or hex ``0x00``–``0x1f``.
   * - ``-f <Hz>``
     - Target samples per second; period is derived from CPU MHz. Mutually exclusive with ``<period>``.
   * - ``<period>``
     - Unsigned decimal: PMU counts between overflows (e.g. ``100000``). Mutually exclusive with ``-f``.
   * - ``-t <sec>``
     - Auto-stop after this many seconds (positive integer). Omit for manual ``perf record stop``.
   * - ``-o <path>``
     - Not implemented; shell warns. Use ``perf record export`` or ``pmu_sampling_export_zperf()`` from code.

**After ``start``**

.. code-block:: text

   perf record stop             # Stop sampling (cancels pending auto-stop)
   perf record dump             # Print recent samples (tstamp, pc, tid, cpu, event)
   perf record export           # Hex-dump zperf v1 blob (UART capture → binary on host)
   perf record clear            # Reset ring buffers (prefer after stop)

**Export size**

``export`` uses a static buffer sized by ``CONFIG_PROFILING_PMU_ZPERF_MAX``. If the
blob would be larger, the command fails with ``-ENOSPC``; increase the Kconfig
value, shorten the run, or ``clear`` before capturing.

**Typical UART workflow**

#. ``perf record start -e cycles -f 1000 -t 5`` (or ``perf record start 100000`` for a fixed count period).
#. Optional: ``perf status`` while running to watch **lost** samples.
#. ``perf record stop`` if you did not use ``-t``.
#. ``perf record export`` — save the **UART text** (``zperf v1:`` line plus ``shell_hexdump`` lines). The decoder accepts this **directly**; you do not need to convert to raw binary first.
#. ``python3 $ZEPHYR_BASE/scripts/profiling/zephyr-perf-decode.py uart_capture.txt``
#. Resolve PCs with ``aarch64-zephyr-elf-addr2line -e zephyr.elf -f -C -p <pc>``.

Enabling
********

Add to ``prj.conf`` (or an overlay):

.. code-block:: none

   CONFIG_PROFILING=y
   CONFIG_PROFILING_PMU_SAMPLING=y
   CONFIG_ARM64_PMUV3=y
   CONFIG_PERF_SHELL=y
   # If your board has no arm,armv8-pmu DT node, set the SoC-specific INTID, e.g.:
   # CONFIG_ARM64_PMU_OVERFLOW_IRQ=48

On hardware, add or enable an ``arm,armv8-pmu`` node with **correct**
``interrupts`` for that SoC (reference the SoC TRM).

What is not included (possible follow-ups)
******************************************

- **Call stacks / threads**: Only **PC** samples are stored. Linux-style stacks
  need frame pointers plus unwind (or separate sampling of link registers).
- **perf.data compatibility**: Use zperf + host script, not Linux perf.
- **-o path**: File export from the shell is not implemented; capture ``export`` output
  or call ``pmu_sampling_export_zperf()`` from your app.

Board example: ``versal_apu`` (Versal Cortex-A72 APU)
===================================================

#. **Devicetree** — ``dts/arm64/xilinx/versal_a72.dtsi`` includes an
   ``arm,armv8-pmu`` node with ``GIC_PPI 7``. No extra board overlay is
   required for ``versal_apu``.

#. **Image config** — extend your application ``prj.conf`` (shell sample or your
   app) with:

   .. code-block:: none

      CONFIG_PROFILING=y
      CONFIG_PROFILING_PMU_SAMPLING=y
      CONFIG_ARM64_PMUV3=y
      CONFIG_PERF_SHELL=y
      CONFIG_SHELL=y

#. **Build** — from ``$ZEPHYR_BASE``:

   .. code-block:: bash

      west build -b versal_apu path/to/app

#. **Flash / boot** — use your normal Versal flow (PDI → TF-A → Zephyr). The
   in-tree board expects ``CONFIG_BUILD_WITH_TFA=y`` (see
   ``boards/amd/versal_apu/versal_apu_defconfig``).

#. **Runtime test** (UART shell) — after boot, follow `Shell usage`_ (e.g.
 ``perf record start -f 1000``, ``perf record stop``, ``dump`` / ``export``).

#. **If you see no samples or init errors**

   - Confirm boot is **EL1** and **non-secure** if you use ``CONFIG_ARMV8_A_NS=y``;
     TF-A / ``MDCR_EL3`` must allow EL1 PMU and IRQ delivery (typical on Versal,
     but platform images vary).
   - If you use a **custom DTS** without the shared ``versal_a72.dtsi`` PMU node,
     add an ``arm,armv8-pmu`` node or set ``CONFIG_ARM64_PMU_OVERFLOW_IRQ`` to the
     correct GIC **INTID** for your hardware (verify against the SoC TRM).

Board example: ``versalnet_apu`` (Versal NET Cortex-A78 APU)
=============================================================

#. **Devicetree** — ``dts/arm64/xilinx/versalnet_a78.dtsi`` includes an
   ``arm,armv8-pmu`` node on ``GIC_PPI 7`` (same choice as Versal A72; confirm
   with the Versal NET TRM if overflow never arrives).

#. **Config / build / shell** — same Kconfig lines and ``perf record`` flow as
   ``versal_apu``, but build with:

   .. code-block:: bash

      west build -b versalnet_apu path/to/app

#. **SMP** — there is one ring buffer **per CPU** (``CONFIG_MP_MAX_NUM_CPUS``);
   ``pmu_sampling_copy()`` / zperf export iterate CPU 0 .. N-1 (not globally
   time-sorted). Watch **lost** counts in ``perf status`` if rings are too small.

Verifying output with ``zephyr.elf``
====================================

Use the **same** ``zephyr.elf`` produced by the build you flashed. If ``addr2line``
shows ``??:0`` or PCs fall outside executable segments, the ELF usually does not
match the image (wrong build path or stale file).

1. **Confirm PCs lie in an executable segment**

   .. code-block:: bash

      aarch64-zephyr-elf-readelf -l build/zephyr/zephyr.elf | grep -A1 LOAD

   Check **VirtAddr** ranges for segments with **R E** (read + execute). Sampled
   ``pc`` values should fall inside one of those ranges.

2. **Resolve PCs to functions and source lines**

   .. code-block:: bash

      aarch64-zephyr-elf-addr2line -e build/zephyr/zephyr.elf -f -C -p 0xfb08 0xfaec

3. **zperf on host**

   ``perf record export`` prints **ASCII** ``shell_hexdump`` lines (not raw bytes). Save that
   log to a file; the decoder **auto-parses** those lines (magic ``5a 50 45 52 46 56 30 31``
   = ``ZPERFV01`` appears after the ``00000000:`` prefix on the first line). If you
   already have a raw binary blob starting with ``ZPERFV01``, pass that instead.

   .. code-block:: bash

      python3 $ZEPHYR_BASE/scripts/profiling/zephyr-perf-decode.py uart_capture.txt

**Rule of thumb:** if most sampled PCs map via ``addr2line`` to real functions in
the **same** ``zephyr.elf`` you built and flashed, the profile is trustworthy;
hotspots reflect **where time was spent**, not a broken sampler.

Build note
**********

A configuration such as ``qemu_cortex_a53`` with the options above **builds**
successfully; whether **overflow IRQs** fire still depends on the platform
wiring the PMU to the GIC and EL3/TF-A policy.
