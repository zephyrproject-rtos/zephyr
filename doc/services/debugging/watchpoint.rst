.. _memory_watchpoints:

Memory watchpoints
##################

A memory watchpoint asks supported hardware to report accesses to a memory range.
Unlike software instrumentation, it can identify the instruction that first reads
or writes an address without changing every load and store in the application.

Enable :kconfig:option:`CONFIG_WATCHPOINT` to use the API. The selected
architecture must provide a hardware watchpoint backend.

Defining and arming a watchpoint
********************************

A watchpoint descriptor contains the monitored address, range size, access type,
callback, and callback argument. It can be defined statically:

.. code-block:: c

   static volatile uint32_t monitored_value;

   static void watchpoint_hit(const struct k_watchpoint *wp,
                              const struct k_watchpoint_event *event,
                              void *arg)
   {
           ARG_UNUSED(wp);
           ARG_UNUSED(arg);

           captured_pc = event->pc;
           atomic_inc(&hit_count);
   }

   static K_WATCHPOINT_DEFINE(wp, (void *)&monitored_value,
                              sizeof(monitored_value),
                              K_WATCHPOINT_WRITE,
                              watchpoint_hit, NULL);

Arm and disarm it from a supervisor thread with interrupts enabled:

.. code-block:: c

   int ret = k_watchpoint_add(&wp);

   if (ret == 0) {
           monitored_value++;
           ret = k_watchpoint_remove(&wp);
   }

A successful add has installed the watchpoint on every online CPU. A successful
remove has removed it from every online CPU and waited for callbacks already in
progress. Calling remove on a disarmed descriptor is supported.

The descriptor, callback argument, and monitored object must remain valid while
the watchpoint is active or changing state. Public descriptor fields can be
changed after a successful remove. They can also be changed after an automatic
one-shot event has completed and :c:func:`k_watchpoint_is_active` returns false.
Applications must serialize field updates against concurrent lifecycle calls.

Callback context
****************

The callback runs synchronously in the exception context that performed the
access. It can run concurrently on multiple CPUs and must be short, bounded, and
non-blocking. A callback should copy the required event fields into preallocated
storage and notify thread context with an atomic or another exception-safe
mechanism.

Do not call :c:func:`k_watchpoint_add` or :c:func:`k_watchpoint_remove` from
the callback. Do not sleep, allocate from a blocking heap, or perform unbounded
logging there.

The event reports the architecture program counter, the access type and timing,
and an access address when the hardware provides one. Address zero is valid, so
test ``access_addr_valid`` instead of testing ``access_addr`` against
``NULL``.

Trigger timing and re-arming
****************************

Hardware can report an event before or after the monitored instruction retires.
Inspect :c:member:`k_watchpoint_event.timing` before interpreting the reported
program counter or the memory value:

* ``K_WATCHPOINT_TIMING_BEFORE`` means the access may not have completed.
* ``K_WATCHPOINT_TIMING_AFTER`` means the access has retired.
* ``K_WATCHPOINT_TIMING_UNKNOWN`` means the backend cannot determine the timing.

A backend can deactivate a watchpoint when it cannot safely resume the access
while preserving continuous monitoring. It then sets
:c:member:`k_watchpoint_event.rearm_required`. The RISC-V backend treats every
event whose timing is not known to be after as one-shot. Defer re-arming to
thread context after the callback has returned and
:c:func:`k_watchpoint_is_active` reports false.

Call stacks
***********

Enable :kconfig:option:`CONFIG_WATCHPOINT_CALLSTACK` to capture a bounded call
stack for each event. The architecture must support stack walking. The maximum
depth is selected with :kconfig:option:`CONFIG_WATCHPOINT_CALLSTACK_DEPTH`.

The call-stack array is valid only during the callback. Copy any entries that
must be symbolized or printed later.

Hardware limitations
********************

Hardware debug resources are limited and architecture-specific:

* Only a small number of watchpoints can normally be active at once.
* Supported range sizes and alignment constraints depend on the backend.
* The requested range is never silently widened. An unrepresentable request
  returns ``-ENOTSUP``.
* An external debugger can use the same hardware resources. The backend
  preserves resources it can identify as foreign, but it is not a transparent
  debugger-resource multiplexer.
* CPU hotplug and delayed SMP boot are not supported by the current framework.

See the :zephyr:code-sample:`watchpoint` sample for a complete minimal
application. The implementation design and architecture details are documented
in :zephyr_file:`subsys/debug/debugpoint/README.md`.

API reference
*************

.. doxygengroup:: watchpoint_apis
