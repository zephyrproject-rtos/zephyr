/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tracing_core.h>
#include <tracing_buffer.h>
#include <tracing_format_common.h>

void tracing_format_string(const char *str, ...)
{
	uint8_t *data;
	va_list args;
	bool put_success;
	uint32_t length, tracing_buffer_size;

	if (!is_tracing_enabled()) {
		return;
	}

	tracing_buffer_size = tracing_buffer_capacity_get();

	va_start(args, str);

	TRACING_LOCK();
	put_success = tracing_format_string_put(str, args);

	if (put_success) {
		length = tracing_buffer_get_claim(&data, tracing_buffer_size);
		tracing_buffer_handle(data, length);
		tracing_buffer_get_finish(length);
	} else {
		tracing_packet_drop_handle();
	}
	TRACING_UNLOCK();

	va_end(args);
}

void tracing_format_raw_data(uint8_t *data, uint32_t length)
{
	if (!is_tracing_enabled()) {
		return;
	}

	TRACING_LOCK();
	tracing_buffer_handle(data, length);
	TRACING_UNLOCK();
}

void tracing_format_data(tracing_data_t *tracing_data_array, uint32_t count)
{
	uint8_t *data;
	bool put_success;
	uint32_t length, tracing_buffer_size;

	if (!is_tracing_enabled()) {
		return;
	}

	tracing_buffer_size = tracing_buffer_capacity_get();

	TRACING_LOCK();
	put_success = tracing_format_data_put(tracing_data_array, count);

	if (put_success) {
		length = tracing_buffer_get_claim(&data, tracing_buffer_size);
		tracing_buffer_handle(data, length);
		tracing_buffer_get_finish(length);
	} else {
		tracing_packet_drop_handle();
	}
	TRACING_UNLOCK();
}
