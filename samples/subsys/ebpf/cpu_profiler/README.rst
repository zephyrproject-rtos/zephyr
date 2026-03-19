.. zephyr:code-sample:: ebpf-cpu-profiler
   :name: eBPF CPU Profiler

   Build a real-time CPU utilisation monitor with an idle-duration histogram
   using eBPF bytecode on the kernel idle path.

Overview
********

This sample runs a 49-instruction eBPF program every time the CPU leaves the
idle state. The program performs five distinct operations in a single
invocation:

1. Computes the idle duration from timestamps.
2. Accumulates total idle nanoseconds in a statistics map.
3. Increments a transition counter in the same map.
4. Classifies the idle period into one of six histogram buckets using a
   chain of ``JLT`` (jump-if-less-than) comparisons.
5. Increments the matching bucket count in a histogram map.

A companion 11-instruction program timestamps entry into the idle thread.

Together the two programs maintain three array maps that the application
drains every two seconds to print:

* **Active / Idle percentage** — derived from accumulated idle nanoseconds
  versus wall-clock time.
* **Transition count** — how often the CPU left idle.
* **ASCII histogram** — distribution of idle durations across six buckets
  ranging from *< 100 µs* to *> 100 ms*.

A worker thread switches to a different sleep duration each report interval
so that the dominant histogram bucket visibly shifts from report to report.
Tickless kernel mode is enabled to minimise background timer noise.

Architecture
************

The following diagram shows how the pieces fit together.

Control path (setup)
====================

.. code-block:: none

   main()
     |
     |-- patch map IDs into bytecode
     |-- attach idle_enter --> IDLE_ENTER tracepoint
     |-- attach idle_exit  --> IDLE_EXIT  tracepoint
     |-- enable both programs (triggers verifier)
     |-- spawn worker thread
     |-- reporting loop (every 2 s):
     |       set worker_phase
     |       read stats_map  --> compute idle %
     |       read histogram  --> compute deltas
     |       print ASCII report
     '-- disable & detach

Event path (runs in idle-thread context, every idle transition)
===============================================================

.. code-block:: none

   Zephyr kernel
     |
     |-- CPU goes idle --> sys_trace_idle()
     |       |
     |       '--> idle_enter eBPF program (11 insns)
     |               ts = ktime_get_ns()
     |               ts_map[0] = ts
     |
     |-- CPU wakes up --> sys_trace_idle_exit()
             |
             '--> idle_exit eBPF program (49 insns)
                    Phase 1: duration = ktime_get_ns() - ts_map[0]
                    Phase 2: stats_map[0] += duration
                    Phase 3: stats_map[1] += 1
                    Phase 4: bucket = classify(duration)  [JLT chain]
                    Phase 5: histogram[bucket] += 1

Data path (shared maps)
=======================

.. code-block:: none

   +------------------+     +---------------------+     +------------------+
   | ts_map           |     | stats_map           |     | histogram        |
   | ARRAY[1] u64     |     | ARRAY[2] u64        |     | ARRAY[6] u64     |
   |                  |     |                     |     |                  |
   | [0] entry_ts  <--+--   | [0] total_idle_ns --+-->  | [0] < 100 us   --+-->
   |   written by     |  |  | [1] transitions   --+-->  | [1] 100-500 us --+-->
   |   idle_enter     |  |  |   written by        |     | [2] 500us-1ms  --+-->
   |   read by        |  |  |   idle_exit         |     | [3] 1-10 ms    --+-->
   |   idle_exit      |  |  |   read by main()    |     | [4] 10-100 ms  --+-->
   +------------------+  |  +---------------------+     | [5] > 100 ms   --+-->
                         |                              |   written by     |
                         |                              |   idle_exit      |
                         |                              |   read by main() |
                         |                              +------------------+
                         |
                         '-- both programs share this timestamp slot

This sample demonstrates:

* a 49-instruction eBPF program with 5 phases, 4 helper calls, and
  10 conditional jumps — all hand-crafted in the eBPF instruction macros,
* three cooperating array maps (timestamp, statistics, histogram),
* the ``ktime_get_ns`` helper for cycle-accurate timing,
* jump-chain classification logic inside eBPF bytecode,
* callee-saved register discipline (R6 duration, R9 bucket) across calls,
* delta-based reporting from cumulative map values,
* per-phase worker scheduling to show the histogram shifting in real time.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ebpf/cpu_profiler
   :host-os: unix
   :board: frdm_mcxn236
   :goals: build run
   :compact:

Sample Output
*************

Each report is captured while the worker sleeps at a single fixed duration,
so the histogram highlights a different bucket every two seconds:

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0 ***
   eBPF CPU profiler
   =================

   eBPF programs attached to IDLE_ENTER / IDLE_EXIT.
   Profiling (reports every 2 s)...

   --- CPU Profiler [1] sleep 300 us ---
   Active: 12%  Idle: 88%  Transitions: 5001

   Idle duration distribution:
          < 100 us : #                         1
      100 - 500 us : ########################  5000
      500 us - 1ms :                           0
         1 - 10 ms :                           0
       10 - 100 ms :                           0
          > 100 ms :                           0

   --- CPU Profiler [2] sleep 800 us ---
   Active: 7%  Idle: 93%  Transitions: 2223

   Idle duration distribution:
          < 100 us :                           0
      100 - 500 us : #                         1
      500 us - 1ms : ########################  2222
         1 - 10 ms :                           0
       10 - 100 ms :                           0
          > 100 ms :                           0

   --- CPU Profiler [3] sleep 5 ms ---
   Active: 3%  Idle: 97%  Transitions: 393

   Idle duration distribution:
          < 100 us :                           0
      100 - 500 us :                           0
      500 us - 1ms : #                         1
         1 - 10 ms : ########################  392
       10 - 100 ms :                           0
          > 100 ms :                           0

   --- CPU Profiler [4] sleep 50 ms ---
   Active: 2%  Idle: 98%  Transitions: 40

   Idle duration distribution:
          < 100 us :                           0
      100 - 500 us :                           0
      500 us - 1ms :                           0
         1 - 10 ms :                           0
       10 - 100 ms : ########################  40
          > 100 ms :                           0

   --- CPU Profiler [5] sleep 200 ms ---
   Active: 2%  Idle: 98%  Transitions: 20

   Idle duration distribution:
          < 100 us :                           0
      100 - 500 us :                           0
      500 us - 1ms :                           0
         1 - 10 ms :                           0
       10 - 100 ms : ########################  10
          > 100 ms : ########################  10

   eBPF programs detached. Profiling complete.

Related Documentation
*********************

For an introduction to the subsystem design and the execution model,
see :ref:`ebpf`.
