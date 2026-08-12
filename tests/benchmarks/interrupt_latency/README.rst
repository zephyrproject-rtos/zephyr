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
* Nested interrupt preemption latency (two lines, two priorities).
* Direct ISR (``IRQ_DIRECT_CONNECT``) and zero-latency IRQ comparison
  scenarios.
* Percentile (p99) reporting and latency distribution histograms.
* Background-load variants and SMP scenarios (IPI latency, ISR on
  non-boot CPU).
