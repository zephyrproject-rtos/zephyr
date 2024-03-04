/*
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <xtensa/config/core-isa.h>
#include <xtensa_mmu_priv.h>
#include <zephyr/cache.h>

#ifdef CONFIG_USERSPACE
BUILD_ASSERT((CONFIG_PRIVILEGED_STACK_SIZE > 0) &&
	     (CONFIG_PRIVILEGED_STACK_SIZE % CONFIG_MMU_PAGE_SIZE) == 0);
#endif

#define ASID_INVALID 0

struct tlb_regs {
	uint32_t rasid;
	uint32_t ptevaddr;
	uint32_t ptepin_as;
	uint32_t ptepin_at;
	uint32_t vecpin_as;
	uint32_t vecpin_at;
};

static void compute_regs(uint32_t user_asid, uint32_t *l1_page, struct tlb_regs *regs)
{
	uint32_t vecbase = XTENSA_RSR("VECBASE");

	__ASSERT_NO_MSG((((uint32_t)l1_page) & 0xfff) == 0);
	__ASSERT_NO_MSG((user_asid == 0) || ((user_asid > 2) &&
				(user_asid < XTENSA_MMU_SHARED_ASID)));

	/* We don't use ring 1, ring 0 ASID must be 1 */
	regs->rasid = (XTENSA_MMU_SHARED_ASID << 24) |
		      (user_asid << 16) | 0x000201;

	/* Derive PTEVADDR from ASID so each domain gets its own PTE area */
	regs->ptevaddr = CONFIG_XTENSA_MMU_PTEVADDR + user_asid * 0x400000;

	/* The ptables code doesn't add the mapping for the l1 page itself */
	l1_page[XTENSA_MMU_L1_POS(regs->ptevaddr)] =
		(uint32_t)l1_page | XTENSA_MMU_PAGE_TABLE_ATTR;

	regs->ptepin_at = (uint32_t)l1_page;
	regs->ptepin_as = XTENSA_MMU_PTE_ENTRY_VADDR(regs->ptevaddr, regs->ptevaddr)
			  | XTENSA_MMU_PTE_WAY;

	/* Pin mapping for refilling the vector address into the ITLB
	 * (for handling TLB miss exceptions). Note: this is NOT an
	 * instruction TLB entry for the vector code itself, it's a
	 * DATA TLB entry for the page containing the vector mapping
	 * so the refill on instruction fetch can find it. The
	 * hardware doesn't have a 4k pinnable instruction TLB way,
	 * frustratingly.
	 */
	uint32_t vb_pte = l1_page[XTENSA_MMU_L1_POS(vecbase)];

	regs->vecpin_at = vb_pte;
	regs->vecpin_as = XTENSA_MMU_PTE_ENTRY_VADDR(regs->ptevaddr, vecbase)
			  | XTENSA_MMU_VECBASE_WAY;
}

/* Switch to a new page table.  There are four items we have to set in
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

	__asm__ volatile("j 1f\n"
			 ".align 16\n" /* enough for 5 insns */
			 "1:\n"
			 "wsr %0, PTEVADDR\n"
			 "wsr %1, RASID\n"
			 "wdtlb %2, %3\n"
			 "wdtlb %4, %5\n"
			 "isync"
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
	uint32_t idtlb_stk = (((uint32_t)&regs) & ~0xfff) | XCHAL_SPANNING_WAY;
	uint32_t iitlb_pc  = (((uint32_t)&z_xt_init_pc) & ~0xfff) | XCHAL_SPANNING_WAY;

	/* Note: the jump is mostly pedantry, as it's almost
	 * inconceivable that a hardware memory region at boot is
	 * going to cross a 512M page boundary.  But we need the entry
	 * symbol to get the address above, so the jump is here for
	 * symmetry with the set_paging() code.
	 */
	__asm__ volatile("j z_xt_init_pc\n"
			 ".align 32\n" /* room for 10 insns */
			 ".globl z_xt_init_pc\n"
			 "z_xt_init_pc:\n"
			 "wsr %0, PTEVADDR\n"
			 "wsr %1, RASID\n"
			 "wdtlb %2, %3\n"
			 "wdtlb %4, %5\n"
			 "idtlb %6\n" /* invalidate pte */
			 "idtlb %7\n" /* invalidate stk */
			 "isync\n"
			 "iitlb %8\n" /* invalidate pc */
			 "isync\n" /* <--- traps a ITLB miss */
			 :: "r"(regs.ptevaddr), "r"(regs.rasid),
			    "r"(regs.ptepin_at), "r"(regs.ptepin_as),
			    "r"(regs.vecpin_at), "r"(regs.vecpin_as),
			    "r"(idtlb_pte), "r"(idtlb_stk), "r"(iitlb_pc));

	/* Invalidate the remaining (unused by this function)
	 * initialization entries. Now we're flying free with our own
	 * page table.
	 */
	for (uint32_t i = 0; i < 8; i++) {
		uint32_t ixtlb = (i * 0x20000000) | XCHAL_SPANNING_WAY;

		if (ixtlb != iitlb_pc) {
			__asm__ volatile("iitlb %0" :: "r"(ixtlb));
		}
		if (ixtlb != idtlb_stk && ixtlb != idtlb_pte) {
			__asm__ volatile("idtlb %0" :: "r"(ixtlb));
		}
	}
	__asm__ volatile("isync");
}
