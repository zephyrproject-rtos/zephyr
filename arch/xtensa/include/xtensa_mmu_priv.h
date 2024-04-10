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

/**
 * @defgroup xtensa_mmu_internal_apis Xtensa Memory Management Unit (MMU) Internal APIs
 * @ingroup xtensa_mmu_apis
 * @{
 */

/** Mask for VPN in PTE */
#define XTENSA_MMU_PTE_VPN_MASK			0xFFFFF000U

/** Mask for PPN in PTE */
#define XTENSA_MMU_PTE_PPN_MASK			0xFFFFF000U

/** Mask for attributes in PTE */
#define XTENSA_MMU_PTE_ATTR_MASK		0x0000000FU

/** Mask for cache mode in PTE */
#define XTENSA_MMU_PTE_ATTR_CACHED_MASK		0x0000000CU

/** Mask used to figure out which L1 page table to use */
#define XTENSA_MMU_L1_MASK			0x3FF00000U

/** Mask used to figure out which L2 page table to use */
#define XTENSA_MMU_L2_MASK			0x3FFFFFU

#define XTENSA_MMU_PTEBASE_MASK 0xFFC00000

/** Number of bits to shift for PPN in PTE */
#define XTENSA_MMU_PTE_PPN_SHIFT		12U

/** Mask for ring in PTE */
#define XTENSA_MMU_PTE_RING_MASK		0x00000030U

/** Number of bits to shift for ring in PTE */
#define XTENSA_MMU_PTE_RING_SHIFT		4U

/** Number of bits to shift for SW reserved ared in PTE */
#define XTENSA_MMU_PTE_SW_SHIFT		6U

/** Mask for SW bits in PTE */
#define XTENSA_MMU_PTE_SW_MASK		0x00000FC0U

/**
 * Internal bit just used to indicate that the attr field must
 * be set in the SW bits too. It is used later when duplicating the
 * kernel page tables.
 */
#define XTENSA_MMU_PTE_ATTR_ORIGINAL BIT(31)

/** Construct a page table entry (PTE) */
#define XTENSA_MMU_PTE(paddr, ring, sw, attr) \
	(((paddr) & XTENSA_MMU_PTE_PPN_MASK) | \
	 (((ring) << XTENSA_MMU_PTE_RING_SHIFT) & XTENSA_MMU_PTE_RING_MASK) | \
	 (((sw) << XTENSA_MMU_PTE_SW_SHIFT) & XTENSA_MMU_PTE_SW_MASK) | \
	 ((attr) & XTENSA_MMU_PTE_ATTR_MASK))

/** Get the attributes from a PTE */
#define XTENSA_MMU_PTE_ATTR_GET(pte) \
	((pte) & XTENSA_MMU_PTE_ATTR_MASK)

/** Set the attributes in a PTE */
#define XTENSA_MMU_PTE_ATTR_SET(pte, attr) \
	(((pte) & ~XTENSA_MMU_PTE_ATTR_MASK) | (attr & XTENSA_MMU_PTE_ATTR_MASK))

/** Set the SW field in a PTE */
#define XTENSA_MMU_PTE_SW_SET(pte, sw) \
	(((pte) & ~XTENSA_MMU_PTE_SW_MASK) | (sw << XTENSA_MMU_PTE_SW_SHIFT))

/** Get the SW field from a PTE */
#define XTENSA_MMU_PTE_SW_GET(pte) \
	(((pte) & XTENSA_MMU_PTE_SW_MASK) >> XTENSA_MMU_PTE_SW_SHIFT)

/** Set the ring in a PTE */
#define XTENSA_MMU_PTE_RING_SET(pte, ring) \
	(((pte) & ~XTENSA_MMU_PTE_RING_MASK) | \
	((ring) << XTENSA_MMU_PTE_RING_SHIFT))

/** Get the ring from a PTE */
#define XTENSA_MMU_PTE_RING_GET(pte) \
	(((pte) & XTENSA_MMU_PTE_RING_MASK) >> XTENSA_MMU_PTE_RING_SHIFT)

/** Get the ASID from the RASID register corresponding to the ring in a PTE */
#define XTENSA_MMU_PTE_ASID_GET(pte, rasid) \
	(((rasid) >> ((((pte) & XTENSA_MMU_PTE_RING_MASK) \
		       >> XTENSA_MMU_PTE_RING_SHIFT) * 8)) & 0xFF)

/** Calculate the L2 page table position from a virtual address */
#define XTENSA_MMU_L2_POS(vaddr) \
	(((vaddr) & XTENSA_MMU_L2_MASK) >> 12U)

/** Calculate the L1 page table position from a virtual address */
#define XTENSA_MMU_L1_POS(vaddr) \
	((vaddr) >> 22U)

/**
 * @def XTENSA_MMU_PAGE_TABLE_ATTR
 *
 * PTE attributes for entries in the L1 page table.  Should never be
 * writable, may be cached in non-SMP contexts only
 */
#if CONFIG_MP_MAX_NUM_CPUS == 1
#define XTENSA_MMU_PAGE_TABLE_ATTR		XTENSA_MMU_CACHED_WB
#else
#define XTENSA_MMU_PAGE_TABLE_ATTR		0
#endif

/** This ASID is shared between all domains and kernel. */
#define XTENSA_MMU_SHARED_ASID			255

/** Fixed data TLB way to map the page table */
#define XTENSA_MMU_PTE_WAY			7

/** Fixed data TLB way to map the vecbase */
#define XTENSA_MMU_VECBASE_WAY			8

/** Kernel specific ASID. Ring field in the PTE */
#define XTENSA_MMU_KERNEL_RING			0

/** User specific ASID. Ring field in the PTE */
#define XTENSA_MMU_USER_RING			2

/** Ring value for MMU_SHARED_ASID */
#define XTENSA_MMU_SHARED_RING			3

/** Number of data TLB ways [0-9] */
#define XTENSA_MMU_NUM_DTLB_WAYS		10

/** Number of instruction TLB ways [0-6] */
#define XTENSA_MMU_NUM_ITLB_WAYS		7

/** Number of auto-refill ways */
#define XTENSA_MMU_NUM_TLB_AUTOREFILL_WAYS	4

/** Indicate PTE is illegal. */
#define XTENSA_MMU_PTE_ILLEGAL			(BIT(3) | BIT(2))

/**
 * PITLB HIT bit.
 *
 * For more information see
 * Xtensa Instruction Set Architecture (ISA) Reference Manual
 * 4.6.5.7 Formats for Probing MMU Option TLB Entries
 */
#define XTENSA_MMU_PITLB_HIT			BIT(3)

/**
 * PDTLB HIT bit.
 *
 * For more information see
 * Xtensa Instruction Set Architecture (ISA) Reference Manual
 * 4.6.5.7 Formats for Probing MMU Option TLB Entries
 */
#define XTENSA_MMU_PDTLB_HIT			BIT(4)

/**
 * Virtual address where the page table is mapped
 */
#define XTENSA_MMU_PTEVADDR			CONFIG_XTENSA_MMU_PTEVADDR

/**
 * Find the PTE entry address of a given vaddr.
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
#define XTENSA_MMU_PTE_ENTRY_VADDR(base, vaddr) \
	((base) + (((vaddr) / KB(4)) * 4))

/**
 * Get ASID for a given ring from RASID register.
 *
 * RASID contains four 8-bit ASIDs, one per ring.
 */
#define XTENSA_MMU_RASID_ASID_GET(rasid, ring) \
	(((rasid) >> ((ring) * 8)) & 0xff)

/**
 * @brief Set RASID register.
 *
 * @param rasid Value to be set.
 */
static ALWAYS_INLINE void xtensa_rasid_set(uint32_t rasid)
{
	__asm__ volatile("wsr %0, rasid\n\t"
			"isync\n" : : "a"(rasid));
}

/**
 * @brief Get RASID register.
 *
 * @return Register value.
 */
static ALWAYS_INLINE uint32_t xtensa_rasid_get(void)
{
	uint32_t rasid;

	__asm__ volatile("rsr %0, rasid" : "=a"(rasid));
	return rasid;
}

/**
 * @brief Set a ring in RASID register to be particular value.
 *
 * @param asid ASID to be set.
 * @param ring ASID of which ring to be manipulated.
 */
static ALWAYS_INLINE void xtensa_rasid_asid_set(uint8_t asid, uint8_t ring)
{
	uint32_t rasid = xtensa_rasid_get();

	rasid = (rasid & ~(0xff << (ring * 8))) | ((uint32_t)asid << (ring * 8));

	xtensa_rasid_set(rasid);
}

/**
 * @brief Invalidate a particular instruction TLB entry.
 *
 * @param entry Entry to be invalidated.
 */
static ALWAYS_INLINE void xtensa_itlb_entry_invalidate(uint32_t entry)
{
	__asm__ volatile("iitlb %0\n\t"
			: : "a" (entry));
}

/**
 * @brief Synchronously invalidate of a particular instruction TLB entry.
 *
 * @param entry Entry to be invalidated.
 */
static ALWAYS_INLINE void xtensa_itlb_entry_invalidate_sync(uint32_t entry)
{
	__asm__ volatile("iitlb %0\n\t"
			"isync\n\t"
			: : "a" (entry));
}

/**
 * @brief Synchronously invalidate of a particular data TLB entry.
 *
 * @param entry Entry to be invalidated.
 */
static ALWAYS_INLINE void xtensa_dtlb_entry_invalidate_sync(uint32_t entry)
{
	__asm__ volatile("idtlb %0\n\t"
			"dsync\n\t"
			: : "a" (entry));
}

/**
 * @brief Invalidate a particular data TLB entry.
 *
 * @param entry Entry to be invalidated.
 */
static ALWAYS_INLINE void xtensa_dtlb_entry_invalidate(uint32_t entry)
{
	__asm__ volatile("idtlb %0\n\t"
			: : "a" (entry));
}

/**
 * @brief Synchronously write to a particular data TLB entry.
 *
 * @param pte Value to be written.
 * @param entry Entry to be written.
 */
static ALWAYS_INLINE void xtensa_dtlb_entry_write_sync(uint32_t pte, uint32_t entry)
{
	__asm__ volatile("wdtlb %0, %1\n\t"
			"dsync\n\t"
			: : "a" (pte), "a"(entry));
}

/**
 * @brief Write to a particular data TLB entry.
 *
 * @param pte Value to be written.
 * @param entry Entry to be written.
 */
static ALWAYS_INLINE void xtensa_dtlb_entry_write(uint32_t pte, uint32_t entry)
{
	__asm__ volatile("wdtlb %0, %1\n\t"
			: : "a" (pte), "a"(entry));
}

/**
 * @brief Synchronously write to a particular instruction TLB entry.
 *
 * @param pte Value to be written.
 * @param entry Entry to be written.
 */
static ALWAYS_INLINE void xtensa_itlb_entry_write(uint32_t pte, uint32_t entry)
{
	__asm__ volatile("witlb %0, %1\n\t"
			: : "a" (pte), "a"(entry));
}

/**
 * @brief Synchronously write to a particular instruction TLB entry.
 *
 * @param pte Value to be written.
 * @param entry Entry to be written.
 */
static ALWAYS_INLINE void xtensa_itlb_entry_write_sync(uint32_t pte, uint32_t entry)
{
	__asm__ volatile("witlb %0, %1\n\t"
			"isync\n\t"
			: : "a" (pte), "a"(entry));
}

/**
 * @brief Invalidate all autorefill DTLB and ITLB entries.
 *
 * This should be used carefully since all refill entries in the data
 * and instruction TLB. At least two pages, the current code page and
 * the current stack, will be repopulated by this code as it returns.
 *
 * This needs to be called in any circumstance where the mappings for
 * a previously-used page table change.  It does not need to be called
 * on context switch, where ASID tagging isolates entries for us.
 */
static inline void xtensa_tlb_autorefill_invalidate(void)
{
	uint8_t way, i, entries;

	entries = BIT(MAX(XCHAL_ITLB_ARF_ENTRIES_LOG2,
			   XCHAL_DTLB_ARF_ENTRIES_LOG2));

	for (way = 0; way < XTENSA_MMU_NUM_TLB_AUTOREFILL_WAYS; way++) {
		for (i = 0; i < entries; i++) {
			uint32_t entry = way + (i << XTENSA_MMU_PTE_PPN_SHIFT);

			xtensa_dtlb_entry_invalidate(entry);
			xtensa_itlb_entry_invalidate(entry);
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

/**
 * @brief Get the current page tables.
 *
 * The page tables is obtained by reading ptevaddr address.
 *
 * @return ptables The page tables address (virtual address)
 */
static ALWAYS_INLINE void *xtensa_ptevaddr_get(void)
{
	uint32_t ptables;

	__asm__ volatile("rsr.ptevaddr %0" : "=a" (ptables));

	return (void *)(ptables & XTENSA_MMU_PTEBASE_MASK);
}

/**
 * @brief Get the virtual address associated with a particular data TLB entry.
 *
 * @param entry TLB entry to be queried.
 */
static ALWAYS_INLINE void *xtensa_dtlb_vaddr_read(uint32_t entry)
{
	uint32_t vaddr;

	__asm__ volatile("rdtlb0  %0, %1\n\t" : "=a" (vaddr) : "a" (entry));
	return (void *)(vaddr & XTENSA_MMU_PTE_VPN_MASK);
}

/**
 * @brief Get the physical address associated with a particular data TLB entry.
 *
 * @param entry TLB entry to be queried.
 */
static ALWAYS_INLINE uint32_t xtensa_dtlb_paddr_read(uint32_t entry)
{
	uint32_t paddr;

	__asm__ volatile("rdtlb1  %0, %1\n\t" : "=a" (paddr) : "a" (entry));
	return (paddr & XTENSA_MMU_PTE_PPN_MASK);
}

/**
 * @brief Get the virtual address associated with a particular instruction TLB entry.
 *
 * @param entry TLB entry to be queried.
 */
static ALWAYS_INLINE void *xtensa_itlb_vaddr_read(uint32_t entry)
{
	uint32_t vaddr;

	__asm__ volatile("ritlb0  %0, %1\n\t" : "=a" (vaddr), "+a" (entry));
	return (void *)(vaddr & XTENSA_MMU_PTE_VPN_MASK);
}

/**
 * @brief Get the physical address associated with a particular instruction TLB entry.
 *
 * @param entry TLB entry to be queried.
 */
static ALWAYS_INLINE uint32_t xtensa_itlb_paddr_read(uint32_t entry)
{
	uint32_t paddr;

	__asm__ volatile("ritlb1  %0, %1\n\t" : "=a" (paddr), "+a" (entry));
	return (paddr & XTENSA_MMU_PTE_PPN_MASK);
}

/**
 * @brief Probe for instruction TLB entry from a virtual address.
 *
 * @param vaddr Virtual address.
 *
 * @return Return of the PITLB instruction.
 */
static ALWAYS_INLINE uint32_t xtensa_itlb_probe(void *vaddr)
{
	uint32_t ret;

	__asm__ __volatile__("pitlb  %0, %1\n\t" : "=a" (ret) : "a" ((uint32_t)vaddr));
	return ret;
}

/**
 * @brief Probe for data TLB entry from a virtual address.
 *
 * @param vaddr Virtual address.
 *
 * @return Return of the PDTLB instruction.
 */
static ALWAYS_INLINE uint32_t xtensa_dtlb_probe(void *vaddr)
{
	uint32_t ret;

	__asm__ __volatile__("pdtlb  %0, %1\n\t" : "=a" (ret) : "a" ((uint32_t)vaddr));
	return ret;
}

/**
 * @brief Invalidate an instruction TLB entry associated with a virtual address.
 *
 * This invalidated an instruction TLB entry associated with a virtual address
 * if such TLB entry exists. Otherwise, do nothing.
 *
 * @param vaddr Virtual address.
 */
static inline void xtensa_itlb_vaddr_invalidate(void *vaddr)
{
	uint32_t entry = xtensa_itlb_probe(vaddr);

	if (entry & XTENSA_MMU_PITLB_HIT) {
		xtensa_itlb_entry_invalidate_sync(entry);
	}
}

/**
 * @brief Invalidate a data TLB entry associated with a virtual address.
 *
 * This invalidated a data TLB entry associated with a virtual address
 * if such TLB entry exists. Otherwise, do nothing.
 *
 * @param vaddr Virtual address.
 */
static inline void xtensa_dtlb_vaddr_invalidate(void *vaddr)
{
	uint32_t entry = xtensa_dtlb_probe(vaddr);

	if (entry & XTENSA_MMU_PDTLB_HIT) {
		xtensa_dtlb_entry_invalidate_sync(entry);
	}
}

/**
 * @brief Tell hardware to use a page table very first time after boot.
 *
 * @param l1_page Pointer to the page table to be used.
 */
void xtensa_init_paging(uint32_t *l1_page);

/**
 * @brief Switch to a new page table.
 *
 * @param asid The ASID of the memory domain associated with the incoming page table.
 * @param l1_page Page table to be switched to.
 */
void xtensa_set_paging(uint32_t asid, uint32_t *l1_page);

/**
 * @}
 */

#endif /* ZEPHYR_ARCH_XTENSA_XTENSA_MMU_PRIV_H_ */
