Interrupt Handling Benchmark
############################

This benchmark measures the individual components of interrupt handling in
Zephyr, with the goal of characterizing a platform for interrupt-heavy
applications. Where possible it uses a *real asynchronous interrupt* raised
through the platform interrupt controller, rather than the synchronous
``irq_offload()`` trap used by the ``latency_measure`` benchmark, so that the
hardware interrupt entry path is part of what is measured.

The benchmarks are built on the :ref:`ztest benchmarking framework
<ztest_benchmarking>` as manually sampled benchmarks
(``ZTEST_BENCHMARK_MANUAL``), because every measured span has at least one
endpoint captured in a different execution context (inside the ISR, or in a
woken thread) which framework-timed benchmarks cannot express.

Scenarios
*********

All benchmarks are in the ``interrupt`` suite:

* ``entry_trigger_to_isr`` -- time from the software write that raises the
  interrupt to the first timestamp taken inside the ISR (entry latency).
* ``exit_resume_interrupted`` -- time from the end of the ISR body back to
  the interrupted thread.
* ``exit_reschedule`` -- time from the end of an ISR that wakes a higher
  priority thread to that thread running (exit plus context switch).
* ``locked_unlock_to_isr`` -- the interrupt is raised while interrupts are
  locked and kept pending for a configurable window; measured is the time
  from ``irq_unlock()`` to ISR entry (latency after a critical section).
* ``throughput_round_trip`` -- the ISR re-triggers itself so the next
  interrupt is already pending while the current one is being serviced;
  each sample runs from one ISR entry to the next and so covers a full
  ISR + exit + entry round trip under back-to-back service. Its inverse
  is the maximum sustainable interrupt rate.
* ``periodic_isr_delay`` -- how much later than its period a periodic timer
  interrupt is actually served while the kernel is busy, which is the delay
  interrupt masking inflicts on an application. See `Masking windows`_ for
  what this number does and does not prove. Needs a tick rate of at least
  100 Hz, so it is enabled by the load overlay rather than the base
  configuration.
* ``dynamic_connect`` -- cost of installing an ISR at runtime with
  ``irq_connect_dynamic()`` (needs ``CONFIG_DYNAMIC_INTERRUPTS``). Not
  available on x86, where connecting an interrupt dynamically is a one
  shot allocation of an IDT vector and an interrupt stub rather than a
  rebindable install, so it cannot be repeated to gather samples.

Each benchmark records ``CONFIG_INT_BENCH_NUM_ITERATIONS`` samples; the
framework reports mean, standard deviation, standard error and min/max with
control-measurement noise correction. The default configuration selects the
CSV output format so that twister records the values (``twister.json`` /
``recording.csv``); build with ``CONFIG_ZTEST_BENCHMARK_OUTPUT_VERBOSE=y``
for human readable output instead.

Trigger backends
****************

Interrupt generation is abstracted behind a small backend API
(``src/trigger.h``) selected with Kconfig:

``CONFIG_INT_BENCH_TRIGGER_SW_IRQ``
   Raises a real interrupt through the interrupt controller. Currently
   implemented for Cortex-M (NVIC STIR/ISPR), Arm GIC v2/v3 (SGI), ARC
   (IRQ_HINT), x86 (local APIC self IPI, both xAPIC and x2APIC) and RISC-V
   (CLIC pending bit, or the CLINT machine software interrupt where there
   is no CLIC), largely using the same mechanisms as
   ``tests/arch/common/interrupt``. The IRQ line is auto-selected and can
   be overridden with ``CONFIG_INT_BENCH_IRQ_LINE`` for SoCs where the
   automatic choice is not a free, implemented line.

   On RISC-V without a CLIC, a hart cannot make one of its own external
   interrupts pending -- PLIC pending bits are driven by the interrupt
   gateways and the ``mip`` software, timer and external bits are read
   only -- so the machine software interrupt is asserted through the
   CLINT, as the SMP IPI code does. That interrupt is level triggered, so
   the ISR deasserts it before running the measurement handler; RISC-V
   entry latency therefore includes one store to the CLINT. This backend
   is unavailable on SMP builds, where the machine software interrupt is
   already used for scheduler IPIs.

``CONFIG_INT_BENCH_TRIGGER_OFFLOAD``
   Fallback for every other architecture, based on ``irq_offload()``. Since
   this is a synchronous trap, only the exit-path scenarios run with this
   backend; entry latency, critical section and throughput scenarios are
   not available.

Background load
***************

Latency measured on an otherwise idle system is a best case: caches hold the
benchmark's own working set, no other interrupt is in service and nothing
else competes for the memory system. Build with the ``prj.load.conf``
overlay to re-run every scenario under background load:

.. code-block:: console

   west build -p -b qemu_x86 tests/benchmarks/interrupt_latency -t run -- \
       -DEXTRA_CONF_FILE=prj.load.conf

Three load sources are enabled independently
(:kconfig:option:`CONFIG_INT_BENCH_LOAD` and the options under it):

``CONFIG_INT_BENCH_LOAD_CACHE``
   Walks a working set larger than the benchmark's own between samples,
   outside the measured span, so that each measured interrupt is served with
   cold caches and TLBs. Runs in the benchmark thread, so it affects every
   scenario. Note that this has little effect under emulation: QEMU does not
   model caches, so the walk costs time but does not slow later accesses.

``CONFIG_INT_BENCH_LOAD_TIMER``
   A periodic timer whose handler runs in the system clock ISR, competing
   with the benchmark interrupt for the interrupt controller and for the
   interrupt-disabled windows of the kernel timeout code. This is the load
   source that dominates under emulation.

``CONFIG_INT_BENCH_LOAD_THREADS``
   Threads of lower priority than the benchmark that continuously write to
   memory. On SMP these run on the other CPUs and contend for the memory
   system throughout; on a uniprocessor they only run while the benchmark
   blocks, so their effect is limited to the rescheduling scenario. They are
   deliberately kept below the benchmark's priority so that they cannot
   preempt it and be mistaken for interrupt latency.

Under load the mean stays close to the idle figure while the maximum and the
standard deviation grow by an order of magnitude or more; on qemu_x86 the
entry latency maximum rises from 1184 cycles idle to over 25000 under load.
The maximum is the number that matters for a real-time application, and it
is only visible with the system doing something.

The load configuration can be impractically slow on emulated targets. The
critical section scenario busy waits with interrupts locked while the load
timer keeps expiring, and under emulation that combination can cost orders
of magnitude more wall clock than the nominal hold time; ARC and
Cortex-M3 are excluded from the load run in ``tests.yaml`` for that reason. On such targets, lower
:kconfig:option:`CONFIG_INT_BENCH_NUM_ITERATIONS`, or disable
:kconfig:option:`CONFIG_INT_BENCH_LOAD_TIMER`.

How much each scenario moves under load is itself informative. On Arm GIC
the benchmark SGI is configured above the system timer, so
``locked_unlock_to_isr`` does not move at all: whatever else is pending when
interrupts are unmasked, the benchmark interrupt is serviced first. On x86
the same scenario grows from 928 to 1726 cycles on average, because the
competing timer interrupt outranks the benchmark vector and is serviced
before it. That difference is a property of the platform's interrupt
priorities rather than of the benchmark.

``throughput_round_trip`` barely moves anywhere. It saturates the interrupt
path by design, so it measures a steady state that stays warm whatever else
the system is doing.

Masking windows
***************

``periodic_isr_delay`` addresses a question the other scenarios cannot: how
long does the system keep interrupts masked? That is a property of the
kernel and the drivers rather than something the benchmark can trigger, and
Zephyr has no instrumentation of ``irq_lock()`` windows, so it is measured
by its effect. A periodic timer runs at the tick rate while the benchmark
exercises kernel primitives that take spinlocks, and each sample is how
much later than its period a timer interrupt was served.

Read the maximum as *the worst delay a periodic real-time event was
actually made to suffer*, which is the number an application cares about.
It is not a measurement of the longest ``irq_lock()`` in the tree, for
three reasons: a sample is only taken at a tick boundary, so a masked
window that does not overlap one is never seen; the measured delay also
contains time spent in interrupts that were served first; and the framework
subtracts its control measurement from every statistic, so samples with no
delay are reported as a small negative number.

:kconfig:option:`CONFIG_INT_BENCH_MASK_INJECT_US` masks interrupts for a
known time in the work loop, to confirm the measurement responds on a new
platform. On qemu_x86 with a 1 kHz tick, injecting 50us raises the reported
maximum from roughly 700 cycles to 57568, or about 58us against a 50us
window. Keep the injected value well below the tick period: a window
comparable to it masks nearly continuously, the timer re-anchors to when it
was last serviced, and the measured delay then falls well short of the
injected one. The same effect limits what the scenario can report about a
system that is genuinely saturated.

The undelayed interval is established empirically, as the median of the
observed intervals, rather than computed from the configured tick rate. The
timing counter does not run at ``timing_freq_get()`` on every platform, and
a baseline off by a constant would either hide every delay or turn every
interval into one; on RISC-V the computed baseline exceeded every observed
interval, so the scenario reported nothing at all until it was made
self-calibrating.

Where the periodic timer does not deliver at the tick rate at all, the
scenario reports that it was skipped and records no samples, which the
framework shows as inconclusive. That is the case on qemu_cortex_m3.

This scenario is the most sensitive of all of them to being run under
emulation, because it measures wall clock delay rather than a span of the
guest's own execution. When the host deschedules the emulator, the guest
sees it as a late interrupt: on qemu_x86_64 that shows up as isolated
maxima of over a hundred million cycles, which say nothing about Zephyr.
Read this scenario on hardware.

Running
*******

.. code-block:: console

   west build -p -b qemu_cortex_m3 tests/benchmarks/interrupt_latency -t run

or via twister, which also collects the metrics:

.. code-block:: console

   scripts/twister -p qemu_cortex_m3 -T tests/benchmarks/interrupt_latency

Sample CSV output (see :kconfig:option:`CONFIG_ZTEST_BENCHMARK_OUTPUT_CSV`
for the column layout)::

   M,interrupt,entry_trigger_to_isr,1000,55000,55.000,0.000,0.000,55,1,55,1
   M,interrupt,exit_resume_interrupted,1000,56000,56.000,0.000,0.000,56,1,56,1

Notes on methodology
********************

* Timestamps use the timing subsystem; the framework subtracts a control
  measurement from every reported statistic.
* The system tick rate is kept low so timer interrupts do not perturb
  most samples; residual hits show up in the maximum and standard
  deviation. It cannot be made arbitrarily low: a tick has to fit the
  timer's counter, and a Cortex-M SysTick is 24 bits, so asking for one
  tick per second on a 150MHz part makes the driver fall back to its
  ten microsecond minimum and interrupt 78000 times a second instead of
  once.
* The first iteration of each scenario runs with cold caches, branch
  predictors and TLBs and is typically an order of magnitude slower than
  the steady state. The framework reports it separately as the cold cost,
  and :kconfig:option:`CONFIG_ZTEST_BENCHMARK_WARMUP` discards further
  iterations before sampling if the steady state is all that matters.
* Entry latency includes the cost of the trigger write itself and, on some
  interrupt controllers, the propagation delay of the software-generated
  interrupt. Numbers are therefore comparable across Zephyr versions and
  configurations on the same platform, and indicative across platforms.

Future work
***********

* sw-irq trigger backend for Xtensa (INTSET).
* Nested interrupt preemption latency and priority interference, both of
  which need the trigger backend extended to a second line at a second
  priority.
* Direct ISR (``IRQ_DIRECT_CONNECT``) and zero-latency IRQ comparison
  scenarios.
* Percentile (p99) reporting and latency distribution histograms, which
  the benchmark framework cannot express today because it keeps no
  sample buffer.
* An external event trigger, for example a GPIO loopback described in
  devicetree, so that entry latency includes the pin and interrupt
  controller propagation delays that a software trigger skips.
* SMP scenarios (IPI latency, ISR on a non-boot CPU).
