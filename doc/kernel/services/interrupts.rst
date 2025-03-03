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

Any number of ISRs can be defined (limited only by available RAM), subject to
the constraints imposed by underlying hardware.

An ISR has the following key properties:

* An **interrupt request (IRQ) signal** that triggers the ISR.
* A **priority level** associated with the IRQ.
* An **interrupt service routine** that is invoked to handle the interrupt.
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

An ISR executes in the kernel's **interrupt context**. This context has its
own dedicated stack area (or, on some architectures, stack areas). The size
of the interrupt context stack must be capable of handling the execution of
multiple concurrent ISRs if interrupt
nesting support is enabled.

.. important::
    Many kernel APIs can be used only by threads, and not by ISRs. In cases
    where a routine may be invoked by both threads and ISRs the kernel
    provides the :c:func:`k_is_in_isr` API to allow the routine to
    alter its behavior depending on whether it is executing as part of
    a thread or as part of an ISR.

.. _multi_level_interrupts:

Multi-level Interrupt Handling
==============================

A hardware platform can support more interrupt lines than natively-provided
through the use of one or more nested interrupt controllers.  Sources of
hardware interrupts are combined into one line that is then routed to
the parent controller.

If nested interrupt controllers are supported, :kconfig:option:`CONFIG_MULTI_LEVEL_INTERRUPTS`
should be enabled, and :kconfig:option:`CONFIG_2ND_LEVEL_INTERRUPTS` and
:kconfig:option:`CONFIG_3RD_LEVEL_INTERRUPTS` configured as well, based on the
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
    then performs an operation that puts itself to sleep (e.g. sleeping
    for N milliseconds), the thread's IRQ lock no longer applies once
    thread A is swapped out and the next ready thread B starts to
    run.

    This means that interrupts can be processed while thread B is
    running unless thread B has also locked out interrupts using its own
    IRQ lock.  (Whether interrupts can be processed while the kernel is
    switching between two threads that are using the IRQ lock is
    architecture-specific.)

    When thread A eventually becomes the current thread once again, the kernel
    re-establishes thread A's IRQ lock. This ensures thread A won't be
    interrupted until it has explicitly unlocked its IRQ lock.

    If thread A does not sleep but does make a higher-priority thread B
    ready, the IRQ lock will inhibit any preemption that would otherwise
    occur.  Thread B will not run until the next :ref:`reschedule point
    <scheduling_v2>` reached after releasing the IRQ lock.

Alternatively, a thread may temporarily **disable** a specified IRQ
so its associated ISR does not execute when the IRQ is signaled.
The IRQ must be subsequently **enabled** to permit the ISR to execute.

.. important::
    Disabling an IRQ prevents *all* threads in the system from being preempted
    by the associated ISR, not just the thread that disabled the IRQ.

Zero Latency Interrupts
-----------------------

Preventing interruptions by applying an IRQ lock may increase the observed
interrupt latency. A high interrupt latency, however, may not be acceptable
for certain low-latency use-cases.

The kernel addresses such use-cases by allowing interrupts with critical
latency constraints to execute at a priority level that cannot be blocked
by interrupt locking. These interrupts are defined as
*zero-latency interrupts*. The support for zero-latency interrupts requires
:kconfig:option:`CONFIG_ZERO_LATENCY_IRQS` to be enabled. In addition to that, the
flag :c:macro:`IRQ_ZERO_LATENCY` must be passed to :c:macro:`IRQ_CONNECT` or
:c:macro:`IRQ_DIRECT_CONNECT` macros to configure the particular interrupt
with zero latency.

Zero-latency interrupts are expected to be used to manage hardware events
directly, and not to interoperate with the kernel code at all. They should
treat all kernel APIs as undefined behavior (i.e. an application that uses the
APIs inside a zero-latency interrupt context is responsible for directly
verifying correct behavior). Zero-latency interrupts may not modify any data
inspected by kernel APIs invoked from normal Zephyr contexts and shall not
generate exceptions that need to be handled synchronously (e.g. kernel panic).

.. important::
    Zero-latency interrupts are supported on an architecture-specific basis.
    The feature is currently implemented in the ARM Cortex-M architecture
    variant.

Offloading ISR Work
===================

An ISR should execute quickly to ensure predictable system operation.
If time consuming processing is required the ISR should offload some or all
processing to a thread, thereby restoring the kernel's ability to respond
to other interrupts.

The kernel supports several mechanisms for offloading interrupt-related
processing to a thread.

* An ISR can signal a helper thread to do interrupt-related processing
  using a kernel object, such as a FIFO, LIFO, or semaphore.

* An ISR can instruct the system workqueue thread to execute a work item.
  (See :ref:`workqueues_v2`.)

When an ISR offloads work to a thread, there is typically a single context
switch to that thread when the ISR completes, allowing interrupt-related
processing to continue almost immediately. However, depending on the
priority of the thread handling the offload, it is possible that
the currently executing cooperative thread or other higher-priority threads
may execute before the thread handling the offload is scheduled.

Sharing interrupt lines
=======================

In the case of some hardware platforms, the same interrupt lines may be used
by different IPs. For example, interrupt 17 may be used by a DMA controller to
signal that a data transfer has been completed or by a DAI controller to signal
that the transfer FIFO has reached its watermark. To make this work, one would
have to either employ some special logic or find a workaround (for example, using
the shared_irq interrupt controller), which doesn't scale very well.

To solve this problem, one may use shared interrupts, which can be enabled using
:kconfig:option:`CONFIG_SHARED_INTERRUPTS`. Whenever an attempt to register
a second ISR/argument pair on the same interrupt line is made (using
:c:macro:`IRQ_CONNECT` or :c:func:`irq_connect_dynamic`), the interrupt line will
become shared, meaning the two ISR/argument pairs (previous one and the one that
has just been registered) will be invoked each time the interrupt is triggered.
The entities that make use of an interrupt line in the shared interrupt context
are known as clients. The maximum number of allowed clients for an interrupt is
controlled by :kconfig:option:`CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS`.

Interrupt sharing is transparent to the user. As such, the user may register
interrupts using :c:macro:`IRQ_CONNECT` and :c:func:`irq_connect_dynamic` as
they normally would. The interrupt sharing is taken care of behind the scenes.

Enabling the shared interrupt support and dynamic interrupt support will
allow users to dynamically disconnect ISRs using :c:func:`irq_disconnect_dynamic`.
After an ISR is disconnected, whenever the interrupt line for which it was
register gets triggered, the ISR will no longer get invoked.

Please note that enabling :kconfig:option:`CONFIG_SHARED_INTERRUPTS` will
result in a non-negligible increase in the binary size. Use with caution.

Implementation
**************

Defining a regular ISR
======================

An ISR is defined at runtime by calling :c:macro:`IRQ_CONNECT`. It must
then be enabled by calling :c:func:`irq_enable`.

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
    #define MY_IRQ_FLAGS 0       /* IRQ flags */

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

Since the :c:macro:`IRQ_CONNECT` macro requires that all its parameters be
known at build time, in some cases this may not be acceptable. It is also
possible to install interrupts at runtime with
:c:func:`irq_connect_dynamic`. It is used in exactly the same way as
:c:macro:`IRQ_CONNECT`:

.. code-block:: c

    void my_isr_installer(void)
    {
       ...
       irq_connect_dynamic(MY_DEV_IRQ, MY_DEV_PRIO, my_isr, MY_ISR_ARG,
                           MY_IRQ_FLAGS);
       irq_enable(MY_DEV_IRQ);
       ...
    }

Dynamic interrupts require the :kconfig:option:`CONFIG_DYNAMIC_INTERRUPTS` option to
be enabled. Removing or re-configuring a dynamic interrupt is currently
unsupported.

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
:c:macro:`IRQ_DIRECT_CONNECT` and whose handlers are declared using
:c:macro:`ISR_DIRECT_DECLARE`. These direct interrupts have some special
implementation requirements and a reduced feature set; see the definitions
of :c:macro:`IRQ_DIRECT_CONNECT` and :c:macro:`ISR_DIRECT_DECLARE` for details.

The following code demonstrates a direct ISR:

.. code-block:: c

    #define MY_DEV_IRQ  24       /* device uses IRQ 24 */
    #define MY_DEV_PRIO  2       /* device uses interrupt priority 2 */
    /* argument passed to my_isr(), in this case a pointer to the device */
    #define MY_IRQ_FLAGS 0       /* IRQ flags */

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

Installation of dynamic direct interrupts is supported on an
architecture-specific basis. (The feature is currently implemented in
ARM Cortex-M architecture variant. Dynamic direct interrupts feature is
exposed to the user via an ARM-only API.)

Sharing an interrupt line
=========================

The following code defines two ISRs using the same interrupt number.

.. code-block:: c

    #define MY_DEV_IRQ 24		/* device uses INTID 24 */
    #define MY_DEV_IRQ_PRIO 2		/* device uses interrupt priority 2 */
    /*  this argument may be anything */
    #define MY_FST_ISR_ARG INT_TO_POINTER(1)
    /*  this argument may be anything */
    #define MY_SND_ISR_ARG INT_TO_POINTER(2)
    #define MY_IRQ_FLAGS 0		/* IRQ flags */

    void my_first_isr(void *arg)
    {
       ... /* some magic happens here */
    }

    void my_second_isr(void *arg)
    {
       ... /* even more magic happens here */
    }

    void my_isr_installer(void)
    {
       ...
       IRQ_CONNECT(MY_DEV_IRQ, MY_DEV_IRQ_PRIO, my_first_isr, MY_FST_ISR_ARG, MY_IRQ_FLAGS);
       IRQ_CONNECT(MY_DEV_IRQ, MY_DEV_IRQ_PRIO, my_second_isr, MY_SND_ISR_ARG, MY_IRQ_FLAGS);
       ...
    }

The same restrictions regarding :c:macro:`IRQ_CONNECT` described in `Defining a regular ISR`_
are applicable here. If :kconfig:option:`CONFIG_SHARED_INTERRUPTS` is disabled, the above
code will generate a build error. Otherwise, the above code will result in the two ISRs
being invoked each time interrupt 24 is triggered.

If :kconfig:option:`CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS` is set to a value lower than 2
(current number of clients), a build error will be generated.

If dynamic interrupts are enabled, :c:func:`irq_connect_dynamic` will allow sharing interrupts
during runtime. Exceeding the configured maximum number of allowed clients will result in
a failed assertion.

Dynamically disconnecting an ISR
================================

The following code defines two ISRs using the same interrupt number. The second
ISR is disconnected during runtime.

.. code-block:: c

    #define MY_DEV_IRQ 24		/* device uses INTID 24 */
    #define MY_DEV_IRQ_PRIO 2		/* device uses interrupt priority 2 */
    /*  this argument may be anything */
    #define MY_FST_ISR_ARG INT_TO_POINTER(1)
    /*  this argument may be anything */
    #define MY_SND_ISR_ARG INT_TO_POINTER(2)
    #define MY_IRQ_FLAGS 0		/* IRQ flags */

    void my_first_isr(void *arg)
    {
       ... /* some magic happens here */
    }

    void my_second_isr(void *arg)
    {
       ... /* even more magic happens here */
    }

    void my_isr_installer(void)
    {
       ...
       IRQ_CONNECT(MY_DEV_IRQ, MY_DEV_IRQ_PRIO, my_first_isr, MY_FST_ISR_ARG, MY_IRQ_FLAGS);
       IRQ_CONNECT(MY_DEV_IRQ, MY_DEV_IRQ_PRIO, my_second_isr, MY_SND_ISR_ARG, MY_IRQ_FLAGS);
       ...
    }

    void my_isr_uninstaller(void)
    {
       ...
       irq_disconnect_dynamic(MY_DEV_IRQ, MY_DEV_IRQ_PRIO, my_first_isr, MY_FST_ISR_ARG, MY_IRQ_FLAGS);
       ...
    }

The :c:func:`irq_disconnect_dynamic` call will result in interrupt 24 becoming
unshared, meaning the system will act as if the first :c:macro:`IRQ_CONNECT`
call never happened. This behaviour is only allowed if
:kconfig:option:`CONFIG_DYNAMIC_INTERRUPTS` is enabled, otherwise a linker
error will be generated.

Implementation Details
======================

Interrupt tables are set up at build time using some special build tools.  The
details laid out here apply to all architectures except x86, which are
covered in the `x86 Details`_ section below.

The invocation of :c:macro:`IRQ_CONNECT` will declare an instance of
struct _isr_list which is placed in a special .intList section.
This section is placed in compiled code on precompilation stages only.
It is meant to be used by Zephyr script to generate interrupt tables
and is removed from the final build.
The script implements different parsers to process the data from .intList section
and produce the required output.

The default parser generates C arrays filled with arguments and interrupt
handlers in a form of addresses directly taken from .intList section entries.
It works with all the architectures and compilers (with the exception mentioned above).
The limitation of this parser is the fact that after the arrays are generated
it is expected for the code not to relocate.
Any relocation on this stage may lead to the situation where the entry in the interrupt array
is no longer pointing to the function that was expected.
It means that this parser, being more compatible is limiting us from using Link Time Optimization.

The local isr declaration parser uses different approach to construct
the same arrays at binnary level.
All the entries to the arrays are declared and defined locally,
directly in the file where :c:macro:`IRQ_CONNECT` is used.
They are placed in a section with the unique, synthesized name.
The name of the section is then placed in .intList section and it is used to create linker script
to properly place the created entry in the right place in the memory.
This parser is now limited to the supported architectures and toolchains but in reward it keeps
the information about object relations for linker thus allowing the Link Time Optimization.

Implementation using C arrays
-----------------------------

This is the default configuration available for all Zephyr supported architectures.

Any invocation of :c:macro:`IRQ_CONNECT` will declare an instance of
struct _isr_list which is placed in a special .intList section:

.. code-block:: c

    struct _isr_list {
        /** IRQ line number */
        int32_t irq;
        /** Flags for this IRQ, see ISR_FLAG_* definitions */
        int32_t flags;
        /** ISR to call */
        void *func;
        /** Parameter for non-direct IRQs */
        void *param;
    };

Zephyr is built in two phases; the first phase of the build produces
``${ZEPHYR_PREBUILT_EXECUTABLE}``.elf which contains all the entries in
the .intList section preceded by a header:

.. code-block:: c

    struct {
        void *spurious_irq_handler;
        void *sw_irq_handler;
        uint32_t num_isrs;
        uint32_t num_vectors;
        struct _isr_list isrs[];  <- of size num_isrs
    };

This data consisting of the header and instances of struct _isr_list inside
``${ZEPHYR_PREBUILT_EXECUTABLE}``.elf is then used by the
gen_isr_tables.py script to generate a C file defining a vector table and
software ISR table that are then compiled and linked into the final
application.

The priority level of any interrupt is not encoded in these tables, instead
:c:macro:`IRQ_CONNECT` also has a runtime component which programs the desired
priority level of the interrupt to the interrupt controller. Some architectures
do not support the notion of interrupt priority, in which case the priority
argument is ignored.

Vector Table
~~~~~~~~~~~~
A vector table is generated when :kconfig:option:`CONFIG_GEN_IRQ_VECTOR_TABLE` is
enabled.  This data structure is used natively by the CPU and is simply an
array of function pointers, where each element n corresponds to the IRQ handler
for IRQ line n, and the function pointers are:

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
which case the :kconfig:option:`CONFIG_GEN_IRQ_VECTOR_TABLE` option should be
disabled.

Some architectures may reserve some initial vectors for system exceptions
and declare this in a table elsewhere, in which case
CONFIG_GEN_IRQ_START_VECTOR needs to be set to properly offset the indices
in the table.

SW ISR Table
~~~~~~~~~~~~
This is an array of struct _isr_table_entry:

.. code-block:: c

    struct _isr_table_entry {
        void *arg;
        void (*isr)(void *);
    };

This is used by the common software IRQ handler to look up the ISR and its
argument and execute it. The active IRQ line is looked up in an interrupt
controller register and used to index this table.

Shared SW ISR Table
~~~~~~~~~~~~~~~~~~~

This is an array of struct z_shared_isr_table_entry:

.. code-block:: c

    struct z_shared_isr_table_entry {
        struct _isr_table_entry clients[CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS];
        size_t client_num;
    };

This table keeps track of the registered clients for each of the interrupt
lines. Whenever an interrupt line becomes shared, :c:func:`z_shared_isr` will
replace the currently registered ISR in _sw_isr_table. This special ISR will
iterate through the list of registered clients and invoke the ISRs.

Implementation using linker script
----------------------------------

This way of prepare and parse .isrList section to implement interrupt vectors arrays
is called local isr declaration.
The name comes from the fact that all the entries to the arrays that would create
interrupt vectors are created locally in place of invocation of :c:macro:`IRQ_CONNECT` macro.
Then automatically generated linker scripts are used to place it in the right place in the memory.

This option requires enabling by the choose of :kconfig:option:`ISR_TABLES_LOCAL_DECLARATION`.
If this configuration is supported by the used architecture and toolchaing the
:kconfig:option:`ISR_TABLES_LOCAL_DECLARATION_SUPPORTED` is set.
See details of this option for the information about currently supported configurations.

Any invocation of :c:macro:`IRQ_CONNECT` or :c:macro:`IRQ_DIRECT_CONNECT` will declare an instance
of ``struct _isr_list_sname`` which is placed in a special .intList section:

.. code-block:: c

    struct _isr_list_sname {
        /** IRQ line number */
        int32_t irq;
        /** Flags for this IRQ, see ISR_FLAG_* definitions */
        int32_t flags;
        /** The section name */
        const char sname[];
    };

Note that the section name is placed in flexible array member.
It means that the size of the initialized structure will vary depending on the
structure name length.
The whole entry is used by the script during the build of the application
and has all the information needed for proper interrupt placement.

Beside of the _isr_list_sname the :c:macro:`IRQ_CONNECT` macro generates an entry
that would be the part of the interrupt array:

.. code-block:: c

    struct _isr_table_entry {
        const void *arg;
        void (*isr)(const void *);
    };

This array is placed in a section with the name saved in _isr_list_sname structure.

The values created by :c:macro:`IRQ_DIRECT_CONNECT` macro depends on the architecture.
It can be changed to variable that points to a interrupt handler:

.. code-block:: c

    static uintptr_t <unique name> = ((uintptr_t)func);

Or to actually naked function that implements a jump to the interrupt handler:

.. code-block:: c

    static void <unique name>(void)
    {
        __asm(ARCH_IRQ_VECTOR_JUMP_CODE(func));
    }

Similar like for :c:macro:`IRQ_CONNECT`, the created variable or function is placed
in a section, saved in _isr_list_sname section.

Files generated by the script
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The interrupt tables generator script creates 3 files:
isr_tables.c, isr_tables_swi.ld, and isr_tables_vt.ld.

The isr_tables.c will contain all the structures for interrupts, direct interrupts and
shared interrupts (if enabled). This file implements only all the structures that
are not implemented by the application, leaving a comment where the interrupt
not implemented here can be found.

Then two linker files are used. The isr_tables_vt.ld file is included in place
where the interrupt vectors are required to be placed in the selected architecture.
The isr_tables_swi.ld file describes the placement of the software interrupt table
elements. The separated file is required as it might be placed in writable on nonwritable
section, depending on the current configuration.

x86 Details
-----------

The x86 architecture has a special type of vector table called the Interrupt
Descriptor Table (IDT) which must be laid out in a certain way per the x86
processor documentation.  It is still fundamentally a vector table, and the
:ref:`gen_idt.py` tool uses the .intList section to create it. However, on APIC-based
systems the indexes in the vector table do not correspond to the IRQ line. The
first 32 vectors are reserved for CPU exceptions, and all remaining vectors (up
to index 255) correspond to the priority level, in groups of 16. In this
scheme, interrupts of priority level 0 will be placed in vectors 32-47, level 1
48-63, and so forth. When the :ref:`gen_idt.py` tool is constructing the IDT, when it
configures an interrupt it will look for a free vector in the appropriate range
for the requested priority level and set the handler there.

On x86 when an interrupt or exception vector is executed by the CPU, there is
no foolproof way to determine which vector was fired, so a software ISR table
indexed by IRQ line is not used. Instead, the :c:macro:`IRQ_CONNECT` call
creates a small assembly language function which calls the common interrupt
code in :c:func:`_interrupt_enter` with the ISR and parameter as arguments.
It is the address of this assembly interrupt stub which gets placed in the IDT.
For interrupts declared with :c:macro:`IRQ_DIRECT_CONNECT` the parameterless
ISR is placed directly in the IDT.

On systems where the position in the vector table corresponds to the
interrupt's priority level, the interrupt controller needs to know at
runtime what vector is associated with an IRQ line. :ref:`gen_idt.py` additionally
creates an _irq_to_interrupt_vector array which maps an IRQ line to its
configured vector in the IDT. This is used at runtime by :c:macro:`IRQ_CONNECT`
to program the IRQ-to-vector association in the interrupt controller.

For dynamic interrupts, the build must generate some 4-byte dynamic interrupt
stubs, one stub per dynamic interrupt in use. The number of stubs is controlled
by the :kconfig:option:`CONFIG_X86_DYNAMIC_IRQ_STUBS` option. Each stub pushes an
unique identifier which is then used to fetch the appropriate handler function
and parameter out of a table populated when the dynamic interrupt was
connected.

Going Beyond the Default Supported Number of Interrupts
-------------------------------------------------------

When generating interrupts in the multi-level configuration, 8-bits per level is the default
mask used when determining which level a given interrupt code belongs to. This can become
a problem when dealing with CPUs that support more than 255 interrupts per single
aggregator. In this case it may be desirable to override these defaults and use a custom
number of bits per level. Regardless of how many bits used for each level, the sum of
the total bits used between all levels must sum to be less than or equal to 32-bits,
fitting into a single 32-bit integer. To modify the bit total per level, override the
default 8 in :file:`Kconfig.multilevel` by setting :kconfig:option:`CONFIG_1ST_LEVEL_INTERRUPT_BITS`
for the  first level, :kconfig:option:`CONFIG_2ND_LEVEL_INTERRUPT_BITS` for the second level and
:kconfig:option:`CONFIG_3RD_LEVEL_INTERRUPT_BITS` for the third level. These masks control the
length of the bit masks and shift to apply when generating interrupt values, when checking the
interrupts level and converting interrupts to a different level. The logic controlling
this can be found in :file:`irq_multilevel.h`

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

* :kconfig:option:`CONFIG_ISR_STACK_SIZE`

Additional architecture-specific and device-specific configuration options
also exist.

API Reference
*************

.. doxygengroup:: isr_apis
