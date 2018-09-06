/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <drivers/lora/lora_context.h>

static void lora_device_ready(bool success)
{

}

static struct lora_context_cb lora_callbacks = {
		.ready = lora_device_ready,
};

void main(void)
{
	printk("Hello World2! %s\n", CONFIG_ARCH);
	lora_context_init(&lora_callbacks);
}
