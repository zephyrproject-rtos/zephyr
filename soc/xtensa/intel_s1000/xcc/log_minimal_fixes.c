/*
 * Copyright (c) 2019, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log_msg.h>
#include <logging/log_instance.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <syscall.h>
#include <sys/util.h>
#include <sys/printk.h>

void log_0(const char *str, struct log_msg_ids src_level)
{
}

void log_1(const char *str,
	   log_arg_t arg1,
	   struct log_msg_ids src_level)
{
}

void log_2(const char *str,
	   log_arg_t arg1,
	   log_arg_t arg2,
	   struct log_msg_ids src_level)
{
}

void log_3(const char *str,
	   log_arg_t arg1,
	   log_arg_t arg2,
	   log_arg_t arg3,
	   struct log_msg_ids src_level)
{
}

void log_n(const char *str,
	   log_arg_t *args,
	   uint32_t narg,
	   struct log_msg_ids src_level)
{
}

void log_hexdump(const char *str,
		 const uint8_t *data,
		 uint32_t length,
		 struct log_msg_ids src_level)
{
}

void log_string_sync(struct log_msg_ids src_level, const char *fmt, ...)
{
}

void log_hexdump_sync(struct log_msg_ids src_level, const char *metadata,
		      const uint8_t *data, uint32_t len)
{
}

void log_generic(struct log_msg_ids src_level, const char *fmt, va_list ap,
		 enum log_strdup_action strdup_action)
{
}

void log_generic_from_user(struct log_msg_ids src_level,
			   const char *fmt, va_list ap)
{
}

bool log_is_strdup(const void *buf)
{
	return false;
}

void log_free(void *buf)
{
}

uint32_t log_get_strdup_pool_utilization(void)
{
	return 0;
}

uint32_t log_get_strdup_longest_string(void)
{
	return 0;
}

void log_dropped(void)
{
}

void __printf_like(2, 3) log_from_user(struct log_msg_ids src_level,
				       const char *fmt, ...)
{
}

void log_hexdump_from_user(struct log_msg_ids src_level, const char *metadata,
			   const uint8_t *data, uint32_t len)
{
}
