.. zephyr:code-sample:: ebpf-hello-trace
   :name: eBPF Hello Trace

   Attach an eBPF program to a Zephyr tracepoint and count context switches.

Overview
********

This sample demonstrates the Zephyr eBPF subsystem by attaching a small eBPF
program to the :c:enumerator:`EBPF_TP_THREAD_SWITCHED_IN` tracepoint. Each
time the scheduler switches to a thread, the program runs, looks up entry
``0`` in an array map, and increments the counter stored there.

The sample highlights four core ideas:

* an eBPF program is defined as a small instruction array,
* maps provide persistent state across repeated program invocations,
* tracepoints decide when the eBPF program runs,
* the eBPF VM and eBPF verifier allow the program to run without adding
  custom kernel code for each new observation idea.

To generate frequent context switches, the sample starts a worker thread that
periodically sleeps. The main thread reads the map once per second and prints
the current counter value together with the average execution time recorded for
the eBPF program.

Building and Running
********************

The sample can be built and run on boards that support the Zephyr eBPF sample
configuration, including ``frdm_mcxn236``:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ebpf/hello_trace
   :host-os: unix
   :board: frdm_mcxn236
   :goals: build run
   :compact:

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0-8937-g519c5fcd89ac ***
   eBPF hello_trace sample
   =======================

   eBPF program 'count_switches' attached and enabled.
   Counting context switches...

   [ 1] Context switches: 21 (avg exec: 10612 ns)
   [ 2] Context switches: 42 (avg exec: 10567 ns)
   [ 3] Context switches: 63 (avg exec: 10552 ns)
   [ 4] Context switches: 84 (avg exec: 10545 ns)
   [ 5] Context switches: 105 (avg exec: 10540 ns)
   [ 6] Context switches: 126 (avg exec: 10537 ns)
   [ 7] Context switches: 147 (avg exec: 10535 ns)
   [ 8] Context switches: 168 (avg exec: 10534 ns)
   [ 9] Context switches: 189 (avg exec: 10533 ns)
   [10] Context switches: 210 (avg exec: 10532 ns)

   eBPF program detached. Sample complete.

Related Documentation
*********************

For an introduction to the subsystem design and the execution model,
see :ref:`ebpf`.
