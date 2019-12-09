/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/ipm.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(ipm_console, CONFIG_IPM_LOG_LEVEL);

static struct device *ipm_console_device;

static int console_out(int c)
{
	/* Send character with busy wait */
	ipm_send(ipm_console_device, 1, c & ~BIT(31), NULL, 0);

	return c;
}

#if defined(CONFIG_STDOUT_CONSOLE)
extern void __stdout_hook_install(int (*hook)(int));
#else
#define __stdout_hook_install(x)	\
	do {    /* nothing */		\
	} while ((0))
#endif

#if defined(CONFIG_PRINTK)
extern void __printk_hook_install(int (*fn)(int));
#else
#define __printk_hook_install(x)	\
	do {    /* nothing */		\
	} while ((0))
#endif

/* Install printk/stdout hooks */
static void ipm_console_hook_install(void)
{
	__stdout_hook_install(console_out);
	__printk_hook_install(console_out);
}

static int ipm_console_init(struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_DBG("");

	ipm_console_device = device_get_binding(CONFIG_IPM_CONSOLE_ON_DEV_NAME);
	if (!ipm_console_device) {
		LOG_ERR("Cannot get %s", CONFIG_IPM_CONSOLE_ON_DEV_NAME);
		return -ENODEV;
	}

	ipm_console_hook_install();

	return 0;
}

/* Need to be initialized after IPM */
SYS_INIT(ipm_console_init,
#if defined(CONFIG_EARLY_CONSOLE)
	 PRE_KERNEL_2,
#else
	 POST_KERNEL,
#endif
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
