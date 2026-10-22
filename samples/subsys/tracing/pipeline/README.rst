.. zephyr:code-sample:: tracing-pipeline
   :name: Tracing a sensor pipeline

   Trace a realistic multi-threaded sensor pipeline and use the trace to
   understand how the RTOS schedules it - and to spot a priority inversion.

Overview
********

This sample is a small but representative real-time application built so that
its trace tells a story. It models a data pipeline of the kind found in many
embedded products and drives most of the kernel's synchronisation primitives,
so that when the trace is captured and opened in the console trace viewer
(:zephyr_file:`scripts/tracing/trace_viewer.py`) the schedule, the blocking and
the contention are all easy to see.

Pipeline stages, from highest to lowest scheduling priority:

.. list-table::
   :header-rows: 1

   * - Thread
     - Priority
     - Role
   * - ``aggregator``
     - 3 (highest)
     - Drains a message queue of sensor readings, updates a mutex-protected
       aggregate, and every few readings publishes a frame to the consumers and
       writes a summary to a shared "bus".
   * - ``consumer0`` / ``consumer1``
     - 6
     - Wait on a condition variable for a new frame, then do a chunk of
       CPU-bound processing.
   * - ``sensor_temp`` / ``sensor_press`` / ``sensor_imu``
     - 8
     - Three periodic producers (different periods) that push readings into the
       message queue and sleep.
   * - ``storage``
     - 9 (lowest)
     - Periodically grabs the shared bus and holds it for a while to simulate a
       slow flush to storage.

Primitives and synchronisation methods exercised:

* :c:struct:`k_msgq` - sensors hand readings to the aggregator.
* :c:struct:`k_mutex` - protects the shared aggregate (and, by default, the bus).
* :c:struct:`k_condvar` - the aggregator wakes the consumers when a frame is ready.
* :c:struct:`k_sem` - optional bus lock used to demonstrate priority inversion.
* :c:struct:`k_work_delayable` on the system work queue - a periodic background flush.
* :c:struct:`k_timer` - a heartbeat firing from the system clock ISR.
* Several thread priorities, so preemption and context switches are visible.

Application phases are annotated with :c:func:`sys_trace_named_event` (for
example ``sensor_sample``, ``frame_publish``, ``consume_begin``, ``store_begin``
and ``bus_write``) so they are easy to locate in the trace.

Building and running
********************

Like the basic tracing sample, the default configuration uses the CTF format
over the semihosting backend, so a single command produces a trace file:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/tracing/pipeline
   :board: mps2/an385
   :goals: build run
   :compact:

This drops a ``tracing.bin`` in the build directory. Open it with the viewer:

.. code-block:: console

   $ZEPHYR_BASE/scripts/tracing/trace_viewer.py build/tracing.bin

On ``native_sim`` use the POSIX backend overlay, which writes the stream to a
``channel0_0`` file in the build directory:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/tracing/pipeline
   :board: native_sim
   :gen-args: -DEXTRA_CONF_FILE=prj_native_ctf.conf
   :goals: build
   :compact:

.. code-block:: console

   ./build/zephyr/zephyr.exe &
   $ZEPHYR_BASE/scripts/tracing/trace_viewer.py build/channel0_0

Reading the trace
*****************

In the viewer the lanes are coloured by thread state, so the pipeline's rhythm
is immediately visible:

* The **sensors** spend most of their time sleeping (``░``) and briefly run
  (``█``) each period to push a reading.
* The **aggregator** sits blocked on the message queue (``▒``, reason
  ``msgq``) and wakes the instant a reading arrives - watch it preempt whatever
  was running, because it has the highest priority.
* The **consumers** are blocked on the condition variable (``▒``, reason
  ``condvar``) until a frame is published, then burst into a run of CPU-bound
  work.
* The metrics panel summarises the visible window: CPU-busy percentage, context
  switch and event rates, and a per-thread utilization bar.

Selecting a lane shows, for the cursor position, exactly *why* a thread is
blocked (the semaphore, mutex, message queue or condition variable it is waiting
on). This is the core of how tracing helps: the blocking reason turns "the
system feels sluggish" into "the aggregator is blocked on the bus".

Finding a priority inversion
============================

The ``storage`` thread (lowest priority) and the ``aggregator`` (highest
priority) share a "bus". The primitive guarding it is selectable at build time:

* ``CONFIG_SAMPLE_BUS_MUTEX`` (default) - the bus is a mutex. When the high
  priority aggregator blocks on it, Zephyr's priority inheritance temporarily
  boosts the low priority ``storage`` thread so it finishes and releases
  quickly. In the trace you can even see ``storage`` jump to priority 3 (a
  ``thread_sched_priority_set`` event) for the duration.
* ``CONFIG_SAMPLE_BUS_SEM`` - the bus is a plain semaphore, which has no
  priority inheritance. Now a medium-priority ``consumer`` can run while
  ``storage`` holds the bus, leaving the highest-priority ``aggregator`` stranded
  - a textbook priority inversion.

Build the inverting variant and capture its trace:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/tracing/pipeline
   :board: mps2/an385
   :gen-args: -DCONFIG_SAMPLE_BUS_SEM=y
   :goals: build run
   :compact:

Zoom in on one of the ``aggregator``'s ``blocked on sem`` spans and the
inversion is unmistakable: the highest-priority thread is blocked (``▒``) while
a medium-priority ``consumer`` runs (``█``) and the lowest-priority ``storage``
thread holds the bus::

   storage     p9 ██████▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░
   consumer0   p6 ▒▒▒▒▒▒██████████████████████▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
   consumer1   p6 ▒▒▒▒▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓████████████████████▒▒▒▒▒
   aggregator  p3 ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒████▒▒

Comparing the two builds in the viewer shows the aggregator's worst-case wait on
the bus growing several-fold once priority inheritance is removed - the kind of
latency bug that is hard to reason about from logs alone but obvious in a trace.
