/*
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/console/console.h>
#include <zephyr/sys/libc-hooks.h>

#define TELINK_W91_CONSOLE_INPUT_BUF_SIZE       CONFIG_CONSOLE_TELINK_W91_ISR_BUFFER_SIZE
#define TELINK_W91_CONSOLE_INPUT_LINE_SIZE      CONFIG_CONSOLE_TELINK_W91_INP_LINE_MAX

K_MSGQ_DEFINE(telink_w91_console_input, sizeof(char), TELINK_W91_CONSOLE_INPUT_BUF_SIZE, 1);

extern void telink_w91_debug_isr_set(bool enabled, void (*on_rx)(char c, void *ctx), void *ctx);
extern int arch_printk_char_out(int c);

static void console_data_received(char c, void *ctx)
{
	struct k_msgq *q = (struct k_msgq *)ctx;

	(void) k_msgq_put(q, &c, K_NO_WAIT);
}

int console_init(void)
{
#if CONFIG_STDOUT_CONSOLE
	__stdout_hook_install(arch_printk_char_out);
#endif
	telink_w91_debug_isr_set(true, console_data_received, &telink_w91_console_input);
	return 0;
}

int console_putchar(char c)
{
	return arch_printk_char_out(c);
}

int console_getchar(void)
{
	char ch;
	int result = k_msgq_get(&telink_w91_console_input, &ch, K_FOREVER);

	if (!result) {
		result = ch;
	}
	return result;
}

ssize_t console_write(void *dummy, const void *buf, size_t size)
{
	ARG_UNUSED(dummy);

	for (size_t i = 0; i < size; ++i) {
		(void)arch_printk_char_out(((char *)buf)[i]);
	}

	return 0;
}

ssize_t console_read(void *dummy, void *buf, size_t size)
{
	ARG_UNUSED(dummy);

	ssize_t received = 0;

	for (size_t i = 0; i < size; ++i) {
		if (k_msgq_get(&telink_w91_console_input, &((char *)buf)[i], K_NO_WAIT)) {
			received = i;
			break;
		}
	}
	return received;
}

void console_getline_init(void)
{
	k_msgq_purge(&telink_w91_console_input);
	telink_w91_debug_isr_set(true, console_data_received, &telink_w91_console_input);
}

char *console_getline(void)
{
	static char line_buffer[TELINK_W91_CONSOLE_INPUT_LINE_SIZE + 1];
	static bool ended_cr;
	size_t i = 0;

	for (bool complete = false; !complete && i < sizeof(line_buffer) - 1;) {
		char ch = console_getchar();

		switch (ch) {
		case '\r':
			complete = true;
			break;
		case '\n':
			if (!ended_cr) {
				complete = true;
			}
			break;
		case '\b':
			if (i) {
				i--;
			}
			break;
		default:
			line_buffer[i++] = ch;
			break;
		}
		if (ch == '\r') {
			ended_cr = true;
		} else {
			ended_cr = false;
		}
	}
	line_buffer[i] = '\0';

	return line_buffer;
}

SYS_INIT(console_init, PRE_KERNEL_1, CONFIG_CONSOLE_INIT_PRIORITY);
