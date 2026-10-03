.. SPDX-License-Identifier: Apache-2.0

PMU overflow sampling (PC profiler)
###################################

This directory implements a **sampling profiler** on top of the architecture-neutral
PMU API: periodic **program-counter** samples are taken when a programmable PMU
counter **overflows**, using the **PMU overflow interrupt** (ARMv8-A:
``PMINTENSET_EL1`` / GIC).

Frozen-counter reads (via the portable PMU API) remain separate; sampling
answers "where was the CPU?" over time.

Architecture and files
**********************

1. **PMU overflow support (ARM64)**

   - ``arch/arm64/core/pmuv3.c``: ``pmu_counter_write32()`` (preload a counter for
     a periodic overflow) and ``pmu_counter_overflow_interrupt_set()`` (enable or
     disable overflow signaling via PMINTENSET_EL1 / PMINTENCLR_EL1). The
     ``CPU_CYCLES`` event is sampled via the dedicated cycle counter
     (``PMCCNTR_EL0``, overflow bit 31), because some ARMv8-A cores do not
     increment a general-purpose counter for that event.
   - ``include/zephyr/arch/pmu.h`` and
     ``include/zephyr/arch/arm64/pmuv3.h``: the portable ``pmu_*`` API and its
     ARM64 dispatch.

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
   - ``PROFILING_PMU_ZPERF_MAX``: Max size for ``pmu_sampling_export_zperf()`` (default 32768).
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

5. **Public API**

   - ``include/zephyr/profiling/pmu_sampling.h`` — the full sampling interface
     (init, start/stop, stats, copy, zperf export). This is how applications and
     tests drive the profiler. An interactive ``perf record`` shell is a planned
     follow-up on top of the Performance Event Subsystem.

6. **Host decode**

   - ``scripts/profiling/zephyr-perf-decode.py`` reads a binary zperf file and prints TSV (or ``--folded`` lines for quick aggregation). Resolve PCs with
     ``addr2line`` / ``llvm-symbolizer`` on the matching ``zephyr.elf``.

Programmatic usage
******************

The profiler is driven through ``include/zephyr/profiling/pmu_sampling.h``.
A typical flow:

.. code-block:: c

   uint32_t period;

   pmu_sampling_init();
   /* ~1 kHz from the calibrated CPU frequency, or pass a raw event period. */
   pmu_sampling_period_from_hz(1000, &period);
   pmu_sampling_start(PMU_EVT_CPU_CYCLES, period);
   /* ... run the workload ... */
   pmu_sampling_stop();

   struct pmu_sampling_stats st;
   pmu_sampling_get_stats(&st);          /* stored / lost / high-water */

   struct pmu_sample_record recs[256];
   size_t n = pmu_sampling_copy(recs, ARRAY_SIZE(recs));

Export a binary zperf v1 blob with ``pmu_sampling_export_zperf()`` (sized by
``CONFIG_PROFILING_PMU_ZPERF_MAX``) and decode it on the host:

.. code-block:: bash

   python3 $ZEPHYR_BASE/scripts/profiling/zephyr-perf-decode.py capture.bin
   # then resolve PCs, e.g.:
   aarch64-zephyr-elf-addr2line -e zephyr.elf -f -C -p <pc>

An interactive ``perf record`` shell is a planned follow-up on top of the
Performance Event Subsystem and is not part of this subsystem.

Enabling
********

Add to ``prj.conf`` (or an overlay):

.. code-block:: none

   CONFIG_PROFILING=y
   CONFIG_PROFILING_PMU_SAMPLING=y
   CONFIG_ARM64_PMUV3=y
   # If your board has no arm,armv8-pmu DT node, set the SoC-specific INTID, e.g.:
   # CONFIG_ARM64_PMU_OVERFLOW_IRQ=48

On hardware, add or enable an ``arm,armv8-pmu`` node with **correct**
``interrupts`` for that SoC (reference the SoC TRM).

What is not included (possible follow-ups)
******************************************

- **Call stacks / threads**: Only **PC** samples are stored. Linux-style stacks
  need frame pointers plus unwind (or separate sampling of link registers).
- **perf.data compatibility**: Use zperf + host script, not Linux perf.
- **Interactive shell**: A ``perf record`` shell front-end is a planned
  follow-up on top of the Performance Event Subsystem; today the profiler is
  driven through ``include/zephyr/profiling/pmu_sampling.h``.

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

#. **Build** — from ``$ZEPHYR_BASE``:

   .. code-block:: bash

      west build -b versal_apu path/to/app

#. **Flash / boot** — use your normal Versal flow (PDI → TF-A → Zephyr). The
   in-tree board expects ``CONFIG_BUILD_WITH_TFA=y`` (see
   ``boards/amd/versal_apu/versal_apu_defconfig``).

#. **Runtime test** — from your application, drive the profiler through the API
   (see `Programmatic usage`_): ``pmu_sampling_start()`` around a workload,
   ``pmu_sampling_stop()``, then ``pmu_sampling_copy()`` / ``pmu_sampling_export_zperf()``.

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

#. **Config / build** — same Kconfig lines and API flow as
   ``versal_apu``, but build with:

   .. code-block:: bash

      west build -b versalnet_apu path/to/app

#. **SMP** — there is one ring buffer **per CPU** (``CONFIG_MP_MAX_NUM_CPUS``);
   ``pmu_sampling_copy()`` / zperf export iterate CPU 0 .. N-1 (not globally
   time-sorted). Watch **lost** counts from ``pmu_sampling_get_stats()`` if rings
   are too small.

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

   If you dump the ``pmu_sampling_export_zperf()`` blob as **ASCII**
   ``shell_hexdump`` lines over UART, the decoder **auto-parses** those lines
   (magic ``5a 50 45 52 46 56 30 31`` = ``ZPERFV01`` appears after the
   ``00000000:`` prefix on the first line). If you already have a raw binary blob
   starting with ``ZPERFV01``, pass that instead.

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
