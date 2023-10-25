/* Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <xtensa/config/core-isa.h>

#define ASID_INVALID 0

#define TLB_PTES_WAY    7 /* where to pin the page table mapping */
#define TLB_VECBASE_WAY 8 /* where to pin the vecbase mapping */

/* expensive check, for debugging only */
/* #define VALIDATE_PTE_ATTRS */

struct tlb_regs {
	uint32_t rasid;
	uint32_t ptevaddr;
	uint32_t ptepin_as;
	uint32_t ptepin_at;
	uint32_t vecpin_as;
	uint32_t vecpin_at;
};

/* Word index (not byte offset) into L1 page table */
static uint32_t l1_idx(uint32_t addr)
{
	return addr >> 22;
}

/* Virtual address of the PTE mapping the specified address */
static uint32_t pte_virt(uint32_t addr, struct tlb_regs *regs)
{
	return 4 * (addr >> 12) + regs->ptevaddr;
}

static bool pte_valid(uint32_t pte)
{
	return ((pte & 0xf) < 12);
}

static uint32_t pte_ring(uint32_t pte)
{
	return (pte >> 4) & 3;
}

static uint32_t pte_addr(uint32_t pte)
{
	return pte & ~0xfff;
}

static uint32_t lookup_pte(uint32_t *l1, uint32_t addr)
{
	uint32_t pte1 = l1[l1_idx(addr)];
	uint32_t *l2 = (void *) (pte1 & ~0xfff);

	return pte_valid(pte1) ? l2[(addr >> 12) & 0x3ff] : 0xf;
}

/* Walks a page table, ensuring that:
 *
 * 1. L1 page table entries (entries used in hardware refill) are
 *    mapped at ring 0 and either invalid or read-only
 *
 * 2. Those hardware addresses mapped by page table pages are also
 *    direct-mapped at their hardware address with ring 0 permissions.
 *
 * 3. The cacheability attribute of the two mappings must be
 *    identical, and in multiprocessor environments they must be
 *    uncached.
 */
void xtensa_page_table_validate(uint32_t *l1)
{
	for (int i = 0; i < 1024; i++) {
		uint32_t pte = l1[i];

		if (!pte_valid(pte)) {
			continue;
		}

		__ASSERT_NO_MSG(pte_ring(pte) == 0);

		uint32_t phys = lookup_pte(l1, pte_addr(pte));

		__ASSERT_NO_MSG(pte_valid(phys));
		__ASSERT_NO_MSG((phys >> 12) == (pte >> 12)); /* map to same page */
		__ASSERT_NO_MSG(pte_ring(phys) == 0);
		__ASSERT_NO_MSG((pte & 0xc) == (phys & 0xc)); /* cache bits identical */
		if (CONFIG_MP_MAX_NUM_CPUS > 1) {
			__ASSERT_NO_MSG((pte & 0xf) == 0);
		}
	}
}

static void compute_regs(uint32_t user_asid, uint32_t *l1_page, struct tlb_regs *regs)
{
	uint32_t vecbase = XTENSA_RSR("VECBASE");

	__ASSERT_NO_MSG((((uint32_t) l1_page) & 0xfff) == 0);
	__ASSERT_NO_MSG(user_asid != 1 && user_asid < CONFIG_XTENSA_MMU_ASID_MAX);

	/* We don't use ring 1/2, ring 0 ASID must be 1 */
	regs->rasid = (user_asid << 24) | 0x000001;

	/* Derive PTEVADDR from ASID so each domain gets its own PTE area */
	regs->ptevaddr = CONFIG_XTENSA_MMU_PTE_BASE + user_asid * 0x400000;

	/* The ptables code doesn't add the mapping for the l1 page itself */
	l1_page[l1_idx(regs->ptevaddr)] = (uint32_t) l1_page;

	regs->ptepin_at = (uint32_t) l1_page;
	regs->ptepin_as = pte_virt(regs->ptevaddr, regs) | TLB_PTES_WAY;

	/* Pin mapping for refilling the vector address into the ITLB
	 * (for handling TLB miss exceptions). Note: this is NOT an
	 * instruction TLB entry for the vector code itself, it's a
	 * DATA TLB entry for the page containing the vector mapping
	 * so the refill on instruction fetch can find it. The
	 * hardware doesn't have a 4k pinnable instruction TLB way,
	 * frustratingly.
	 */
	uint32_t vb_pte = l1_page[l1_idx(vecbase)];

	__ASSERT_NO_MSG(pte_valid(vb_pte));
	regs->vecpin_at = vb_pte;
	regs->vecpin_as = pte_virt(vecbase, regs) | TLB_VECBASE_WAY;
}

/* Swtich to a new page table.  There are four items we have to set in
 * the hardware: the PTE virtual address, the ring/ASID mapping
 * register, and two pinned entries in the data TLB handling refills
 * for the page tables and the vector handlers.
 *
 * These can be done in any order, provided that we ensure that no
 * memory access which cause a TLB miss can happen during the process.
 * This means that we must work entirely within registers in a single
 * asm block.  Also note that instruction fetches are memory accesses
 * too, which means we cannot cross a page boundary which might reach
 * a new page not in the TLB (a single jump to an aligned address that
 * holds our five instructions is sufficient to guarantee that: I
 * couldn't think of a way to do the alignment statically that also
 * interoperated well with inline assembly).
 */
void xtensa_set_paging(uint32_t user_asid, uint32_t *l1_page)
{
	/* Optimization note: the registers computed here are pure
	 * functions of the two arguments.  With a minor API tweak,
	 * they could be cached in e.g. a thread struct instead of
	 * being recomputed.  This is called on context switch paths
	 * and is performance-sensitive.
	 */
	struct tlb_regs regs;

	compute_regs(user_asid, l1_page, &regs);

	__asm__ volatile("   j 1f             \n"
			 ".align 16           \n" /* enough for 5 insns */
			 "1:                  \n"
			 "   wsr %0, PTEVADDR \n"
			 "   wsr %1, RASID    \n"
			 "   wdtlb %2, %3     \n"
			 "   wdtlb %4, %5     \n"
			 "   isync"
			 :: "r"(regs.ptevaddr), "r"(regs.rasid),
			    "r"(regs.ptepin_at), "r"(regs.ptepin_as),
			    "r"(regs.vecpin_at), "r"(regs.vecpin_as));
}

/* This is effectively the same algorithm from xtensa_set_paging(),
 * but it also disables the hardware-initialized 512M TLB entries in
 * way 6 (because the hardware disallows duplicate TLB mappings).  For
 * instruction fetches this produces a critical ordering constraint:
 * the instruction following the invalidation of ITLB entry mapping
 * the current PC will by definition create a refill condition, which
 * will (because the data TLB was invalidated) cause a refill
 * exception.  Therefore this step must be the very last one, once
 * everything else is setup up and working, which includes the
 * invalidation of the virtual PTEVADDR area so that the resulting
 * refill can complete.
 *
 * Note that we can't guarantee that the compiler won't insert a data
 * fetch from our stack memory after exit from the asm block (while it
 * might be double-mapped), so we invalidate that data TLB inside the
 * asm for correctness.  The other 13 entries get invalidated in a C
 * loop at the end.
 */
void xtensa_init_paging(uint32_t *l1_page)
{
	extern char z_xt_init_pc; /* defined in asm below */
	struct tlb_regs regs;

#if CONFIG_MP_MAX_NUM_CPUS > 1
	/* The incoherent cache can get into terrible trouble if it's
	 * allowed to cache PTEs differently across CPUs.  We require
	 * that all page tables supplied by the OS have exclusively
	 * uncached mappings for page data, but can't do anything
	 * about earlier code/firmware.  Dump the cache to be safe.
	 */
	sys_cache_data_flush_and_invd_all();
#endif

	compute_regs(ASID_INVALID, l1_page, &regs);

	uint32_t idtlb_pte = (regs.ptevaddr & 0xe0000000) | XCHAL_SPANNING_WAY;
	uint32_t idtlb_stk = (((uint32_t) &regs) & ~0xfff) | XCHAL_SPANNING_WAY;
	uint32_t iitlb_pc  = (((uint32_t) &z_xt_init_pc) & ~0xfff) | XCHAL_SPANNING_WAY;

	/* Note: the jump is mostly pedantry, as it's almost
	 * inconceivable that a hardware memory region at boot is
	 * going to cross a 512M page boundary.  But we need the entry
	 * symbol to get the address above, so the jump is here for
	 * symmetry with the set_paging() code.
	 */
	__asm__ volatile("   j z_xt_init_pc   \n"
			 ".align 32           \n" /* room for 10 insns */
			 ".globl z_xt_init_pc \n"
			 "z_xt_init_pc:       \n"
			 "   wsr %0, PTEVADDR \n"
			 "   wsr %1, RASID    \n"
			 "   wdtlb %2, %3     \n"
			 "   wdtlb %4, %5     \n"
			 "   idtlb %6         \n" /* invalidate pte */
			 "   idtlb %7         \n" /* invalidate stk */
			 "   isync            \n"
			 "   iitlb %8         \n" /* invalidate pc */
			 "   isync            \n" /* <--- traps a ITLB miss */
			 :: "r"(regs.ptevaddr), "r"(regs.rasid),
			    "r"(regs.ptepin_at), "r"(regs.ptepin_as),
			    "r"(regs.vecpin_at), "r"(regs.vecpin_as),
			    "r"(idtlb_pte), "r"(idtlb_stk), "r"(iitlb_pc));

	/* Invalidate the remaining (unused by this function)
	 * initialization entries. Now we're flying free with our own
	 * page table.
	 */
	for (int i = 0; i < 8; i++) {
		uint32_t ixtlb = (i * 0x2000000000) | XCHAL_SPANNING_WAY;

		if (ixtlb != iitlb_pc) {
			__asm__ volatile("iitlb %0" :: "r"(ixtlb));
		}
		if (ixtlb != idtlb_stk && ixtlb != idtlb_pte) {
			__asm__ volatile("idtlb %0" :: "r"(ixtlb));
		}
	}
	__asm__ volatile("isync");
}

/* Invalidate all the entries in the refill TLB (though at least two,
 * for the current code page and the current stack, will be
 * repopulated by this code as it returns; but ring0/kernel address
 * should be mapped identically at all times, so that's safe).  This
 * is very simple on Xtensa: the refill TLB is architecturally defined
 * as four ways (0-3) of 4k pages, with a fixed (and small) number of
 * entries that can be directly addressed by the IxTLB instructions.
 *
 * This needs to be called in any circumstance where the mappings for
 * a previously-used page table change.  It does not need to be called
 * on context switch, where ASID tagging isolates entries for us.
 */
void xtensa_invalidate_refill_tlb(void)
{
	/* Note: this will emit some needless extra invalidations if
	 * the I/D TLBs are different sizes, but we make up for that in
	 * the reduced loop management code.
	 */
	int nent = BIT(MAX(XCHAL_ITLB_ARF_ENTRIES_LOG2,
			   XCHAL_DTLB_ARF_ENTRIES_LOG2));

	for(int way = 0; way < 4; way++) {
		for(int i = 0; i < nent; i++) {
			uint32_t arg = (i << 12) | way;

			__asm__ volatile("idtlb %0; isync; iitlb %0; isync" :: "r"(arg));
		}
	}
}
