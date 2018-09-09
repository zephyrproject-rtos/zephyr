/* rtt_console.c - Console messages to a RAM buffer that is then read by
 * the Segger J-Link debugger
 */

/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <kernel.h>
#include <misc/printk.h>
#include <device.h>
#include <init.h>
#include <rtt/SEGGER_RTT.h>

#include <console/console.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <ctype.h>

extern void __printk_hook_install(int (*fn)(int));
extern void __stdout_hook_install(int (*fn)(int));

static int rtt_console_out(int character)
{
	unsigned int key;
	char c = (char)character;

	key = irq_lock();
	SEGGER_RTT_WriteNoLock(0, &c, 1);
	irq_unlock(key);

	return character;
}

static int rtt_console_init(struct device *d)
{
	ARG_UNUSED(d);

	SEGGER_RTT_Init();

	__printk_hook_install(rtt_console_out);
	__stdout_hook_install(rtt_console_out);

	return 0;
}

#ifdef CONFIG_CONSOLE_HANDLER

static struct k_fifo *avail_queue;
static struct k_fifo *lines_queue;
static struct k_thread rtt_rx_thread;
static K_THREAD_STACK_DEFINE(rtt_rx_stack, 1024);

static void rtt_console_rx_process(void)
{
	struct console_input *cmd = NULL;
	unsigned int key, count;

	while (true) {

		if (!cmd) {
			cmd = k_fifo_get(avail_queue, K_FOREVER);
		}

		/*
		 * Read and Echo back if available
		 */
		key = irq_lock();

		/* Reserve a space for '\0' */
		count = SEGGER_RTT_ReadNoLock(0, cmd->line, sizeof(cmd->line));
		if (count > 0) {
			SEGGER_RTT_WriteNoLock(0, cmd->line, count);
		}

		irq_unlock(key);

		if (count > 0) {
			/* Replace last '\n' to null */
			cmd->line[count - 1] = '\0';
			k_fifo_put(lines_queue, cmd);
			cmd = NULL;
		}

		k_sleep(K_MSEC(10));
	}
}

void rtt_register_input(struct k_fifo *avail, struct k_fifo *lines,
						u8_t (*completion)(char *str, u8_t len))
{
	avail_queue = avail;
	lines_queue = lines;
	ARG_UNUSED(completion);

	k_thread_create(&rtt_rx_thread, rtt_rx_stack,
					K_THREAD_STACK_SIZEOF(rtt_rx_stack),
					(k_thread_entry_t)rtt_console_rx_process,
					NULL, NULL, NULL, K_PRIO_COOP(8), 0, K_NO_WAIT);
}
#else
#define rtt_register_input(x)		\
	do {				\
	} while ((0))
#endif /* CONFIG_CONSOLE_HANDLER */

SYS_INIT(rtt_console_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);