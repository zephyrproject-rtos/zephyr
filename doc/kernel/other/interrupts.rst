.. _interrupts_v2:

Interrupts
##########

An :dfn:`interrupt service routine` (ISR) is a function that executes
asynchronously in response to a hardware or software interrupt.
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

Multi-level Interrupt handling
==============================

A hardware platform can support more interrupt lines than natively-provided
through the use of one or more nested interrupt controllers.  Sources of
hardware interrupts are combined into one line that is then routed to
the parent controller.

If nested interrupt controllers are supported, :option:`CONFIG_MULTI_LEVEL_INTERRUPTS`
should be set to 1, and :option:`CONFIG_2ND_LEVEL_INTERRUPTS` and
:option:`CONFIG_3RD_LEVEL_INTERRUPTS` configured as well, based on the
hardware architecture.

A unique 32-bit interrupt number is assigned with information
embedded in it to select and invoke the correct Interrupt
Service Routine (ISR). Each interrupt level is given a byte within this 32-bit
number, providing support for up to four interrupt levels using this arch, as
illustrated and explained below:

.. code-block:: none

                 9             2   0
           _ _ _ _ _ _ _ _ _ _ _ _ _         (LEVEL 1)
         5       |         A   |
       _ _ _ _ _ _ _         _ _ _ _ _ _ _   (LEVEL 2)
         |   C                       B
       _ _ _ _ _ _ _                         (LEVEL 3)
               D

There are three interrupt levels shown here.

* '-' means interrupt line and is numbered from 0 (right most).
* LEVEL 1 has 12 interrupt lines, with two lines (2 and 9) connected
  to nested controllers and one device 'A' on line 4.
* One of the LEVEL 2 controllers has interrupt line 5 connected to
  a LEVEL 3 nested controller and one device 'C' on line 3.
* The other LEVEL 2 controller has no nested controllers but has one
  device 'B' on line 2.
* The LEVEL 3 controller has one device 'D' on line 2.

Here's how unique interrupt numbers are generated for each
hardware interrupt.  Let's consider four interrupts shown above
as A, B, C, and D:

.. code-block:: none

   A -> 0x00000004
   B -> 0x00000302
   C -> 0x00000409
   D -> 0x00030609

.. note::
   The bit positions for LEVEL 2 and onward are offset by 1, as 0 means that
   interrupt number is not present for that level. For our example, the LEVEL 3
   controller has device D on line 2, connected to the LEVEL 2 controller's line
   5, that is connected to the LEVEL 1 controller's line 9 (2 -> 5 -> 9).
   Because of the encoding offset for LEVEL 2 and onward, device D is given the
   number 0x00030609.

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
so its associated ISR does not execute when the IRQ is signaled.
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

Defining a regular ISR
======================

An ISR is defined at runtime by calling :c:macro:`IRQ_CONNECT`. It must
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

Defining a 'direct' ISR
=======================

Regular Zephyr interrupts introduce some overhead which may be unacceptable
for some low-latency use-cases. Specifically:

* The argument to the ISR is retrieved and passed to the ISR

* If power management is enabled and the system was idle, all the hardware
  will be resumed from low-power state before the ISR is executed, which can be
  very time-consuming

* Although some architectures will do this in hardware, other architectures
  need to switch to the interrupt stack in code

* After the interrupt is serviced, the OS then performs some logic to
  potentially make a scheduling decision.

Zephyr supports so-called 'direct' interrupts, which are installed via
:c:macro:`IRQ_DIRECT_CONNECT`. These direct interrupts have some special
implementation requirements and a reduced feature set; see the definition
of :c:macro:`IRQ_DIRECT_CONNECT` for details.

The following code demonstrates a direct ISR:

.. code-block:: c

    #define MY_DEV_IRQ  24       /* device uses IRQ 24 */
    #define MY_DEV_PRIO  2       /* device uses interrupt priority 2 */
    /* argument passed to my_isr(), in this case a pointer to the device */
    #define MY_IRQ_FLAGS 0       /* IRQ flags. Unused on non-x86 */

    ISR_DIRECT_DECLARE(my_isr)
    {
       do_stuff();
       ISR_DIRECT_PM(); /* PM done after servicing interrupt for best latency */
       return 1; /* We should check if scheduling decision should be made */
    }

    void my_isr_installer(void)
    {
       ...
       IRQ_DIRECT_CONNECT(MY_DEV_IRQ, MY_DEV_PRIO, my_isr, MY_IRQ_FLAGS);
       irq_enable(MY_DEV_IRQ);
       ...
    }

Implementation Details
======================

Interrupt tables are set up at build time using some special build tools.  The
details laid out here apply to all architectures except x86, which are
covered in the `x86 Details`_ section below.

Any invocation of :c:macro:`IRQ_CONNECT` will declare an instance of
struct _isr_list which is placed in a special .intList section:

.. code-block:: c

    struct _isr_list {
        /** IRQ line number */
        s32_t irq;
        /** Flags for this IRQ, see ISR_FLAG_* definitions */
        s32_t flags;
        /** ISR to call */
        void *func;
        /** Parameter for non-direct IRQs */
        void *param;
    };

Zephyr is built in two phases; the first phase of the build produces
zephyr_prebuilt.elf which contains all the entries in the .intList section
preceded by a header:

.. code-block:: c

    struct {
        void *spurious_irq_handler;
        void *sw_irq_handler;
        u32_t num_isrs;
        u32_t num_vectors;
        struct _isr_list isrs[];  <- of size num_isrs
    };

This data consisting of the header and instances of struct _isr_list inside
zephyr_prebuilt.elf is then used by the gen_isr_tables.py script to generate a
C file defining a vector table and software ISR table that are then compiled
and linked into the final application.

The priority level of any interrupt is not encoded in these tables, instead
:c:macro:`IRQ_CONNECT` also has a runtime component which programs the desired
priority level of the interrupt to the interrupt controller. Some architectures
do not support the notion of interrupt priority, in which case the priority
argument is ignored.

Vector Table
------------
A vector table is generated when CONFIG_GEN_IRQ_VECTOR_TABLE is enabled.  This
data structure is used natively by the CPU and is simply an array of function
pointers, where each element n corresponds to the IRQ handler for IRQ line n,
and the function pointers are:

#. For 'direct' interrupts declared with :c:macro:`IRQ_DIRECT_CONNECT`, the
   handler function will be placed here.
#. For regular interrupts declared with :c:macro:`IRQ_CONNECT`, the address
   of the common software IRQ handler is placed here. This code does common
   kernel interrupt bookkeeping and looks up the ISR and parameter from the
   software ISR table.
#. For interrupt lines that are not configured at all, the address of the
   spurious IRQ handler will be placed here. The spurious IRQ handler
   causes a system fatal error if encountered.

Some architectures (such as the Nios II internal interrupt controller) have a
common entry point for all interrupts and do not support a vector table, in
which case the CONFIG_GEN_IRQ_VECTOR_TABLE option should be disabled.

Some architectures may reserve some initial vectors for system exceptions
and declare this in a table elsewhere, in which case
CONFIG_GEN_IRQ_START_VECTOR needs to be set to properly offset the indices
in the table.

SW ISR Table
------------
This is an array of struct _isr_table_entry:

.. code-block:: c

    struct _isr_table_entry {
        void *arg;
        void (*isr)(void *);
    };

This is used by the common software IRQ handler to look up the ISR and its
argument and execute it. The active IRQ line is looked up in an interrupt
controller register and used to index this table.

x86 Details
-----------

The x86 architecture has a special type of vector table called the Interrupt
Descriptor Table (IDT) which must be laid out in a certain way per the x86
processor documentation.  It is still fundamentally a vector table, and the
gen_idt tool uses the .intList section to create it. However, on APIC-based
systems the indexes in the vector table do not correspond to the IRQ line. The
first 32 vectors are reserved for CPU exceptions, and all remaining vectors (up
to index 255) correspond to the priority level, in groups of 16. In this
scheme, interrupts of priority level 0 will be placed in vectors 32-47, level 1
48-63, and so forth. When the gen_idt tool is constructing the IDT, when it
configures an interrupt it will look for a free vector in the appropriate range
for the requested priority level and set the handler there.

There are some APIC variants (such as MVIC) where priorities cannot be set
by the user and the position in the vector table does correspond to the
IRQ line. Systems like this will enable CONFIG_X86_FIXED_IRQ_MAPPING.

On x86 when an interrupt or exception vector is executed by the CPU, there is
no foolproof way to determine which vector was fired, so a software ISR table
indexed by IRQ line is not used. Instead, the :c:macro:`IRQ_CONNECT` call
creates a small assembly language function which calls the common interrupt
code in :cpp:func:`_interrupt_enter` with the ISR and parameter as arguments.
It is the address of this assembly interrupt stub which gets placed in the IDT.
For interrupts declared with :c:macro:`IRQ_DIRECT_CONNECT` the parameterless
ISR is placed directly in the IDT.

On systems where the position in the vector table corresponds to the
interrupt's priority level, the interrupt controller needs to know at
runtime what vector is associated with an IRQ line. gen_idt additionally
creates an _irq_to_interrupt_vector array which maps an IRQ line to its
configured vector in the IDT. This is used at runtime by :c:macro:`IRQ_CONNECT`
to program the IRQ-to-vector association in the interrupt controller.

Suggested Uses
**************

Use a regular or direct ISR to perform interrupt processing that requires a
very rapid response, and can be done quickly without blocking.

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

* :c:macro:`IRQ_CONNECT`
* :c:macro:`IRQ_DIRECT_CONNECT`
* :c:macro:`ISR_DIRECT_HEADER`
* :c:macro:`ISR_DIRECT_FOOTER`
* :c:macro:`ISR_DIRECT_PM`
* :c:macro:`ISR_DIRECT_DECLARE`
* :cpp:func:`irq_lock()`
* :cpp:func:`irq_unlock()`
* :cpp:func:`irq_enable()`
* :cpp:func:`irq_disable()`
* :cpp:func:`irq_is_enabled()`

The following interrupt-related APIs are provided by :file:`kernel.h`:

* :cpp:func:`k_is_in_isr()`
* :cpp:func:`k_is_preempt_thread`
