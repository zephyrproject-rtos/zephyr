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

struct multiboot_info {
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

extern struct multiboot_info multiboot_info;

extern void z_multiboot_init(struct multiboot_info *);

/*
 * the mmap_addr field points to a series of entries of the following form.
 */

struct multiboot_mmap {
	u32_t size;
	u64_t base;
	u64_t length;
	u32_t type;
} __packed;

#endif /* _ASMLANGUAGE */

/*
 * Possible values for multiboot_mmap.type field.
 * Other values should be assumed to be unusable ranges.
 */

#define MULTIBOOT_MMAP_RAM		1	/* available RAM */
#define MULTIBOOT_MMAP_ACPI		3	/* reserved for ACPI */
#define MULTIBOOT_MMAP_NVS		4	/* ACPI non-volatile */
#define MULTIBOOT_MMAP_DEFECTIVE	5	/* defective RAM module */

/*
 * Magic numbers: the kernel multiboot header (see crt0.S) begins with
 * MULTIBOOT_HEADER_MAGIC to signal to the booter that it supports
 * multiboot. On kernel entry, EAX is set to MULTIBOOT_EAX_MAGIC to
 * signal that the boot loader is multiboot compliant.
 */

#define MULTIBOOT_HEADER_MAGIC		0x1BADB002
#define MULTIBOOT_EAX_MAGIC		0x2BADB002

/*
 * Typically, we put no flags in the multiboot header, as it exists solely
 * to reassure the loader that we're a valid binary. The exception to this
 * is when we want the loader to configure the framebuffer for us.
 */

#define MULTIBOOT_HEADER_FLAG_MEM	BIT(1)	/* want mem_/mmap_* info */
#define MULTIBOOT_HEADER_FLAG_FB	BIT(2)	/* want fb_* info */

#ifdef CONFIG_MULTIBOOT_FRAMEBUF
#define MULTIBOOT_HEADER_FLAGS \
	(MULTIBOOT_HEADER_FLAG_FB | MULTIBOOT_HEADER_FLAG_MEM)
#else
#define MULTIBOOT_HEADER_FLAGS MULTIBOOT_HEADER_FLAG_MEM
#endif

/* The flags in the boot info structure tell us which fields are valid. */

#define MULTIBOOT_INFO_FLAGS_MEM	(1 << 0)	/* mem_* valid */
#define MULTIBOOT_INFO_FLAGS_MMAP	(1 << 6)	/* mmap_* valid */
#define MULTIBOOT_INFO_FLAGS_FB		(1 << 12)	/* fb_* valid */

/* The only fb_type we support is RGB. No text modes and no color palettes. */

#define MULTIBOOT_INFO_FB_TYPE_RGB	1

#endif /* ZEPHYR_INCLUDE_ARCH_X86_MULTIBOOT_H_ */
