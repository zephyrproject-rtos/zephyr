.. _arm_cortex_m_developer_guide:

Arm Cortex-M Developer Guide
############################

Overview
********

This page contains detailed information about the status of the Arm Cortex-M
architecture porting in the Zephyr RTOS and describes key aspects when
developing Zephyr applications for Arm Cortex-M-based platforms.

Key supported features
**********************

The table below summarizes the status of key OS features in the different
Arm Cortex-M implementation variants.


+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
|                                 |                                   | **Processor families**                                                                      |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| Architecture variant            |                                   | Arm v6-M                  | Arm v7-M                    | Arm v8-M             | Arm v8.1-M |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
|                                 |                                   | **M0/M1**       | **M0+** | **M3** |   **M4**  | **M7** | **M23** |   **M33**  |  **M55**   |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| **OS Features**                 |                                   |                                                                                             |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| Programmable fault              |                                   |                 |         |        |           |        |         |            |            |
| IRQ priorities                  |                                   |        Y        |   N     |   Y    |    Y      |    Y   |    N    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| Single-thread kernel support    |                                   |        Y        |   Y     |   Y    |    Y      |    Y   |    Y    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| Thread local storage support    |                                   |        Y        |   Y     |   Y    |    Y      |    Y   |    Y    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| Interrupt handling              |                                   |                                                                                             |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
|                                 |   Regular interrupts              |        Y        |   Y     |   Y    |    Y      |    Y   |    Y    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
|                                 |   Dynamic interrupts              |        Y        |   Y     |   Y    |    Y      |    Y   |    Y    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
|                                 |   Direct  interrupts              |        Y        |   Y     |   Y    |    Y      |    Y   |    Y    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
|                                 |   Zero Latency interrupts         |        N        |   N     |   Y    |    Y      |    Y   |    Y    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| CPU idling                      |                                   |        Y        |   Y     |   Y    |    Y      |    Y   |    Y    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| Native system timer (SysTick)   |                                   |        N [#f1]_ |   Y     |   Y    |    Y      |    Y   |    Y    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| Memory protection               |                                   |                                                                                             |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
|                                 |   User mode                       |        N        |   Y     |   Y    |    Y      |    Y   |    Y    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
|                                 |   HW stack protection (MPU)       |        N        |   N     |   Y    |    Y      |    Y   |    Y    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
|                                 | HW-assisted stack limit checking  |        N        |   N     |   N    |    N      |    N   |Y [#f2]_ |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| HW-assisted null-pointer        |                                   |                 |         |        |           |        |         |            |            |
| dereference detection           |                                   |        N        |   N     |   Y    |    Y      |    Y   |    Y    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| HW-assisted atomic operations   |                                   |        N        |   N     |   Y    |    Y      |    Y   |    N    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
|Support for non-cacheable regions|                                   |        N        |   N     |   Y    |    Y      |    Y   |    N    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| Execute SRAM functions          |                                   |        N        |   N     |   Y    |    Y      |    Y   |    N    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| Floating Point Services         |                                   |        N        |   N     |   N    |    Y      |    Y   |    N    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| DSP ISA                         |                                   |        N        |   N     |   N    |    Y      |    Y   |    N    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| Trusted-Execution               |                                                                                                                                 |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
|                                 | Native TrustZone-M support        |        N        |   N     |   N    |    N      |    N   |    Y    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
|                                 | TF-M integration                  |        N        |   N     |   N    |    N      |    N   |    N    |     Y      |   N        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| Code relocation                 |                                   |        Y        |   Y     |   Y    |    Y      |    Y   |    Y    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| SW-based vector table relaying  |                                   |        Y        |   Y     |   Y    |    Y      |    Y   |    Y    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+
| HW-assisted timing functions    |                                   |        N        |   N     |   Y    |    Y      |    Y   |    N    |     Y      |   Y        |
+---------------------------------+-----------------------------------+-----------------+---------+--------+-----------+--------+---------+------------+------------+

Notes
=====

.. [#f1] SysTick is optional in Cortex-M1
.. [#f2] Stack limit checking only in Secure builds in Cortex-M23

OS features
***********

Threads
=======

Thread stack alignment
----------------------

Each Zephyr thread is defined with its own stack memory. By default, Cortex-M enforces a double word thread stack alignment, see
:kconfig:`CONFIG_STACK_ALIGN_DOUBLE_WORD`. If MPU-based HW-assisted stack overflow detection (:kconfig:`CONFIG_MPU_STACK_GUARD`)
is enabled, thread stacks need to be aligned with a larger value, reflected by :kconfig:`CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE`.
In Arm v6-M and Arm v7-M architecture variants, thread stacks are additionally required to be align with a value equal to their size,
in applications that need to support user mode (:kconfig:`CONFIG_USERSPACE`). The thread stack sizes in that case need to be a power
of two. This is all reflected by :kconfig:`CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT`, that is enforced in Arm v6-M and Arm v7-M
builds with user mode support.

Stack pointers
--------------

While executing in thread mode the processor is using the Process Stack Pointer (PSP). The processor uses the Main Stack Pointer (MSP)
while executing in handler mode, that is, while servicing exceptions and HW interrupts. Using PSP in thread mode *facilitates thread
stack pointer manipulation* during thread context switching, without affecting the current execution context flow in
handler mode.

In Arm Cortex-M builds a single interrupt stack memory is shared among exceptions and interrupts. The size of the interrupt stack needs
to be selected taking into consideration nested interrupts, each pushing an additional stack frame. Deverlopers can modify the interrupt
stack size using :kconfig:`CONFIG_ISR_STACK_SIZE`.

The interrupt stack is also used during early boot so the kernel can initialize the main thread's stack before switching to the main thread.

Thread context switching
========================

In Arm Cortex-M builds, the PendSV exception is used in order to trigger a context switch to a different thread.
PendSV exception is always present in Cortex-M implementations. PendSV is configured with the lowest possible
interrupt priority level, in all Cortex-M variants. The main reasons for that design are

* to utilize the tail chaining feature of Cortex-M processors, and thus limit the number of context switch
  operations that occur.
* to not impact the interrupt latency observed by HW interrupts.

As a result, context switch in Cortex-M is non-atomic, i.e. it may be *preempted* by HW interrupts,
however, a context-switch operation must be completed before a new thread context-switch may start.

Typically a thread context-switch will perform the following operations

* When switching-out the current thread, the processor stores

   * the callee-saved registers (R4 - R11) in the thread's container for callee-saved registers,
     which is located in kernel memory
   * the thread's current operation *mode*

        * user or privileged execution mode
        * presense of an active floating point context
        * the EXC_RETURN value of the current handler context (PendSV)

   * the floating point callee-saved registers (S16 - S31) in the thread's container for FP
     callee-saved registers, if the current thread has an active FP context
   * the PSP of the current thread which points to the beginning of the current thread's exception
     stack frame. The latter contains the caller-saved context and the return address of the switched-out
     thread.

* When switching-in a new thread the processor

   * restores the new thread's callee-saved registers from the thread's
     container for callee-saved registers
   * restores the new thread's operation *mode*
   * restores the FP callee-saved registers if the switched-in thread had
     an active FP context before being switched-out
   * re-programs the dynamic MPU regions to allow a user thread access its stack and application
     memories, and/or programs a stack-overflow MPU guard at the bottom of the thread's
     privileged stack
   * restores the PSP for the incoming thread and re-programs the stack pointer limit
     register (if applicable, see :kconfig:`CONFIG_BUILTIN_STACK_GUARD`)
   * optionally does a stack limit checking for the switched-in thread, if
     sentinel-based stack limit checking is enabled (see :kconfig:`CONFIG_STACK_SENTINEL`).

PendSV exception return sequence restores the new thread's caller-saved registers and the
return address, as part of unstacking the exception stack frame.

The implementation of the context-switch mechanism is present in
:file:`arch/arm/core/aarch32/swap_helper.S`.

Stack limit checking (Arm v8-M)
-------------------------------

Armv8-M and Armv8.1-M variants support stack limit checking using the MSPLIM and PSPLIM
core registers. The feature is enabled when :kconfig:`CONFIG_BUILTIN_STACK_GUARD` is set.
When stack limit checking is enabled, both the thread's privileged or user stack, as well
as the interrupt stack are guarded by PSPLIM and MSPLIM registers, respectively. MSPLIM is
configured *once* during kernel boot, while PSLIM is re-programmed during every thread
context-switch or during system calls, when the thread switches from using its default
stack to using its privileged stack, and vice versa. PSPLIM re-programming

* has a relatively low runtime overhead (programming is done with MSR instructions)
* does not impact interrupt latency
* does not require any memory areas to be reserved for stack guards
* does not make use of MPU regions

It is, therefore, considered as a lightweight but very efficient stack overflow
detection mechanism in Cortex-M applications.

Stack overflows trigger the dedicated UsageFault exception provided by Arm v8-M.

Interrupt handling features
===========================

This section describes certain aspects around exception and interrupt
handling in Arm Cortex-M.

Interrupt priority levels
-------------------------

The number of available (configurable) interrupt priority levels is
determined by the number of implemented interrupt priority bits in
NVIC; this needs to be described for each Cortex-M platform using
DeviceTree:

.. code-block:: devicetree

    &nvic {
            arm,num-irq-priority-bits = <#priority-bits>;
    };


Reserved priority levels
------------------------

A number of interrupt priority levels are reserved for the OS.

By design, system fault exceptions have the highest priority level. In
*Baseline* Cortex-M, this is actually enforced by hardware, as HardFault
is the only available processor fault exception, and its priority is
higher than any configurable exception priority.

In *Mainline* Cortex-M, the available fault exceptions (e.g. MemManageFault,
UsageFault, etc.) are assigned the highest *configurable* priority level.
(:kconfig:`CONFIG_CPU_CORTEX_M_HAS_PROGRAMMABLE_FAULT_PRIOS` signifies explicitly
that the Cortex-M implementation supports configurable fault priorities.)

This priority level is never shared with HW interrupts (an exception to
this rule is described below). As a result, processor faults occurring in regular
ISRs will be handled by the corresponding fault handler and will not escalate to
a HardFault, *similar to processor faults occurring in thread mode*.

SVC exception is normally configured with the highest conigurable priority level
(an exception to this rule will be described below).
SVCs are used by the Zephyr kernel to dispatch system calls, trigger runtime
system errors (e.g. Kernel oops or panic), or implement IRQ offloading.

In Baseline Cortex-M the priority level of SVC may be shared with other exceptions
or HW interrupts that are also given the highest configurable priority level (As a
result of this, kernel runtime errors during interrupt handling will escalate to
HardFault. Additional logic in the fault handling routines ensures that such
runtime errors are detected successfully).

In Mainline Cortex-M, however, the SVC priority level is *reserved*, thus normally it
is only shared with the fault exceptions of configurable priority. This simplifies the
fault handling routines in Mainline Cortex-M architecture, since runtime kernel errors
are serviced by the SVC handler (i.e no HardFault escalation, even if the kernel errors
occur in ISR context).

HW interrupts in Mainline Cortex-M builds are allocated a priority level lower than the SVC.

One exception to the above rules is when Zephyr applications support Zero Latency Interrupts
(ZLIs). Such interrupts are designed to have a priority level higher than any HW or system
interrupt. If the ZLI feature is enabled in Mainline Cortex-M builds (see
:kconfig:`CONFIG_ZERO_LATENCY_IRQS`), then

* ZLIs are assigned the highest configurable priority level
* SVCs are assigned the second highest configurable priority level
* Regular HW interrupts are assigned priority levels lower than SVC.

The priority level configuration in Cortex-M is implemented in
:file:`include/arch/arm/aarch32/exc.h`.

Locking and unlocking IRQs
--------------------------

In Baseline Cortex-M locking interrupts is implemented using the PRIMASK register.

.. code-block:: c

  arch_irq_lock()

will set the PRIMASK register to 1, eventually, masking all IRQs with configurable
priority. While this fulfils the OS requirement of locking interrupts, the consequence
is that kernel runtime errors (triggering SVCs) will escalate to HardFault.

In Mainline Cortex-M locking interrupts is implemented using the BASEPRI register (Mainline
Cortex-M builds select :kconfig:`CONFIG_CPU_CORTEX_M_HAS_BASEPRI` to signify that BASEPRI register is
implemented.). By modifying BASEPRI (or BASEPRI_MAX) arch_irq_lock() masks all system and HW
interrupts with the exception of

* SVCs
* processor faults
* ZLIs

This allows zero latency interrupts to be triggered inside OS critical sections.
Additionally, this allows system (processor and kernel) faults to be handled by Zephyr
in *exactly the same way*, regardless of whether IRQs have been locked or not when the
error occurs. It also allows for system calls to be dispatched while IRQs are locked.

.. note::

   Mainline Cortex-M fault handling is designed and configured in a way that all processor
   and kernel faults are handled by the corresponding exception handlers and never result
   in HardFault escalation. In other words, a HardFault may only occur in Zephyr applications
   that have modified the default fault handling configurations. The main reason for this
   design was to reserve the HardFault exception for handling exceptional error conditions
   in safety critical applications.

Dynamic direct interrupts
-------------------------

Cortex-M builds support the installation of direct interrupt service routines during
runtime. Direct interrupts are designed for performance-critical interrupt
handling and do not go through all of the common Zephyr interrupt handling
code.

Direct dynamic interrupts are enabled via switching on
:kconfig:`CONFIG_DYNAMIC_DIRECT_INTERRUPTS`.

Note that enabling direct dynamic interrupts requires enabling support for
dynamic interrupts in the kernel, as well (see :kconfig:`CONFIG_DYNAMIC_INTERRUPTS`).

Zero Latency interrupts
-----------------------

As described above, in Mainline Cortex-M applications, the Zephyr kernel reserves
the highest configurable interrupt priority level for its own use (SVC). SVCs will
not be masked by interrupt locking. Zero-latency interrupt can be used to set up
an interrupt at the highest interrupt priority which will not be blocked by interrupt
locking. To use the ZLI feature :kconfig:`CONFIG_ZERO_LATENCY_IRQS` needs to be enabled.

Zero latency IRQs have minimal interrupt latency, as they will always preempt regular HW
or system interrupts.

Note, however, that since ZLI ISRs will run at a priority level higher than the kernel
exceptions they **cannot use** any kernel functionality. Additionally, since the ZLI
interrupt priority level is equal to processor fault priority level, faults occurring
in ZLI ISRs will escalate to HardFault and will not be handled in the same way as regular
processor faults. Developers need to be aware of this limitation.

CPU Idling
==========

The Cortex-M architecture port implements both k_cpu_idle()
and k_cpu_atomic_idle(). The implementation is present in
:file:`arch/arm/core/aarch32/cpu_idle.S`.

In both implementations, the processor
will attempt to put the core to low power mode.
In k_cpu_idle() the processor ends up executing WFI (Wait For Interrupt)
instruction, while in k_cpu_atomic_idle() the processor will
execute a WFE (Wait For Event) instruction.

When using the CPU idling API in Cortex-M it is important to note the
following:

* Both k_cpu_idle() and k_cpu_atomic_idle() are *assumed* to be invoked
  with interrupts locked. This is taken care of by the kernel if the APIs
  are called by the idle thread.
* After waking up from low power mode, both functions will *restore*
  interrupts unconditionally, that is, regardless of the interrupt lock
  status before the CPU idle API was called.

The Zephyr CPU Idling mechanism is detailed in :ref:`cpu_idle`.
