/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "syslogger"

#include <zephyr.h>
#include <misc/printk.h>
#include <logging/sys_log.h>
#include <stdio.h>
#include <ring_buffer.h>

#define LOG_BUF_SIZE (512)

/**
 * @file
 * @brief using logger hook demo
 */

u32_t logger_buffer[LOG_BUF_SIZE];

struct log_cbuffer {
	struct ring_buf ring_buffer;
} log_cbuffer;

static inline void ring_buf_print(struct ring_buf *buf);

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

void main(void)
{
#ifndef CONFIG_SYS_LOG
	printk("syslog hook sample configuration is not set correctly %s\n",
	       CONFIG_ARCH);
#else
	sys_ring_buf_init(&log_cbuffer.ring_buffer, LOG_BUF_SIZE,
			  logger_buffer);
	syslog_hook_install(log_cbuf_put);
	SYS_LOG_ERR("SYS LOG ERR is ACTIVE");
	ring_buf_print(&log_cbuffer.ring_buffer);
	SYS_LOG_WRN("SYS LOG WRN is ACTIVE");
	ring_buf_print(&log_cbuffer.ring_buffer);
	SYS_LOG_INF("SYS LOG INF is ACTIVE");
	ring_buf_print(&log_cbuffer.ring_buffer);
#endif
}

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
