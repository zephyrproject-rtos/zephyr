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

/**
 * @brief Boot argument block passed from the EFI stub (zefi) to the kernel.
 *
 * Populated by the zefi EFI loader before jumping to the Zephyr entry point.
 * GOP fields are zero/NULL when GOP is not available.
 */
struct efi_boot_arg {
	void *efi_systab;           /**< EFI system table pointer. */
	unsigned long long efi_cr3; /**< EFI page table (CR3) value. */
	void *acpi_rsdp;            /**< ACPI RSDP pointer, or NULL. */
	/* GOP (Graphics Output Protocol) info from bootloader; 0/NULL if not set */
	uint64_t gop_fb_base;       /**< Physical address of the GOP framebuffer. */
	uintptr_t gop_fb_size;      /**< GOP framebuffer size in bytes. */
	uint32_t gop_width;         /**< GOP horizontal resolution in pixels. */
	uint32_t gop_height;        /**< GOP vertical resolution in pixels. */
	uint32_t gop_pitch;         /**< GOP pixels per scan line. */
	uint32_t gop_pixel_format;  /**< Raw EFI GOP pixel format enum value. */
};

/** @brief GOP pixel formats supported by the runtime driver API. */
enum efi_gop_display_format {
	EFI_GOP_DISPLAY_FORMAT_NONE = 0,
	EFI_GOP_DISPLAY_FORMAT_ARGB_8888,
	EFI_GOP_DISPLAY_FORMAT_ABGR_8888,
};

#if defined(CONFIG_X86_EFI)

/** @brief Initialize EFI subsystem using boot arguments from the EFI stub.
 *
 * Maps the efi_boot_arg structure and stores a pointer for later use by
 * other EFI accessors. Must be called early in boot before any other
 * efi_get_* function.
 *
 * @param efi_arg Physical pointer to the EFI boot argument block.
 */
void efi_init(struct efi_boot_arg *efi_arg);

/** @brief Get the ACPI RSDP table pointer from EFI boot argument.
 *
 * @return A valid pointer to ACPI RSDP table, or NULL if not available.
 */
void *efi_get_acpi_rsdp(void);

/** @brief Get the physical address of the GOP framebuffer.
 *
 * The caller is responsible for mapping the returned address before
 * accessing it (e.g. via k_mem_map_phys_bare()).
 *
 * @return Physical address of the GOP framebuffer, or 0 if not available.
 */
uint64_t efi_get_gop_fb_base(void);

/** @brief Get the size of the GOP framebuffer in bytes.
 *
 * @return Size in bytes, or 0 if GOP is not available.
 */
uintptr_t efi_get_gop_fb_size(void);

/** @brief Get GOP display mode parameters.
 *
 * Retrieves the current framebuffer resolution and pitch as saved by
 * the zefi EFI stub. Any output pointer may be NULL if that value is
 * not needed.
 *
 * @param width  Receives the horizontal resolution in pixels (may be NULL).
 * @param height Receives the vertical resolution in pixels (may be NULL).
 * @param pitch  Receives the number of pixels per scan line (may be NULL).
 * @return true if GOP mode information is valid, false otherwise.
 */
bool efi_get_gop_mode(uint32_t *width, uint32_t *height, uint32_t *pitch);

/** @brief Get the GOP framebuffer pixel format saved by the EFI stub.
 *
 * @return A runtime display format understood by the EFI GOP driver, or
 * EFI_GOP_DISPLAY_FORMAT_NONE if GOP is unavailable or unsupported.
 */
enum efi_gop_display_format efi_get_gop_pixel_format(void);

#else /* CONFIG_X86_EFI */

#define efi_init(...)
#define efi_get_acpi_rsdp(...) NULL
#define efi_get_gop_fb_base() 0ULL
#define efi_get_gop_fb_size() 0UL
#define efi_get_gop_mode(...) false
#define efi_get_gop_pixel_format(...) EFI_GOP_DISPLAY_FORMAT_NONE

#endif /* CONFIG_X86_EFI */

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_EFI_H_ */
