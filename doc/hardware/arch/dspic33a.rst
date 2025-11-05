.. _dspic33a_developer_guide:

Microchip dsPIC33A Developer Guide
##################################

Overview
********

This page contains detailed information about the status of the Microchip dsPIC33A
architecture porting in the Zephyr RTOS and describes key aspects when
developing Zephyr applications for dsPIC33A-based platforms.

The dsPIC33A family is a 32-bit digital signal controller (DSC) family from
Microchip Technology featuring a high-performance 32-bit CPU core with DSP
capabilities, integrated FPU, and rich peripheral set. The family is designed
for real-time control applications requiring signal processing capabilities.

Key supported features
**********************

The table below summarizes the status of key OS features in dsPIC33A implementation.

+---------------------------------+-----------------------------------+
| **OS Features**                 | **Status**                        |
+=================================+===================================+
| **Threading**                   |                                   |
+---------------------------------+-----------------------------------+
| Single-thread kernel support    | Y                                 |
+---------------------------------+-----------------------------------+
| Multi-threading                 | Y                                 |
+---------------------------------+-----------------------------------+
| Thread local storage support    | Y                                 |
+---------------------------------+-----------------------------------+
| Thread stack alignment          | 4-byte (word) alignment           |
+---------------------------------+-----------------------------------+
| **Interrupt handling**          |                                   |
+---------------------------------+-----------------------------------+
| Regular interrupts              | Y                                 |
+---------------------------------+-----------------------------------+
| Dynamic interrupts              | N                                 |
+---------------------------------+-----------------------------------+
| Direct interrupts               | N                                 |
+---------------------------------+-----------------------------------+
| Zero Latency interrupts         | N                                 |
+---------------------------------+-----------------------------------+
| **Hardware features**           |                                   |
+---------------------------------+-----------------------------------+
| CPU idling                      | Y                                 |
+---------------------------------+-----------------------------------+
| Hardware stack protection       | Y (SPLIM register)                |
+---------------------------------+-----------------------------------+
| Floating Point Unit (FPU)       | Y                                 |
+---------------------------------+-----------------------------------+
| DSP ISA                         | Y                                 |
+---------------------------------+-----------------------------------+
| Hardware-assisted atomic        |                                   |
| operations                      | Y (limited set)                   |
+---------------------------------+-----------------------------------+
| **Memory protection**           |                                   |
+---------------------------------+-----------------------------------+
| User mode                       | N                                 |
+---------------------------------+-----------------------------------+
| MPU (Memory Protection Unit)    | N                                 |
+---------------------------------+-----------------------------------+
| HW-assisted stack limit         |                                   |
| checking                        | Y (SPLIM register)                |
+---------------------------------+-----------------------------------+
| **System features**             |                                   |
+---------------------------------+-----------------------------------+
| System timer                    | Y (Timer-based)                   |
+---------------------------------+-----------------------------------+
| Tickless kernel                 | Y                                 |
+---------------------------------+-----------------------------------+
| Vector table relocation         | Y                                 |
+---------------------------------+-----------------------------------+

Architecture Overview
*********************

The dsPIC33A architecture port supports the following processor families:

* **dsPIC33AK128MC106** - 128 KB Flash, 16 KB RAM
* **dsPIC33AK512MPS512** - 512 KB Flash, 64 KB RAM

Key architectural characteristics:

* **32-bit CPU core** with DSP instructions
* **Upward-growing stack** (stack grows from low to high addresses)
* **Little-endian** byte ordering
* **32-bit wide instruction** bus for improved performance
* **Modified Harvard architecture** allowing data access from program memory

OS features
***********

Threads
=======

Thread stack alignment
----------------------

Each Zephyr thread is defined with its own stack memory. The dsPIC33A architecture
enforces a word (4-byte) thread stack alignment by default. This alignment is
required for proper stack frame handling and register saving during context switches.

Thread stacks are aligned according to :kconfig:option:`CONFIG_STACK_ALIGN_DOUBLE_WORD`
if enabled, otherwise defaulting to word alignment.

Stack pointers
--------------

The dsPIC33A uses a unified stack pointer (W15) that grows upward. During thread
execution, the processor uses the thread's stack pointer. During interrupt
handling, the processor uses the thread stack.

The dsPIC33A architecture implements hardware stack protection using the Stack
Pointer Limit (SPLIM) register. When enabled via :kconfig:option:`CONFIG_HW_STACK_PROTECTION`,
the SPLIM register is configured for each thread to detect stack overflow conditions.
A stack overflow triggers a hardware trap, which is handled by the kernel's
fatal error handler.

Thread stack size is configurable per thread. The default interrupt stack size
is set to 128 bytes via :kconfig:option:`CONFIG_ISR_STACK_SIZE`, but can be
adjusted based on application requirements.

Thread context switching
========================

Thread context switching in dsPIC33A builds is performed using a cooperative
swap mechanism. The context switch operation saves and restores the following
registers:

* **Callee-saved registers**: W0-W13 (32-bit working registers)
* **Stack pointer**: W15 (32-bit)
* **Frame pointer**: W14 (Link Register, 32-bit)
* **Program Counter**: PC (32-bit)
* **Status register**: FSR (File Select Register, 32-bit)
* **Floating-point registers**: FCR (if FPU context is present)
* **Repeat counter**: RCOUNT (for DSP operations)
* **Accumlators**: ACCA, ACCB (72 bit)

The context switch implementation handles:

* **Register preservation**: All callee-saved registers are saved to the thread's
  stack frame structure
* **Stack pointer management**: Thread stack pointer is updated during context switch
* **Interrupt state**: Interrupt enable/disable state is preserved per thread
* **Return value propagation**: Thread swap return values are properly handled

Context switching is atomic and may be preempted by hardware interrupts,
however, a context-switch operation must be completed before a new thread
context-switch may start.

Interrupt handling
==================

The dsPIC33A architecture supports vectored interrupts through the Interrupt
Controller (INTC). The interrupt system provides:

* **Interrupt sources** (depends on the DIM, configurable via :kconfig:option:`CONFIG_NUM_IRQS`)
* **Interrupt priority levels**: Multiple priority levels supported
* **Software-triggerable interrupts**: For irq_offload() functionality
* **Dynamic interrupt vector table**: Generated at build time

Interrupt vector table
----------------------

The interrupt vector table is generated at build time and placed in program memory.
The vector table contains entries for all configured interrupts, pointing to
corresponding interrupt service routines (ISRs).

The vector table base address can be configured, and the table is aligned according
to :kconfig:option:`CONFIG_ARCH_IRQ_VECTOR_TABLE_ALIGN` (default: 4 bytes).

Interrupt priority levels
-------------------------

The dsPIC architecture supports **8 interrupt priority levels (0-7)** to determine
the urgency of interrupt handling.

* **Priority 0:** Lowest priority
* **Priority 4:** Default priority
* **Priority 7:** Highest priority

The Zephyr kernel uses ``ARCH_IRQ_CONNECT()`` to register and configure ISRs.
During initialization, each interrupt source is assigned a priority.

Interrupt locking and unlocking
-------------------------------

In the dsPIC architecture, interrupt locking and unlocking are managed through
the **Global Interrupt Enable (GIE)** bit in the **Status Register (SR)**.
When **GIE = 0**, all maskable interrupts are disabled
when **GIE = 1**, interrupts are globally enabled.

.. code-block:: c

  arch_irq_lock()

Disables global interrupts by clearing the GIE bit and returns a key.

.. code-block:: c

  arch_irq_unlock()

Restores the global interrupt state using the key returned by ``arch_irq_lock()``.

IRQ offload
-----------

The architecture supports IRQ offload functionality for executing kernel
work in interrupt context. A dedicated interrupt (default: IRQ 34) is
configured via :kconfig:option:`CONFIG_DSPIC33_IRQ_OFFLOAD_IRQ` for this purpose.

Memory model
============

Memory layout
-------------

The dsPIC33A uses a Harvard architecture with separate program and data memory:

* **Program Memory (Flash)**: Stores code and constant data
  * Base address configurable via :kconfig:option:`CONFIG_FLASH_BASE_ADDRESS`
  * Size configurable via :kconfig:option:`CONFIG_FLASH_SIZE`
  * Executable code and read-only data reside in program memory
  * 32-bit addressable address space

* **Data Memory (RAM)**: Stores variables and stack
  * Base address configurable via :kconfig:option:`CONFIG_SRAM_BASE_ADDRESS`
  * Size configurable via :kconfig:option:`CONFIG_SRAM_SIZE`
  * Stack, heap, and data sections reside in data memory
  * 32-bit addressable address space

Memory protection
------------------

The dsPIC33A does not include a Memory Protection Unit (MPU), therefore user
mode (:kconfig:option:`CONFIG_USERSPACE`) is not supported. However, hardware
stack protection is available using the SPLIM register.

Stack protection
================

Hardware stack protection is implemented using the Stack Pointer Limit (SPLIM)
register. When enabled via :kconfig:option:`CONFIG_HW_STACK_PROTECTION`:

* Each thread's SPLIM register is configured to point to the end of its stack
* Stack overflow detection triggers a hardware trap
* The trap handler reports the error and can halt the system

The stack protection feature automatically selects :kconfig:option:`CONFIG_THREAD_STACK_INFO`
to track stack boundaries.

Floating Point Unit (FPU)
==========================

The dsPIC33A includes an integrated Floating Point Unit supporting:

* **Single-precision floating-point operations** (32-bit IEEE 754)
* **Hardware acceleration** for common floating-point operations
* **Automatic FPU context saving** during context switches (when FPU is used)

The FPU is enabled by default when available on the target device. FPU support
is controlled via :kconfig:option:`CONFIG_FPU`.

DSP Capabilities
================

The dsPIC33A architecture includes Digital Signal Processing (DSP) instructions:

* **DSP multiply-accumulate (MAC)** operations
* **Saturating arithmetic** operations
* **DSP register file** for efficient signal processing
* **Barrel shifter** for efficient bit manipulation

DSP support is enabled via :kconfig:option:`CONFIG_CPU_HAS_DSP`.

System clock
============
The dsPIC33A architecture provides a configurable hardware system clock that
serves as the primary timing source for the processor and peripheral subsystems.
This clock drives both the CPU core and the system timer used by the Zephyr kernel.
The system clock frequency is configured via :kconfig:option:`CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC`
(default: 8 MHz).

System timer
============
The dsPIC33A architecture uses a hardware timer to generate the system tick
for Zephyr’s kernel timing operations. The system timer provides the basis for
thread scheduling, timeouts, and delay handling the tick rate is configured via
:kconfig:option:`CONFIG_SYS_CLOCK_TICKS_PER_SEC` (default: 10000 ticks per second).
The system timer provides timer APIs for threads and applications that can be used
without needing a separate hardware timer. Applications can use kernel timing APIs
such as
:c:func:`k_sleep()` and :c:func:`k_msleep()` - delay execution for a specified time.
:c:func:`z_thread_timeout()` - set thread timeouts.
:c:func:`k_timer_init()`, :c:func:`k_timer_start()`, :c:func:`k_timer_stop()` - manage
periodic and one-shot software timers and can be used to set up
periodic and one-shot timers without requiring a separate hardware timer peripheral.
This eliminates the need for applications to configure and manage dedicated hardware
timers for basic timing operations, as the kernel handles all timer functionality
through the system timer.

Tickless kernel support
-----------------------

The architecture supports tickless kernel operation via :kconfig:option:`CONFIG_TICKLESS_KERNEL`,
allowing the system to enter low-power states between timer interrupts.

Exception handling
==================

The dsPIC33A architecture supports several hardware exception types:

* **Bus Error**: Invalid bus access detected
* **Address Error**: Invalid address access
* **Illegal Instruction**: Undefined instruction execution
* **Math Error**: Floating-point or DSP operation errors
* **Stack Error**: Stack overflow/underflow detected
* **General Trap**: Software-generated traps

All exception handlers are implemented in :file:`arch/dspic/core/fatal.c` and
log the error reason and address before halting the system.

CPU idle
========

The architecture provides CPU idle functionality for power management:

* **arch_cpu_idle()**: Enters low-power idle state
* **arch_cpu_atomic_idle()**: Enters idle state with interrupt key management

The idle implementation uses the dsPIC33A's built-in ``Idle()`` instruction,
which places the CPU in a low-power state until an interrupt occurs.



Vector table relocation
=======================

The interrupt vector table can be relocated to different memory addresses
to support bootloader scenarios or memory remapping. Vector table relocation
is controlled via :kconfig:option:`CONFIG_ARCH_HAS_VECTOR_TABLE_RELOCATION`
and is enabled by default for dsPIC33A architecture.

Thread Local Storage (TLS)
==========================

The dsPIC33A architecture supports Thread Local Storage (TLS) for per-thread
variables. TLS is implemented using the toolchain's TLS support
(:kconfig:option:`CONFIG_THREAD_LOCAL_STORAGE`).

TLS setup is performed during thread initialization in :file:`arch/dspic/core/tls.c`,
which aligns the TLS area and initializes TLS data/bss sections.

Thread abort
============

The architecture supports thread abort functionality via :kconfig:option:`CONFIG_ARCH_HAS_THREAD_ABORT`,
allowing threads to be terminated by the kernel. When a thread is aborted,
its resources are cleaned up and the thread is removed from the scheduling queues.

Build configuration
===================

Key configuration options for dsPIC33A architecture:

* :kconfig:option:`CONFIG_ARCH` - Set to "dspic"
* :kconfig:option:`CONFIG_ISR_STACK_SIZE` - Interrupt stack size (default: 128)
* :kconfig:option:`CONFIG_DSPIC33_IRQ_OFFLOAD_IRQ` - IRQ offload interrupt number (default: 34)
* :kconfig:option:`CONFIG_HW_STACK_PROTECTION` - Enable hardware stack protection
* :kconfig:option:`CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC` - System clock frequency
* :kconfig:option:`CONFIG_SYS_CLOCK_TICKS_PER_SEC` - System tick rate

Toolchain support
=================

The dsPIC33A architecture requires the Microchip XC-DSC (Extended C - Digital Signal
Controller) toolchain for compilation. The toolchain provides:

* **32-bit dsPIC33 instruction set** support
* **32-bit register** operations
* **DSP instruction** support
* **FPU instruction** support
* **PIC30 library** support for runtime functions

The toolchain variant is specified via ``-DZEPHYR_TOOLCHAIN_VARIANT=xcdsc``
during build configuration.

Known limitations
=================

* **Dynamic interrupts**: Not supported (dynamic interrupt installation not available)
* **User mode**: Not supported (no MPU available)
* **Zero-latency interrupts**: Not supported (all interrupts go through ISR wrapper)
* **MPU**: Not available on dsPIC33A architecture
* **Cache**: Not available (Harvard architecture without cache)

References
==========

* `dsPIC33AK128MC106 Family Data Sheet <https://ww1.microchip.com/downloads/aemDocuments/documents/MCU16/ProductDocuments/DataSheets/dsPIC33AK128MC106-Family-Data-Sheet-DS70005539.pdf>`_
* `dsPIC33AK512MPS512 Family Data Sheet <https://ww1.microchip.com/downloads/aemDocuments/documents/MCU16/ProductDocuments/DataSheets/dsPIC33AK512MPS512-Family-Data-Sheet-DS70005591.pdf>`_
* `Microchip XC-DSC Compiler User's Guide <https://ww1.microchip.com/downloads/en/DeviceDoc/50002053E.pdf>`_
