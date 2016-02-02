.. _common_float:

Floating Point Services
#######################

.. note::
   Floating point services are currently available only for platforms
   based on the Intel x86 architecture.

Concepts
********

The kernel allows an application's tasks and fibers to use floating point
registers on board configurations that support these registers.
Threads that use the x87 FPU/MMX registers are known as "FPU users",
while threads that use SSE registers are known as "SSE users".

.. note::
   The kernel does not support the use of floating point registers by ISRs.

The kernel can be configured to provide only the floating point services
required by an application. Three modes of operation are supported,
which are described below. In addition, the kernel's support for the SSE
registers can be included or omitted, as desired.

No FP registers mode
====================

This mode is used when the application has no tasks or fibers that use
floating point registers. It is the kernel's default floating point services
mode.

If a task or fiber uses any floating point register,
the kernel generates a fatal error condition and aborts the thread.

Unshared FP registers mode
==========================

This mode is used when the application has only a single task or fiber
that uses floating point registers.

The kernel initializes the floating point registers so they can be used
by any task or fiber. The floating point registers are left unchanged
whenever a context switch occurs.

.. note::
   Incorrect operation may result if two or more tasks or fibers use
   floating point registers, as the kernel does not attempt to detect
   (or prevent) multiple threads from using these registers.

Shared FP registers mode
========================

This mode is used when the application has two or more tasks or fibers
that use floating point registers.

The kernel initializes the floating point registers so they can be used
by any task or fiber, then saves and restores these registers during
context switches to ensure the computations performed by each FPU user
or SSE user are not impacted by the computations performed by the other users.
A "lazy save" algorithm is used during context switching which updates the
floating point registers only when it is absolutely necessary---for example,
the registers are *not* saved when switching from an FPU user to a thread
that does not use the floating point registers, and then switching back
to the original FPU user.

Every task that uses the floating point registers must provide stack space
where the kernel can save the registers during context switches. An FPU user
must provide 108 bytes of added stack space, above and beyond its normal
requirements; an SSE user must provide 464 bytes of added stack space.

.. note::
   A task that does *not* use the floating point registers does not need
   to provide any added stack space. A fiber does *not* need to provide any
   added stack space, regardless of whether or not it uses the floating
   point registers.

The kernel automatically detects that a given task or fiber is using
the floating point registers the first time the thread accesses them.
The thread is tagged as an SSE user if the kernel has been configured
to support the SSE registers, or as an FPU user if the SSE registers are
not supported. If this would result in a thread that is an FPU user being
tagged as an SSE user, or if the application wants to avoid the exception
handling overhead involved in auto-tagging threads, it is possible to
pre-tag a thread using one of the techniques listed below.

* A task or fiber can tag itself as an FPU user or SSE user by calling
  :c:func:`task_float_enable()` or :c:func:`fiber_float_enable()`
  once it has started executing.

* A fiber can be tagged as an FPU user or SSE user by its creator
  when the fiber is started.

* A microkernel task can be tagged as an FPU user or SSE user by adding it
  to the :c:macro:`FPU` task group or the :c:macro:`SSE` task group
  when the task is defined.

.. note::
   Adding the task to the :c:macro:`FPU` or :c:macro:`SSE` task groups
   by calling :c:func:`task_group_join()` does *not* tag the task
   as an FPU user or SSE user.

If a task or fiber uses the floating point registers infrequently
it can call :c:func:`task_float_disable()` or :c:func:`fiber_float_disable()`
to remove its tagging as an FPU user or SSE user. This eliminates the need
for the kernel to take steps to preserve the contents of the floating point
registers during context switches when there is no need to do so.
When the thread again needs to use the floating point registers it can re-tag
itself as an FPU user or SSE user using one of the techniques listed above.


Purpose
*******

Use the kernel floating point services when an application needs to
perform floating point operations.


Usage
*****

Configuring Floating Point Services
===================================

To configure unshared FP registers mode, enable the :option:`FLOAT`
configuration option and leave the :option:`FP_SHARING` configuration option
disabled.

To configure shared FP registers mode, enable both the :option:`FLOAT`
configuration option and the :option:`FP_SHARING` configuration option.
Also, ensure that any task that uses the floating point registers has
sufficient added stack space for saving floating point register values
during context switches, as described above.

Use the :option:`SSE` configuration option to enable support for
SSEx instructions.


Example: Performing Floating Point Arithmetic
=============================================
This code shows how a routine can use floating point arithmetic to avoid
overflow issues when computing the average of a series of integer values.
Note that no special coding is required if the kernel is properly configured.

.. code-block:: c

   int average(int *values, int num_values)
   {
       double sum;
       int i;

       sum = 0.0;

       for (i = 0; i < num_values; i++) {
           sum += *values;
           values++;
       }

       return (int)((sum / num_values) + 0.5);
   }

APIs
****

The following floating point services APIs are provided by
:file:`microkernel.h` and by :file:`nanokernel.h`:

:c:func:`fiber_float_enable()`
   Tells the kernel that the specified task or fiber is now an FPU user
   or SSE user.

:c:func:`task_float_enable()`
   Tells the kernel that the specified task or fiber is now an FPU user
   or SSE user.

:c:func:`fiber_float_disable()`
   Tells the kernel that the specified task or fiber is no longer an FPU user
   or SSE user.

:c:func:`task_float_disable()`
   Tells the kernel that the specified task or fiber is no longer an FPU user
   or SSE user.
