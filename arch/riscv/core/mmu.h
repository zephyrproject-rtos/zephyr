/*
 * Copyright (c) 2026 Alexios Lyrakis <alexios.lyrakis@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RISCV_CORE_MMU_H_
#define ZEPHYR_ARCH_RISCV_CORE_MMU_H_

/* This header is only valid when MMU is enabled */
#ifndef CONFIG_RISCV_MMU
#error "This header requires CONFIG_RISCV_MMU to be enabled"
#endif

#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/riscv/csr.h>
#include <stdbool.h>

/* Set below flag to get debug prints */
#define MMU_DEBUG_PRINTS 0

#if MMU_DEBUG_PRINTS
#define MMU_DEBUG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define MMU_DEBUG(...)
#endif

/*
 * RISC-V Virtual Memory Layout (assumes 4KB page size)
 *
 * Sv39 (39-bit virtual address):
 * +------------+------------+------------+-----------+
 * | VA [38:30] | VA [29:21] | VA [20:12] | VA [11:0] |
 * +------------------------------------------------+
 * |    L0      |    L1      |    L2      | page off  |
 * +------------+------------+------------+-----------+
 *
 * Sv48 (48-bit virtual address):
 * +------------+------------+------------+------------+-----------+
 * | VA [47:39] | VA [38:30] | VA [29:21] | VA [20:12] | VA [11:0] |
 * +-----------------------------------------------------------+
 * |    L0      |    L1      |    L2      |    L3      | page off  |
 * +------------+------------+------------+------------+-----------+
 */

/*
 * SATP (Supervisor Address Translation and Protection) Register Layout
 *
 * RV64 SATP (64-bit):
 * +--------+--------+-------------------------------------+
 * | 63:60  | 59:44  |               43:0                  |
 * |  MODE  |  ASID  |               PPN                   |
 * +--------+--------+-------------------------------------+
 *
 * RV32 SATP (32-bit):
 * +----+--------+---------------------------+
 * | 31 | 30:22  |          21:0             |
 * |MODE|  ASID  |          PPN              |
 * +----+--------+---------------------------+
 *
 * MODE: Translation mode (Off, Sv32, Sv39, Sv48, Sv57)
 * ASID: Address Space Identifier (process/context ID)
 * PPN:  Physical Page Number of root page table.
 *       Stores physical_address / PAGE_SIZE (a page frame number).
 *       Physical address of root table = PPN * PAGE_SIZE.
 *       RV64: 44-bit field (bits 43:0), RV32: 22-bit field (bits 21:0).
 */

/*
 * PTE (Page Table Entry) Layout - RISC-V 64-bit
 *
 * +----------+---------------------+-----+---+---+---+---+---+---+---+---+
 * | 63:54    |       53:10         | 9:8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 * | RESERVED |        PPN          | RSW | D | A | G | U | X | W | R | V |
 * +----------+---------------------+-----+---+---+---+---+---+---+---+---+
 *
 * V: Valid - PTE is valid
 * R: Read - Page is readable
 * W: Write - Page is writable
 * X: Execute - Page is executable
 * U: User - Page is accessible in User mode
 * G: Global - Translation is valid for all address spaces (ignores ASID)
 * A: Accessed - Page has been accessed (read/written/executed)
 * D: Dirty - Page has been written to
 * RSW: Reserved for Software Use (bits 8-9)
 * PPN: Physical Page Number (44-bit, supports up to 56-bit physical addresses)
 * RESERVED: Reserved for future RISC-V extensions
 *
 * PTE Types:
 * - Leaf PTE (page): V=1, and at least one of R/W/X is 1
 * - Non-leaf PTE (table): V=1, R=W=X=0 (points to next page table level)
 * - Invalid PTE: V=0
 */

/* RISC-V supports multiple page sizes; starting with 4KB base pages */
#define PAGE_SIZE_SHIFT 12U
#define PAGE_SIZE       BIT(PAGE_SIZE_SHIFT)
#define PAGE_MASK       (PAGE_SIZE - 1)

/* RISC-V page table constants */
#define RISCV_PGLEVEL_BITS    9U
#define RISCV_PGTABLE_ENTRIES BIT(RISCV_PGLEVEL_BITS) /* 512 entries */

/* SATP register definitions */
#if defined(CONFIG_64BIT)

#define SATP_PPN_SHIFT  0U
#define SATP_ASID_SHIFT 44U
#define SATP_MODE_SHIFT 60U
#define SATP_ASID_MASK  GENMASK64(59, 44)
#define SATP_PPN_MASK   GENMASK64(43, 0)

#else /* !CONFIG_64BIT */

#define SATP_PPN_SHIFT  0U
#define SATP_ASID_SHIFT 22U
#define SATP_MODE_SHIFT 31U
#define SATP_ASID_MASK  GENMASK(30, 22)
#define SATP_PPN_MASK   GENMASK(21, 0)

#endif /* CONFIG_64BIT */

/* Current SATP mode based on configuration */
#if defined(CONFIG_RISCV_MMU_SV39)
#define SATP_MODE      SATP_MODE_SV39
#define VA_BITS        39
#define PGTABLE_LEVELS 3
#elif defined(CONFIG_RISCV_MMU_SV48)
#define SATP_MODE      SATP_MODE_SV48
#define VA_BITS        48
#define PGTABLE_LEVELS 4
#else
/* Default to SV39 if MMU is enabled but mode not specified */
#define SATP_MODE      SATP_MODE_SV39
#define VA_BITS        39
#define PGTABLE_LEVELS 3
#endif

/* Calculate level shift based on page level */
#define RISCV_PGLEVEL_SHIFT(level)                                                                 \
	(PAGE_SIZE_SHIFT + RISCV_PGLEVEL_BITS * (PGTABLE_LEVELS - 1 - (level)))

/* Extract VPN (Virtual Page Number) at specific level */
#define VPN_MASK         GENMASK(RISCV_PGLEVEL_BITS - 1, 0)
#define VPN(addr, level) (((addr) >> RISCV_PGLEVEL_SHIFT(level)) & VPN_MASK)

/* Page Table Entry (PTE) definitions */
#define PTE_VALID    BIT(0)
#define PTE_READ     BIT(1)
#define PTE_WRITE    BIT(2)
#define PTE_EXEC     BIT(3)
#define PTE_USER     BIT(4)
#define PTE_GLOBAL   BIT(5)
#define PTE_ACCESSED BIT(6)
#define PTE_DIRTY    BIT(7)

/* Software-defined PTE bits (bits 8-9 are reserved for software use) */
#define PTE_SW_WRITABLE BIT(8) /* Actually writable (for demand paging) */
#define PTE_SW_RESERVED BIT(9) /* Reserved for future use */

/* PTE permission combinations */
#define PTE_TYPE_TABLE (PTE_VALID) /* flags for a non-leaf (table) PTE */
#define PTE_PAGE_READ  (PTE_VALID | PTE_READ | PTE_ACCESSED)
#define PTE_PAGE_WRITE (PTE_VALID | PTE_READ | PTE_WRITE | PTE_ACCESSED | PTE_DIRTY)
#define PTE_PAGE_EXEC  (PTE_VALID | PTE_EXEC | PTE_ACCESSED)
#define PTE_PAGE_RW    (PTE_VALID | PTE_READ | PTE_WRITE | PTE_ACCESSED | PTE_DIRTY)
#define PTE_PAGE_RX    (PTE_VALID | PTE_READ | PTE_EXEC | PTE_ACCESSED)
#define PTE_PAGE_RWX   (PTE_VALID | PTE_READ | PTE_WRITE | PTE_EXEC | PTE_ACCESSED | PTE_DIRTY)

/* Check if PTE is a leaf (page) or branch (table) */
#define PTE_IS_LEAF(pte)  ((pte) & (PTE_READ | PTE_WRITE | PTE_EXEC))
#define PTE_IS_TABLE(pte) (((pte) & PTE_VALID) && !PTE_IS_LEAF(pte))

/* Extract/create PPN (Physical Page Number) */
/* PTE_PPN_SHIFT (10) is defined in csr.h */
#define PTE_PPN_MASK      GENMASK64(53, 10)
#define PTE_TO_PPN(pte)   (((pte) & PTE_PPN_MASK) >> PTE_PPN_SHIFT)
#define PPN_TO_PTE(ppn)   (((ppn) << PTE_PPN_SHIFT) & PTE_PPN_MASK)
#define PTE_TO_PHYS(pte)  (PTE_TO_PPN(pte) << PAGE_SIZE_SHIFT)
#define PHYS_TO_PTE(phys) PPN_TO_PTE((phys) >> PAGE_SIZE_SHIFT)

/* Address conversion macros */
#define VA_TO_VPN_INDEX(va, level) VPN(va, level)
#define PA_TO_PPN(pa)              ((pa) >> PAGE_SIZE_SHIFT)
#define PPN_TO_PA(ppn)             ((ppn) << PAGE_SIZE_SHIFT)

/* TLB flush operations */
#define SFENCE_VMA_ALL()      __asm__ volatile("sfence.vma" ::: "memory")
#define SFENCE_VMA_ADDR(addr) __asm__ volatile("sfence.vma %0" ::"r"(addr) : "memory")
#define SFENCE_VMA_ASID(asid) __asm__ volatile("sfence.vma zero, %0" ::"r"(asid) : "memory")
#define SFENCE_VMA_ADDR_ASID(addr, asid)                                                           \
	__asm__ volatile("sfence.vma %0, %1" ::"r"(addr), "r"(asid) : "memory")

/* RISC-V CSR access macros */
#define READ_SATP()     csr_read(satp)
#define WRITE_SATP(val) csr_write(satp, val)

/* Page table entry type - 64-bit on both RV32 and RV64 for Sv32+ modes */
typedef uint64_t pte_t;

#ifndef _ASMLANGUAGE

/* Number of page table entries per table */
#define PTRS_PER_PTE RISCV_PGTABLE_ENTRIES

/* Convert between various address formats */
static inline uintptr_t pte_to_phys(pte_t pte)
{
	return PTE_TO_PHYS(pte);
}

static inline pte_t phys_to_pte(uintptr_t phys)
{
	return PHYS_TO_PTE(phys);
}

static inline pte_t pte_create(uintptr_t phys, uint64_t flags)
{
	return phys_to_pte(phys) | flags;
}

/* Check PTE status */
static inline bool pte_is_valid(pte_t pte)
{
	return !!(pte & PTE_VALID);
}

static inline bool pte_is_leaf(pte_t pte)
{
	return PTE_IS_LEAF(pte);
}

static inline bool pte_is_table(pte_t pte)
{
	return PTE_IS_TABLE(pte);
}

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_RISCV_CORE_MMU_H_ */
