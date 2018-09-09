/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <drivers/lora/lora_context.h>

static struct k_sem quit_lock;
static struct k_thread* pMyThread;
static void lora_device_ready(bool success)
{

}

static struct lora_context_cb lora_callbacks = {
		.ready = lora_device_ready,
};

void main(void)
{
    k_sem_init(&quit_lock, 0, UINT_MAX);

	printk("Hello World! %s\n", CONFIG_ARCH);
	lora_context_init(&lora_callbacks);

    k_sem_take(&quit_lock, K_FOREVER);
}
