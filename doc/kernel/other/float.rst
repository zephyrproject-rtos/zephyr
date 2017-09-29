.. _float_v2:

Floating Point Services
#######################

The kernel allows threads to use floating point registers on board
configurations that support these registers.

.. note::
    Floating point services are currently available only for boards
    based on the ARM Cortex-M4 or the Intel x86 architectures. The
    services provided are architecture specific.

    The kernel does not support the use of floating point registers by ISRs.

.. contents::
    :local:
    :depth: 2

Concepts
********

The kernel can be configured to provide only the floating point services
required by an application. Three modes of operation are supported,
which are described below. In addition, the kernel's support for the SSE
registers can be included or omitted, as desired.

No FP registers mode
====================

This mode is used when the application has no threads that use floating point
registers. It is the kernel's default floating point services mode.

If a thread uses any floating point register,
the kernel generates a fatal error condition and aborts the thread.

Unshared FP registers mode
==========================

This mode is used when the application has only a single thread
that uses floating point registers.

The kernel initializes the floating point registers so they can be used
by any thread. The floating point registers are left unchanged
whenever a context switch occurs.

.. note::
    Incorrect operation may result if two or more threads use
    floating point registers, as the kernel does not attempt to detect
    (or prevent) multiple threads from using these registers.

Shared FP registers mode
========================

This mode is used when the application has two or more threads that use
floating point registers. Depending upon the underlying CPU architecture,
the kernel supports one or more of the following thread sub-classes:

* non-user: A thread that cannot use any floating point registers

* FPU user: A thread that can use the standard floating point registers

* SSE user: A thread that can use both the standard floating point registers
  and SSE registers

The kernel initializes the floating point registers so they can be used
by any thread, then saves and restores these registers during
context switches to ensure the computations performed by each FPU user
or SSE user are not impacted by the computations performed by the other users.

On the ARM Cortex-M4 architecture the kernel treats *all* threads
as FPU users when shared FP registers mode is enabled. This means that the
floating point registers are saved and restored during a context switch, even
when the associated threads are not using them. Each thread must provide
an extra 132 bytes of stack space where these register values can be saved.

On the x86 architecture the kernel treats each thread as a non-user,
FPU user or SSE user on a case-by-case basis. A "lazy save" algorithm is used
during context switching which updates the floating point registers only when
it is absolutely necessary. For example, the registers are *not* saved when
switching from an FPU user to a non-user thread, and then back to the original
FPU user. The following table indicates the amount of additional stack space a
thread must provide so the registers can be saved properly.

=========== =============== ==========================
Thread type FP register use Extra stack space required
=========== =============== ==========================
cooperative any             0 bytes
preemptive  none            0 bytes
preemptive  FPU             108 bytes
preemptive  SSE             464 bytes
=========== =============== ==========================

The x86 kernel automatically detects that a given thread is using
the floating point registers the first time the thread accesses them.
The thread is tagged as an SSE user if the kernel has been configured
to support the SSE registers, or as an FPU user if the SSE registers are
not supported. If this would result in a thread that is an FPU user being
tagged as an SSE user, or if the application wants to avoid the exception
handling overhead involved in auto-tagging threads, it is possible to
pretag a thread using one of the techniques listed below.

* A statically-created x86 thread can be pretagged by passing the
  :c:macro:`K_FP_REGS` or :c:macro:`K_SSE_REGS` option to
  :c:macro:`K_THREAD_DEFINE`.

* A dynamically-created x86 thread can be pretagged by passing the
  :c:macro:`K_FP_REGS` or :c:macro:`K_SSE_REGS` option to
  :cpp:func:`k_thread_create()`.

* An already-created x86 thread can pretag itself once it has started
  by passing the :c:macro:`K_FP_REGS` or :c:macro:`K_SSE_REGS` option to
  :cpp:func:`k_float_enable()`.

If an x86 thread uses the floating point registers infrequently it can call
:cpp:func:`k_float_disable()` to remove its tagging as an FPU user or SSE user.
This eliminates the need for the kernel to take steps to preserve
the contents of the floating point registers during context switches
when there is no need to do so.
When the thread again needs to use the floating point registers it can re-tag
itself as an FPU user or SSE user by calling :cpp:func:`k_float_enable()`.

Implementation
**************

Performing Floating Point Arithmetic
====================================

No special coding is required for a thread to use floating point arithmetic
if the kernel is properly configured.

The following code shows how a routine can use floating point arithmetic
to avoid overflow issues when computing the average of a series of integer
values.

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

Suggested Uses
**************

Use the kernel floating point services when an application needs to
perform floating point operations.

Configuration Options
*********************

To configure unshared FP registers mode, enable the :option:`CONFIG_FLOAT`
configuration option and leave the :option:`CONFIG_FP_SHARING` configuration
option disabled.

To configure shared FP registers mode, enable both the :option:`CONFIG_FLOAT`
configuration option and the :option:`CONFIG_FP_SHARING` configuration option.
Also, ensure that any thread that uses the floating point registers has
sufficient added stack space for saving floating point register values
during context switches, as described above.

Use the :option:`CONFIG_SSE` configuration option to enable support for
SSEx instructions (x86 only).

APIs
****

The following floating point APIs (x86 only) are provided by :file:`kernel.h`:

* :cpp:func:`k_float_enable()`
* :cpp:func:`k_float_disable()`
