.. _interrupts_v2:

Interrupts
##########

An :dfn:`interrupt service routine` (ISR) is a function that executes
asychronously in response to a hardware or software interrupt.
An ISR normally preempts the execution of the current thread,
allowing the response to occur with very low overhead.
Thread execution resumes only once all ISR work has been completed.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of ISRs can be defined, subject to the constraints imposed
by underlying hardware.

An ISR has the following key properties:

* An **interrupt request (IRQ) signal** that triggers the ISR.
* A **priority level** associated with the IRQ.
* An **interrupt handler function** that is invoked to handle the interrupt.
* An **argument value** that is passed to that function.

An :abbr:`IDT (Interrupt Descriptor Table)` or a vector table is used
to associate a given interrupt source with a given ISR.
Only a single ISR can be associated with a specific IRQ at any given time.

Multiple ISRs can utilize the same function to process interrupts,
allowing a single function to service a device that generates
multiple types of interrupts or to service multiple devices
(usually of the same type). The argument value passed to an ISR's function
allows the function to determine which interrupt has been signaled.

The kernel provides a default ISR for all unused IDT entries. This ISR
generates a fatal system error if an unexpected interrupt is signaled.

The kernel supports **interrupt nesting**. This allows an ISR to be preempted
in mid-execution if a higher priority interrupt is signaled. The lower
priority ISR resumes execution once the higher priority ISR has completed
its processing.

An ISR's interrupt handler function executes in the kernel's **interrupt
context**. This context has its own dedicated stack area (or, on some
architectures, stack areas). The size of the interrupt context stack must be
capable of handling the execution of multiple concurrent ISRs if interrupt
nesting support is enabled.

.. important::
    Many kernel APIs can be used only by threads, and not by ISRs. In cases
    where a routine may be invoked by both threads and ISRs the kernel
    provides the :cpp:func:`k_is_in_isr()` API to allow the routine to
    alter its behavior depending on whether it is executing as part of
    a thread or as part of an ISR.

Preventing Interruptions
========================

In certain situations it may be necessary for the current thread to
prevent ISRs from executing while it is performing time-sensitive
or critical section operations.

A thread may temporarily prevent all IRQ handling in the system using
an **IRQ lock**. This lock can be applied even when it is already in effect,
so routines can use it without having to know if it is already in effect.
The thread must unlock its IRQ lock the same number of times it was locked
before interrupts can be once again processed by the kernel while the thread
is running.

.. important::
    The IRQ lock is thread-specific. If thread A locks out interrupts
    then performs an operation that allows thread B to run
    (e.g. giving a semaphore or sleeping for N milliseconds), the thread's
    IRQ lock no longer applies once thread A is swapped out. This means
    that interrupts can be processed while thread B is running unless
    thread B has also locked out interrupts using its own IRQ lock.
    (Whether interrupts can be processed while the kernel is switching
    between two threads that are using the IRQ lock is architecture-specific.)

    When thread A eventually becomes the current thread once again, the kernel
    re-establishes thread A's IRQ lock. This ensures thread A won't be
    interrupted until it has explicitly unlocked its IRQ lock.

Alternatively, a thread may temporarily **disable** a specified IRQ
so its associated ISR does not execute when the IRQ is signalled.
The IRQ must be subsequently **enabled** to permit the ISR to execute.

.. important::
    Disabling an IRQ prevents *all* threads in the system from being preempted
    by the associated ISR, not just the thread that disabled the IRQ.

Offloading ISR Work
===================

An ISR should execute quickly to ensure predictable system operation.
If time consuming processing is required the ISR should offload some or all
processing to a thread, thereby restoring the kernel's ability to respond
to other interrupts.

The kernel supports several mechanisms for offloading interrupt-related
processing to a thread.

* An ISR can signal a helper thread to do interrupt-related processing
  using a kernel object, such as a fifo, lifo, or semaphore.

* An ISR can signal an alert which causes the system workqueue thread
  to execute an associated alert handler function.
  (See :ref:`alerts_v2`.)

* An ISR can instruct the system workqueue thread to execute a work item.
  (See TBD.)

When an ISR offloads work to a thread, there is typically a single context
switch to that thread when the ISR completes, allowing interrupt-related
processing to continue almost immediately. However, depending on the
priority of the thread handling the offload, it is possible that
the currently executing cooperative thread or other higher-priority threads
may execute before the thread handling the offload is scheduled.

Implementation
**************

Defining an ISR
===============

An ISR is defined at run-time by calling :c:macro:`IRQ_CONNECT()`. It must
then be enabled by calling :cpp:func:`irq_enable()`.

.. important::
    IRQ_CONNECT() is not a C function and does some inline assembly magic
    behind the scenes. All its arguments must be known at build time.
    Drivers that have multiple instances may need to define per-instance
    config functions to configure each instance of the interrupt.

The following code defines and enables an ISR.

.. code-block:: c

    #define MY_DEV_IRQ  24       /* device uses IRQ 24 */
    #define MY_DEV_PRIO  2       /* device uses interrupt priority 2 */
    /* argument passed to my_isr(), in this case a pointer to the device */
    #define MY_ISR_ARG  DEVICE_GET(my_device)
    #define MY_IRQ_FLAGS 0       /* IRQ flags. Unused on non-x86 */

    void my_isr(void *arg)
    {
       ... /* ISR code */
    }

    void my_isr_installer(void)
    {
       ...
       IRQ_CONNECT(MY_DEV_IRQ, MY_DEV_PRIO, my_isr, MY_ISR_ARG, MY_IRQ_FLAGS);
       irq_enable(MY_DEV_IRQ);
       ...
    }

Suggested Uses
**************

Use an ISR to perform interrupt processing that requires a very rapid
response, and can be done quickly without blocking.

.. note::
    Interrupt processing that is time consuming, or involves blocking,
    should be handed off to a thread. See `Offloading ISR Work`_ for
    a description of various techniques that can be used in an application.

Configuration Options
*********************

Related configuration options:

* :option:`CONFIG_ISR_STACK_SIZE`

Additional architecture-specific and device-specific configuration options
also exist.

APIs
****

The following interrupt-related APIs are provided by :file:`irq.h`:

* :c:macro:`IRQ_CONNECT()`
* :cpp:func:`irq_lock()`
* :cpp:func:`irq_unlock()`
* :cpp:func:`irq_enable()`
* :cpp:func:`irq_disable()`
* :cpp:func:`irq_is_enabled()`

The following interrupt-related APIs are provided by :file:`kernel.h`:

* :cpp:func:`k_is_in_isr()`
