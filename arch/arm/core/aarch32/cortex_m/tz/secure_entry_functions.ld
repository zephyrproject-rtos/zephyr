/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_ARM_NSC_REGION_BASE_ADDRESS != 0
	#define NSC_ALIGN . = ABSOLUTE(CONFIG_ARM_NSC_REGION_BASE_ADDRESS)
#elif defined(CONFIG_CPU_HAS_NRF_IDAU)
	/* The nRF9160 needs the NSC region to be at the end of a 32 kB region. */
	#define NSC_ALIGN . = ALIGN(0x8000) - (1 << LOG2CEIL(__sg_size))
#else
	#define NSC_ALIGN . = ALIGN(4)
#endif

#ifdef CONFIG_CPU_HAS_NRF_IDAU
	#define NSC_ALIGN_END . = ALIGN(0x8000)
#else
	#define NSC_ALIGN_END . = ALIGN(4)
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

#ifdef CONFIG_CPU_HAS_NRF_IDAU
	ASSERT(1 << LOG2CEIL(0x8000 - (__sg_start % 0x8000))
			 == (0x8000 - (__sg_start % 0x8000))
		&& (0x8000 - (__sg_start % 0x8000)) >= 32
		&& (0x8000 - (__sg_start % 0x8000)) <= 4096,
		"The Non-Secure Callable region size must be a power of 2 \
between 32 and 4096 bytes.")
#endif
