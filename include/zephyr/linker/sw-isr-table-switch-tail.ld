/* SPDX-License-Identifier: Apache-2.0 */

/*
 * CONFIG_GEN_SW_ISR_TABLE_SWITCH generates get_isr_entry() from zephyr_pre0.elf
 * intList data, then replaces the pre0 placeholder in the final link. The switch
 * body is much larger than the stub; this fragment must be included at the end of
 * the ROM load region (see arch linker scripts) so the size change cannot shift
 * symbols whose addresses are baked into the generated cases.
 */

#if defined(CONFIG_GEN_SW_ISR_TABLE) && !defined(CONFIG_SRAM_SW_ISR_TABLE) && \
	defined(CONFIG_GEN_SW_ISR_TABLE_SWITCH)
	SECTION_PROLOGUE(sw_isr_table,,)
	{
		/*
		 * No ALIGN() here: CONFIG_ARCH_SW_ISR_TABLE_ALIGN constrains the
		 * table as data, for interrupt controllers that require an
		 * aligned base, and get_isr_entry() is code.
		 */
		*(_SW_ISR_TABLE_SECTION_SYMS)
	} GROUP_LINK_IN(
#if defined(CONFIG_XIP)
		ROMABLE_REGION
#else
		RAMABLE_REGION
#endif
		)

#endif
