/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DISABLE_SYSCALL_TRACING

#include <tracing_core.h>
#include <tracing_buffer.h>
#include <tracing_format_common.h>

void tracing_format_string(const char *str, ...)
{
	va_list args;
	bool put_success, before_put_is_empty;

	if (!is_tracing_enabled() || is_tracing_thread()) {
		return;
	}

	va_start(args, str);

	TRACING_LOCK();
	before_put_is_empty = tracing_buffer_is_empty();
	put_success = tracing_format_string_put(str, args);
	TRACING_UNLOCK();

	va_end(args);

	if (put_success) {
		tracing_trigger_output(before_put_is_empty);
	} else {
		tracing_packet_drop_handle();
	}
}

void tracing_format_raw_data(uint8_t *data, uint32_t length)
{
	bool put_success, before_put_is_empty;

	if (!is_tracing_enabled() || is_tracing_thread()) {
		return;
	}

	TRACING_LOCK();
	before_put_is_empty = tracing_buffer_is_empty();
	put_success = tracing_format_raw_data_put(data, length);
	TRACING_UNLOCK();

	if (put_success) {
		tracing_trigger_output(before_put_is_empty);
	} else {
		tracing_packet_drop_handle();
	}
}

void tracing_format_data(tracing_data_t *tracing_data_array, uint32_t count)
{
	bool put_success, before_put_is_empty;

	if (!is_tracing_enabled() || is_tracing_thread()) {
		return;
	}

	TRACING_LOCK();
	before_put_is_empty = tracing_buffer_is_empty();
	put_success = tracing_format_data_put(tracing_data_array, count);
	TRACING_UNLOCK();

	if (put_success) {
		tracing_trigger_output(before_put_is_empty);
	} else {
		tracing_packet_drop_handle();
	}
}
