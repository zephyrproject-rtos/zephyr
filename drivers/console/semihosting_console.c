/*
 * Copyright (c) 2020 Maciej Sopyło <maciek134@gmail.com>
 * Copyright (c) 2018 Josef Gajdusek <atx@atx.name>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <kernel.h>
#include <sys/printk.h>
#include <device.h>
#include <init.h>
#include <string.h>

extern void __printk_hook_install(int (*fn)(int));
extern void __stdout_hook_install(int (*fn)(int));

static char buffer[CONFIG_SEMIHOSTING_BUFFER_SIZE];
static uint32_t buffer_pos;

static void semihosting_flush_buffer(struct k_timer *timer);
static K_TIMER_DEFINE(send_timer, semihosting_flush_buffer, NULL);

static void semihosting_flush_buffer(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	if (buffer[buffer_pos] != 0) {
		buffer[buffer_pos++] = 0;
	}

	__asm__ __volatile__ (
		"movs	r1, %0\n"
		"movs	r0, #4\n"
		"bkpt 0xab\n"
		:
		: "r" (&buffer)
		: "r0", "r1");

	buffer_pos = 0;
}

static int semihosting_console_out(int character)
{
	int key = irq_lock();
	bool end;

	buffer[buffer_pos++] = character;
	/* leave space for the trailing zero */
	end = buffer_pos >= ARRAY_SIZE(buffer) - 1;

	/* Reset the timer */
	k_timer_start(&send_timer,
		K_MSEC(CONFIG_SEMIHOSTING_SEND_TIMEOUT_MS),
		K_NO_WAIT);

	irq_unlock(key);

	if (end) {
		k_timer_status_sync(&send_timer);
	}
	return 0;
}

static int semihosting_console_init(const struct device *d)
{
	ARG_UNUSED(d);

	buffer_pos = 0;

	__printk_hook_install(semihosting_console_out);
	__stdout_hook_install(semihosting_console_out);

	return 0;
}

SYS_INIT(semihosting_console_init,
	PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
