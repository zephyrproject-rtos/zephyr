.. SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
.. SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
.. SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
..
.. SPDX-License-Identifier: Apache-2.0

.. _tricore_developer_guide:

TriCore AURIX Developer Guide
#############################

Overview
********

This page describes the status of the Infineon TriCore AURIX architecture port
in Zephyr and the aspects that matter when developing Zephyr applications for
AURIX platforms.

The `TriCore <https://www.infineon.com/cms/en/product/microcontroller/32-bit-tricore-microcontroller/>`_
architecture is a unified 32-bit microcontroller-DSP architecture for real-time
embedded systems. It combines microcontroller real-time capability, DSP
computational power and RISC load/store performance in a single core.

The port covers two AURIX SoC families:

- `AURIX TC3xx <https://www.infineon.com/products/microcontroller/32-bit-tricore/aurix-tc3xx>`_
  using the TC1.6.2P ISA
- `AURIX TC4x <https://www.infineon.com/products/microcontroller/32-bit-tricore/aurix-tc4x>`_
  using the TC1.8P ISA

Both are currently supported through QEMU boards only, see
:ref:`tricore_qemu_boards`.

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
     - N
     - N
   * - **Interrupt handling**
     -
     -
   * - Regular interrupts (SW ISR table)
     - Y
     - Y
   * - Nested interrupts
     - Y
     - Y
   * - Dynamic interrupts
     - Y
     - Y
   * - Direct interrupts
     - N
     - N
   * - CPU idling
     - Y
     - Y
   * - **Memory protection**
     -
     -
   * - MPU
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
     - N
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

The TC1.8P cores of TC4x implement a hypervisor extension and start in
virtualization mode. The boot code drops to the default virtual machine when
the SoC selects :kconfig:option:`CONFIG_CPU_HAS_TRICORE_VIRTUALIZATION`.

Context Save Areas (CSA)
========================

The TriCore architecture saves and restores context in hardware using Context
Save Areas (CSAs). Each CSA is a 64-byte memory block aligned to a 64-byte
boundary. CSAs are linked together to form two lists:

- **Free Context List (FCX)**: Pool of available CSAs
- **Previous Context List (PCX)**: Chain of saved contexts for the current
  execution path

The register set is split into an upper context and a lower context:

- **Upper context** (saved automatically on interrupts, traps and calls):
  A[10]-A[15], D[8]-D[15], PSW and PCXI.
- **Lower context** (saved explicitly with the SVLCX or BISR instructions):
  A[2]-A[7], D[0]-D[7], A[11] and PCXI.

Software therefore does not push and pop registers on context switches,
interrupt entry or function calls.

In Zephyr, the CSA pool is a static array of
:kconfig:option:`CONFIG_TRICORE_CSA_COUNT` entries. The boot code in
:file:`arch/tricore/core/reset.S` links all CSAs into the free list and
programs the FCX and LCX registers. LCX is set
:kconfig:option:`CONFIG_TRICORE_CSA_FCD_RESERVE` entries before the end of the
list, so the free context list depletion (FCD) trap handler has CSAs left to
report the fault and abort the thread.

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
The PSW.IS bit tells whether the processor runs on the thread stack or on
the interrupt stack. On interrupt or trap entry with PSW.IS == 0, the hardware
loads A[10] from the Interrupt Stack Pointer (ISP) register and sets
PSW.IS = 1.

In Zephyr, a single interrupt stack is shared among all exceptions and
interrupts. The size is controlled by :kconfig:option:`CONFIG_ISR_STACK_SIZE`.

Thread context switching
========================

Context switching uses the ``SYSCALL`` instruction. ``arch_switch()`` issues
``syscall 1``, which enters the system call trap handler in supervisor mode.
The handler saves the lower context and calls ``z_tricore_switch()``, which:

- stores the current PCXI, the head of the outgoing thread's CSA chain, in
  the outgoing thread,
- loads the PCXI of the incoming thread and returns.

The handler then restores the lower context of the incoming thread with RSLCX
and returns with RFE, which restores its upper context.

When a thread aborted itself, the kernel hands it over in ``to_reclaim`` and
the handler returns its CSA chain to the free list after the switch.

A context switch is also triggered from interrupt context. ``_isr_wrapper``
asks the scheduler for the next thread after the outermost interrupt returns
(``nested == 0``) and no higher-priority interrupt is pending
(``ICR.PIPN == 0``).

The implementation is in :file:`arch/tricore/core/switch.S`,
:file:`arch/tricore/core/syscall_wrapper.S` and
:file:`arch/tricore/core/isr_wrapper.S`.

Interrupt handling
==================

Interrupt priority
------------------

The TriCore architecture supports up to 255 interrupt priority levels.
Each interrupt source is assigned a Service Request Priority Number (SRPN)
in its Service Request Control (SRC) register of the Interrupt Router (IR).
The ICR (Interrupt Control Register) holds the Current CPU Priority
Number (CCPN) and the Pending Interrupt Priority Number (PIPN).

An interrupt is taken when ``ICR.IE == 1`` and ``PIPN > CCPN``.

On interrupt entry, the hardware:

- saves the upper context to a CSA,
- sets ``PCXI.PIE = ICR.IE`` and ``PCXI.PCPN = ICR.CCPN``,
- sets ``ICR.CCPN = ICR.PIPN`` and ``ICR.IE = 0``,
- loads A[10] from ISP if not already on the interrupt stack,
- jumps to the vector entry at ``BIV + 32 * PIPN``.

Zephyr provides 256 vector entries with 32-byte spacing. Each entry saves the
lower context and jumps to ``_isr_wrapper``, which reads the active interrupt
number from the interrupt router and dispatches through the software ISR
table. ``_isr_wrapper`` re-enables interrupts before calling the handler, so
a higher-priority interrupt can nest.

On return from interrupt (RFE), the hardware restores the upper context and
the previous ICR state from PCXI.

The vectors are in :file:`arch/tricore/core/vectors.c`.

Locking and unlocking IRQs
--------------------------

``arch_irq_lock()`` returns the current ICR value as the key and executes the
``DISABLE`` instruction, which clears ICR.IE. ``arch_irq_unlock()`` executes
``ENABLE`` if ICR.IE was set in the key.

Dynamic interrupts
------------------

Interrupts can be connected at run time through the software ISR table with
:kconfig:option:`CONFIG_DYNAMIC_INTERRUPTS`.

CPU idling
==========

``arch_cpu_idle()`` enables interrupts and ``arch_cpu_atomic_idle()`` restores
the interrupt state from its key; both then execute ``WAIT``, and the core
wakes on the next interrupt.

The implementation is in :file:`arch/tricore/core/cpu_idle.c`.

Trap handling
=============

The TriCore architecture defines 8 trap classes with 32-byte vector spacing,
addressed via the BTV (Base Trap Vector) register:

- Class 0: MMU traps
- Class 1: Internal protection traps (privileged instruction, memory
  protection, global register write protection)
- Class 2: Instruction errors (illegal or unimplemented opcode, invalid
  operand, alignment, invalid local memory address)
- Class 3: Context management (free context depletion, call depth overflow)
- Class 4: System bus and peripheral errors
- Class 5: Assertion traps
- Class 6: System call (SYSCALL instruction)
- Class 7: Non-Maskable Interrupt (NMI)

Zephyr routes trap classes 0 to 5 and 7 through ``z_tricore_fault()``, which
logs the trap class, the Trap Identification Number (TIN) and a register dump
and then calls ``z_fatal_error()``. A free context list underflow (FCU, class
3 TIN 4) leaves no CSA to handle the trap and halts the core.

The system call trap (class 6) is used for context switching (``syscall 1``),
raising exceptions (``syscall 2``) and IRQ offloading (``syscall 3``).

The implementation is in :file:`arch/tricore/core/fatal.c` and
:file:`arch/tricore/core/vectors.c`.

Floating point
==============

The TriCore FPU is integrated into the data path and operates on the
D[0]-D[15] data registers. There are no separate FPU data registers. The FPU
rounding mode and exception flags are held in the PSW, which is saved and
restored as part of the upper context on interrupts, traps and calls.

The FPU trap control and capture registers (``FPU_TRAP_CON`` and the
``FPU_TRAP_*`` registers after it) are not part of the context. Zephyr does
not enable the FPU traps, so the FPU state that matters for threads is in the
saved contexts and needs no extra handling in multi-threaded applications.

- TC3xx (TC1.6.2P): Single-precision FPU (IEEE 754)
- TC4x (TC1.8P): Single-precision and double-precision FPU (IEEE 754)

Toolchain
*********

The TriCore port builds with the AURIX GCC toolchain provided by Infineon,
version 11.3.1:
`Infineon AURIX GCC <https://softwaretools.infineon.com/assets/com.ifx.tb.tool.aurixgcc>`_

.. code-block:: console

   export ZEPHYR_TOOLCHAIN_VARIANT=tricore
   export TRICORE_TOOLCHAIN_PATH=/path/to/tricore-gcc

``TRICORE_TOOLCHAIN_PATH`` defaults to ``/opt/toolchains/tricore-elf``.

.. note::

   The Zephyr SDK does not include a TriCore toolchain.

.. _tricore_qemu_boards:

Supported boards
****************

QEMU boards
===========

+----------------------------------------------+----------+------------------+
| Board                                        | SoC      | ISA              |
+----------------------------------------------+----------+------------------+
| :zephyr:board:`qemu_tc3x`                    | TC39x    | TC1.6.2P         |
+----------------------------------------------+----------+------------------+
| :zephyr:board:`qemu_tc4x`                    | TC4Dx    | TC1.8P           |
+----------------------------------------------+----------+------------------+

QEMU for TriCore AURIX is maintained at
`linumiz/qemu-aurix <https://gitlab.com/linumiz/infineon/release/qemu-aurix>`_, which adds
AURIX peripheral emulation. The board documentation describes how to set it up
and run an application.

Drivers
*******

The following table lists the peripheral drivers available in the TriCore port:

+-----------------------------------+-----------+-----------+
| Driver                            | **TC3xx** | **TC4x**  |
+-----------------------------------+-----------+-----------+
| Interrupt Router (IR)             |     Y     |     Y     |
+-----------------------------------+-----------+-----------+
| System Timer (STM)                |     Y     |     Y     |
+-----------------------------------+-----------+-----------+
| UART (ASCLIN), polling mode only  |     Y     |     Y     |
+-----------------------------------+-----------+-----------+

Architecture references
***********************

- `TC1.8 Architecture Manual Volume 1 <https://www.infineon.com/assets/row/public/documents/10/44/infineon-infineon-tricore-tc1.8-architecture-usermanual-en.pdf>`_
- `TC1.8 Architecture Manual Volume 2 <https://www.infineon.com/assets/row/public/documents/10/44/infineon-tricore-tc1.8-architecture-volume2-usermanual-en.pdf>`_
- `TC1.6 Architecture Manual Volume 1 <https://www.infineon.com/assets/row/public/documents/10/44/infineon-aurix-architecture-vol1-usermanual-en.pdf>`_
- `TC1.6 Architecture Manual Volume 2 <https://www.infineon.com/assets/row/public/documents/10/44/infineon-aurix-architecture-vol2-usermanual-en.pdf>`_

Maintainers and collaborators
*****************************

The status of the TriCore AURIX architecture port in Zephyr is: *maintained*.

The list of maintainers and collaborators can be found in
:file:`MAINTAINERS.yml` under the ``TriCore arch`` and
``Infineon AURIX Platforms`` areas.
