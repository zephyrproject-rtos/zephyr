.. _samples_scheduler_metairq_dispatch:

MetaIRQ Thread Priority Demonstration
#####################################

Overview
********

This sample demonstrates the use of a thread running at a MetaIRQ
priority level to implement "bottom half" style processing
synchronously with the end of a hardware ISR.  It implements a
simulated "device" that produces messages that need to be dispatched
to asynchronous queues feeding several worker threads, each running at
a different priority.  The dispatch is handled by a MetaIRQ thread fed
via a queue from the device ISR (really just a timer interrupt).

Each message has a random (and non-trivial) amount of processing that
must happen in the worker thread.  This implements a "bursty load"
environment where occasional spikes in load require preemption of
running threads and delay scheduling of lower priority threads.
Messages are accompanied by a timestamp that allows per-message
latencies to be computed at several points:

* The cycle time between message creation in the ISR and receipt by
  the MetaIRQ thread for dispatch.

* The time between ISR and receipt by the worker thread.

* The real time spent processing the message in the worker thread, for
  comparison with the required processing time.  This provides a way
  to measure preemption overhead where the thread is not scheduled.

Aspects to note in the results:

* On average, higher priority (lower numbered) threads have better
  latencies and lower processing delays, as expected.

* Cooperatively scheduled threads have significantly better processing
  delay behavior than preemptible ones, as they can only be preempted
  by the MetaIRQ thread.

* Because of queueing and the bursty load, all worker threads of any
  priority will experience some load-dependent delays, as the CPU
  occasionally has more work to do than time available.

* But, no matter the system load or thread configuration, the MetaIRQ
  thread always runs immediately after the ISR.  It shows reliable,
  constant latency under all circumstances because it can preempt all
  other threads, including cooperative ones that cannot normally be
  preempted.

Requirements
************

This sample should run well on any Zephyr platform that provides
preemption of running threads by interrupts, a working timer driver,
and working log output.  For precision reasons, it produces better
(and more) data on systems with a high timer tick rate (ideally 10+
kHz).

Note that because the test is fundamentally measuring thread
preemption behavior, it does not run without modification on
native_posix platforms.  In that emulation environment, threads will
not be preempted except at specific instrumentation points (e.g. in
k_busy_wait()) where they will voluntarily release the CPU.

Building and Running
********************

This application can be built and executed on frdm_k64f as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/kernel/metairq_dispatch
   :board: frdm_k64f
   :goals: build flash
   :compact:

To build for another board, change "frdm_k64f" above to that board's name.

Sample Output
=============

Sample output shown below is from frdm_k64f. The numbers will
look different based upon the clock frequency of the board and other
factors.

Note: The 'real' values may be significantly higher than corresponding 'proc' values
(intended) for non-cooperative threads like T2 and T3 which is attributed to delays
due to thread preemption.


.. code-block:: console

   I: Starting Thread0 at priority -2
   I: Starting Thread1 at priority -1
   II: M0 T0 mirq 4478 disp 7478 proc 24336 real 24613
   I: M8 T0 mirq 4273 disp 86983 proc 9824 real 16753
   I: M10 T0 mirq 4273 disp 495455 proc 21177 real 28273
   I: M11 T0 mirq 4273 disp 981565 proc 48337 real 48918
   I: M14 T0 mirq 4273 disp 1403627 proc 7079 real 7690
   I: M17 T0 mirq 4273 disp 1810028 proc 42143 real 42925
   I: M19 T0 mirq 4273 disp 2369217 proc 42471 real 42925
   I: M20 T0 mirq 4273 disp 2940429 proc 30427 real 30775
   I: M21 T0 mirq 4273 disp 3524151 proc 35871 real 36850
   I: M22 T0 mirq 4273 disp 4042148 proc 33738 real 34420
   I: M23 T0 mirq 4273 disp 4557010 proc 5757 real 6475
   I: M29 T0 mirq 4273 disp 4759422 proc 49748 real 50215
   I: M33 T0 mirq 4273 disp 5218935 proc 32105 real 33205
   I: M35 T0 mirq 4273 disp 5732769 proc 32678 real 33205
   I: M36 T0 mirq 4273 disp 6294586 proc 9303 real 10120
   I: M37 T0 mirq 4273 disp 6819398 proc 29960 real 30775
   I: M40 T0 mirq 3189 disp 7199139 proc 19692 real 21055
   I: M9 T1 mirq 4273 disp 9417562 proc 53844 real 55075
   I: M18 T1 mirq 4273 disp 9522780 proc 4560 real 5260
   I: M25 T1 mirq 4273 disp 9713189 proc 56656 real 57505
   I: M27 T1 mirq 4273 disp 10238978 proc 48347 real 49000
   : Starting Thread2 at priority 0
   I: M1 T2 mirq 4273 disp 12740821 proc 40449 real 41710
   I: M2 T2 mirq 4273 disp 13311098 proc 43926 real 44140
   I: M3 T2 mirq 4273 disp 13824365 proc 41212 real 41710
   I: M4 T2 mirq 4273 disp 14382522 proc 12859 real 13765
   I: M5 T2 mirq 4273 disp 14869754 proc 17303 real 18625
   I: M7 T2 mirq 4273 disp 15368993 proc 10666 real 11335
   I: M15 T2 mirq 4273 disp 15364239 proc 45304 real 46570
   I: M24 T2 mirq 4273 disp 15602017 proc 39637 real 40495
   I: M28 T2 mirq 4273 disp 15989555 proc 24436 real 24700
   I: M30 T2 mirq 4273 disp 16493444 proc 44374 real 45355
   I: M31 T2 mirq 4273 disp 17078141 proc 21947 real 22270
   I: M34 T2 mirq 4273 disp 17555966 proc 47779 real 49000
   I: M39 T2 mirq 4273 disp 17843806 proc 10954 real 11335
   I: Starting Thread3 at priority 1
   I: M6 T3 mirq 4273 disp 20625899 proc 13459 real 13765
   I: M12 T3 mirq 4273 disp 20813171 proc 13534 real 13765
   I: M13 T3 mirq 4273 disp 21334833 proc 37091 real 38065
   I: M16 T3 mirq 4273 disp 21744289 proc 42514 real 42925
   I: M26 T3 mirq 4273 disp 21846939 proc 36261 real 36850
   I: M32 T3 mirq 4273 disp 22207336 proc 49987 real 50215
   I: M38 T3 mirq 4273 disp 22532228 proc 37164 real 38065
   I:         ---------- Latency (cyc) ----------
   I:             Best    Worst     Mean    Stdev
   I: MetaIRQ     4273     4478     4278       32
   I: Thread0     7478  6819398  3190200  2183592
   I: Thread1  9417562 10238978  9723127   316113
   I: Thread2 12740821 17843806 15417286  1525493
   I: Thread3 20625899 22532228 21586385   649911
   I: MetaIRQ Test Complete
