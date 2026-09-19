.. zephyr:code-sample:: object_monitor
   :name: Object monitor
   :relevant-api: obj_core_apis obj_core_stats_apis

   Watch kernel objects come and go under a running application.

Overview
********

This sample runs a small application and, next to it, a monitor thread that
uses :ref:`object cores <object_cores_api>` to draw a live table of the kernel
objects in the system. Where the :zephyr:code-sample:`object_cores` sample
walks through the facility step by step, this one shows it the way a debugging
tool would use it: continuously, on objects it did not create itself.

The application is a job server:

* a sensor thread posts samples into a message queue;
* a dispatcher starts short-lived job threads, up to four at a time, each with
  a heap-allocated request that embeds a semaphore, a mutex and a timer;
* every job waits on a stack-local semaphore for each of its processing steps,
  then frees its request and exits.

Threads, requests and their embedded objects therefore appear and disappear
all the time. The monitor refreshes every second and prints:

* the number of objects of each type, with the ``skipped`` count of objects that
  were not registered because they live in stack storage (the job step
  semaphores) and the ``dropped`` count of registrations refused because the
  registry was full;
* every thread with its state and its share of the CPU over the last period,
  computed from the runtime statistics that
  :c:func:`k_obj_core_stats_query` returns for threads and for the kernel;
* the number of jobs started, finished and running.

Lowering :kconfig:option:`CONFIG_OBJ_CORE_MAX_DYNAMIC_OBJECTS` below the peak
number of run-time objects (about 24 here) makes the ``dropped`` column count
up as the registry overflows, without any effect on the application.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/kernel/object_monitor
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

To build for another board, change ``qemu_x86`` above to that board's name.
On :zephyr:board:`native_sim <native_sim>` threads run on host stacks, so the
sample enables :kconfig:option:`CONFIG_ARCH_POSIX_UPDATE_STACK_INFO` to let the
kernel recognize objects in stack storage.

By default the monitor clears the terminal and redraws in place. Set
:kconfig:option:`CONFIG_SAMPLE_MONITOR_CLEAR_SCREEN` to ``n`` to let the output
scroll, and :kconfig:option:`CONFIG_SAMPLE_MONITOR_PERIOD_MS` to change the
refresh period.

The ``sample.kernel.object_monitor.tracking`` configuration in
:file:`tests.yaml` builds the same program with
:kconfig:option:`CONFIG_TRACING_OBJECT_TRACKING`, which enables the object core
framework through the tracing subsystem.

Sample Output
=============

.. code-block:: console

   Object monitor   uptime 92.3s   refresh 181

     type           objects  skipped  dropped
     thread               6        0        0
     semaphore            2      801        0
     mutex                2        0        0
     timer                2        0        0
     message queue        1        0        0

     thread       address    state        cpu
     monitor      0x1105a0  running     0.5%
     idle         0x110500  ready      96.7%
     dispatcher   0x110160  sleeping    0.0%
     sensor       0x1100c0  sleeping    0.0%
     job-132      0x1103e0  pending     2.3%
     job-133      0x110340  pending     0.2%

     jobs: 133 started, 131 finished, 2 running; samples dropped: 23

Two jobs are running, so two requests exist and the semaphore, mutex and timer
counts are each two objects above the static ones (the message queue is the
only static object besides the threads). The 801 skipped semaphores are the
step semaphores of all the jobs so far; none of them was ever registered.
