Zephyr support status on RISC-V processors
##########################################

Overview
********

This page describes current state of Zephyr for RISC-V processors.
Currently, there's support for some boards, as well as Qemu support
and support for some FPGA implementations such as neorv32 and
litex_vexriscv.

Zephyr support includes PMP, :ref:`user mode<usermode_api>`, several
ISA extensions as well as :ref:`semihosting<semihost_guide>`.

User mode and PMP support
**************************

When the platform has Physical Memory Protection (PMP) support, enabling
it on Zephyr allows user space support and stack protection to be
selected.

ISA extensions
**************

It's possible to set in Zephyr which ISA extensions (RV32/64I(E)MAFD(G)QC)
are available on a given platform, by setting the appropriate ``CONFIG_RISCV_ISA_*``
kconfig. Look at :file:`arch/riscv/Kconfig.isa` for more information.

Note that Zephyr SDK toolchain support may not be defined for all
combinations.

Supervisor mode (S-mode)
************************

When :kconfig:option:`CONFIG_RISCV_S_MODE` is enabled, the Zephyr kernel
runs in RISC-V Supervisor mode (S-mode) rather than Machine mode (M-mode).
This follows the standard RISC-V privilege architecture used by
application-class operating systems and is required for MMU support.

A minimal in-tree M-mode runtime (``arch/riscv/core/sbi.S``) acts as
firmware.  It performs the initial M-mode to S-mode transition at boot and
handles S-mode requests via the RISC-V Supervisor Binary Interface (SBI):

- **Boot sequence**: M-mode configures PMP, ``medeleg``, ``mideleg``,
  ``mcounteren`` and ``mtvec``, then drops to S-mode via ``mret``.
- **Timer**: Machine timer interrupts (MTIP) are forwarded to S-mode as
  supervisor timer interrupts (STIP).  S-mode programs the next timer
  deadline through the SBI ``TIME`` extension (``sbi_set_timer``).
- **Ecalls**: S-mode ecalls (exception cause 9) are routed to the M-mode
  runtime via ``medeleg``; all other exceptions and interrupts are delegated
  to S-mode.  See :ref:`riscv-smode-ecall-constraint` below.

No external firmware is required; the in-tree runtime is self-contained.

Known limitations
=================

**PLIC external interrupts**: The PLIC driver always configures the M-mode
context for hart 0.  In S-mode the correct context is the supervisor context,
so PLIC-delivered external interrupts (UART, GPIO, SPI, I2C, etc.) do not
reach the CPU.  The timer interrupt is unaffected as it is forwarded directly
by the M-mode runtime via ``sip.STIP``, bypassing the PLIC.

.. _riscv-smode-ecall-constraint:

S-mode ecall constraint
=======================

In M-mode, Zephyr uses the ``ecall`` instruction as a kernel-internal
self-trap: because the kernel owns M-mode unconditionally, ``ecall`` raises
exception cause 11 (M-mode ecall), which ``_isr_wrapper`` dispatches to
whatever kernel service was requested (fatal error, ``irq_offload``, context
switch).

In S-mode this mechanism is unavailable.  ``ecall`` from S-mode always raises
cause 9, and cause 9 is **not** delegated to S-mode (``medeleg`` bit 9 = 0)
because it must reach the M-mode SBI handler — that is the only path by which
S-mode can request M-mode services such as programming the timer via
``sbi_set_timer``.  The ``medeleg`` bit offers no per-register-value
discrimination: delegating cause 9 to S-mode would break SBI; keeping it in
M-mode makes cause 9 unavailable for kernel-internal use.

The practical rule for S-mode ports is:

   ``ecall`` is reserved for the SBI interface.  Any kernel mechanism that
   uses ``ecall`` in M-mode must be reimplemented with a direct S-mode path.

Two concrete examples in this port:

- :c:macro:`ARCH_EXCEPT` (``include/zephyr/arch/riscv/error.h``) — instead
  of issuing ``ecall`` to trigger a fatal-error entry through ``_isr_wrapper``,
  it calls :c:func:`z_riscv_fatal_error` directly.

- ``arch_irq_offload`` (``arch/riscv/core/irq_offload.c``) — instead of
  issuing ``ecall`` to enter ``_isr_wrapper``'s ``do_irq_offload`` path, it
  simulates ISR context directly: disables interrupts, increments
  ``nested``, calls the routine, decrements ``nested``, re-enables interrupts,
  then calls :c:func:`z_reschedule_unlocked` to handle any thread that became
  ready inside the routine.

Support for using an external SBI implementation such as `OpenSBI`_ as a
West module is left as future work.

SMP support
***********

SMP is supported on RISC-V for both QEMU-virtualized and hardware-based
platforms. In order to test the SMP support, one can use
:zephyr:board:`qemu_riscv32` or :zephyr:board:`qemu_riscv64` for QEMU-based
platforms, or :zephyr:board:`beaglev_fire` or :zephyr:board:`mpfs_icicle` for
hardware-based platforms.

.. _OpenSBI: https://github.com/riscv-software-src/opensbi
