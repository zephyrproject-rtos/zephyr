.. _api_terms:

API Terminology
###############

The following terms may be used as shorthand API tags to indicate the
allowed calling context (thread, ISR, pre-kernel), the effect of a call
on the current thread state, and other behavioral characteristics.

:ref:`api_term_reschedule`
   if executing the function reaches a reschedule point
:ref:`api_term_sleep`
   if executing the function can cause the invoking thread to sleep
:ref:`api_term_no-wait`
   if a parameter to the function can prevent the invoking thread from
   trying to sleep
:ref:`api_term_isr-ok`
   if the function can be safely called and will have its specified
   effect whether invoked from interrupt or thread context
:ref:`api_term_pre-kernel-ok`
   if the function can be safely called before the kernel has been fully
   initialized and will have its specified effect when invoked from that
   context.
:ref:`api_term_async`
   if the function may return before the operation it initializes is
   complete (i.e. function return and operation completion are
   asynchronous)
:ref:`api_term_supervisor`
   if the calling thread must have supervisor privileges to execute the
   function

Details on the behavioral impact of each attribute are in the following
sections.

.. _api_term_reschedule:

reschedule
==========

The reschedule attribute is used on a function that can reach a
:ref:`reschedule point <scheduling_v2>` within its execution.

Details
-------

The significance of this attribute is that when a rescheduling function
is invoked by a thread it is possible for that thread to be suspended as
a consequence of a higher-priority thread being made ready.  Whether the
suspension actually occurs depends on the operation associated with the
reschedule point and the relative priorities of the invoking thread and
the head of the ready queue.

Note that in the case of timeslicing, or reschedule points executed from
interrupts, any thread may be suspended in any function.

Functions that are not **reschedule** may be invoked from either thread
or interrupt context.

Functions that are **reschedule** may be invoked from thread context.

Functions that are **reschedule** but not **sleep** may be invoked from
interrupt context.

.. _api_term_sleep:

sleep
=====

The sleep attribute is used on a function that can cause the invoking
thread to :ref:`sleep <scheduling_v2>`.

Explanation
-----------

This attribute is of relevance specifically when considering
applications that use only non-preemptible threads, because the kernel
will not replace a running cooperative-only thread at a reschedule point
unless that thread has explicitly invoked an operation that caused it to
sleep.

This attribute does not imply the function will sleep unconditionally,
but that the operation may require an invoking thread that would have to
suspend, wait, or invoke :c:func:`k_yield` before it can complete
its operation.  This behavior may be mediated by **no-wait**.

Functions that are **sleep** are implicitly **reschedule**.

Functions that are **sleep** may be invoked from thread context.

Functions that are **sleep** may be invoked from interrupt and
pre-kernel contexts if and only if invoked in **no-wait** mode.

.. _api_term_no-wait:

no-wait
=======

The no-wait attribute is used on a function that is also **sleep** to
indicate that a parameter to the function can force an execution path
that will not cause the invoking thread to sleep.

Explanation
-----------

The paradigmatic case of a no-wait function is a function that takes a
timeout, to which :c:macro:`K_NO_WAIT` can be passed.  The semantics of
this special timeout value are to execute the function's operation as
long as it can be completed immediately, and to return an error code
rather than sleep if it cannot.

It is use of the no-wait feature that allows functions like
:c:func:`k_sem_take` to be invoked from ISRs, since it is not
permitted to sleep in interrupt context.

A function with a no-wait path does not imply that taking that path
guarantees the function is synchronous.

Functions with this attribute may be invoked from interrupt and
pre-kernel contexts only when the parameter selects the no-wait path.

.. _api_term_isr-ok:

isr-ok
======

The isr-ok attribute is used on a function to indicate that it works
whether it is being invoked from interrupt or thread context.

Explanation
-----------

Any function that is not **sleep** is inherently **isr-ok**.  Functions
that are **sleep** are **isr-ok** if the implementation ensures that the
documented behavior is implemented even if called from an interrupt
context.  This may be achieved by having the implementation detect the
calling context and transfer the operation that would sleep to a thread,
or by documenting that when invoked from a non-thread context the
function will return a specific error (generally ``-EWOULDBLOCK``).

Note that a function that is **no-wait** is safe to call from interrupt
context only when the no-wait path is selected.  **isr-ok** functions
need not provide a no-wait path.

.. _api_term_pre-kernel-ok:

pre-kernel-ok
=============

The pre-kernel-ok attribute is used on a function to indicate that it
works as documented even when invoked before the kernel main thread has
been started.

Explanation
-----------

This attribute is similar to **isr-ok** in function, but is intended for
use by any API that is expected to be called in :c:macro:`DEVICE_DEFINE()`
or :c:macro:`SYS_INIT()` calls that may be invoked with ``PRE_KERNEL_1``
or ``PRE_KERNEL_2`` initialization levels.

Generally a function that is **pre-kernel-ok** checks
:c:func:`k_is_pre_kernel` when determining whether it can fulfill its
required behavior.  In many cases it would also check
:c:func:`k_is_in_isr` so it can be **isr-ok** as well.

.. _api_term_async:

async
=====

A function is **async** (i.e. asynchronous) if it may return before the
operation it initiates has completed.  An asynchronous function will
generally provide a mechanism by which operation completion is reported,
e.g. a callback or event.

A function that is not asynchronous is synchronous, i.e. the operation
will always be complete when the function returns.  As most functions
are synchronous this behavior does not have a distinct attribute to
identify it.

Explanation
-----------

Be aware that **async** is orthogonal to context-switching.  Some APIs
may provide completion information through a callback, but may suspend
while waiting for the resource necessary to initiate the operation; an
example is :c:func:`spi_transceive_async`.

If a function is both **no-wait** and **async** then selecting the
no-wait path only guarantees that the function will not sleep.  It does
not affect whether the operation will be completed before the function
returns.

.. _api_term_supervisor:

supervisor
==========

The supervisor attribute is relevant only in user-mode applications, and
indicates that the function cannot be invoked from user mode.
