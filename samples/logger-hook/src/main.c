/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define SYS_LOG_DOMAIN "sample-unified"

#include <zephyr.h>
#include <misc/printk.h>
#include <misc/sys_log.h>
#include <stdio.h>
#include <misc/ring_buffer.h>

#define LOG_BUF_SIZE (512)

/**
 * @file
 * @brief using logger hook demo
 * unified version of logger hook usage demo
 */

uint32_t logger_buffer[LOG_BUF_SIZE];

struct log_cbuffer {
	struct ring_buf ring_buffer;
} log_cbuffer;

static inline void ring_buf_print(struct ring_buf *buf);

int logger_put(struct log_cbuffer *logger, uint8_t *data, uint32_t data_size)
{
	int ret;

	ret = sys_ring_buf_put(&logger->ring_buffer, 0, 0,
			       (uint32_t *)data, data_size);

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
	uint8_t size;
	uint8_t data[512];
	int ret = EAGAIN, count = 0;

	while (ret == EAGAIN && count < 2) {
		count++;
		ret = sys_ring_buf_get(&log_cbuffer.ring_buffer, 0, 0,
				       (uint32_t *)data, &size);
	}

	if (ret == 0) {
		printk("%s", data);
	} else {
		printk("Error when reading ring buffer\n");
	}
}
