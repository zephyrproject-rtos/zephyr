/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* nRF-specific defines. */
#if defined(CONFIG_CPU_HAS_NRF_IDAU) && CONFIG_ARM_NSC_REGION_BASE_ADDRESS == 0
	/* This SOC needs the NSC region to be at the end of an SPU region. */
	#define __NSC_ALIGN (ALIGN(CONFIG_NRF_SPU_FLASH_REGION_SIZE) \
			- MAX(32, (1 << LOG2CEIL(__sg_size))))
	#define NSC_ALIGN \
		. =  (__NSC_ALIGN + ((ABSOLUTE(.) > __NSC_ALIGN) \
				? CONFIG_NRF_SPU_FLASH_REGION_SIZE : 0))
	#define NSC_ALIGN_END . = ALIGN(CONFIG_NRF_SPU_FLASH_REGION_SIZE)
#endif /* CONFIG_CPU_HAS_NRF_IDAU && CONFIG_ARM_NSC_REGION_BASE_ADDRESS != 0 */

#ifndef NSC_ALIGN
#if CONFIG_ARM_NSC_REGION_BASE_ADDRESS != 0
	#define NSC_ALIGN . = ABSOLUTE(CONFIG_ARM_NSC_REGION_BASE_ADDRESS)
#else
	/* The ARM SAU requires regions to be 32-byte-aligned. */
	#define NSC_ALIGN . = ALIGN(32)
#endif /* CONFIG_ARM_NSC_REGION_BASE_ADDRESS */
#endif /* !NSC_ALIGN */

#ifndef NSC_ALIGN_END
	#define NSC_ALIGN_END . = ALIGN(32)
#endif

/*
 * Placement of .gnu.sgstubs depends on how the NSC base address relates to
 * the board's ROMABLE_REGION:
 *
 * - NSC_BASE == 0: relative alignment within ROMABLE_REGION (nRF IDAU, etc.)
 * - NSC_BASE != 0 and ARM_NSC_REGION_IN_NS_ALIAS: absolute address in the
 *   NS-alias range.  The board supplies a "NSC_NS_ALIAS" DTS memory node so
 *   the linker has a valid MEMORY region at that address; without it the GNU
 *   ld location-counter check fires "cannot move location counter backwards"
 *   because the default output position is already past the NS-alias range.
 * - NSC_BASE != 0 and !ARM_NSC_REGION_IN_NS_ALIAS: absolute address within
 *   ROMABLE_REGION (S-alias flash); no GROUP_LINK_IN needed because the
 *   address is forward from the S-image load origin.
 */
#if CONFIG_ARM_NSC_REGION_BASE_ADDRESS != 0
#if defined(CONFIG_ARM_NSC_REGION_IN_NS_ALIAS)
SECTION_PROLOGUE(.gnu.sgstubs,,)
{
	NSC_ALIGN;
	__sg_start = .;
	/* No input section necessary, since the Secure Entry Veneers are
	   automatically placed after the .gnu.sgstubs output section. */
} GROUP_LINK_IN(NSC_NS_ALIAS)
/*
 * Place cmse_nonsecure_entry functions at NS-alias VMA so the CMSE
 * toolchain's B.W in each stub can reach them.  After SG, the CPU is in
 * Secure state and may execute from NS-alias addresses.  Long-range calls
 * from these shims back into S-alias code are resolved via linker veneers
 * (movw/movt/bx thunks) placed in the same output section.
 */
SECTION_PROLOGUE(.cmse_entry_thunks,,)
{
	KEEP(*(.cmse_entry_thunks))
	. = ALIGN(32);
} GROUP_LINK_IN(NSC_NS_ALIAS)
#else
SECTION_PROLOGUE(.gnu.sgstubs,,)
{
	NSC_ALIGN;
	__sg_start = .;
	/* No input section necessary, since the Secure Entry Veneers are
	   automatically placed after the .gnu.sgstubs output section. */
}
#endif /* CONFIG_ARM_NSC_REGION_IN_NS_ALIAS */
#else
SECTION_PROLOGUE(.gnu.sgstubs,,)
{
	NSC_ALIGN;
	__sg_start = .;
	/* No input section necessary, since the Secure Entry Veneers are
	   automatically placed after the .gnu.sgstubs output section. */
} GROUP_LINK_IN(ROMABLE_REGION)
#endif
__sg_end = .;
__sg_size = __sg_end - __sg_start;
NSC_ALIGN_END;
__nsc_size = . - __sg_start;

/* nRF-specific ASSERT. */
#ifdef CONFIG_CPU_HAS_NRF_IDAU
	#define NRF_SG_START (__sg_start % CONFIG_NRF_SPU_FLASH_REGION_SIZE)
	#define NRF_SG_SIZE (CONFIG_NRF_SPU_FLASH_REGION_SIZE - NRF_SG_START)
	ASSERT((__sg_size == 0)
		|| (((1 << LOG2CEIL(NRF_SG_SIZE)) == NRF_SG_SIZE) /* Pow of 2 */
			&& (NRF_SG_SIZE >= 32)
			&& (NRF_SG_SIZE <= 4096)),
		"The Non-Secure Callable region size must be a power of 2 \
between 32 and 4096 bytes.")
#endif
