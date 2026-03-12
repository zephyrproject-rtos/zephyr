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
This is required for MMU support and follows the standard RISC-V privilege
architecture used by application-class operating systems.

A minimal in-tree M-mode runtime (``arch/riscv/core/sbi.S``) acts as
firmware.  It performs the initial M-mode to S-mode transition at boot and
handles S-mode requests via the RISC-V Supervisor Binary Interface (SBI):

- **Boot sequence**: M-mode configures PMP, ``medeleg``, ``mideleg``,
  ``mcounteren`` and ``mtvec``, then drops to S-mode via ``mret``.
- **Timer**: Machine timer interrupts (MTIP) are forwarded to S-mode as
  supervisor timer interrupts (STIP).  S-mode programs the next timer
  deadline through the SBI ``TIME`` extension (``sbi_set_timer``).
- **Ecalls**: S-mode ecalls are handled by the M-mode runtime; all other
  exceptions and interrupts are delegated to S-mode.

No external firmware (e.g. OpenSBI) is required.

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
