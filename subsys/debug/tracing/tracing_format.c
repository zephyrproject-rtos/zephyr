/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <sys/printk.h>
#include <debug/tracing_core.h>
#include <tracing_packet.h>
#include <syscall_handler.h>
#include <debug/tracing_format.h>

static void tracing_format_string_handler(const char *str, va_list args)
{
	int length;
	struct tracing_packet *packet = tracing_packet_alloc();

	if (packet) {
		length = vsnprintk(packet->buf, sizeof(packet->buf), str, args);
		packet->length = MIN(length, sizeof(packet->buf));
		packet->direction = TRACING_OUT;

		tracing_list_add_packet(packet);
	}
}

#ifdef CONFIG_USERSPACE
void z_impl_z_tracing_format_raw_str(const char *data, u32_t length)
{
	ARG_UNUSED(data);
	ARG_UNUSED(length);

	__ASSERT(false, "can only be called from user mode.");
}

void z_vrfy_z_tracing_format_raw_str(const char *data, u32_t length)
{
	struct tracing_packet *packet = tracing_packet_alloc();

	Z_OOPS(Z_SYSCALL_VERIFY_MSG(length == 0,
		"Invalid parameter length"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(data == NULL,
		"Invalid parameter data"));

	if (packet) {
		length = MIN(length, sizeof(packet->buf));

		memcpy(packet->buf, data, length);

		packet->length = length;
		packet->direction = TRACING_OUT;

		tracing_list_add_packet(packet);
	}
}
#include <syscalls/z_tracing_format_raw_str_mrsh.c>
#else
void z_impl_z_tracing_format_raw_str(const char *data, u32_t length)
{
	ARG_UNUSED(data);
	ARG_UNUSED(length);
}
#endif

void tracing_format_string(const char *str, ...)
{
	va_list args;

	if (!is_tracing_enabled() || is_tracing_thread()) {
		return;
	}

	if (_is_user_context()) {
		int length;
		char data[CONFIG_TRACING_PACKET_BUF_SIZE];

		va_start(args, str);
		length = vsnprintk(data, sizeof(data), str, args);
		length = MIN(length, sizeof(data));
		va_end(args);

		z_tracing_format_raw_str(data, length);
	} else {
		va_start(args, str);
		tracing_format_string_handler(str, args);
		va_end(args);
	}
}

void z_impl_tracing_format_data(const char *data, u32_t length)
{
	struct tracing_packet *packet;

	if (!is_tracing_enabled() || is_tracing_thread()) {
		return;
	}

	packet = tracing_packet_alloc();

	if (packet) {
		length = MIN(length, sizeof(packet->buf));

		memcpy(packet->buf, data, length);

		packet->length = length;
		packet->direction = TRACING_OUT;

		tracing_list_add_packet(packet);
	}
}

#ifdef CONFIG_USERSPACE
void z_vrfy_tracing_format_data(const char *data, u32_t length)
{
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(data == NULL,
		"Invalid parameter data"));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(length == 0,
		"Invalid parameter length"));

	z_impl_tracing_format_data(data, length);
}
#include <syscalls/tracing_format_data_mrsh.c>
#endif
