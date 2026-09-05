/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/sys/printk.h>
#ifdef CONFIG_OPENTHREAD_SHELL_DEFERRED_OUTPUT
#include <zephyr/sys/ring_buffer.h>
#endif
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include <openthread.h>

#include <openthread/cli.h>
#include <openthread/instance.h>

#include <platform-zephyr.h>

#define OT_SHELL_BUFFER_SIZE CONFIG_SHELL_CMD_BUFF_SIZE

static char rx_buffer[OT_SHELL_BUFFER_SIZE];

static const struct shell *shell_p;
static bool is_shell_initialized;

#ifdef CONFIG_OPENTHREAD_SHELL_DEFERRED_OUTPUT
/*
 * Deferred console output - avoids blocking the OT thread on synchronous
 * shell UART writes. Also eliminates the ABBA deadlock risk between
 * OT mutex and shell mutex.
 *
 * The OT CLI emits output as many small fragments via consecutive
 * ot_console_cb calls. We accumulate them into a character ring buffer
 * and drain it from the system workqueue, emitting one shell_fprintf per
 * complete line.
 */
RING_BUF_DECLARE(ot_console_ring_buf, CONFIG_OPENTHREAD_SHELL_DEFERRED_OUTPUT_BUF_SIZE);
static struct k_work deferred_print_work;

static void deferred_print_handler(struct k_work *work)
{
	char line[256];
	uint32_t pos = 0;
	uint8_t c;

	ARG_UNUSED(work);

	if (shell_p == NULL) {
		ring_buf_reset(&ot_console_ring_buf);
		return;
	}

	while (ring_buf_get(&ot_console_ring_buf, &c, 1) == 1) {
		if (pos < sizeof(line) - 1) {
			line[pos++] = (char)c;
		}
		if (c == '\n' || pos >= sizeof(line) - 1) {
			line[pos] = '\0';
			shell_fprintf(shell_p, SHELL_NORMAL, "%s", line);
			pos = 0;
		}
	}

	/* Flush any remaining partial line (no trailing newline). */
	if (pos > 0) {
		line[pos] = '\0';
		shell_fprintf(shell_p, SHELL_NORMAL, "%s", line);
	}
}
#endif /* CONFIG_OPENTHREAD_SHELL_DEFERRED_OUTPUT */

static int ot_console_cb(void *context, const char *format, va_list arg)
{
	ARG_UNUSED(context);

	if (shell_p == NULL) {
		return 0;
	}

#ifdef CONFIG_OPENTHREAD_SHELL_DEFERRED_OUTPUT
	char tmp[256];
	uint32_t len;

	len = (uint32_t)vsnprintk(tmp, sizeof(tmp), format, arg);
	if (len >= sizeof(tmp)) {
		len = sizeof(tmp) - 1;
	}

	ring_buf_put(&ot_console_ring_buf, (uint8_t *)tmp, len);
	k_work_submit(&deferred_print_work);
#else
	shell_vfprintf(shell_p, SHELL_NORMAL, format, arg);
#endif

	return 0;
}

#define SHELL_HELP_OT	"OpenThread subcommands\n" \
			"Use \"ot help\" to get the list of subcommands"

static int ot_cmd(const struct shell *sh, size_t argc, char *argv[])
{
	char *buf_ptr = rx_buffer;
	size_t buf_len = OT_SHELL_BUFFER_SIZE;
	size_t arg_len = 0;
	int i;

	if (!is_shell_initialized) {
		return -ENOEXEC;
	}

	for (i = 1; i < argc; i++) {
		if (arg_len) {
			buf_len -= arg_len + 1;
			if (buf_len) {
				buf_ptr[arg_len] = ' ';
			}
			buf_ptr += arg_len + 1;
		}

		arg_len = snprintk(buf_ptr, buf_len, "%s", argv[i]);

		if (arg_len >= buf_len) {
			shell_fprintf(sh, SHELL_WARNING,
				      "OT shell buffer full\n");
			return -ENOEXEC;
		}
	}

	if (i == argc) {
		buf_len -= arg_len;
	}

	shell_p = sh;

	openthread_mutex_lock();
	otCliInputLine(rx_buffer);
	openthread_mutex_unlock();

	return 0;
}

SHELL_CMD_ARG_REGISTER(ot, NULL, SHELL_HELP_OT, ot_cmd, 2, 255);

void platformShellInit(otInstance *aInstance)
{
	if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL)) {
		shell_p = shell_backend_uart_get_ptr();
	} else {
		shell_p = NULL;
	}

#ifdef CONFIG_OPENTHREAD_SHELL_DEFERRED_OUTPUT
	k_work_init(&deferred_print_work, deferred_print_handler);
#endif

	otCliInit(aInstance, ot_console_cb, NULL);
	is_shell_initialized = true;
}
