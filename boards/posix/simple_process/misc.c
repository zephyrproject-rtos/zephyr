/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <init.h>

/**
 *
 * @brief Initialize the driver that provides the printk output
 *
 * @return 0 if successful, otherwise failed.
 */
static int printk_init(struct device *arg)
{
	ARG_UNUSED(arg);

	/* Let's ensure that even if we are redirecting to a file, we get stdout
	 * and stderr line buffered (default for console). Note that glibc
	 * ignores size. But just in case we set a reasonable number in case
	 * somebody tries to compile against a different library
	 */
	setvbuf(stdout, NULL, _IOLBF, 512);
	setvbuf(stderr, NULL, _IOLBF, 512);

	extern void __printk_hook_install(int (*fn)(int));
	__printk_hook_install(putchar);

	return 0;
}

/*TODO move this define to autoconf*/
#define CONFIG_POSIX_PRINTK_INIT_PRIORITY 50
SYS_INIT(printk_init, PRE_KERNEL_1, CONFIG_POSIX_PRINTK_INIT_PRIORITY);
/*TODO: move all this to a driver in the driver/ folder*/


