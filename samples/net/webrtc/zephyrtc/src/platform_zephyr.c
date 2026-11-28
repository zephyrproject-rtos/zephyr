/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

#include "platform.h"

LOG_MODULE_REGISTER(libpeer_platform, CONFIG_LOG_DEFAULT_LEVEL);

void peer_log(char *level_tag, const char *file_name, int line_number, const char *format, ...);

uint64_t platform_now_ms(void)
{
	return (uint64_t)k_uptime_get();
}

void platform_sleep_ms(unsigned int ms)
{
	k_msleep(ms);
}

void platform_log(const char *level, const char *event, const char *format, ...)
{
	char message[256];
	va_list arguments;

	va_start(arguments, format);
	(void)vsnprintk(message, sizeof(message), format, arguments);
	va_end(arguments);

	if (strcmp(level, "error") == 0) {
		LOG_ERR("event=%s %s", event, message);
	} else if (strcmp(level, "warning") == 0) {
		LOG_WRN("event=%s %s", event, message);
	} else {
		LOG_INF("event=%s %s", event, message);
	}
}

void peer_log(char *level_tag, const char *file_name, int line_number, const char *format, ...)
{
	char message[256];
	va_list arguments;

	va_start(arguments, format);
	(void)vsnprintk(message, sizeof(message), format, arguments);
	va_end(arguments);

	if (strcmp(level_tag, "ERROR") == 0) {
		platform_log("error", "libpeer", "%s:%d %s", file_name, line_number, message);
	} else if (strcmp(level_tag, "WARN") == 0) {
		platform_log("warning", "libpeer", "%s:%d %s", file_name, line_number, message);
	} else {
		platform_log("info", "libpeer", "%s:%d %s", file_name, line_number, message);
	}
}

void platform_log_sdp(const char *direction, const char *sdp)
{
	if (sdp == NULL) {
		LOG_ERR("event=sdp direction=%s value=null", direction);
		return;
	}
	LOG_INF("event=sdp direction=%s bytes=%zu", direction, strlen(sdp));
}
