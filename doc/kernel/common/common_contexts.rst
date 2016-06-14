.. _common_contexts:

Execution Context Services
##########################

Concepts
********

Every kernel execution context has an associated *type* that indicates whether
the context is a task, a fiber, or the kernel's interrupt handling context.

All task and fiber contexts have a unique *thread identifier* value used to
uniquely identify them. Each task and fiber can also support a 32-bit *thread
custom data* value. This value is accessible only by the task or fiber itself,
and may be used by the application for any purpose. The default custom data
value for a task or fiber is zero.

.. note::
   The custom data value is not available to ISRs because these operate
   only within the shared kernel interrupt handling context.

The kernel permits a task or a fiber to perform a ``busy wait``, thus delaying
its processing for a specified time period. Such a delay occurs without
requiring the kernel to perform context switching, as it typically does with
timer and timeout services.


Purpose
*******

Use kernel execution context services when writing code that should
operate differently when it is executed by different contexts.

Use the ``busy wait`` service when the required delay is too short to
warrant context switching to another task or fiber. The ``busy wait``
service may also be used when performing a delay as part of the
nanokernel's background task in a nanokernel-only system; this task is
not allowed to voluntarily relinquish the CPU.


Usage
*****

Configuring Custom Data Support
===============================

Use the :option:`CONFIG_THREAD_CUSTOM_DATA` configuration option
to enable support for thread custom data. By default, custom data
support is disabled.


Example: Performing Execution Context-Specific Processing
=========================================================

This code shows how a routine can use a thread's custom data value
to limit the number of times a thread may call the routine. Note that
counting is not performed when the routine is called by an ISR; ISRs
do not have a custom data value.

.. note::
   Obviously, only a single routine can use this technique
   since it monopolizes the use of the custom data value.

.. code-block:: c

   #define CALL_LIMIT 7

   int call_tracking_routine(void)
   {
       uint32_t call_count;

       if (sys_execution_context_type_get() != NANO_CTX_ISR) {
          call_count = (uint32_t)sys_thread_custom_data_get();
          if (call_count == CALL_LIMIT)
         return -1;
     call_count++;
     sys_thread_custom_data_set((void *)call_count);
       }

       /* do rest of routine's processing */
       ...
   }

APIs
****

The following kernel execution context APIs are common to both
:file:`microkernel.h` and :file:`nanokernel.h`:


:c:func:`sys_thread_self_get()`
   Gets thread identifier of currently executing task or fiber.

:c:func:`sys_execution_context_type_get()`
   Gets type of currently executing context (i.e. task, fiber, or ISR).

:c:func:`sys_thread_custom_data_set()`
   Writes custom data for currently executing task or fiber.

:c:func:`sys_thread_custom_data_get()`
   Reads custom data for currently executing task or fiber.
