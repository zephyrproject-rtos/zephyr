.. _applet:

Applets
#######

An applet is a lightweight sub-application within Zephyr: a group of threads that share a
lifecycle and, optionally, a memory domain. The applet subsystem manages that group as a single
unit, so an application can start, join, kill and unload a set of related threads through one
descriptor instead of tracking each thread individually.

The subsystem can also be used as a convenience wrapper around :ref:`llext`, simplifying startup
and user mode memory domain management for extensions.

Applets are enabled with :kconfig:option:`CONFIG_APPLET`.

Applet kinds
************

An applet is one of two kinds, selected by the function used to set it up.

Native
   The threads run entry functions that are statically linked into the main Zephyr image. No ELF
   loading or linking is involved. Created with :c:func:`applet_init`.

LLEXT-backed
   The code is loaded at runtime from an ELF binary through the LLEXT subsystem, and thread entry
   points are resolved from the extension's export table by symbol name. Created with
   :c:func:`applet_load_llext`, or created and started in one step with :c:func:`applet_spawn`.
   Requires :kconfig:option:`CONFIG_APPLET_LLEXT`.

Both kinds share the same lifecycle, threading and memory domain behaviour. Only the origin of the
code differs.

Lifecycle
*********

An applet moves through the states in :c:enum:`applet_state`:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - State
     - Meaning
   * - ``APPLET_STATE_UNLOADED``
     - Descriptor holds no resources. The initial state, and the state after
       :c:func:`applet_unload`.
   * - ``APPLET_STATE_LOADED``
     - Ready to run. Threads may be attached and partitions added.
   * - ``APPLET_STATE_RUNNING``
     - At least one attached thread has been started and has not yet terminated.
   * - ``APPLET_STATE_DEAD``
     - Every started thread has terminated.

A descriptor must be zero-initialised before its first use, which static storage already
guarantees. After :c:func:`applet_unload` it may be re-initialised and used again; the memory
domain is retained and reused, because most architectures cannot deinitialise one.

The transition from ``APPLET_STATE_RUNNING`` to ``APPLET_STATE_DEAD`` only happens when the state
is observed. Applet threads never report their own exit: a user mode thread has no access to the
descriptor, and an aborted thread never runs cleanup code. :c:func:`applet_get_state` instead
samples the threads and reports ``APPLET_STATE_DEAD`` once all of them have terminated, whether
they returned normally, were aborted, or died on a fatal error. Always call
:c:func:`applet_get_state` rather than reading the descriptor directly.

Threads
*******

Threads are attached to a loaded applet with :c:func:`applet_add_thread`, or with
:c:func:`applet_add_thread_sym` to resolve the entry point from an LLEXT export table. The caller
always supplies the stack; the subsystem never allocates one.
:kconfig:option:`CONFIG_APPLET_THREAD_STACK_SIZE_DEFAULT` is a suggested size for applications to
use.

The number of threads per applet is not statically bounded. Each attached thread consumes one
bookkeeping slot from a dedicated heap for the subsystem sized by
:kconfig:option:`CONFIG_APPLET_HEAP_SIZE`, and :c:func:`applet_add_thread` returns ``-ENOMEM``
once that heap is exhausted. Under :kconfig:option:`CONFIG_USERSPACE` the thread object itself
is allocated from the kernel heap, so :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` may become
the binding limit instead.

:c:func:`applet_start` starts every attached thread that has not been started yet, so threads may
be added before or between starts. For LLEXT-backed applets the extension's ``.init_array`` runs
in supervisor mode on the first call.

All threads of an applet share the scheduling priority and CPU affinity given in
:c:struct:`applet_opts`. Priority defaults to
:kconfig:option:`CONFIG_APPLET_THREAD_PRIORITY_DEFAULT`.

Memory isolation
****************

When :kconfig:option:`CONFIG_USERSPACE` is enabled, each applet owns a :ref:`memory_domain`. For
LLEXT-backed applets the extension's TEXT, DATA, RODATA and BSS regions are added to that domain
automatically, so the applet is hardware-isolated from the rest of the system. Native applets get
an empty domain, which still lets their threads share partitions while remaining isolated.

Additional regions are granted with :c:func:`applet_add_partition`. Every thread of the applet,
current and future, can then access the partition with the access mode the caller set on it. This
is the mechanism for sharing a buffer between the threads of an applet, or for granting an applet
access to a kernel-resident region.

Threads run in unprivileged mode when ``opts.user_mode`` is set and the applet has a memory
domain. Note the asymmetry in the defaults: LLEXT-backed applets follow
:kconfig:option:`CONFIG_USERSPACE`, whereas :c:func:`applet_init` called with ``NULL`` options
forces ``user_mode`` off, because a native entry function is ordinary in-image code that usually
calls APIs which are not user-callable. Applications wanting native user mode threads must opt in
explicitly.

See :ref:`usermode_api` for the constraints that apply to unprivileged threads.

Fault handling
**************

With :kconfig:option:`CONFIG_APPLET_FATAL_HANDLER` the subsystem installs a
``k_sys_fatal_error_handler()`` that identifies the applet owning the faulting thread and reacts
according to ``opts.halt_on_fault``:

``APPLET_HALT_ON_FAULT_THREAD``
   Abort only the faulting thread.

``APPLET_HALT_ON_FAULT_APPLET``
   Abort every thread of the offending applet. This is the default.

``APPLET_HALT_ON_FAULT_SYSTEM``
   Halt the whole system.

Do not enable this option if the application already provides its own
``k_sys_fatal_error_handler()``.

Thread safety
*************

All applet API functions may be called concurrently from any number of threads, including on the
same descriptor. They are serialised by an internal mutex, so they must be called from thread
context only.

:c:func:`applet_join` releases that mutex while it waits, so a long join on one applet does not
block operations on another. :c:func:`applet_unload` waits for any in-flight join on the same
applet to return before it frees anything.

Two cases remain the caller's responsibility:

* Concurrent :c:func:`applet_init` or :c:func:`applet_load_llext` calls on the *same* descriptor.
  There is nothing to serialise against until the descriptor exists.
* The :kconfig:option:`CONFIG_APPLET_FATAL_HANDLER` path runs in fault context and cannot take the
  mutex, so it is best-effort. Faulting in an applet that another thread is unloading at the same
  time is not covered.

Example
*******

A native applet with a single thread:

.. code-block:: c

   #include <zephyr/applet/applet.h>

   #define WORKER_STACK_SIZE 2048

   APPLET_THREAD_STACK_DEFINE(worker_stack, WORKER_STACK_SIZE);

   static struct applet worker_applet;

   static void worker_entry(void *p1, void *p2, void *p3)
   {
           ARG_UNUSED(p1);
           ARG_UNUSED(p2);
           ARG_UNUSED(p3);

           printk("hello from the applet\n");
   }

   int main(void)
   {
           struct applet_opts opts = APPLET_OPTS_DEFAULT;

           if (applet_init(&worker_applet, "worker", &opts) < 0) {
                   return -1;
           }

           if (applet_add_thread(&worker_applet, worker_stack, WORKER_STACK_SIZE,
                                 worker_entry, NULL, NULL) < 0) {
                   applet_unload(&worker_applet);
                   return -1;
           }

           if (applet_start(&worker_applet) < 0) {
                   applet_unload(&worker_applet);
                   return -1;
           }

           applet_join(&worker_applet, K_FOREVER);
           applet_unload(&worker_applet);

           return 0;
   }

The :zephyr:code-sample:`applet-hello-world` sample shows the same lifecycle for both a native and
an LLEXT-backed applet.

.. toctree::
   :maxdepth: 1

   api
