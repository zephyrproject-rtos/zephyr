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

SECTION_PROLOGUE(.gnu.sgstubs,,)
{
	NSC_ALIGN;
	__sg_start = .;
	/* No input section necessary, since the Secure Entry Veneers are
	   automatically placed after the .gnu.sgstubs output section. */
} GROUP_LINK_IN(ROMABLE_REGION)
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
