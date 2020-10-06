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

For information on Kconfig configuration, see
:ref:`setting_configuration_values`. Architectures use a Kconfig configuration
scheme similar to boards.

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

* locking interrupts: :c:macro:`irq_lock()`, :c:macro:`irq_unlock()`.
* registering interrupts: :c:macro:`IRQ_CONNECT()`.
* programming the priority if possible :c:func:`irq_priority_set`.
* enabling/disabling interrupts: :c:macro:`irq_enable()`, :c:macro:`irq_disable()`.

.. note::

  :c:macro:`IRQ_CONNECT` is a macro that uses assembler and/or linker script
  tricks to connect interrupts at build time, saving boot time and text size.

The vector table should contain a handler for each interrupt and exception that
can possibly occur. The handler can be as simple as a spinning loop. However,
we strongly suggest that handlers at least print some debug information. The
information helps figuring out what went wrong when hitting an exception that
is a fault, like divide-by-zero or invalid memory access, or an interrupt that
is not expected (:dfn:`spurious interrupt`). See the ARM implementation in
:zephyr_file:`arch/arm/core/aarch32/cortex_m/fault.c` for an example.

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
:c:func:`k_thread_abort`, and setting the Kconfig option
:option:`CONFIG_ARCH_HAS_THREAD_ABORT` as needed for the architecture (e.g. see
:zephyr_file:`arch/arm/core/aarch32/cortex_m/Kconfig`).

Thread Local Storage
********************

To enable thread local storage on a new architecture:

#. Implement :c:func:`arch_tls_stack_setup` to setup the TLS storage area in
   stack. Refer to the toolchain documentation on how the storage area needs
   to be structured. Some helper functions can be used:

   * Function :c:func:`z_tls_data_size` returns the size
     needed for thread local variables (excluding any extra data required by
     toolchain and architecture).
   * Function :c:func:`z_tls_copy` prepares the TLS storage area for
     thread local variables. This only copies the variable themselves and
     does not do architecture and/or toolchain specific data.

#. In the context switching, grab the ``tls`` field inside the new thread's
   ``struct k_thread`` and put it into an appropriate register (or some
   other variable) for access to the TLS storage area. Refer to toolchain
   and architecture documentation on which registers to use.
#. In kconfig, add ``select CONFIG_ARCH_HAS_THREAD_LOCAL_STORAGE`` to
   kconfig related to the new architecture.
#. Run the ``tests/kernel/threads/tls`` to make sure the new code works.

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
:c:func:`arch_cpu_idle` and :c:func:`arch_cpu_atomic_idle`.

:c:func:`arch_cpu_idle` can be as simple as calling the power saving
instruction for the architecture with interrupts unlocked, for example
:code:`hlt` on x86, :code:`wfi` or :code:`wfe` on ARM, :code:`sleep` on ARC.
This function can be called in a loop within a context that does not care if it
get interrupted or not by an interrupt before going to sleep. There are
basically two scenarios when it is correct to use this function:

* In a single-threaded system, in the only thread when the thread is not used
  for doing real work after initialization, i.e. it is sitting in a loop doing
  nothing for the duration of the application.

* In the idle thread.

:c:func:`arch_cpu_atomic_idle`, on the other hand, must be able to atomically
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

Memory Management
*****************

If the target platform enables paging and requires drivers to memory-map
their I/O regions, :option:`CONFIG_MMU` needs to be enabled and the
:c:func:`arch_mem_map` API implemented.

Stack Objects
*************

The presence of memory protection hardware affects how stack objects are
created. All architecture ports must specify the required alignment of the
stack pointer, which is some combination of CPU and ABI requirements. This
is defined in architecture headers with :c:macro:`ARCH_STACK_PTR_ALIGN` and
is typically something small like 4, 8, or 16 bytes.

Two types of thread stacks exist:

- "kernel" stacks defined with :c:macro:`K_KERNEL_STACK_DEFINE()` and related
  APIs, which can host kernel threads running in supervisor mode or
  used as the stack for interrupt/exception handling. These have significantly
  relaxed alignment requirements and use less reserved data. No memory is
  reserved for prvilege elevation stacks.

- "thread" stacks which typically use more memory, but are capable of hosting
  thread running in user mode, as well as any use-cases for kernel stacks.

If :c:option:`CONFIG_USERSPACE` is not enabled, "thread" and "kernel" stacks
are equivalent.

Additional macros may be defined in the architecture layer to specify
the alignment of the base of stack objects, any reserved data inside the
stack object not used for the thread's stack buffer, and how to round up
stack sizes to support user mode threads. In the absence of definitions
some defaults are assumed:

- :c:macro:`ARCH_KERNEL_STACK_RESERVED`: default no reserved space
- :c:macro:`ARCH_THREAD_STACK_RESERVED`: default no reserved space
- :c:macro:`ARCH_KERNEL_STACK_OBJ_ALIGN`: default align to
  :c:macro:`ARCH_STACK_PTR_ALIGN`
- :c:macro:`ARCH_THREAD_STACK_OBJ_ALIGN`: default align to
  :c:macro:`ARCH_STACK_PTR_ALIGN`
- :c:macro:`ARCH_THREAD_STACK_SIZE_ALIGN`: default round up to
  :c:macro:`ARCH_STACK_PTR_ALIGN`

All stack creation macros are defined in terms of these.

Stack objects all have the following layout, with some regions potentially
zero-sized depending on configuration. There are always two main parts:
reserved memory at the beginning, and then the stack buffer itself. The
bounds of some areas can only be determined at runtime in the context of
its associated thread object. Other areas are entirely computable at build
time.

Some architectures may need to carve-out reserved memory at runtime from the
stack buffer, instead of unconditionally reserving it at build time, or to
supplement an existing reserved area (as is the case with the ARM FPU).
Such carve-outs will always be tracked in ``thread.stack_info.start``.
The region specified by	``thread.stack_info.start`` and
``thread.stack_info.size`` is always fully accessible by a user mode thread.
``thread.stack_info.delta`` denotes an offset which can be used to compute
the initial stack pointer from the very end of the stack object, taking into
account storage for TLS and ASLR random offsets.

::

	+---------------------+ <- thread.stack_obj
	| Reserved Memory     | } K_(THREAD|KERNEL)_STACK_RESERVED
	+---------------------+
	| Carved-out memory   |
	|.....................| <- thread.stack_info.start
	| Unused stack buffer |
	|                     |
	|.....................| <- thread's current stack pointer
	| Used stack buffer   |
	|                     |
	|.....................| <- Initial stack pointer. Computable
	| ASLR Random offset  |      with thread.stack_info.delta
	+---------------------| <- thread.userspace_local_data
	| Thread-local data   |
	+---------------------+ <- thread.stack_info.start +
	                             thread.stack_info.size


At present, Zephyr does not support stacks that grow upward.

No Memory Protection
====================

If no memory protection is in use, then the defaults are sufficient.

HW-based stack overflow detection
=================================

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

Two forms of HW-based stack overflow detection are supported: dedicated
CPU features for this purpose, or special read-only guard regions immediately
preceding stack buffers.

:option:`CONFIG_HW_STACK_PROTECTION` only catches stack overflows for
supervisor threads. This is not required to catch stack overflow from user
threads; :option:`CONFIG_USERSPACE` is orthogonal.

This feature only detects supervisor mode stack overflows, including stack
overflows when handling system calls. It doesn't guarantee that the kernel has
not been corrupted. Any stack overflow in supervisor mode should be treated as
a fatal error, with no assertions about the integrity of the overall system
possible.

Stack overflows in user mode are recoverable (from the kernel's perspective)
and require no special configuration; :option:`CONFIG_HW_STACK_PROTECTION`
only applies to catching overflows when the CPU is in sueprvisor mode.

CPU-based stack overflow detection
----------------------------------

If we are detecting stack overflows in supervisor mode via special CPU
registers (like ARM's SPLIM), then the defaults are sufficient.



Guard-based stack overflow detection
------------------------------------

We are detecting supervisor mode stack overflows via special memory protection
region located immediately before the stack buffer that generates an exception
on write. Reserved memory will be used for the guard region.

:c:macro:`ARCH_KERNEL_STACK_RESERVED` should be defined to the minimum size
of a memory protection region. On most ARM CPUs this is 32 bytes.
:c:macro:`ARCH_KERNEL_STACK_OBJ_ALIGN` should also be set to the required
alignment for this region.

MMU-based systems should not reserve RAM for the guard region and instead
simply leave an non-present virtual page below every stack when it is mapped
into the address space. The stack object will still need to be properly aligned
and sized to page granularity.

::

   +-----------------------------+ <- thread.stack_obj
   | Guard reserved memory       | } K_KERNEL_STACK_RESERVED
   +-----------------------------+
   | Guard carve-out             |
   |.............................| <- thread.stack_info.start
   | Stack buffer                |
   .                             .

Guard carve-outs for kernel stacks are uncommon and should be avoided if
possible. They tend to be needed for two situations:

* The same stack may be re-purposed to host a user thread, in which case
  the guard is unnecessary and shouldn't be unconditionally reserved.
  This is the case when privilege elevation stacks are not inside the stack
  object.

* The required guard size is variable and depends on context. For example, some
  ARM CPUs have lazy floating point stacking during exceptions and may
  decrement the stack pointer by a large value without writing anything,
  completely overshooting a minimally-sized guard and corrupting adjacent
  memory. Rather than unconditionally reserving a larger guard, the extra
  memory is carved out if the thread uses floating point.

User mode enabled
=================

Enabling user mode activates two new requirements:

* A separate fixed-sized privilege mode stack, specified by
  :option:`CONFIG_PRIVILEGED_STACK_SIZE`, must be allocated that the user
  thread cannot access. It is used as the stack by the kernel when handling
  system calls. If stack guards are implemented, a stack guard region must
  be able to be placed before it, with support for carve-outs if necessary.

* The memory protection hardware must be able to program a region that exactly
  covers the thread's stack buffer, tracked in ``thread.stack_info``. This
  implies that :c:macro:`ARCH_THREAD_STACK_SIZE_ADJUST()` will need to round
  up the requested stack size so that a region may cover it, and that
  :c:macro:`ARCH_THREAD_STACK_OBJ_ALIGN()` is also specified per the
  granularity of the memory protection hardware.

This becomes more complicated if the memory protection hardware requires that
all memory regions be sized to a power of two, and aligned to their own size.
This is common on older MPUs and is known with
:option:`CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT`.

``thread.stack_info`` always tracks the user-accessible part of the stack
object, it must always be correct to program a memory protection region with
user access using the range stored within.

Non power-of-two memory region requirements
-------------------------------------------

On systems without power-of-two region requirements, the reserved memory area
for threads stacks defined by :c:macro:`K_THREAD_STACK_RESERVED` may be used to
contain the privilege mode stack. The layout could be something like:

::

   +------------------------------+ <- thread.stack_obj
   | Other platform data          |
   +------------------------------+
   | Guard region (if enabled)    |
   +------------------------------+
   | Guard carve-out (if needed)  |
   |..............................|
   | Privilege elevation stack    |
   +------------------------------| <- thread.stack_obj +
   | Stack buffer                 |      K_THREAD_STACK_RESERVED =
   .                              .      thread.stack_info.start

The guard region, and any carve-out (if needed) would be configured as a
read-only region when the thread is created.

* If the thread is a supervisor thread, the privilege elevation region is just
  extra stack memory. An overflow will eventually crash into the guard region.

* If the thread is running in user mode, a memory protection region will be
  configured to allow user threads access to the stack buffer, but nothing
  before or after it. An overflow in user mode will crash into the privilege
  elevation stack, which the user thread has no access to. An overflow when
  handling a system call will crash into the guard region.

On an MMU system there should be no physical guards; the privilege mode stack
will be mapped into kernel memory, and the stack buffer in the user part of
memory, each with non-present virtual guard pages below them to catch runtime
stack overflows.

Other platform data may be stored before the guard region, but this is highly
discouraged if such data could be stored in ``thread.arch`` somewhere.

:c:macro:`ARCH_THREAD_STACK_RESERVED` will need to be defined to capture
the size of the reserved region containing platform data, privilege elevation
stacks, and guards. It must be appropriately sized such that an MPU region
to grant user mode access to the stack buffer can be placed immediately
after it.

Power-of-two memory region requirements
---------------------------------------

Thread stack objects must be sized and aligned to the same power of two,
without any reserved memory to allow efficient packing in memory. Thus,
any guards in the thread stack must be completely carved out, and the
privilege elevation stack must be allocated elsewhere.

:c:macro:`ARCH_THREAD_STACK_SIZE_ADJUST()` and
:c:macro:`ARCH_THREAD_STACK_OBJ_ALIGN()` should both be defined to
:c:macro:`Z_POW2_CEIL()`. :c:macro:`K_THREAD_STACK_RESERVED` must be 0.

For the privilege stacks, the :option:`CONFIG_GEN_PRIV_STACKS` must be,
enabled. For every thread stack found in the system, a corresponding fixed-
size kernel stack used for handling system calls is generated. The address
of the privilege stacks can be looked up quickly at runtime based on the
thread stack address using :c:func:`z_priv_stack_find()`. These stacks are
laid out the same way as other kernel-only stacks.

::

   +-----------------------------+ <- z_priv_stack_find(thread.stack_obj)
   | Reserved memory             | } K_KERNEL_STACK_RESERVED
   +-----------------------------+
   | Guard carve-out (if needed) |
   |.............................|
   | Privilege elevation stack   |
   |                             |
   +-----------------------------+ <- z_priv_stack_find(thread.stack_obj) +
                                        K_KERNEL_STACK_RESERVED +
                                        CONFIG_PRIVILEGED_STACK_SIZE

   +-----------------------------+ <- thread.stack_obj
   | MPU guard carve-out         |
   | (supervisor mode only)      |
   |.............................| <- thread.stack_info.start
   | Stack buffer                |
   .                             .

The guard carve-out in the thread stack object is only used if the thread is
running in supervisor mode. If the thread drops to user mode, there is no guard
and the entire object is used as the stack buffer, with full access to the
associated user mode thread and ``thread.stack_info`` updated appropriately.

User Mode Threads
*****************

To support user mode threads, several kernel-to-arch APIs need to be
implemented, and the system must enable the :option:`CONFIG_ARCH_HAS_USERSPACE`
option. Please see the documentation for each of these functions for more
details:

* :c:func:`arch_buffer_validate` to test whether the current thread has
  access permissions to a particular memory region

* :c:func:`arch_user_mode_enter` which will irreversibly drop a supervisor
  thread to user mode privileges. The stack must be wiped.

* :c:func:`arch_syscall_oops` which generates a kernel oops when system
  call parameters can't be validated, in such a way that the oops appears to be
  generated from where the system call was invoked in the user thread

* :c:func:`arch_syscall_invoke0` through
  :c:func:`arch_syscall_invoke6` invoke a system call with the
  appropriate number of arguments which must all be passed in during the
  privilege elevation via registers.

* :c:func:`arch_is_user_context` return nonzero if the CPU is currently
  running in user mode

* :c:func:`arch_mem_domain_max_partitions_get` which indicates the max
  number of regions for a memory domain. MMU systems have an unlimited amount,
  MPU systems have constraints on this.

Some architectures may need to update software memory management structures
or modify hardware registers on another CPU when memory domain APIs are invoked.
If so, CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API must be selected by the
architecture and some additional APIs must be implemented. This is common
on MMU systems and uncommon on MPU systems:

* :c:func:`arch_mem_domain_thread_add`

* :c:func:`arch_mem_domain_thread_remove`

* :c:func:`arch_mem_domain_partition_add`

* :c:func:`arch_mem_domain_partition_remove`

* :c:func:`arch_mem_domain_destroy`

Please see the doxygen documentation of these APIs for details.

In addition to implementing these APIs, there are some other tasks as well:

* :c:func:`_new_thread` needs to spawn threads with :c:macro:`K_USER` in
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
  :c:enum:`K_SYSCALL_BAD` handler. Upon completion of the system call, care
  must be taken not to leak any register state back to user mode.

API Reference
*************

Timing
======

.. doxygengroup:: arch-timing
   :project: Zephyr

Threads
=======

.. doxygengroup:: arch-threads
   :project: Zephyr

.. doxygengroup:: arch-tls
   :project: Zephyr

Power Management
================

.. doxygengroup:: arch-pm
   :project: Zephyr

Symmetric Multi-Processing
==========================

.. doxygengroup:: arch-smp
   :project: Zephyr

Interrupts
==========

.. doxygengroup:: arch-irq
   :project: Zephyr

Userspace
=========

.. doxygengroup:: arch-userspace
   :project: Zephyr

Memory Management
=================

.. doxygengroup:: arch-mmu
   :project: Zephyr

Miscellaneous Architecture APIs
===============================

.. doxygengroup:: arch-misc
   :project: Zephyr
