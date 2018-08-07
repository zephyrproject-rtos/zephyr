/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>

#define SZ(x)  #x " = %u\n", (u32_t)sizeof(x)
#define SZ_ALIAS(name, x)  name " = %u\n", (u32_t)sizeof(x)

void main(void)
{
	printk(SZ(_wait_q_t));
	printk(SZ_ALIAS("_POLL_EVENT", sys_dlist_t));
	printk(SZ(sys_slist_t));
	printk(SZ(sys_dlist_t));
	printk(SZ(struct k_queue));
	printk(SZ(struct k_fifo));
	printk(SZ(struct k_lifo));
	printk(SZ(struct k_thread));
	printk(SZ(struct k_mutex));
	printk(SZ(struct k_sem));
	printk(SZ(struct k_poll_event));
	printk(SZ(struct k_poll_signal));
	printk(SZ(struct k_timer));
	printk(SZ(struct _k_object));
	printk(SZ(struct _k_object_assignment));
}
