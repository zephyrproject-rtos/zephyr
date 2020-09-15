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

#if defined(CONFIG_X86_EFI)

/** @brief Initialize usage of EFI gathered information
 */
void efi_init(void);

#else /* CONFIG_X86_EFI */

#define efi_init(...)

#endif /* CONFIG_X86_EFI */

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_EFI_H_ */
