/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief EFI console driver
 *
 * @details EFI console driver.
 * Hooks into the printk and fputc (for printf) modules.
 */

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>


extern int efi_console_putchar(int c);

#if defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE)
/**
 *
 * @brief Output one character to EFI console
 *
 * Outputs both line feed and carriage return in the case of a '\n'.
 *
 * @param c Character to output
 *
 * @return The character passed as input.
 */

static int console_out(int c)
{
	return efi_console_putchar(c);
}

#endif

#if defined(CONFIG_STDOUT_CONSOLE)
extern void __stdout_hook_install(int (*hook)(int));
#endif

#if defined(CONFIG_PRINTK)
extern void __printk_hook_install(int (*fn)(int));
#endif

/**
 * @brief Install printk/stdout hook for EFI console output
 */

static void efi_console_hook_install(void)
{
#if defined(CONFIG_STDOUT_CONSOLE)
	__stdout_hook_install(console_out);
#endif
#if defined(CONFIG_PRINTK)
	__printk_hook_install(console_out);
#endif
}

/**
 * @brief Initialize one EFI as the console port
 *
 * @return 0 if successful, otherwise failed.
 */
static int efi_console_init(const struct device *arg)
{

	ARG_UNUSED(arg);

	efi_console_hook_install();

	return 0;
}

/* EFI console initializes */
SYS_INIT(efi_console_init,
	PRE_KERNEL_1,
	0);
