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
