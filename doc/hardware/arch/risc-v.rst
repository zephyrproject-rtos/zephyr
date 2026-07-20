Zephyr support status on RISC-V processors
##########################################

Overview
********

This page describes current state of Zephyr for RISC-V processors.
Currently, there's support for some boards, as well as Qemu support
and support for some FPGA implementations such as neorv32 and
litex_vexriscv.

Zephyr support includes PMP, MMU, :ref:`user mode<usermode_api>`, several
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

Memory Management Unit (MMU)
*****************************

When :kconfig:option:`CONFIG_RISCV_MMU` is enabled (requires
:kconfig:option:`CONFIG_RISCV_S_MODE`), Zephyr uses hardware page-table
address translation.  The following virtual addressing modes are supported:

- **Sv39** (:kconfig:option:`CONFIG_RISCV_MMU_SV39`): 39-bit virtual
  addresses, 3-level page tables.  Suitable for most 64-bit embedded
  targets.
- **Sv48** (:kconfig:option:`CONFIG_RISCV_MMU_SV48`): 48-bit virtual
  addresses, 4-level page tables.  For platforms requiring a larger
  virtual address space.

The addressing mode is selected automatically from devicetree: it is derived
from the ``mmu-type`` property of the RISC-V CPU node(s) (for example
``mmu-type = "riscv,sv39"`` or ``"riscv,sv48"``), which selects the matching
:kconfig:option:`CONFIG_RISCV_MMU_SV39` / :kconfig:option:`CONFIG_RISCV_MMU_SV48`
option.  :kconfig:option:`CONFIG_RISCV_MMU` itself is only available when every
enabled hart declares an ``mmu-type`` other than ``riscv,none``.

The kernel maintains an identity mapping (VA = PA) for its own code and
data so that the transition from physical to virtual addressing is seamless
at boot.

Userspace (:kconfig:option:`CONFIG_USERSPACE`) and memory domains
(:kconfig:option:`CONFIG_MEM_DOMAIN_ISOLATED_STACKS`) are fully supported
with the MMU.  Each memory domain gets a private root page table.  The
kernel ``.text`` and ``.rodata`` regions are mapped with ``PTE_USER=1``
in domain page tables so user threads can execute kernel entry points
(``z_thread_entry``, syscall stubs), while the reset vector and exception
entry pages keep ``PTE_USER=0``.  16-bit ASIDs are used for efficient
per-domain TLB management, avoiding full TLB flushes on domain switches.
MMU-based stack guard pages are supported via
:kconfig:option:`CONFIG_HW_STACK_PROTECTION`.

MMU and S-mode current limitations
************************************

The following limitations apply to the current
:kconfig:option:`CONFIG_RISCV_MMU` and :kconfig:option:`CONFIG_RISCV_S_MODE`
implementation:

- **No SMP support in S-mode**: Secondary harts are not brought up in S-mode.
  ``arch_proc_id()`` always returns 0.  S-mode SMP requires passing the hart
  ID via ``a0`` at boot and per-hart initialisation of the SBI stack and
  page tables.  TLB shootdown on mapping changes is also not implemented;
  ``sfence.vma`` only flushes the local hart's TLB.

- **No demand paging**: All virtual memory mappings are established at boot.
  Page-fault driven demand paging (``CONFIG_DEMAND_PAGING``) is not
  implemented.

- **No huge pages**: Only 4 KB leaf pages are used.  2 MB (megapage) and
  1 GB (gigapage) superpage mappings are not supported.

- **Static page table pool**: The number of page tables is fixed at build
  time via :kconfig:option:`CONFIG_MAX_XLAT_TABLES`.  There is no dynamic
  allocation of page table memory at runtime.

- **ASID exhaustion**: ASID values are allocated monotonically and never
  recycled.  Creating more than 65533 memory domains triggers a kernel
  panic.  This is sufficient for all practical embedded workloads but a
  future improvement should implement ASID recycling with a generation
  counter and full TLB flush.

SMP support
***********

SMP is supported on RISC-V for both QEMU-virtualized and hardware-based
platforms. In order to test the SMP support, one can use
:zephyr:board:`qemu_riscv32` or :zephyr:board:`qemu_riscv64` for QEMU-based
platforms, or :zephyr:board:`beaglev_fire` or :zephyr:board:`mpfs_icicle` for
hardware-based platforms.

.. _OpenSBI: https://github.com/riscv-software-src/opensbi
