/*
 * Copyright (c) 2025 Cadence Design Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#if defined(CONFIG_MULTIBOOT_INFO)

__pinned_noinit char multiboot_cmdline[CONFIG_BOOTARGS_ARGS_BUFFER_SIZE];

const char *get_bootargs(void)
{
	return multiboot_cmdline;
}

#elif defined(CONFIG_X86_EFI)

__pinned_noinit char efi_bootargs[CONFIG_BOOTARGS_ARGS_BUFFER_SIZE];

const char *get_bootargs(void)
{
	return efi_bootargs;
}

#endif
