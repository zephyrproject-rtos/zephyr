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

struct efi_boot_arg {
	void *acpi_rsdp;
};

#if defined(CONFIG_X86_EFI)

/** @brief Initialize usage of EFI gathered information
 *
 * @param efi_arg The given pointer to EFI prepared boot argument
 */
void efi_init(struct efi_boot_arg *efi_arg);

#else /* CONFIG_X86_EFI */

#define efi_init(...)

#endif /* CONFIG_X86_EFI */

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_EFI_H_ */
