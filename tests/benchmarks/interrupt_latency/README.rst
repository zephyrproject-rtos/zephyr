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
  ``irq_connect_dynamic()`` (needs ``CONFIG_DYNAMIC_INTERRUPTS``).

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
   implemented for Cortex-M (NVIC STIR/ISPR), Arm GIC v2/v3 (SGI) and ARC
   (IRQ_HINT), using the same mechanisms as ``tests/arch/common/interrupt``.
   The IRQ line is auto-selected (an SGI on GIC, ``CONFIG_NUM_IRQS - 1``
   otherwise) and can be overridden with ``CONFIG_INT_BENCH_IRQ_LINE`` for
   SoCs where the automatic choice is not a free, implemented line.

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
* Entry latency includes the cost of the trigger write itself and, on some
  interrupt controllers, the propagation delay of the software-generated
  interrupt. Numbers are therefore comparable across Zephyr versions and
  configurations on the same platform, and indicative across platforms.

Future work
***********

* sw-irq trigger backends for x86 (LOAPIC self-IPI), RISC-V (CLIC/mip) and
  Xtensa (INTSET).
* Nested interrupt preemption latency (two lines, two priorities).
* Direct ISR (``IRQ_DIRECT_CONNECT``) and zero-latency IRQ comparison
  scenarios.
* Percentile (p99) reporting and latency distribution histograms.
* Background-load variants and SMP scenarios (IPI latency, ISR on
  non-boot CPU).
