.. zephyr:code-sample:: object_cores
   :name: Object cores
   :relevant-api: obj_core_apis obj_core_stats_apis

   Enumerate the kernel objects of a running system and query their statistics.

Overview
********

:ref:`Object cores <object_cores_api>` are a kernel debugging facility that lets
tools find every kernel object of a given type, whether it was defined
statically or initialized at run time, and read the statistics an object type
provides. The same facility backs object tracking
(:kconfig:option:`CONFIG_TRACING_OBJECT_TRACKING`) in the tracing subsystem.

The sample walks through the facility one aspect at a time:

* **Object inventory**: how many objects of each type exist, and how many were
  refused registration because they live in stack storage (``skipped``) or
  because the registry was full (``dropped``). A type whose kernel code is not
  linked into the image is reported as not present.
* **Static and run-time objects**: a semaphore created with
  :c:macro:`K_SEM_DEFINE` is reported without any call, while a semaphore in
  ordinary storage is reported once :c:func:`k_sem_init` has registered it.
* **Transient objects**: a semaphore declared inside a function is not
  registered, because its storage ends with the function, and the semaphore
  type counts it as skipped instead.
* **Threads**: a thread is reported from its creation until it is aborted.
* **Released memory**: a mutex embedded in a heap-allocated structure stops
  being reported when the structure is freed.
* **Explicit unregistration**: :c:func:`k_obj_core_unlink` removes an object
  from the walks at once.
* **Statistics**: with :kconfig:option:`CONFIG_OBJ_CORE_STATS`, the runtime
  statistics of every thread, the usage of a memory slab and the cycles
  consumed by all CPUs are read through one API,
  :c:func:`k_obj_core_stats_query`.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/kernel/object_cores
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

To build for another board, change ``qemu_x86`` above to that board's name.
On :zephyr:board:`native_sim <native_sim>` threads run on host stacks, so the
sample enables :kconfig:option:`CONFIG_ARCH_POSIX_UPDATE_STACK_INFO` to let the
kernel recognize objects in stack storage.

Object tracking
===============

The ``sample.kernel.object_cores.tracking`` configuration in :file:`tests.yaml`
builds the same program with :kconfig:option:`CONFIG_TRACING` and
:kconfig:option:`CONFIG_TRACING_OBJECT_TRACKING` instead of
:kconfig:option:`CONFIG_OBJ_CORE` in :file:`prj.conf`:

.. code-block:: console

   west build -b qemu_x86 samples/kernel/object_cores -T sample.kernel.object_cores.tracking -t run

Object tracking selects the object core framework, so the program runs unchanged
and the first line notes how the framework was enabled.

Sample Output
=============

.. code-block:: console

   Object cores sample

   Object inventory
     type           objects  skipped  dropped
     thread               3        0        0
     semaphore            1        0        0
     mutex                1        0        0
     message queue        1        0        0
     timer                1        0        0
     memory slab          1        0        0
     cpu                  1        0        0

   Static and run-time objects
     static semaphore                               reported: yes
     runtime semaphore before k_sem_init            reported: no
     runtime semaphore                              reported: yes

   Transient objects
     stack semaphore                                reported: no
     semaphores skipped: 0 -> 1

   Threads
     worker thread                                  reported: yes
     dynamic thread                                 reported: yes
     aborted thread                                 reported: no

   Released memory
     heap mutex                                     reported: yes
     heap mutex after k_free                        reported: no

   Explicit unregistration
     runtime semaphore after k_obj_core_unlink      reported: no

   Thread statistics
     main             0x110280      2212080 cycles
     idle             0x1101e0            0 cycles
     worker           0x1100a0            0 cycles

   Memory slab statistics
     slab 0x109634: allocated 128 bytes, free 128 bytes, peak 128 bytes

   System statistics
     all CPUs: 2319380 cycles, 2319380 non-idle

   Done
