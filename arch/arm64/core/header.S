/*
 * Copyright (c) 2020 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include "mmu.h"

#if CONFIG_MMU_PAGE_SIZE == 4096 || defined(CONFIG_ARM_MPU)
#define HEADER_PGSIZE	1
#elif CONFIG_MMU_PAGE_SIZE == 16384
#define HEADER_PGSIZE	2
#elif CONFIG_MMU_PAGE_SIZE == 65536
#define HEADER_PGSIZE	3
#else
#define HEADER_PGSIZE 0
#warning "Can't determine page size for header flags"
#endif

#define HEADER_FLAGS	(HEADER_PGSIZE << 1)

_ASM_FILE_PROLOGUE

SECTION_SUBSEC_FUNC(image_header,_image_header_section,_image_header)
	b	__start				// branch to kernel start
	.long	0				// reserved
	.quad	0				// Image load offset from start
						// of RAM, little-endian

	.quad	_flash_used			// Effective size of kernel
						// image, little-endian

	.quad	HEADER_FLAGS			// Informative flags,
						// little-endian

	.quad	0				// reserved
	.quad	0				// reserved
	.quad	0				// reserved
	.ascii	"ARM\x64"			// Magic number
	.long	0				// reserved
