/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * wpa_supplicant/hostapd / Debug prints
 * Copyright (c) 2002-2013, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include <utils/includes.h>
#include <utils/common.h>

#include "wpa_debug.h"

#include <zephyr/logging/log.h>

#define WPA_DEBUG_MAX_LINE_LENGTH 256

LOG_MODULE_REGISTER(wpa_supp, CONFIG_WIFI_NM_WPA_SUPP_LOG_LEVEL);

int wpa_debug_level = MSG_INFO;
int wpa_debug_show_keys;
int wpa_debug_timestamp;

#ifndef CONFIG_NO_STDOUT_DEBUG

void wpa_printf_impl(int level, const char *fmt, ...)
{
	va_list ap;
	char buffer[WPA_DEBUG_MAX_LINE_LENGTH];

	if (level < wpa_debug_level)
		return;

	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	switch (level) {
	case MSG_ERROR:
		LOG_ERR("%s", buffer);
		break;
	case MSG_WARNING:
		LOG_WRN("%s", buffer);
		break;
	case MSG_INFO:
		LOG_INF("%s", buffer);
		break;
	case MSG_DEBUG:
	case MSG_MSGDUMP:
	case MSG_EXCESSIVE:
		LOG_DBG("%s", buffer);
		break;
	default:
		break;
	}

	forced_memzero(buffer, sizeof(buffer));
}

static void _wpa_hexdump(int level, const char *title, const u8 *buf, size_t len, int show)
{
	size_t i;
	const char *content;
	char *content_buf = NULL;

	if (level < wpa_debug_level)
		return;

	if (buf == NULL) {
		content = " [NULL]";
	} else if (show) {
		content = content_buf = os_malloc(3 * len + 1);

		if (content == NULL) {
			wpa_printf(MSG_ERROR, "wpa_hexdump: Failed to allocate message buffer");
			return;
		}

		for (i = 0; i < len; i++) {
			os_snprintf(&content_buf[i * 3], 4, " %02x", buf[i]);
		}
	} else {
		content = " [REMOVED]";
	}

	wpa_printf(level, "%s - hexdump(len=%lu):%s", title, (unsigned long)len, content);
	bin_clear_free(content_buf, 3 * len + 1);
}

void wpa_hexdump_impl(int level, const char *title, const void *buf, size_t len)
{
	_wpa_hexdump(level, title, buf, len, 1);
}

void wpa_hexdump_key_impl(int level, const char *title, const void *buf, size_t len)
{
	_wpa_hexdump(level, title, buf, len, wpa_debug_show_keys);
}

static void _wpa_hexdump_ascii(int level, const char *title, const void *buf, size_t len, int show)
{
	const char *content;

	if (level < wpa_debug_level) {
		return;
	}

	if (buf == NULL) {
		content = " [NULL]";
	} else if (show) {
		content = "";
	} else {
		content = " [REMOVED]";
	}

	wpa_printf(level, "%s - hexdump_ascii(len=%lu):%s", title, (unsigned long)len, content);

	if (buf == NULL || !show) {
		return;
	}

	switch (level) {
	case MSG_ERROR:
		LOG_HEXDUMP_ERR(buf, len, "");
		break;
	case MSG_WARNING:
		LOG_HEXDUMP_WRN(buf, len, "");
		break;
	case MSG_INFO:
		LOG_HEXDUMP_INF(buf, len, "");
		break;
	case MSG_DEBUG:
	case MSG_MSGDUMP:
	case MSG_EXCESSIVE:
		LOG_HEXDUMP_DBG(buf, len, "");
		break;
	default:
		break;
	}
}

void wpa_hexdump_ascii_impl(int level, const char *title, const void *buf, size_t len)
{
	_wpa_hexdump_ascii(level, title, buf, len, 1);
}

void wpa_hexdump_ascii_key_impl(int level, const char *title, const void *buf, size_t len)
{
	_wpa_hexdump_ascii(level, title, buf, len, wpa_debug_show_keys);
}

#endif /* CONFIG_NO_STDOUT_DEBUG */

#ifndef CONFIG_NO_WPA_MSG

static wpa_msg_cb_func wpa_msg_cb;

void wpa_msg_register_cb(wpa_msg_cb_func func)
{
	wpa_msg_cb = func;
}

static wpa_msg_get_ifname_func wpa_msg_ifname_cb;

void wpa_msg_register_ifname_cb(wpa_msg_get_ifname_func func)
{
	wpa_msg_ifname_cb = func;
}

void wpa_msg_impl(void *ctx, int level, const char *fmt, ...)
{
	va_list ap;
	char *buf;
	int buflen;
	int len;
	char prefix[130];

	va_start(ap, fmt);
	buflen = vsnprintf(NULL, 0, fmt, ap) + 1;
	va_end(ap);

	buf = os_malloc(buflen);
	if (buf == NULL) {
		wpa_printf(MSG_ERROR, "wpa_msg: Failed to allocate message buffer");
		return;
	}
	va_start(ap, fmt);
	prefix[0] = '\0';
	if (wpa_msg_ifname_cb) {
		const char *ifname = wpa_msg_ifname_cb(ctx);

		if (ifname) {
			int res = os_snprintf(prefix, sizeof(prefix), "%s: ", ifname);

			if (os_snprintf_error(sizeof(prefix), res))
				prefix[0] = '\0';
		}
	}
	len = vsnprintf(buf, buflen, fmt, ap);
	va_end(ap);
	wpa_printf(level, "%s%s", prefix, buf);
	if (wpa_msg_cb)
		wpa_msg_cb(ctx, level, WPA_MSG_PER_INTERFACE, buf, len);
	bin_clear_free(buf, buflen);
}

void wpa_msg_ctrl_impl(void *ctx, int level, const char *fmt, ...)
{
	va_list ap;
	char *buf;
	int buflen;
	int len;

	if (!wpa_msg_cb)
		return;

	va_start(ap, fmt);
	buflen = vsnprintf(NULL, 0, fmt, ap) + 1;
	va_end(ap);

	buf = os_malloc(buflen);
	if (buf == NULL) {
		wpa_printf(MSG_ERROR, "wpa_msg_ctrl: Failed to allocate message buffer");
		return;
	}
	va_start(ap, fmt);
	len = vsnprintf(buf, buflen, fmt, ap);
	va_end(ap);
	wpa_msg_cb(ctx, level, WPA_MSG_PER_INTERFACE, buf, len);
	bin_clear_free(buf, buflen);
}

void wpa_msg_global_impl(void *ctx, int level, const char *fmt, ...)
{
	va_list ap;
	char *buf;
	int buflen;
	int len;

	va_start(ap, fmt);
	buflen = vsnprintf(NULL, 0, fmt, ap) + 1;
	va_end(ap);

	buf = os_malloc(buflen);
	if (buf == NULL) {
		wpa_printf(MSG_ERROR, "wpa_msg_global: Failed to allocate message buffer");
		return;
	}
	va_start(ap, fmt);
	len = vsnprintf(buf, buflen, fmt, ap);
	va_end(ap);
	wpa_printf(level, "%s", buf);
	if (wpa_msg_cb)
		wpa_msg_cb(ctx, level, WPA_MSG_GLOBAL, buf, len);
	bin_clear_free(buf, buflen);
}

void wpa_msg_global_ctrl_impl(void *ctx, int level, const char *fmt, ...)
{
	va_list ap;
	char *buf;
	int buflen;
	int len;

	if (!wpa_msg_cb)
		return;

	va_start(ap, fmt);
	buflen = vsnprintf(NULL, 0, fmt, ap) + 1;
	va_end(ap);

	buf = os_malloc(buflen);
	if (buf == NULL) {
		wpa_printf(MSG_ERROR, "wpa_msg_global_ctrl: Failed to allocate message buffer");
		return;
	}
	va_start(ap, fmt);
	len = vsnprintf(buf, buflen, fmt, ap);
	va_end(ap);
	wpa_msg_cb(ctx, level, WPA_MSG_GLOBAL, buf, len);
	bin_clear_free(buf, buflen);
}

void wpa_msg_no_global_impl(void *ctx, int level, const char *fmt, ...)
{
	va_list ap;
	char *buf;
	int buflen;
	int len;

	va_start(ap, fmt);
	buflen = vsnprintf(NULL, 0, fmt, ap) + 1;
	va_end(ap);

	buf = os_malloc(buflen);
	if (buf == NULL) {
		wpa_printf(MSG_ERROR, "wpa_msg_no_global: Failed to allocate message buffer");
		return;
	}
	va_start(ap, fmt);
	len = vsnprintf(buf, buflen, fmt, ap);
	va_end(ap);
	wpa_printf(level, "%s", buf);
	if (wpa_msg_cb)
		wpa_msg_cb(ctx, level, WPA_MSG_NO_GLOBAL, buf, len);
	bin_clear_free(buf, buflen);
}

void wpa_msg_global_only_impl(void *ctx, int level, const char *fmt, ...)
{
	va_list ap;
	char *buf;
	int buflen;
	int len;

	va_start(ap, fmt);
	buflen = vsnprintf(NULL, 0, fmt, ap) + 1;
	va_end(ap);

	buf = os_malloc(buflen);
	if (buf == NULL) {
		wpa_printf(MSG_ERROR, "%s: Failed to allocate message buffer", __func__);
		return;
	}
	va_start(ap, fmt);
	len = vsnprintf(buf, buflen, fmt, ap);
	va_end(ap);
	wpa_printf(level, "%s", buf);
	if (wpa_msg_cb)
		wpa_msg_cb(ctx, level, WPA_MSG_ONLY_GLOBAL, buf, len);
	os_free(buf);
}

#endif /* CONFIG_NO_WPA_MSG */

#ifndef CONFIG_NO_HOSTAPD_LOGGER

static hostapd_logger_cb_func hostapd_logger_cb;

void hostapd_logger_register_cb(hostapd_logger_cb_func func)
{
	hostapd_logger_cb = func;
}

void hostapd_logger(void *ctx, const u8 *addr, unsigned int module, int level, const char *fmt, ...)
{
	va_list ap;
	char *buf;
	int buflen;
	int len;

	va_start(ap, fmt);
	buflen = vsnprintf(NULL, 0, fmt, ap) + 1;
	va_end(ap);

	buf = os_malloc(buflen);
	if (buf == NULL) {
		wpa_printf(MSG_ERROR, "%s: Failed to allocate message buffer", __func__);
		return;
	}
	va_start(ap, fmt);
	len = vsnprintf(buf, buflen, fmt, ap);
	va_end(ap);
	if (hostapd_logger_cb)
		hostapd_logger_cb(ctx, addr, module, level, buf, len);
	else if (addr)
		wpa_printf(MSG_DEBUG, "%s: STA " MACSTR " - %s", __func__, MAC2STR(addr), buf);
	else
		wpa_printf(MSG_DEBUG, "%s: %s", __func__, buf);
	bin_clear_free(buf, buflen);
}

#endif /* CONFIG_NO_HOSTAPD_LOGGER */

const char *debug_level_str(int level)
{
	switch (level) {
	case MSG_EXCESSIVE:
		return "EXCESSIVE";
	case MSG_MSGDUMP:
		return "MSGDUMP";
	case MSG_DEBUG:
		return "DEBUG";
	case MSG_INFO:
		return "INFO";
	case MSG_WARNING:
		return "WARNING";
	case MSG_ERROR:
		return "ERROR";
	default:
		return "?";
	}
}

int str_to_debug_level(const char *s)
{
	if (os_strcasecmp(s, "EXCESSIVE") == 0)
		return MSG_EXCESSIVE;
	if (os_strcasecmp(s, "MSGDUMP") == 0)
		return MSG_MSGDUMP;
	if (os_strcasecmp(s, "DEBUG") == 0)
		return MSG_DEBUG;
	if (os_strcasecmp(s, "INFO") == 0)
		return MSG_INFO;
	if (os_strcasecmp(s, "WARNING") == 0)
		return MSG_WARNING;
	if (os_strcasecmp(s, "ERROR") == 0)
		return MSG_ERROR;
	return -1;
}
