.. _context_services:

Execution Context Services
##########################

Concepts
********

Each kernel execution context has an associated *type*, which indicates whether
the context is a task, a fiber, or the kernel's interrupt handling context.
Task and fiber contexts also have an associated *thread identifier* value,
which is used to uniquely identify these threads.

Each task and fiber may also support a 32-bit *thread custom data* value.
This value is accessible only by the task or fiber itself, and can be used
by the application for any purpose. The default custom data value for a
task or fiber is zero.

.. note::
   The custom data value is not available to ISRs, which operate in the shared
   kernel interrupt handling context.


Purpose
*******

Use the kernel execution context services when writing code that needs to
operate differently when executed by different contexts.


Usage
*****

Configuring Custom Data Support
===============================

Use the :option:`THREAD_CUSTOM_DATA` configuration option
to enable support for thread custom data. By default, custom data
support is disabled.


Example: Performing Execution Context-Specific Processing
=========================================================
This code shows how a routine can use a thread's custom data value
to limit the number of times a thread may call the routine.
Counting is not performed when the routine is called by an ISR, which does not
have a custom data value.

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

The following kernel execution context APIs are provided by
:file:`microkernel.h` and by :file:`nanokernel.h`:

+--------------------------------------------+---------------------------------------+
| Call                                       | Description                           |
+============================================+=======================================+
| :c:func:`sys_thread_self_get()`            | Gets thread identifier of currently   |
|                                            | executing task or fiber.              |
+--------------------------------------------+---------------------------------------+
| :c:func:`sys_execution_context_type_get()` | Gets type of currently executing      |
|                                            | context (i.e. task, fiber, or ISR).   |
+--------------------------------------------+---------------------------------------+
| :c:func:`sys_thread_custom_data_set()`     | Writes custom data for currently      |
|                                            | executing task or fiber.              |
+--------------------------------------------+---------------------------------------+
| :c:func:`sys_thread_custom_data_get()`     | Reads custom data for currently       |
|                                            | executing task or fiber.              |
+--------------------------------------------+---------------------------------------+
