/*
 * Copyright (c) 2026 Jacob Beningo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Counting frontend that occupies the frontend filter slot. */

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_frontend.h>

uint32_t frontend_rx_cnt;
const void *frontend_test_source;

static void count(const void *source)
{
	if (source == frontend_test_source) {
		frontend_rx_cnt++;
	}
}

void log_frontend_init(void)
{
}

void log_frontend_msg(const void *source, const struct log_msg_desc desc, uint8_t *package,
		      const void *data)
{
	ARG_UNUSED(desc);
	ARG_UNUSED(package);
	ARG_UNUSED(data);
	count(source);
}

void log_frontend_simple_0(const void *source, uint32_t level, const char *fmt)
{
	ARG_UNUSED(level);
	ARG_UNUSED(fmt);
	count(source);
}

void log_frontend_simple_1(const void *source, uint32_t level, const char *fmt, uint32_t arg)
{
	ARG_UNUSED(level);
	ARG_UNUSED(fmt);
	ARG_UNUSED(arg);
	count(source);
}

void log_frontend_simple_2(const void *source, uint32_t level, const char *fmt, uint32_t arg0,
			   uint32_t arg1)
{
	ARG_UNUSED(level);
	ARG_UNUSED(fmt);
	ARG_UNUSED(arg0);
	ARG_UNUSED(arg1);
	count(source);
}

void log_frontend_panic(void)
{
}
