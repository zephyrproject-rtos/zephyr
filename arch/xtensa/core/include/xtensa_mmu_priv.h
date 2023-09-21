/*
 * Xtensa MMU support
 *
 * Private data declarations
 *
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_XTENSA_XTENSA_MMU_PRIV_H_
#define ZEPHYR_ARCH_XTENSA_XTENSA_MMU_PRIV_H_

#include <stdint.h>
#include <xtensa/config/core-isa.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util_macro.h>

#define Z_XTENSA_PTE_VPN_MASK 0xFFFFF000U
#define Z_XTENSA_PTE_PPN_MASK 0xFFFFF000U
#define Z_XTENSA_PTE_ATTR_MASK 0x0000000FU
#define Z_XTENSA_L1_MASK 0x3FF00000U
#define Z_XTENSA_L2_MASK 0x3FFFFFU

#define Z_XTENSA_PPN_SHIFT 12U

#define Z_XTENSA_PTE_RING_MASK 0x00000030U

#define Z_XTENSA_PTE(paddr, ring, attr) \
	(((paddr) & Z_XTENSA_PTE_PPN_MASK) | \
	(((ring) << 4) & Z_XTENSA_PTE_RING_MASK) | \
	((attr) & Z_XTENSA_PTE_ATTR_MASK))

#define Z_XTENSA_TLB_ENTRY(vaddr, way) \
	(((vaddr) & Z_XTENSA_PTE_PPN_MASK) | (way))

#define Z_XTENSA_AUTOFILL_TLB_ENTRY(vaddr) \
	(((vaddr) & Z_XTENSA_PTE_PPN_MASK) | \
	 (((vaddr) >> Z_XTENSA_PPN_SHIFT) & 0x03U))

#define Z_XTENSA_L2_POS(vaddr) \
	(((vaddr) & Z_XTENSA_L2_MASK) >> Z_XTENSA_PPN_SHIFT)

/* Kernel specific ASID. Ring field in the PTE */
#define Z_XTENSA_KERNEL_RING 0

/* Number of data TLB ways [0-9] */
#define Z_XTENSA_DTLB_WAYS 10

/* Number of instruction TLB ways [0-6] */
#define Z_XTENSA_ITLB_WAYS 7

/* Number of auto-refill ways */
#define Z_XTENSA_TLB_AUTOREFILL_WAYS 4


/* PITLB HIT bit. For more information see
 * Xtensa Instruction Set Architecture (ISA) Reference Manual
 * 4.6.5.7 Formats for Probing MMU Option TLB Entries
 */
#define Z_XTENSA_PITLB_HIT BIT(3)

/* PDTLB HIT bit. For more information see
 * Xtensa Instruction Set Architecture (ISA) Reference Manual
 * 4.6.5.7 Formats for Probing MMU Option TLB Entries
 */
#define Z_XTENSA_PDTLB_HIT BIT(4)

/*
 * Virtual address where the page table is mapped
 */
#define Z_XTENSA_PTEVADDR CONFIG_XTENSA_MMU_PTEVADDR

/*
 * Find the pte entry address of a given vaddr.
 *
 * For example, assuming PTEVADDR in 0xE0000000,
 * the page spans from 0xE0000000 - 0xE03FFFFF

 *
 * address 0x00 is in 0xE0000000
 * address 0x1000 is in 0xE0000004
 * .....
 * address 0xE0000000 (where the page is) is in 0xE0380000
 *
 * Generalizing it, any PTE virtual address can be calculated this way:
 *
 * PTE_ENTRY_ADDRESS = PTEVADDR + ((VADDR / 4096) * 4)
 */
#define Z_XTENSA_PTE_ENTRY_VADDR(vaddr) \
	(Z_XTENSA_PTEVADDR + (((vaddr) / KB(4)) * 4))

/*
 * The address of the top level page where the page
 * is located in the virtual address.
 */
#define Z_XTENSA_PAGE_TABLE_VADDR \
	Z_XTENSA_PTE_ENTRY_VADDR(Z_XTENSA_PTEVADDR)

static ALWAYS_INLINE void xtensa_rasid_set(uint32_t rasid)
{
	__asm__ volatile("wsr %0, rasid\n\t"
			"isync\n" : : "a"(rasid));
}

static ALWAYS_INLINE uint32_t xtensa_rasid_get(void)
{
	uint32_t rasid;

	__asm__ volatile("rsr %0, rasid" : "=a"(rasid));
	return rasid;
}

static ALWAYS_INLINE void xtensa_itlb_entry_invalidate(uint32_t entry)
{
	__asm__ volatile("iitlb %0\n\t"
			: : "a" (entry));
}

static ALWAYS_INLINE void xtensa_itlb_entry_invalidate_sync(uint32_t entry)
{
	__asm__ volatile("iitlb %0\n\t"
			"isync\n\t"
			: : "a" (entry));
}

static ALWAYS_INLINE void xtensa_dtlb_entry_invalidate_sync(uint32_t entry)
{
	__asm__ volatile("idtlb %0\n\t"
			"dsync\n\t"
			: : "a" (entry));
}

static ALWAYS_INLINE void xtensa_dtlb_entry_invalidate(uint32_t entry)
{
	__asm__ volatile("idtlb %0\n\t"
			: : "a" (entry));
}

static ALWAYS_INLINE void xtensa_dtlb_entry_write_sync(uint32_t pte, uint32_t entry)
{
	__asm__ volatile("wdtlb %0, %1\n\t"
			"dsync\n\t"
			: : "a" (pte), "a"(entry));
}

static ALWAYS_INLINE void xtensa_dtlb_entry_write(uint32_t pte, uint32_t entry)
{
	__asm__ volatile("wdtlb %0, %1\n\t"
			: : "a" (pte), "a"(entry));
}

static ALWAYS_INLINE void xtensa_itlb_entry_write(uint32_t pte, uint32_t entry)
{
	__asm__ volatile("witlb %0, %1\n\t"
			: : "a" (pte), "a"(entry));
}

static ALWAYS_INLINE void xtensa_itlb_entry_write_sync(uint32_t pte, uint32_t entry)
{
	__asm__ volatile("witlb %0, %1\n\t"
			"isync\n\t"
			: : "a" (pte), "a"(entry));
}

/**
 * @brief Invalidate all ITLB entries.
 *
 * This should be used carefully since all entries in the instruction TLB
 * will be erased and the only way to find lookup a physical address will be
 * through the page tables.
 */
static inline void xtensa_itlb_invalidate_sync(void)
{
	uint8_t way, i;

	for (way = 0; way < Z_XTENSA_ITLB_WAYS; way++) {
		for (i = 0; i < (1 << XCHAL_ITLB_ARF_ENTRIES_LOG2); i++) {
			uint32_t entry = way + (i << Z_XTENSA_PPN_SHIFT);

			xtensa_itlb_entry_invalidate(entry);
		}
	}
	__asm__ volatile("isync");
}

/**
 * @brief Invalidate all DTLB entries.
 *
 * This should be used carefully since all entries in the data TLB will be
 * erased and the only way to find lookup a physical address will be through
 * the page tables.
 */
static inline void xtensa_dtlb_invalidate_sync(void)
{
	uint8_t way, i;

	for (way = 0; way < Z_XTENSA_DTLB_WAYS; way++) {
		for (i = 0; i < (1 << XCHAL_DTLB_ARF_ENTRIES_LOG2); i++) {
			uint32_t entry = way + (i << Z_XTENSA_PPN_SHIFT);

			xtensa_dtlb_entry_invalidate(entry);
		}
	}
	__asm__ volatile("isync");
}

/**
 * @brief Invalidates an autorefill DTLB entry.
 *
 * Invalidates the page table enrty that maps a given virtual address.
 */
static inline void xtensa_dtlb_autorefill_invalidate_sync(void *vaddr)
{
	uint8_t way;

	for (way = 0; way < Z_XTENSA_TLB_AUTOREFILL_WAYS; way++) {
		xtensa_dtlb_entry_invalidate(Z_XTENSA_TLB_ENTRY((uint32_t)vaddr, way));
	}
	__asm__ volatile("dsync");
}

/**
 * @brief Invalidates an autorefill ITLB entry.
 *
 * Invalidates the page table enrty that maps a given virtual address.
 */
static inline void xtensa_itlb_autorefill_invalidate_sync(void *vaddr)
{
	uint8_t way;

	for (way = 0; way < Z_XTENSA_TLB_AUTOREFILL_WAYS; way++) {
		xtensa_itlb_entry_invalidate(Z_XTENSA_TLB_ENTRY((uint32_t)vaddr, way));
	}
	__asm__ volatile("isync");
}
/**
 * @brief Invalidate all autorefill ITLB entries.
 *
 * This should be used carefully since all entries in the instruction TLB
 * will be erased and the only way to find lookup a physical address will be
 * through the page tables.
 */
static inline void xtensa_itlb_autorefill_invalidate_all_sync(void)
{
	uint8_t way, i;

	for (way = 0; way < Z_XTENSA_TLB_AUTOREFILL_WAYS; way++) {
		for (i = 0; i < (1 << XCHAL_ITLB_ARF_ENTRIES_LOG2); i++) {
			uint32_t entry = way + (i << Z_XTENSA_PPN_SHIFT);

			xtensa_itlb_entry_invalidate(entry);
		}
	}
	__asm__ volatile("isync");
}

/**
 * @brief Invalidate all autorefill DTLB entries.
 *
 * This should be used carefully since all entries in the data TLB will be
 * erased and the only way to find lookup a physical address will be through
 * the page tables.
 */
static inline void xtensa_dtlb_autorefill_invalidate_all_sync(void)
{
	uint8_t way, i;

	for (way = 0; way < Z_XTENSA_TLB_AUTOREFILL_WAYS; way++) {
		for (i = 0; i < (1 << XCHAL_DTLB_ARF_ENTRIES_LOG2); i++) {
			uint32_t entry = way + (i << Z_XTENSA_PPN_SHIFT);

			xtensa_dtlb_entry_invalidate(entry);
		}
	}
	__asm__ volatile("isync");
}


/**
 * @brief Set the page tables.
 *
 * The page tables is set writing ptevaddr address.
 *
 * @param ptables The page tables address (virtual address)
 */
static ALWAYS_INLINE void xtensa_ptevaddr_set(void *ptables)
{
	__asm__ volatile("wsr.ptevaddr %0" : : "a"((uint32_t)ptables));
}

/*
 * The following functions are helpful when debugging.
 */
static ALWAYS_INLINE void *xtensa_dtlb_vaddr_read(uint32_t entry)
{
	uint32_t vaddr;

	__asm__ volatile("rdtlb0  %0, %1\n\t" : "=a" (vaddr) : "a" (entry));
	return (void *)(vaddr & Z_XTENSA_PTE_VPN_MASK);
}

static ALWAYS_INLINE uint32_t xtensa_dtlb_paddr_read(uint32_t entry)
{
	uint32_t paddr;

	__asm__ volatile("rdtlb1  %0, %1\n\t" : "=a" (paddr) : "a" (entry));
	return (paddr & Z_XTENSA_PTE_PPN_MASK);
}

static ALWAYS_INLINE void *xtensa_itlb_vaddr_read(uint32_t entry)
{
	uint32_t vaddr;

	__asm__ volatile("ritlb0  %0, %1\n\t" : "=a" (vaddr), "+a" (entry));
	return (void *)(vaddr & Z_XTENSA_PTE_VPN_MASK);
}

static ALWAYS_INLINE uint32_t xtensa_itlb_paddr_read(uint32_t entry)
{
	uint32_t paddr;

	__asm__ volatile("ritlb1  %0, %1\n\t" : "=a" (paddr), "+a" (entry));
	return (paddr & Z_XTENSA_PTE_PPN_MASK);
}

static ALWAYS_INLINE uint32_t xtensa_itlb_probe(void *vaddr)
{
	uint32_t ret;

	__asm__ __volatile__("pitlb  %0, %1\n\t" : "=a" (ret) : "a" ((uint32_t)vaddr));
	return ret;
}

static ALWAYS_INLINE uint32_t xtensa_dtlb_probe(void *vaddr)
{
	uint32_t ret;

	__asm__ __volatile__("pdtlb  %0, %1\n\t" : "=a" (ret) : "a" ((uint32_t)vaddr));
	return ret;
}

static inline void xtensa_itlb_vaddr_invalidate(void *vaddr)
{
	uint32_t entry = xtensa_itlb_probe(vaddr);

	if (entry & Z_XTENSA_PITLB_HIT) {
		xtensa_itlb_entry_invalidate_sync(entry);
	}
}

static inline void xtensa_dtlb_vaddr_invalidate(void *vaddr)
{
	uint32_t entry = xtensa_dtlb_probe(vaddr);

	if (entry & Z_XTENSA_PDTLB_HIT) {
		xtensa_dtlb_entry_invalidate_sync(entry);
	}
}

#endif /* ZEPHYR_ARCH_XTENSA_XTENSA_MMU_PRIV_H_ */
