/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Vector table is not necessarily at the start of the ram region: in the case that you also use
 * code relocation, it will place the vector table after the relocated code. Relocated code
 * will always be at the start of the RAM SECTION because of the hardcoded generated value in file
 * with "gen_relocate_app.py". It can create quite big gap and lose about the size of alignment
 * of RAM Space
 */

SECTION_PROLOGUE(.sram_vt,,)
{
	/* Heritage of vector table alignment in flash (see vector_table.ld in arch/arm/core/) */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	. = ALIGN( 1 << LOG2CEIL(4 * 64) );
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	. = ALIGN( 1 << LOG2CEIL(4 * 32) );
#else
#error "Unsupported architecture variant"
#endif

	. = ALIGN( 1 << LOG2CEIL(4 * (16 + CONFIG_NUM_IRQS)) );

	_sram_vector_start = .;
	. += _vector_end - _vector_start;
	MPU_ALIGN(_sram_vector_size);
	_sram_vector_end = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
_sram_vector_size = _sram_vector_end - _sram_vector_start;
