.. _interrupts_v2:

Interrupts [TBD]
################

Concepts
********

:abbr:`ISRs (Interrupt Service Routines)` are functions
that execute in response to a hardware or software interrupt.
They are used to preempt the execution of the current thread,
allowing the response to occur with very low overhead.
Thread execution resumes only once all ISR work has been completed.

Any number of ISRs can be utilized by an application, subject to
any hardware constraints imposed by the underlying hardware.
Each ISR has the following key properties:

* The **:abbr:`IRQ (Interrupt ReQuest)` signal** that triggers the ISR.
* The **priority level** associated with the IRQ.
* The **address of the function** that is invoked to handle the interrupt.
* The **argument value** that is passed to that function.

An :abbr:`IDT (Interrupt Descriptor Table)` is used to associate
a given interrupt source with a given ISR.
Only a single ISR can be associated with a specific IRQ at any given time.

Multiple ISRs can utilize the same function to process interrupts,
allowing a single function to service a device that generates
multiple types of interrupts or to service multiple devices
(usually of the same type). The argument value passed to an ISR's function
can be used to allow the function to determine which interrupt has been
signaled.

The kernel provides a default ISR for all unused IDT entries. This ISR
generates a fatal system error if an unexpected interrupt is signaled.

The kernel supports interrupt nesting. This allows an ISR to be preempted
in mid-execution if a higher priority interrupt is signaled. The lower
priority ISR resumes execution once the higher priority ISR has completed
its processing.

The kernel allows a thread to temporarily lock out the execution
of ISRs, either individually or collectively, should the need arise.
The collective lock can be applied repeatedly; that is, the lock can
be applied when it is already in effect. The collective lock must be
unlocked an equal number of times before interrupts are again processed
by the kernel.

Examples
********

Installing an ISR
=================

It's important to note that IRQ_CONNECT() is not a C function and does
some inline assembly magic behind the scenes. All its arguments must be known
at build time. Drivers that have multiple instances may need to define
per-instance config functions to configure the interrupt for that instance.

The following code illustrates how to install an ISR:

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
       irq_enable(MY_DEV_IRQ);            /* enable IRQ */
       ...
    }

Offloading ISR Work
*******************

Interrupt service routines should generally be kept short
to ensure predictable system operation.
In situations where time consuming processing is required
an ISR can quickly restore the kernel's ability to respond
to other interrupts by offloading some or all of the interrupt-related
processing work to a thread.

The kernel provides a variety of mechanisms to allow an ISR to offload work
to a thread.

1. An ISR can signal a helper thread to do interrupt-related work
   using a kernel object, such as a fifo, lifo, or semaphore.

2. An ISR can signal the kernel's system workqueue to do interrupt-related
   work by sending an event that has an associated event handler.

When an ISR offloads work to a thread there is typically a single
context switch to that thread when the ISR completes.
Thus, interrupt-related processing usually continues almost immediately.
Additional intermediate context switches may be required
to execute a currently executing cooperative thread
or any higher-priority threads that are ready to run.

Suggested Uses
**************

Use an ISR to perform interrupt processing that requires a very rapid
response, and which can be done quickly and without blocking.

.. note::
    Interrupt processing that is time consuming, or which involves blocking,
    should be handed off to a thread. See `Offloading ISR Work`_ for
    a description of various techniques that can be used in an application.

Configuration Options
*********************

[TBD]

APIs
****

These are the interrupt-related Application Program Interfaces.

* :cpp:func:`irq_enable()`
* :cpp:func:`irq_disable()`
* :cpp:func:`irq_lock()`
* :cpp:func:`irq_unlock()`
* :cpp:func:`k_is_in_isr()`
