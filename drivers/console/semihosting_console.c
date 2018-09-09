/*
 * Copyright (c) 2018 Josef Gajdusek <atx@atx.name>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <kernel.h>
#include <misc/printk.h>
#include <device.h>
#include <init.h>
#include <string.h>

extern void __printk_hook_install(int (*fn)(int));
extern void __stdout_hook_install(int (*fn)(int));

/* See the Semihosting for AArch32 and AArch64 document for reference */
#define SYS_OPEN 0x01
#define SYS_WRITE 0x05

#define OPEN_MODE_R		0
#define OPEN_MODE_RB	1
#define OPEN_MODE_RP	2
#define OPEN_MODE_RPB	3
#define OPEN_MODE_W		4
#define OPEN_MODE_WB	5
#define OPEN_MODE_WP	6
#define OPEN_MODE_WPB	7
#define OPEN_MODE_A		8
#define OPEN_MODE_AB	9
#define OPEN_MODE_AP	10
#define OPEN_MODE_APB	11

static char buffer[CONFIG_SEMIHOSTING_BUFFER_SIZE];
static u32_t buffer_pos;
static int stderr_handle = -1;

static void semihosting_flush_buffer(struct k_timer *timer);
static K_TIMER_DEFINE(send_timer, semihosting_flush_buffer, NULL);


static int semihosting_request(u32_t command, void *message)
{
	register int r0 __asm__("r0") = command;
	register u32_t *r1 __asm__("r1") = message;
	register int ret __asm__("r0");

	__asm__ volatile("bkpt $0xab"
			: "=r" (ret)
			: "r" (r0), "r" (r1)
			: "memory");
	return ret;
}


static int semihosting_open(const char *filename, u32_t mode)
{
	u32_t message[3] = {
		(u32_t)filename, mode, strlen(filename)
	};

	return semihosting_request(SYS_OPEN, message);
}


static int semihosting_write(int handle, void *buffer, u32_t length)
{
	u32_t message[3] = {
		handle, (u32_t)buffer, length
	};
	return semihosting_request(SYS_WRITE, &message);
}


static void semihosting_flush_buffer(struct k_timer *timer)
{
	semihosting_write(stderr_handle, buffer, buffer_pos);
	buffer_pos = 0;
}


static int semihosting_console_out(int character)
{
	int key = irq_lock();
	bool end;

	buffer[buffer_pos++] = character;
	end = buffer_pos >= ARRAY_SIZE(buffer);

	/* Restart timer if it expired */
	if (k_timer_remaining_get(&send_timer) == 0) {
		k_timer_start(&send_timer,
				K_MSEC(CONFIG_SEMIHOSTING_SEND_TIMEOUT_MS), 0);
	}

	irq_unlock(key);

	if (end) {
		k_timer_status_sync(&send_timer);
	}
	return 0;
}


static int semihosting_console_init(struct device *d)
{
	ARG_UNUSED(d);

	buffer_pos = 0;

	/* This corresponds to a "stderr" output as per the specifications */
	stderr_handle = semihosting_open(":tt", OPEN_MODE_A);
	if (stderr_handle < 0) {
		return -EIO;
	}
	__printk_hook_install(semihosting_console_out);
	__stdout_hook_install(semihosting_console_out);

	return 0;
}

SYS_INIT(semihosting_console_init,
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
