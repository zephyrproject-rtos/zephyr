/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_MULTIBOOT_H_
#define ZEPHYR_INCLUDE_ARCH_X86_MULTIBOOT_H_

#ifndef _ASMLANGUAGE

/*
 * Multiboot (version 1) boot information structure.
 *
 * Only fields/values of interest to Zephyr are enumerated: at
 * present, that means only those pertaining to the framebuffer.
 */

struct x86_multiboot_info {
	u32_t flags;
	u32_t mem_lower;
	u32_t mem_upper;
	u32_t unused0[8];
	u32_t mmap_length;
	u32_t mmap_addr;
	u32_t unused1[9];
	u32_t fb_addr_lo;
	u32_t fb_addr_hi;
	u32_t fb_pitch;
	u32_t fb_width;
	u32_t fb_height;
	u8_t  fb_bpp;
	u8_t  fb_type;
	u8_t  fb_color_info[6];
};

extern struct x86_multiboot_info x86_multiboot_info;

extern void z_x86_multiboot_init(struct x86_multiboot_info *);

#endif /* _ASMLANGUAGE */

/*
 * Magic numbers: the kernel multiboot header (see crt0.S) begins with
 * X86_MULTIBOOT_HEADER_MAGIC to signal to the booter that it supports
 * multiboot. On kernel entry, EAX is set to X86_MULTIBOOT_EAX_MAGIC to
 * signal that the boot loader is multiboot compliant.
 */

#define X86_MULTIBOOT_HEADER_MAGIC	0x1BADB002
#define X86_MULTIBOOT_EAX_MAGIC		0x2BADB002

/*
 * Typically, we put no flags in the multiboot header, as it exists solely
 * to reassure the loader that we're a valid binary. The exception to this
 * is when we want the loader to configure the framebuffer for us.
 */

#define X86_MULTIBOOT_HEADER_FLAG_MEM	(1 << 1)  /* want mem_/mmap_* info */
#define X86_MULTIBOOT_HEADER_FLAG_FB	(1 << 2)  /* want fb_* info */

#ifdef CONFIG_X86_MULTIBOOT_FRAMEBUF
#define X86_MULTIBOOT_HEADER_FLAGS \
	(X86_MULTIBOOT_HEADER_FLAG_FB | X86_MULTIBOOT_HEADER_FLAG_MEM)
#else
#define X86_MULTIBOOT_HEADER_FLAGS	X86_MULTIBOOT_HEADER_FLAG_MEM
#endif

/* The flags in the boot info structure tell us which fields are valid. */

#define X86_MULTIBOOT_INFO_FLAGS_MEM	(1 << 0)	/* mem_* valid */
#define X86_MULTIBOOT_INFO_FLAGS_MMAP	(1 << 6)	/* mmap_* valid */
#define X86_MULTIBOOT_INFO_FLAGS_FB	(1 << 12)	/* fb_* valid */

/* The only fb_type we support is RGB. No text modes and no color palettes. */

#define X86_MULTIBOOT_INFO_FB_TYPE_RGB	1

#endif /* ZEPHYR_INCLUDE_ARCH_X86_MULTIBOOT_H_ */
