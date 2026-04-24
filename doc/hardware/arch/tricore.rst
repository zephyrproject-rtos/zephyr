.. _tricore_developer_guide:

TriCore AURIX Developer Guide
#############################

Overview
********

This page contains detailed information about the status of the Infineon
TriCore AURIX architecture port in the Zephyr RTOS and describes key aspects
when developing Zephyr applications for AURIX platforms.

The `TriCore <https://www.infineon.com/cms/en/product/microcontroller/32-bit-tricore-microcontroller/>`_
architecture is a unified 32-bit microcontroller-DSP architecture optimized for
real-time embedded systems. It combines microcontroller real-time capability,
DSP computational power, and RISC load/store performance in a single core.

The Zephyr port supports two AURIX SoC families:

- `AURIX TC3xx <https://www.infineon.com/products/microcontroller/32-bit-tricore/aurix-tc3xx>`_
  using the TC1.6.2P ISA
- `AURIX TC4x <https://www.infineon.com/products/microcontroller/32-bit-tricore/aurix-tc4x>`_
  using the TC1.8P ISA

Key supported features
**********************

The table below summarizes the status of key OS features across the
supported AURIX SoC families.

.. list-table:: Feature support
   :header-rows: 1
   :widths: 50 25 25

   * - Feature
     - **TC3xx**
     - **TC4x**
   * - ISA version
     - TC1.6.2P
     - TC1.8P
   * - **OS features**
     -
     -
   * - Single-thread kernel support
     - Y
     - Y
   * - Multi-thread kernel support
     - Y
     - Y
   * - Userspace
     - Y
     - Y
   * - **Interrupt handling**
     -
     -
   * - Regular interrupts (SW ISR table)
     - Y
     - Y
   * - Dynamic interrupts
     - Y
     - Y
   * - CPU idling
     - Y
     - Y
   * - **Memory protection**
     -
     -
   * - HW range-based MPU (DPR/CPR)
     - N
     - N
   * - MPU stack guard
     - N
     - N
   * - **Timer**
     -
     -
   * - System timer (STM)
     - Y
     - Y
   * - **Cache**
     -
     -
   * - Instruction/data cache control
     - Y
     - N
   * - **Floating point**
     -
     -
   * - Single-precision FPU
     - Y
     - Y
   * - Double-precision FPU
     - N
     - Y
   * - **Multiprocessing**
     -
     -
   * - SMP
     - N
     - N
   * - AMP
     - N
     - N

Architecture
************

Registers
=========

The TriCore architecture provides 32 General Purpose Registers (GPRs) split
into sixteen 32-bit data registers (D[0] through D[15]) and sixteen 32-bit
address registers (A[0] through A[15]). In addition, the Program Counter (PC),
Program Status Word (PSW) and Previous Context Information register (PCXI) form
the core system state.

Several address and data registers have dedicated roles:

- A[10]: Stack Pointer (SP)
- A[11]: Return Address (RA)
- A[0], A[1], A[8], A[9]: Global address registers
- D[15]: Implicit data register
- A[15]: Implicit base address register

The TC1.8P ISA adds the PPRS (Previous Protection Register Set) and FCX
(Free CSA List Head Pointer) as architecturally visible system registers.

Context Save Areas (CSA)
========================

A defining feature of the TriCore architecture is its hardware-managed context
save and restore mechanism using Context Save Areas (CSAs). Each CSA is a
64-byte memory block aligned to a 16-word boundary. CSAs are linked together
to form two lists:

- **Free Context List (FCX)**: Pool of available CSAs
- **Previous Context List (PCX)**: Chain of saved contexts for the current
  execution path

The register set is split into an upper context and a lower context:

- **Upper context** (automatically saved on interrupts, traps, and calls):
  A[10]-A[15], D[8]-D[15], PSW, PCXI, and the return address A[11].
- **Lower context** (saved explicitly via SVLCX or BISR instructions):
  A[2]-A[7], D[0]-D[7], and A[11].

This hardware mechanism eliminates the need for software to manually push and
pop registers during context switches, interrupt entry, or function calls.

In Zephyr, the CSA pool is allocated as a static array controlled by
:kconfig:option:`CONFIG_TRICORE_CSA_COUNT`. The boot code in ``reset.S``
initializes the free context list by linking all CSAs and programming the
FCX and LCX registers.

OS features
***********

Threads
=======

Thread stack alignment
----------------------

Zephyr threads on TriCore use an 8-byte stack alignment
(``ARCH_STACK_PTR_ALIGN = 8``), matching the EABI requirement for the
stack pointer.

Stack pointers
--------------

The TriCore architecture uses a single stack pointer register A[10].
The PSW.IS bit controls whether the processor is using the user stack or
the interrupt stack. On interrupt entry, if PSW.IS == 0, the hardware
automatically loads A[10] from the Interrupt Stack Pointer (ISP) register
and sets PSW.IS = 1.

In Zephyr, a single interrupt stack is shared among all exceptions and
interrupts. The size is controlled by :kconfig:option:`CONFIG_ISR_STACK_SIZE`.

Thread context switching
========================

In TriCore builds, context switching is triggered via the ``SYSCALL``
instruction. The ``arch_switch()`` function issues ``syscall 1`` which
enters the syscall wrapper in supervisor mode. The wrapper calls
``z_tricore_switch()`` which performs the actual context swap.

A context switch performs the following operations:

- **Switching out** the current thread:

  - Saves the current PCXI and PRS values into the thread's callee-saved
    storage.
  - If the thread is dead (aborted), its CSA chain is reclaimed back to the
    free list.

- **Switching in** the new thread:

  - Loads the saved PCXI and PRS values from the new thread.
  - Programs PCXI and PPRS (TC1.8) via MTCR instructions.
  - Returns via FRET, which restores the upper context from the CSA chain.

The context switch can also be triggered from ISR context. The
``_isr_wrapper`` checks for pending reschedule after the last nested interrupt
returns (``nested == 0``) and the hardware has no pending higher-priority
interrupt (``ICR.PIPN == 0``).

The implementation is in :file:`arch/tricore/core/switch.S` and
:file:`arch/tricore/core/isr_wrapper.S`.

Interrupt handling
==================

Interrupt priority
------------------

The TriCore architecture supports up to 255 interrupt priority levels.
Each interrupt source is assigned a Service Request Priority Number (SRPN)
via the Service Request Control registers (SRC) in the Interrupt Router (IR)
module. The ICR (Interrupt Control Register) holds the Current CPU Priority
Number (CCPN) and the Pending Interrupt Priority Number (PIPN).

An interrupt is taken when: ``ICR.IE == 1`` AND ``PIPN > CCPN``.

On interrupt entry, the hardware automatically:

- Saves the upper context to a CSA
- Sets ``PCXI.PIE = ICR.IE``, ``PCXI.PCPN = ICR.CCPN``
- Sets ``ICR.CCPN = ICR.PIPN``, ``ICR.IE = 0``
- Loads A[10] from ISP if not already on the interrupt stack
- Jumps to the vector entry calculated from BIV and PIPN

The BIV (Base Interrupt Vector) register determines the vector table base.
Two vector spacings are supported: 32-byte (VSS=0) and 8-byte (VSS=1).
Zephyr uses a single-entry BIV configuration where all interrupt priorities
vector to the common ``_isr_wrapper``, which dispatches through the
software ISR table.

On return from interrupt (RFE), the hardware restores the upper context and
the previous ICR state from PCXI.

Locking and unlocking IRQs
--------------------------

By default, ``arch_irq_lock()`` executes the ``DISABLE`` instruction which
clears ICR.IE, masking all interrupts. ``arch_irq_unlock()`` conditionally
executes ``ENABLE`` based on the saved ICR state.

Dynamic interrupts
------------------

Zephyr on TriCore supports dynamic interrupt registration through the
standard software ISR table (:kconfig:option:`CONFIG_DYNAMIC_INTERRUPTS`).
The ``_isr_wrapper`` reads the active interrupt number from the interrupt
controller and dispatches to the registered handler via ``_sw_isr_table``.

CPU idling
==========

The TriCore port implements ``arch_cpu_idle()`` and ``arch_cpu_atomic_idle()``.
Both functions put the core into a low-power idle state after unlocking
interrupts. The core wakes on the next interrupt.

Each SoC family has its own implementation:

- TC3xx: :file:`soc/infineon/aurix/tc3x/cpu_idle.c`
- TC4x: :file:`soc/infineon/aurix/tc4x/cpu_idle.c`

Trap handling
=============

The TriCore architecture defines 8 trap classes with 32-byte vector spacing,
addressed via the BTV (Base Trap Vector) register:

- Class 0: MMU traps
- Class 1: Internal protection traps (MPR, MPW, MPX, MPP)
- Class 2: Instruction errors (illegal opcode, privilege violation)
- Class 3: Context management (free context depletion, call depth overflow)
- Class 4: System bus and peripheral errors
- Class 5: Assertion traps
- Class 6: System call (SYSCALL instruction)
- Class 7: NMI (Non-Maskable Interrupt)

Zephyr routes all trap classes through ``z_tricore_fault()`` which logs
the trap class, TIN (Trap Identification Number), and full register dump
before invoking ``z_fatal_error()``.

The system call trap (class 6) is used by Zephyr for context switching
(``syscall 1``), userspace syscall dispatch (``syscall 0``), and exception
raising (``syscall 2``).

The implementation is in :file:`arch/tricore/core/fatal.c` and
:file:`arch/tricore/core/vectors.S`.

Userspace
=========

Zephyr supports userspace on TriCore using the architecture's Protection
Register Set (PRS) mechanism. The PSW.PRS field selects which set of Data
Protection Range (DPR) and Code Protection Range (CPR) registers are active.

PRS 0 is reserved for ISR and syscall context. User threads are assigned
protection register sets via their thread options. On context switch, the
PRS is reprogrammed to match the incoming thread.

System calls are dispatched via the ``SYSCALL`` instruction (trap class 6).
The syscall wrapper in :file:`arch/tricore/core/syscall_wrapper.S` switches
to the privileged stack, looks up the handler from ``_k_syscall_table``,
and calls it. On return, the original user stack and unprivileged mode
are restored.

The PSW.IO field controls the I/O privilege level: User-0 (no peripheral
access), User-1 (configurable), or Supervisor (full access). Zephyr sets
User-1 mode for user threads and Supervisor mode for kernel threads.

.. note::

   Userspace support requires the range-based memory protection hardware
   (DPR/CPR registers) to be configured. The MPU driver is not yet available
   in the initial port but the syscall and privilege-switching infrastructure
   is functional.

Floating point
==============

The TriCore FPU is integrated into the data path and operates on the
standard D[0]-D[15] data registers. There are no separate FPU register
banks. FPU state (rounding mode, exception flags) is held in the PSW
register which is automatically saved and restored as part of the upper
context on interrupts, traps, and calls.

Because the FPU context is part of the standard upper context CSA, there is
no additional overhead or configuration needed for floating point in
multi-threaded environments. Every thread inherently preserves its FPU state
through the normal CSA save/restore mechanism.

- TC3xx (TC1.6.2P): Single-precision FPU (IEEE 754)
- TC4x (TC1.8P): Single-precision and double-precision FPU (IEEE 754)

Toolchain
*********

The TriCore port uses the AURIX GCC toolchain provided by Infineon. The
toolchain is available at:
`Infineon AURIX GCC <https://softwaretools.infineon.com/assets/com.ifx.tb.tool.aurixgcc>`_

The current supported version is GCC 11.3.1.

Configure the toolchain as follows:

.. code-block:: console

   export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
   export CROSS_COMPILE=/path/to/tricore-gcc/bin/tricore-elf-

.. note::

   Zephyr SDK integration for TriCore is planned for the future.

C library
=========

The port uses picolibc (software floating point). Newlib is available in the
toolchain if needed.

.. note::

   Native picolibc support for the TriCore target is in progress upstream.

Supported boards
****************

Hardware boards
===============

+----------------------------------------------+----------+------------------+
| Board                                        | SoC      | ISA              |
+----------------------------------------------+----------+------------------+
| :zephyr:board:`kit_a2g_tc375_lite`           | TC375    | TC1.6.2P         |
+----------------------------------------------+----------+------------------+
| :zephyr:board:`kit_a2g_tc397xa_3v3_tft`      | TC397    | TC1.6.2P         |
+----------------------------------------------+----------+------------------+
| :zephyr:board:`kit_a3g_tc4d7_lite`           | TC4D7    | TC1.8P           |
+----------------------------------------------+----------+------------------+

QEMU boards
============

+----------------------------------------------+----------+------------------+
| Board                                        | SoC      | ISA              |
+----------------------------------------------+----------+------------------+
| :zephyr:board:`qemu_tc3x`                    | TC397    | TC1.6.2P         |
+----------------------------------------------+----------+------------------+
| :zephyr:board:`qemu_tc4x`                    | TC4D7    | TC1.8P           |
+----------------------------------------------+----------+------------------+

QEMU for TriCore AURIX is maintained at
`linumiz/qemu-tricore <https://github.com/linumiz/qemu-tricore>`_.
This is currently the only QEMU fork with AURIX peripheral emulation
support.

The following table shows emulated features per QEMU target:

.. list-table:: QEMU emulated features
   :header-rows: 1
   :widths: 50 25 25

   * - Feature
     - **qemu_tc3x**
     - **qemu_tc4x**
   * - Interrupt Router (IR)
     - Y
     - Y
   * - UART (ASCLIN)
     - Y
     - Y
   * - System Timer (STM)
     - Y
     - Y
   * - GPIO
     - N
     - N
   * - Clock Control (CCU)
     - Y
     - Y
   * - Watchdog (WDT)
     - Y
     - Y

Drivers
*******

The following table lists the peripheral drivers available in the TriCore port:

+---------------------------+-----------+-----------+
| Driver                    | **TC3xx** | **TC4x**  |
+---------------------------+-----------+-----------+
| UART (ASCLIN)             |     Y     |     Y     |
+---------------------------+-----------+-----------+
| GPIO                      |     Y     |     Y     |
+---------------------------+-----------+-----------+
| System Timer (STM)        |     Y     |     Y     |
+---------------------------+-----------+-----------+
| Clock Control (CCU)       |     Y     |     Y     |
+---------------------------+-----------+-----------+
| Pin Control               |     Y     |     Y     |
+---------------------------+-----------+-----------+
| Interrupt Router (IR)     |     Y     |     Y     |
+---------------------------+-----------+-----------+

.. note::

   GPIO interrupt support is available on TC4x (via ERU and EGTM) but not
   yet on TC3x.

Flashing and debugging
**********************

Flashing and debugging AURIX devices can be done using a variety of
commercial tools. A commonly used combination is
`iSYSTEM winIDEA <https://www.isystem.com/products/software/winidea.html>`_
together with the
`AURIX Development Studio (ADS) <https://softwaretools.infineon.com/assets/com.ifx.tb.tool.aurixide>`_.

A full list of supported flash programming tools is available at:
`Infineon AURIX Flash Tools <https://www.infineon.com/design-resources/platforms/aurix-software-tools/aurix-tools/flash>`_

Architecture references
***********************

- `TC1.8 Architecture Manual Volume 1 <https://www.infineon.com/assets/row/public/documents/10/44/infineon-infineon-tricore-tc1.8-architecture-usermanual-en.pdf>`_
- `TC1.8 Architecture Manual Volume 2 <https://www.infineon.com/assets/row/public/documents/10/44/infineon-tricore-tc1.8-architecture-volume2-usermanual-en.pdf>`_
- `TC1.6 Architecture Manual Volume 1 <https://www.infineon.com/assets/row/public/documents/10/44/infineon-aurix-architecture-vol1-usermanual-en.pdf>`_
- `TC1.6 Architecture Manual Volume 2 <https://www.infineon.com/assets/row/public/documents/10/44/infineon-aurix-architecture-vol2-usermanual-en.pdf>`_

Maintainers and collaborators
*****************************

The status of the TriCore AURIX architecture port in Zephyr is: *maintained*.

- Christoph Seitz, `Infineon Technologies <https://www.infineon.com>`_ (`@go2sh <https://github.com/infineon>`_)
- Parthiban Nallathambi, `Linumiz <https://linumiz.com>`_ (`@parthitce <https://github.com/linumiz>`_)
