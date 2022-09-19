/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/ipm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ipm_console, CONFIG_IPM_LOG_LEVEL);

const struct device *ipm_dev;

static int console_out(int c)
{
	static char buf[CONFIG_IPM_CONSOLE_LINE_BUF_LEN];
	static size_t len;
	int ret;

	if (c != '\n' && len < sizeof(buf)) {
		buf[len++] = c;
		return c;
	}

	ret = ipm_send(ipm_dev, 1, len, buf, len);
	if (ret) {
		LOG_ERR("Error sending character %c over IPM, ret %d", c, ret);
	}

	memset(buf, 0, sizeof(buf));
	len = 0;

	/* After buffer is full start a new one */
	if (c != '\n') {
		buf[len++] = c;
	}

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

static int ipm_console_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_DBG("IPM console initialization");

	ipm_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	if (!device_is_ready(ipm_dev)) {
		LOG_ERR("%s is not ready", ipm_dev->name);
		return -ENODEV;
	}

	if (ipm_max_id_val_get(ipm_dev) < CONFIG_IPM_CONSOLE_LINE_BUF_LEN) {
		LOG_ERR("IPM driver does not support buffer length %d",
			CONFIG_IPM_CONSOLE_LINE_BUF_LEN);
		return -ENOTSUP;
	}

	ipm_console_hook_install();

	return 0;
}

/* Need to be initialized after IPM */
SYS_INIT(ipm_console_init, POST_KERNEL, CONFIG_CONSOLE_INIT_PRIORITY);
