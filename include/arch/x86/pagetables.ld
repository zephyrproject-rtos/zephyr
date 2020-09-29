/*
 * Copyright (c) 2020 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

/* Pagetables. These are produced by arch/x86/gen-mmu.py based on
 * data in zephyr_prebuilt.elf (the result of linker pass 1).
 * For the pass 1 build, an equal-sized dummy area is provided as
 * to not shift memory addresses that occur after this.
 */
#ifdef CONFIG_MMU
    SECTION_DATA_PROLOGUE(pagetables,,)
    {
    . = ALIGN(4096);
    z_x86_pagetables_start = .;
#ifdef LINKER_PASS2
    KEEP(*(pagetables))         /* gen_mmu.py */
#else
    KEEP(*(.dummy_pagetables))  /* from x86_mmu.c, just an empty array */
#endif /* LINKER_PASS2 */

    /* Top-level paging structure is the last thing in this section */
#if CONFIG_X86_PAE
    /* 4-entry PDPT */
    z_x86_kernel_ptables = . - 32;
#else
    /* Page directory or PML4 */
    z_x86_kernel_ptables = . - 4096;
#endif /* CONFIG_X86_PAE */
    } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#ifdef LINKER_PASS2
    /DISCARD/ :
    {
    /* We have the real ones in this build */
    *(.dummy_pagetables)
    }
#endif /* LINKER_PASS2 */
#endif /* CONFIG_MMU */
