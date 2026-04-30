.. zephyr:code-sample:: ebpf-isr-latency
   :name: eBPF ISR Latency Profiler

   Measure interrupt service routine execution time with two cooperating eBPF
   programs and a ring buffer map.

Overview
********

This sample demonstrates advanced usage of the Zephyr eBPF subsystem by
profiling interrupt service routine (ISR) latency at runtime. Two eBPF
programs cooperate through shared maps:

* **isr_entry** — attached to :c:enumerator:`EBPF_TP_ISR_ENTER`, captures a
  nanosecond timestamp via the ``ktime_get_ns`` helper and stores it in an
  array map.
* **isr_exit** — attached to :c:enumerator:`EBPF_TP_ISR_EXIT`, reads the
  entry timestamp, computes the elapsed time, and writes the duration to a
  ring buffer map using ``ringbuf_output``.

The application thread drains the ring buffer once per second and prints
min / avg / max latency statistics.

This one demonstrates:

* two eBPF programs cooperating through a shared timestamp map,
* the ring buffer map type for variable-rate event streaming,
* the ``ktime_get_ns`` helper for high-resolution timing,
* the ``ringbuf_output`` helper called from eBPF bytecode,
* statistical aggregation on the application side.

.. note::

   A single timestamp slot is used, so nested interrupts may cause the outer
   ISR's duration to be measured inaccurately.

Building and Running
********************

The sample can be built and run on boards that support the Zephyr eBPF
configuration, including ``frdm_mcxn236``:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ebpf/isr_latency
   :host-os: unix
   :board: frdm_mcxn236
   :goals: build run
   :compact:

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0-8937-g6c78d4a856d1 ***
   eBPF ISR latency profiler
   ========================

   eBPF programs attached to ISR_ENTER and ISR_EXIT.
   Profiling ISR latency...

   [ 1] ISRs: 9  avg: 15446 ns  min: 14873 ns  max: 17500 ns
   [ 2] ISRs: 9  avg: 15228 ns  min: 14906 ns  max: 17240 ns
   [ 3] ISRs: 9  avg: 15228 ns  min: 14900 ns  max: 17240 ns
   [ 4] ISRs: 9  avg: 15229 ns  min: 14906 ns  max: 17240 ns
   [ 5] ISRs: 9  avg: 15228 ns  min: 14906 ns  max: 17240 ns
   [ 6] ISRs: 9  avg: 15226 ns  min: 14900 ns  max: 17233 ns
   [ 7] ISRs: 9  avg: 15227 ns  min: 14906 ns  max: 17240 ns
   [ 8] ISRs: 9  avg: 15228 ns  min: 14906 ns  max: 17240 ns
   [ 9] ISRs: 9  avg: 15228 ns  min: 14906 ns  max: 17240 ns
   [10] ISRs: 9  avg: 15228 ns  min: 14900 ns  max: 17240 ns

   eBPF programs detached. Profiling complete.

Related Documentation
*********************

For an introduction to the subsystem design and the execution model,
see :ref:`ebpf`.
