/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_MULTIBOOT_INFO_H_
#define ZEPHYR_INCLUDE_ARCH_X86_MULTIBOOT_INFO_H_

#include <stdint.h>

/*
 * Multiboot (version 1) boot information structure.
 *
 * Only fields/values of interest to Zephyr are enumerated
 */

struct multiboot_info {
	uint32_t flags;
	uint32_t mem_lower;
	uint32_t mem_upper;
	uint32_t unused0;
	uint32_t cmdline;
	uint32_t unused1[6];
	uint32_t mmap_length;
	uint32_t mmap_addr;
	uint32_t unused2[9];
	uint32_t fb_addr_lo;
	uint32_t fb_addr_hi;
	uint32_t fb_pitch;
	uint32_t fb_width;
	uint32_t fb_height;
	uint8_t  fb_bpp;
	uint8_t  fb_type;
	uint8_t  fb_color_info[6];
};

typedef struct multiboot_info multiboot_info_t;

#endif
