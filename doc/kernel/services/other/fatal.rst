.. _fatal:

Fatal Errors
############

Software Errors Triggered in Source Code
****************************************

Zephyr provides several methods for inducing fatal error conditions through
either build-time checks, conditionally compiled assertions, or deliberately
invoked panic or oops conditions.

Failed runtime assertions (``ZASSERT()`` and the legacy ``__ASSERT()``) induce
a fatal error. See :ref:`assert` for details on the assertion APIs and how
their failure behavior is configured.

Kernel Oops
===========

A kernel oops is a software triggered fatal error invoked by
:c:func:`k_oops()`.  This should be used to indicate an unrecoverable condition
in application logic.

The fatal error reason code generated will be ``K_ERR_KERNEL_OOPS``.

Kernel Panic
============

A kernel error is a software triggered fatal error invoked by
:c:func:`k_panic()`.  This should be used to indicate that the Zephyr kernel is
in an unrecoverable state. Implementations of
:c:func:`k_sys_fatal_error_handler()` should not return if the kernel
encounters a panic condition, as the entire system needs to be reset.

Threads running in user mode are not permitted to invoke :c:func:`k_panic()`,
and doing so will generate a kernel oops instead. Otherwise, the fatal error
reason code generated will be ``K_ERR_KERNEL_PANIC``.

Exceptions
**********

Spurious Interrupts
===================

If the CPU receives a hardware interrupt on an interrupt line that has not had
a handler installed with ``IRQ_CONNECT()`` or :c:func:`irq_connect_dynamic()`,
then the kernel will generate a fatal error with the reason code
``K_ERR_SPURIOUS_IRQ()``.

Stack Overflows
===============

In the event that a thread pushes more data onto its execution stack than its
stack buffer provides, the kernel may be able to detect this situation and
generate a fatal error with a reason code of ``K_ERR_STACK_CHK_FAIL``.

If a thread is running in user mode, then stack overflows are always caught,
as the thread will simply not have permission to write to adjacent memory
addresses outside of the stack buffer. Because this is enforced by the
memory protection hardware, there is no risk of data corruption to memory
that the thread would not otherwise be able to write to.

If a thread is running in supervisor mode, or if :kconfig:option:`CONFIG_USERSPACE` is
not enabled, depending on configuration stack overflows may or may not be
caught.  :kconfig:option:`CONFIG_HW_STACK_PROTECTION` is supported on some
architectures and will catch stack overflows in supervisor mode, including
when handling a system call on behalf of a user thread. Typically this is
implemented via dedicated CPU features, or read-only MMU/MPU guard regions
placed immediately adjacent to the stack buffer. Stack overflows caught in this
way can detect the overflow, but cannot guarantee against data corruption and
should be treated as a very serious condition impacting the health of the
entire system.

If a platform lacks memory management hardware support,
:kconfig:option:`CONFIG_STACK_SENTINEL` is a software-only stack overflow detection
feature which periodically checks if a sentinel value at the end of the stack
buffer has been corrupted. It does not require hardware support, but provides
no protection against data corruption. Since the checks are typically done at
interrupt exit, the overflow may be detected a nontrivial amount of time after
the stack actually overflowed.

Finally, Zephyr supports GCC compiler stack canaries via
:kconfig:option:`CONFIG_STACK_CANARIES`.  If enabled, the compiler will insert a canary
value randomly generated at boot into function stack frames, checking that the
canary has not been overwritten at function exit. If the check fails, the
compiler invokes :c:func:`__stack_chk_fail()`, whose Zephyr implementation
invokes a fatal stack overflow error. An error in this case does not indicate
that the entire stack buffer has overflowed, but instead that the current
function stack frame has been corrupted. See the compiler documentation for
more details.

By default a single canary value, generated at boot, is shared by all threads.
When :kconfig:option:`CONFIG_STACK_CANARIES_TLS` is enabled, the canary is
instead stored in :ref:`thread-local storage <thread_local_storage>` so that
each thread has its own value, making the canary location and value harder to
predict at the cost of additional per-thread setup.

As a complementary hardening measure, :kconfig:option:`CONFIG_STACK_POINTER_RANDOM`
applies a randomized offset to each thread's initial stack pointer when the
thread is created. This is a limited form of address space layout randomization
that makes the location of any given stack frame non-deterministic, hindering
some classes of security attack, at the cost of consuming up to the configured
number of bytes from each thread's stack region.

Other Exceptions
================

Any other type of unhandled CPU exception will generate an error code of
``K_ERR_CPU_EXCEPTION``.

Fatal Error Handling
********************

The policy for what to do when encountering a fatal error is determined by the
implementation of the :c:func:`k_sys_fatal_error_handler()` function.  This
function has a default implementation with weak linkage that calls
``LOG_PANIC()`` to dump all pending logging messages and then unconditionally
halts the system with :c:func:`k_fatal_halt()`.

Applications are free to implement their own error handling policy by
overriding the implementation of :c:func:`k_sys_fatal_error_handler()`.
If the implementation returns, the faulting thread will be aborted and
the system will otherwise continue to function. See the documentation for
this function for additional details and constraints.

API Reference
*************

.. doxygengroup:: fatal_apis
