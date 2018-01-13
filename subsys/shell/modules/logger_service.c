/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/printk.h>
#include <shell/shell.h>
#include <init.h>
#include <ring_buffer.h>
#include <logging/sys_log.h>
#include <device.h>
#include <init.h>
#include <stdio.h>
#include <debug/object_tracing.h>

#define SHELL_LOGGER "logger"

#define LOG_BUF_SIZE (CONFIG_LOGGER_BUFFER_SIZE)

u32_t logger_buffer[LOG_BUF_SIZE];

struct log_cbuffer {
	struct ring_buf ring_buffer;
} log_cbuffer;

int logger_put(struct log_cbuffer *logger, char *data, u32_t data_size)
{
	int ret;
	u8_t size32;
	int key;

	size32 = (data_size + 3) / 4;

	key = irq_lock();
	ret = sys_ring_buf_put(&logger->ring_buffer, 0, 0,
			       (u32_t *)data, size32);
	irq_unlock(key);

	return ret;
}

void vlog_cbuf_put(const char *format, va_list args)
{
	char buf[512];
	int buf_size = 0;

	buf_size += vsnprintf(&buf[buf_size], sizeof(buf), format, args);
	logger_put(&log_cbuffer, buf, buf_size);
}

void log_cbuf_put(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vlog_cbuf_put(format, args);
	va_end(args);
}

/**
 * Initialize the buffer and install hook very early on to capture syslog
 * during the boot process
 */
int init_logger_hook(struct device *dev)
{
	ARG_UNUSED(dev);
	sys_ring_buf_init(&log_cbuffer.ring_buffer, LOG_BUF_SIZE,
			  logger_buffer);
	syslog_hook_install(log_cbuf_put);
	return 0;
}

SYS_INIT(init_logger_hook, PRE_KERNEL_1, 0);

static inline void ring_buf_print(struct ring_buf *buf)
{
	u8_t data[512];
	int ret, key;
	u8_t size32 = sizeof(data) / 4;
	u16_t type;
	u8_t val;

	key = irq_lock();
	ret = sys_ring_buf_get(&log_cbuffer.ring_buffer, &type, &val,
			       (u32_t *)data, &size32);
	irq_unlock(key);

	if (ret == 0) {
		printk("%s", data);
	} else {
		printk("Error when reading ring buffer (%d)\n", ret);
	}
}

static int shell_cmd_show(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	while (sys_ring_buf_is_empty(&log_cbuffer.ring_buffer) == 0) {
		ring_buf_print(&log_cbuffer.ring_buffer);
	}
	return 0;
}

struct shell_cmd logger_commands[] = {
	{ "show", shell_cmd_show, "Show all log entries." },
	{ NULL, NULL, NULL }
};


SHELL_REGISTER(SHELL_LOGGER, logger_commands);
