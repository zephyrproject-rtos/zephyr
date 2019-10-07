.. _architecture_porting_guide:

Architecture Porting Guide
##########################

An architecture port is needed to enable Zephyr to run on an :abbr:`ISA
(instruction set architecture)` or an :abbr:`ABI (Application Binary
Interface)` that is not currently supported.

The following are examples of ISAs and ABIs that Zephyr supports:

* x86_32 ISA with System V ABI
* ARMv7-M ISA with Thumb2 instruction set and ARM Embedded ABI (aeabi)
* ARCv2 ISA

For information on Kconfig configuration, see the
:ref:`setting_configuration_values` section in the :ref:`board_porting_guide`.
Architectures use a similar Kconfig configuration scheme. The
:ref:`kconfig_tips_and_tricks` page has some general recommendations and tips
for writing Kconfig files as well.

An architecture port can be divided in several parts; most are required and
some are optional:

* **The early boot sequence**: each architecture has different steps it must
  take when the CPU comes out of reset (required).

* **Interrupt and exception handling**: each architecture handles asynchronous
  and unrequested events in a specific manner (required).

* **Thread context switching**: the Zephyr context switch is dependent on the
  ABI and each ISA has a different set of registers to save (required).

* **Thread creation and termination**: A thread's initial stack frame is ABI
  and architecture-dependent, and thread abortion possibly as well (required).

* **Device drivers**: most often, the system clock timer and the interrupt
  controller are tied to the architecture (some required, some optional).

* **Utility libraries**: some common kernel APIs rely on a
  architecture-specific implementation for performance reasons (required).

* **CPU idling/power management**: most architectures implement instructions
  for putting the CPU to sleep (partly optional, most likely very desired).

* **Fault management**: for implementing architecture-specific debug help and
  handling of fatal error in threads (partly optional).

* **Linker scripts and toolchains**: architecture-specific details will most
  likely be needed in the build system and when linking the image (required).

Early Boot Sequence
*******************

The goal of the early boot sequence is to take the system from the state it is
after reset to a state where is can run C code and thus the common kernel
initialization sequence. Most of the time, very few steps are needed, while
some architectures require a bit more work to be performed.

Common steps for all architectures:

* Setup an initial stack.
* If running an :abbr:`XIP (eXecute-In-Place)` kernel, copy initialized data
* from ROM to RAM.
* If not using an ELF loader, zero the BSS section.
* Jump to :code:`_Cstart()`, the early kernel initialization

  * :code:`_Cstart()` is responsible for context switching out of the fake
    context running at startup into the main thread.

Some examples of architecture-specific steps that have to be taken:

* If given control in real mode on x86_32, switch to 32-bit protected mode.
* Setup the segment registers on x86_32 to handle boot loaders that leave them
  in an unknown or broken state.
* Initialize a board-specific watchdog on Cortex-M3/4.
* Switch stacks from MSP to PSP on Cortex-M.
* Use a different approach than calling into _Swap() on Cortex-M to prevent
  race conditions.
* Setup FIRQ and regular IRQ handling on ARCv2.

Interrupt and Exception Handling
********************************

Each architecture defines interrupt and exception handling differently.

When a device wants to signal the processor that there is some work to be done
on its behalf, it raises an interrupt. When a thread does an operation that is
not handled by the serial flow of the software itself, it raises an exception.
Both, interrupts and exceptions, pass control to a handler. The handler is
known as an :abbr:`ISR (Interrupt Service Routine)` in the case of
interrupts. The handler perform the work required the exception or the
interrupt.  For interrupts, that work is device-specific. For exceptions, it
depends on the exception, but most often the core kernel itself is responsible
for providing the handler.

The kernel has to perform some work in addition to the work the handler itself
performs. For example:

* Prior to handing control to the handler:

  * Save the currently executing context.
  * Possibly getting out of power saving mode, which includes waking up
    devices.
  * Updating the kernel uptime if getting out of tickless idle mode.

* After getting control back from the handler:

  * Decide whether to perform a context switch.
  * When performing a context switch, restore the context being context
    switched in.

This work is conceptually the same across architectures, but the details are
completely different:

* The registers to save and restore.
* The processor instructions to perform the work.
* The numbering of the exceptions.
* etc.

It thus needs an architecture-specific implementation, called the
interrupt/exception stub.

Another issue is that the kernel defines the signature of ISRs as:

.. code-block:: C

    void (*isr)(void *parameter)

Architectures do not have a consistent or native way of handling parameters to
an ISR. As such there are two commonly used methods for handling the
parameter.

* Using some architecture defined mechanism, the parameter value is forced in
  the stub. This is commonly found in X86-based architectures.

* The parameters to the ISR are inserted and tracked via a separate table
  requiring the architecture to discover at runtime which interrupt is
  executing. A common interrupt handler demuxer is installed for all entries of
  the real interrupt vector table, which then fetches the device's ISR and
  parameter from the separate table. This approach is commonly used in the ARC
  and ARM architectures via the :option:`CONFIG_GEN_ISR_TABLES` implementation.
  You can find examples of the stubs by looking at :code:`_interrupt_enter()` in
  x86, :code:`_IntExit()` in ARM, :code:`_isr_wrapper()` in ARM, or the full
  implementation description for ARC in :zephyr_file:`arch/arc/core/isr_wrapper.S`.

Each architecture also has to implement primitives for interrupt control:

* locking interrupts: :c:func:`irq_lock`, :c:func:`irq_unlock`.
* registering interrupts: :c:func:`IRQ_CONNECT`.
* programming the priority if possible :c:func:`irq_priority_set`.
* enabling/disabling interrupts: :c:func:`irq_enable`, :c:func:`irq_disable`.

.. note::

  :c:macro:`IRQ_CONNECT` is a macro that uses assembler and/or linker script
  tricks to connect interrupts at build time, saving boot time and text size.

The vector table should contain a handler for each interrupt and exception that
can possibly occur. The handler can be as simple as a spinning loop. However,
we strongly suggest that handlers at least print some debug information. The
information helps figuring out what went wrong when hitting an exception that
is a fault, like divide-by-zero or invalid memory access, or an interrupt that
is not expected (:dfn:`spurious interrupt`). See the ARM implementation in
:zephyr_file:`arch/arm/core/cortex_m/fault.c` for an example.

Thread Context Switching
************************

Multi-threading is the basic purpose to have a kernel at all. Zephyr supports
two types of threads: preemptible and cooperative.

Two crucial concepts when writing an architecture port are the following:

* Cooperative threads run at a higher priority than preemptible ones, and
  always preempt them.

* After handling an interrupt, if a cooperative thread was interrupted, the
  kernel always goes back to running that thread, since it is not preemptible.

A context switch can happen in several circumstances:

* When a thread executes a blocking operation, such as taking a semaphore that
  is currently unavailable.

* When a preemptible thread unblocks a thread of higher priority by releasing
  the object on which it was blocked.

* When an interrupt unblocks a thread of higher priority than the one currently
  executing, if the currently executing thread is preemptible.

* When a thread runs to completion.

* When a thread causes a fatal exception and is removed from the running
  threads. For example, referencing invalid memory,

Therefore, the context switching must thus be able to handle all these cases.

The kernel keeps the next thread to run in a "cache", and thus the context
switching code only has to fetch from that cache to select which thread to run.

There are two types of context switches: :dfn:`cooperative` and :dfn:`preemptive`.

* A *cooperative* context switch happens when a thread willfully gives the
  control to another thread. There are two cases where this happens

  * When a thread explicitly yields.
  * When a thread tries to take an object that is currently unavailable and is
    willing to wait until the object becomes available.

* A *preemptive* context switch happens either because an ISR or a
  thread causes an operation that schedules a thread of higher priority than the
  one currently running, if the currently running thread is preemptible.
  An example of such an operation is releasing an object on which the thread
  of higher priority was waiting.

.. note::

  Control is never taken from cooperative thread when one of them is the
  running thread.

A cooperative context switch is always done by having a thread call the
:code:`_Swap()` kernel internal symbol. When :code:`_Swap` is called, the
kernel logic knows that a context switch has to happen: :code:`_Swap` does not
check to see if a context switch must happen. Rather, :code:`_Swap` decides
what thread to context switch in. :code:`_Swap` is called by the kernel logic
when an object being operated on is unavailable, and some thread
yielding/sleeping primitives.

.. note::

  On x86 and Nios2, :code:`_Swap` is generic enough and the architecture
  flexible enough that :code:`_Swap` can be called when exiting an interrupt
  to provoke the context switch. This should not be taken as a rule, since
  neither the ARM Cortex-M or ARCv2 port do this.

Since :code:`_Swap` is cooperative, the caller-saved registers from the ABI are
already on the stack. There is no need to save them in the k_thread structure.

A context switch can also be performed preemptively. This happens upon exiting
an ISR, in the kernel interrupt exit stub:

* :code:`_interrupt_enter` on x86 after the handler is called.
* :code:`_IntExit` on ARM.
* :code:`_firq_exit` and :code:`_rirq_exit` on ARCv2.

In this case, the context switch must only be invoked when the interrupted
thread was preemptible, not when it was a cooperative one, and only when the
current interrupt is not nested.

The kernel also has the concept of "locking the scheduler". This is a concept
similar to locking the interrupts, but lighter-weight since interrupts can
still occur. If a thread has locked the scheduler, is it temporarily
non-preemptible.

So, the decision logic to invoke the context switch when exiting an interrupt
is simple:

* If the interrupted thread is not preemptible, do not invoke it.
* Else, fetch the cached thread from the ready queue, and:

  * If the cached thread is not the current thread, invoke the context switch.
  * Else, do not invoke it.

This is simple, but crucial: if this is not implemented correctly, the kernel
will not function as intended and will experience bizarre crashes, mostly due
to stack corruption.

.. note::

  If running a coop-only system, i.e. if :option:`CONFIG_NUM_PREEMPT_PRIORITIES`
  is 0, no preemptive context switch ever happens. The interrupt code can be
  optimized to not take any scheduling decision when this is the case.

Thread Creation and Termination
*******************************

To start a new thread, a stack frame must be constructed so that the context
switch can pop it the same way it would pop one from a thread that had been
context switched out. This is to be implemented in an architecture-specific
:code:`_new_thread` internal routine.

The thread entry point is also not to be called directly, i.e. it should not be
set as the :abbr:`PC (program counter)` for the new thread. Rather it must be
wrapped in :code:`_thread_entry`. This means that the PC in the stack
frame shall be set to :code:`_thread_entry`, and the thread entry point shall
be passed as the first parameter to :code:`_thread_entry`. The specifics of
this depend on the ABI.

The need for an architecture-specific thread termination implementation depends
on the architecture. There is a generic implementation, but it might not work
for a given architecture.

One reason that has been encountered for having an architecture-specific
implementation of thread termination is that aborting a thread might be
different if aborting because of a graceful exit or because of an exception.
This is the case for ARM Cortex-M, where the CPU has to be taken out of handler
mode if the thread triggered a fatal exception, but not if the thread
gracefully exits its entry point function.

This means implementing an architecture-specific version of
:cpp:func:`k_thread_abort`, and setting the Kconfig option
:option:`CONFIG_ARCH_HAS_THREAD_ABORT` as needed for the architecture (e.g. see
:zephyr_file:`arch/arm//core/cortex_m/Kconfig`).

Device Drivers
**************

The kernel requires very few hardware devices to function. In theory, the only
required device is the interrupt controller, since the kernel can run without a
system clock. In practice, to get access to most, if not all, of the sanity
check test suite, a system clock is needed as well. Since these two are usually
tied to the architecture, they are part of the architecture port.

Interrupt Controllers
=====================

There can be significant differences between the interrupt controllers and the
interrupt concepts across architectures.

For example, x86 has the concept of an :abbr:`IDT (Interrupt Descriptor Table)`
and different interrupt controllers. The position of an interrupt in the IDT
determines its priority.

On the other hand, the ARM Cortex-M has the :abbr:`NVIC (Nested Vectored
Interrupt Controller)` as part of the architecture definition. There is no need
for an IDT-like table that is separate from the NVIC vector table. The position
in the table has nothing to do with priority of an IRQ: priorities are
programmable per-entry.

The ARCv2 has its interrupt unit as part of the architecture definition, which
is somewhat similar to the NVIC. However, where ARC defines interrupts as
having a one-to-one mapping between exception and interrupt numbers (i.e.
exception 1 is IRQ1, and device IRQs start at 16), ARM has IRQ0 being
equivalent to exception 16 (and weirdly enough, exception 1 can be seen as
IRQ-15).

All these differences mean that very little, if anything, can be shared between
architectures with regards to interrupt controllers.

System Clock
============

x86 has APIC timers and the HPET as part of its architecture definition. ARM
Cortex-M has the SYSTICK exception. Finally, ARCv2 has the timer0/1 device.

Kernel timeouts are handled in the context of the system clock timer driver's
interrupt handler.

Tickless Idle
-------------

The kernel has support for tickless idle. Tickless idle is the concept where no
system clock timer interrupt is to be delivered to the CPU when the kernel is
about to go idle and the closest timeout expiry is passed a certain threshold.
When this condition happens, the system clock is reprogrammed far in the future
instead of for a periodic tick. For this to work, the system clock timer driver
must support it.

Tickless idle is optional but strongly recommended to achieve low-power
consumption.

The kernel has built-in support for going into tickless idle.

The system clock timer driver must implement some hooks to support tickless
idle. See existing drivers for examples.

The interrupt entry stub (:code:`_interrupt_enter`, :code:`_isr_wrapper`) needs
to be adapted to handle exiting tickless idle. See examples in the code for
existing architectures.

Console Over Serial Line
========================

There is one other device that is almost a requirement for an architecture
port, since it is so useful for debugging. It is a simple polling, output-only,
serial port driver on which to send the console (:code:`printk`,
:code:`printf`) output.

It is not required, and a RAM console (:option:`CONFIG_RAM_CONSOLE`)
can be used to send all output to a circular buffer that can be read
by a debugger instead.

Utility Libraries
*****************

The kernel depends on a few functions that can be implemented with very few
instructions or in a lock-less manner in modern processors. Those are thus
expected to be implemented as part of an architecture port.

* Atomic operators.

  * If instructions do not exist for a given architecture,
    a generic version that wraps :c:func:`irq_lock` or :c:func:`irq_unlock`
    around non-atomic operations exists. It is configured using the
    :option:`CONFIG_ATOMIC_OPERATIONS_C` Kconfig option.

* Find-least-significant-bit-set and find-most-significant-bit-set.

  * If instructions do not exist for a given architecture, it is always
    possible to implement these functions as generic C functions.

It is possible to use compiler built-ins to implement these, but be careful
they use the required compiler barriers.

CPU Idling/Power Management
***************************

The kernel provides support for CPU power management with two functions:
:c:func:`z_arch_cpu_idle` and :c:func:`z_arch_cpu_atomic_idle`.

:c:func:`z_arch_cpu_idle` can be as simple as calling the power saving
instruction for the architecture with interrupts unlocked, for example
:code:`hlt` on x86, :code:`wfi` or :code:`wfe` on ARM, :code:`sleep` on ARC.
This function can be called in a loop within a context that does not care if it
get interrupted or not by an interrupt before going to sleep. There are
basically two scenarios when it is correct to use this function:

* In a single-threaded system, in the only thread when the thread is not used
  for doing real work after initialization, i.e. it is sitting in a loop doing
  nothing for the duration of the application.

* In the idle thread.

:c:func:`z_arch_cpu_atomic_idle`, on the other hand, must be able to atomically
re-enable interrupts and invoke the power saving instruction. It can thus be
used in real application code, again in single-threaded systems.

Normally, idling the CPU should be left to the idle thread, but in some very
special scenarios, these APIs can be used by applications.

Both functions must exist for a given architecture. However, the implementation
can be simply the following steps, if desired:

#. unlock interrupts
#. NOP

However, a real implementation is strongly recommended.

Fault Management
****************

In the event of an unhandled CPU exception, the architecture
code must call into :c:func:`z_fatal_error`.  This function dumps
out architecture-agnostic information and makes a policy
decision on what to do next by invoking :c:func:`k_sys_fatal_error`.
This function can be overridden to implement application-specific
policies that could include locking interrupts and spinning forever
(the default implementation) or even powering off the
system (if supported).

Toolchain and Linking
*********************

Toolchain support has to be added to the build system.

Some architecture-specific definitions are needed in :zephyr_file:`include/toolchain/gcc.h`.
See what exists in that file for currently supported architectures.

Each architecture also needs its own linker script, even if most sections can
be derived from the linker scripts of other architectures. Some sections might
be specific to the new architecture, for example the SCB section on ARM and the
IDT section on x86.

Hardware Stack Protection
=========================

This option uses hardware features to generate a fatal error if a thread
in supervisor mode overflows its stack. This is useful for debugging, although
for a couple reasons, you can't reliably make any assertions about the state
of the system after this happens:

* The kernel could have been inside a critical section when the overflow
  occurs, leaving important global data structures in a corrupted state.
* For systems that implement stack protection using a guard memory region,
  it's possible to overshoot the guard and corrupt adjacent data structures
  before the hardware detects this situation.

To enable the :option:`CONFIG_HW_STACK_PROTECTION` feature, the system must
provide some kind of hardware-based stack overflow protection, and enable the
:option:`CONFIG_ARCH_HAS_STACK_PROTECTION` option.

There are no C APIs that need to be implemented to support stack protection,
and it's entirely implemented within the ``arch/`` code.  However in most cases
(such as if a guard region needs to be defined) the architecture will need to
declare its own versions of the K_THREAD_STACK macros in ``arch/cpu.h``:

* :c:macro:`_ARCH_THREAD_STACK_DEFINE()`
* :c:macro:`_ARCH_THREAD_STACK_ARRAY_DEFINE()`
* :c:macro:`_ARCH_THREAD_STACK_MEMBER()`
* :c:macro:`_ARCH_THREAD_STACK_SIZEOF()`

For systems that implement stack protection using a Memory Protection Unit
(MPU) or Memory Management Unit (MMU), this is typically done by declaring a
guard memory region immediately before the stack area.

* On MMU systems, this guard area is an entire page whose permissions in the
  page table will generate a fault on writes. This page needs to be
  configured in the arch's _new_thread() function.

* On MPU systems, one of the MPU regions needs to be reserved for the thread
  stack guard area, whose size should be minimized. The region in the MPU
  should be reconfigured on context switch such that the guard region
  for the incoming thread is not writable.

User Mode Threads
=================

To support user mode threads, several kernel-to-arch APIs need to be
implemented, and the system must enable the :option:`CONFIG_ARCH_HAS_USERSPACE`
option. Please see the documentation for each of these functions for more
details:

* :cpp:func:`z_arch_buffer_validate()` to test whether the current thread has
  access permissions to a particular memory region

* :cpp:func:`z_arch_user_mode_enter()` which will irreversibly drop a supervisor
  thread to user mode privileges. The stack must be wiped.

* :cpp:func:`z_arch_syscall_oops()` which generates a kernel oops when system
  call parameters can't be validated, in such a way that the oops appears to be
  generated from where the system call was invoked in the user thread

* :cpp:func:`z_arch_syscall_invoke0()` through
  :cpp:func:`z_arch_syscall_invoke6()` invoke a system call with the
  appropriate number of arguments which must all be passed in during the
  privilege elevation via registers.

* :cpp:func:`z_arch_is_user_context()` return nonzero if the CPU is currently
  running in user mode

* :cpp:func:`z_arch_mem_domain_max_partitions_get()` which indicates the max
  number of regions for a memory domain. MMU systems have an unlimited amount,
  MPU systems have constraints on this.

* :cpp:func:`z_arch_mem_domain_partition_remove()` Remove a partition from
  a memory domain if the currently executing thread was part of that domain.

* :cpp:func:`z_arch_mem_domain_destroy()` Reset the thread's memory domain
  configuration

In addition to implementing these APIs, there are some other tasks as well:

* :cpp:func:`_new_thread()` needs to spawn threads with :c:macro:`K_USER` in
  user mode

* On context switch, the outgoing thread's stack memory should be marked
  inaccessible to user mode by making the appropriate configuration changes in
  the memory management hardware.. The incoming thread's stack memory should
  likewise be marked as accessible. This ensures that threads can't mess with
  other thread stacks.

* On context switch, the system needs to switch between memory domains for
  the incoming and outgoing threads.

* Thread stack areas must include a kernel stack region. This should be
  inaccessible to user threads at all times. This stack will be used when
  system calls are made. This should be fixed size for all threads, and must
  be large enough to handle any system call.

* A software interrupt or some kind of privilege elevation mechanism needs to
  be established. This is closely tied to how the _arch_syscall_invoke macros
  are implemented. On system call, the appropriate handler function needs to
  be looked up in _k_syscall_table. Bad system call IDs should jump to the
  :cpp:enum:`K_SYSCALL_BAD` handler. Upon completion of the system call, care
  must be taken not to leak any register state back to user mode.
