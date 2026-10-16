/*
 * Copyright (c) 2026 Beken Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BK7258 console: route printk to the system controller through the mailbox LOG channel.
 *
 * Why this exists
 * ---------------
 * This core has no UART of its own: UART0 belongs to the system controller.
 * Without this file the only way to print is to write straight into the FIFO
 * of the port the system controller is using, relying on UART_BK7258_NO_HW_INIT to keep
 * this core's hands off the system controller's UART registers. That works, but it takes
 * over the system controller's device, and it falls apart the day the system controller changes the
 * baud rate or resets the port.
 *
 * This file instead sends a command on the LOG logical channel (0x1E with
 * MB_CMD_LOG_OUT) with word1 = string pointer and word2 = length; the system controller
 * prints it to UART0 verbatim (verified on hardware).
 *
 * Three implementation points
 * ---------------------------
 * 1. Buffer whole lines before sending. The mailbox is metered per message, so
 * one character per message would swamp the channel. Same approach as
 * Zephyr's own drivers/console/ipm_console.c: flush on '\n' or when the
 * buffer is full.
 *
 * 2. The hand-over to the system controller (rotating slots, command block, tx_seq) is not
 * done here: it belongs to the mailbox transport and lives in
 * soc_bk7258_mbox_log_write() (soc_ipc.c), shared with the shell transport
 * of SERIAL_BK7258_MBOX. Two independent copies of it would each carry their
 * own rotating slots and tx_seq counter.
 *
 * 3. There has to be a fallback before the mailbox is up. This file only swaps
 * the hook at POST_KERNEL, and until 'ready' is set everything goes to the
 * previous hook (UART). Otherwise the window between power-on and
 * POST_KERNEL would be completely blind, which is exactly when logs matter.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk-hooks.h>
#include <zephyr/sys/__assert.h>

#include <soc.h>

#define LINE_LEN SOC_MB_LOG_LINE_LEN

static char cur_buf[LINE_LEN];
static size_t cur_len;

static bool ready;
static printk_hook_fn_t orig_hook;

/* Count of whole lines dropped while the channel was busy: losing console
 * output is acceptable, but it has to be visible.
 */
static uint32_t dropped_lines;

static void flush_line(void)
{
	if (cur_len == 0U) {
		return;
	}

	if (soc_bk7258_mbox_log_write(cur_buf, cur_len) != 0) {
		dropped_lines++;
	}

	cur_len = 0U;
}

static int mbox_console_out(int c)
{
	/*
	 * Not ready yet -> fall back to the previous hook (UART).
	 *
	 * This matters: every boot log before POST_KERNEL, and the case where
	 * the mailbox itself is broken (when logs are needed most), depends on
	 * it.
	 */
	if (!ready) {
		return (orig_hook != NULL) ? orig_hook(c) : c;
	}

	if (c == '\n') {
		/*
		 * Terminals want CRLF and the system controller prints it
		 * verbatim, so add '\r' here.
		 */
		if (cur_len < (LINE_LEN - 1U)) {
			cur_buf[cur_len++] = '\r';
		}
		if (cur_len < LINE_LEN) {
			cur_buf[cur_len++] = '\n';
		}
		flush_line();
		return c;
	}

	if (cur_len >= LINE_LEN) {
		/* Overlong line: send what we have, then start over with this
		 * character.
		 */
		flush_line();
	}

	cur_buf[cur_len++] = (char)c;

	return c;
}

static int bk7258_mbox_console_init(void)
{
	orig_hook = __printk_get_hook();
	__printk_hook_install(mbox_console_out);
	ready = true;

	printk("[console] printk now goes to the mailbox LOG channel (printed by"
	       " the system controller)\n");

	return 0;
}

/*
 * This has to run after soc_ipc.c's SYS_INIT(POST_KERNEL, 0), which is what
 * starts the heartbeat timer and drains the receive FIFO.
 */
SYS_INIT(bk7258_mbox_console_init, POST_KERNEL, 1);
