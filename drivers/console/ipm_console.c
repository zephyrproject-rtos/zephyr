/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/ipm.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(ipm_console, CONFIG_IPM_LOG_LEVEL);

#if defined(CONFIG_IPM_CONSOLE_IMMEDIATE)
struct device *ipm_dev;
#else
/*
 * Stack for the message queue handling thread
 */
static K_THREAD_STACK_DEFINE(thread_stack, 1024);
static struct k_thread thread_data;

K_MSGQ_DEFINE(msg_queue, sizeof(char), CONFIG_IPM_CONSOLE_QUEUE_SIZE, 4);

static void msgq_thread(void *arg1, void *arg2, void *arg3)
{
	struct device *ipm_dev = arg1;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_DBG("started thread");

	while (true) {
		int character = 0;
		int ret;

		/* Get character from message queue */
		k_msgq_get(&msg_queue, &character, K_FOREVER);

		/* Blocking write over IPM */
		ret = ipm_send(ipm_dev, 1, character, NULL, 0);
		if (ret) {
			LOG_ERR("Error sending character %c over IPM, ret %d",
				character, ret);
		}

		k_yield();
	}
}
#endif /* ! CONFIG_IPM_CONSOLE_IMMEDIATE */

static int console_out(int c)
{
	int ret;

#if defined(CONFIG_IPM_CONSOLE_IMMEDIATE)
	ret = ipm_send(ipm_dev, 1, c & ~BIT(31), NULL, 0);
	if (ret) {
		LOG_ERR("Error sending character %c over IPM, ret %d", c, ret);
	}
#else
	char character = c;

	ret = k_msgq_put(&msg_queue, &character, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Error queueing character %c, ret %d", character, ret);
	}
#endif

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
	struct device *ipm;

	ARG_UNUSED(dev);

	LOG_DBG("IPM console initialization");

	ipm = device_get_binding(CONFIG_IPM_CONSOLE_ON_DEV_NAME);
	if (!ipm) {
		LOG_ERR("Cannot get %s", CONFIG_IPM_CONSOLE_ON_DEV_NAME);
		return -ENODEV;
	}

	if (ipm_max_id_val_get(ipm) < 0xFF) {
		LOG_ERR("IPM driver does not support 8 bit ID values");
		return -ENOTSUP;
	}

#if defined(CONFIG_IPM_CONSOLE_IMMEDIATE)
	ipm_dev = ipm;
#else
	k_thread_create(&thread_data, thread_stack,
			K_THREAD_STACK_SIZEOF(thread_stack),
			msgq_thread, ipm, NULL, NULL,
			CONFIG_MAIN_THREAD_PRIORITY - 1,
			0, K_NO_WAIT);
	k_thread_name_set(&thread_data, "ipm_console_thread");
#endif /* CONFIG_IPM_CONSOLE_IMMEDIATE */

	ipm_console_hook_install();

	return 0;
}

/* Need to be initialized after IPM */
SYS_INIT(ipm_console_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
