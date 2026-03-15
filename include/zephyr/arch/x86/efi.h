/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_EFI_H_
#define ZEPHYR_ARCH_X86_INCLUDE_EFI_H_

/* Boot type value (see prep_c.c) */
#define EFI_BOOT_TYPE 2

#ifndef _ASMLANGUAGE

#include <stdbool.h>
#include <zephyr/types.h>

struct efi_boot_arg {
	void *efi_systab;           /* EFI system table */
	unsigned long long efi_cr3; /* EFI page table */
	void *acpi_rsdp;
	/* GOP (Graphics Output Protocol) info from bootloader; 0/NULL if not set */
	uint64_t gop_fb_base;       /* framebuffer physical address */
	uintptr_t gop_fb_size;      /* framebuffer size in bytes */
	uint32_t gop_width;         /* horizontal resolution */
	uint32_t gop_height;        /* vertical resolution */
	uint32_t gop_pitch;         /* pixels per scan line */
};

#if defined(CONFIG_X86_EFI)

/** @brief Initialize usage of EFI gathered information
 *
 * @param efi_arg The given pointer to EFI prepared boot argument
 */
void efi_init(struct efi_boot_arg *efi_arg);

/** @brief Get the ACPI RSDP table pointer from EFI boot argument
 *
 * @return A valid pointer to ACPI RSDP table or NULL otherwise.
 */
void *efi_get_acpi_rsdp(void);

/** @brief Get GOP framebuffer address (physical). Caller must map it for access.
 *
 * @return Physical address of GOP framebuffer, or 0 if no GOP.
 */
uint64_t efi_get_gop_fb_base(void);

/** @brief Get GOP framebuffer size in bytes.
 *
 * @return Size, or 0 if no GOP.
 */
uintptr_t efi_get_gop_fb_size(void);

/** @brief Get GOP mode (width, height, pitch in pixels).
 *
 * @param width  Output horizontal resolution (may be NULL).
 * @param height Output vertical resolution (may be NULL).
 * @param pitch  Output pixels per scan line (may be NULL).
 * @return true if GOP mode is valid, false otherwise.
 */
bool efi_get_gop_mode(uint32_t *width, uint32_t *height, uint32_t *pitch);

#else /* CONFIG_X86_EFI */

#define efi_init(...)
#define efi_get_acpi_rsdp(...) NULL
#define efi_get_gop_fb_base() 0ULL
#define efi_get_gop_fb_size() 0UL
#define efi_get_gop_mode(...) false

#endif /* CONFIG_X86_EFI */

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_EFI_H_ */
