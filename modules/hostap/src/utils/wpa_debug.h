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

#ifndef WPA_DEBUG_H
#define WPA_DEBUG_H

#include <utils/wpabuf.h>

#include <zephyr/sys/__assert.h>

extern int wpa_debug_level;
extern int wpa_debug_show_keys;
extern int wpa_debug_timestamp;

enum { MSG_EXCESSIVE, MSG_MSGDUMP, MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR };

/**
 * wpa_debug_level_enabled - determine if the given priority level is enabled
 * by compile-time configuration.
 *
 * @level: priority level of a message
 */
#define wpa_debug_level_enabled(level) (CONFIG_WIFI_NM_WPA_SUPP_DEBUG_LEVEL <= (level))

/**
 * wpa_debug_cond_run - run the action if the given condition is met
 *
 * @cond: condition expression
 * @action: action to run
 */
#define wpa_debug_cond_run(cond, action)                                                           \
	do {                                                                                       \
		if (cond) {                                                                        \
			action;                                                                    \
		}                                                                                  \
	} while (0)


#ifdef CONFIG_NO_STDOUT_DEBUG

#define wpa_printf(args...) do { } while (0)
#define wpa_hexdump(l, t, b, le) do { } while (0)
#define wpa_hexdump_buf(l, t, b) do { } while (0)
#define wpa_hexdump_key(l, t, b, le) do { } while (0)
#define wpa_hexdump_buf_key(l, t, b) do { } while (0)
#define wpa_hexdump_ascii(l, t, b, le) do { } while (0)
#define wpa_hexdump_ascii_key(l, t, b, le) do { } while (0)
#define wpa_dbg(args...) do { } while (0)

#else /* CONFIG_NO_STDOUT_DEBUG */

/**
 * wpa_printf - conditional printf
 *
 * @level: priority level (MSG_*) of the message
 * @fmt: printf format string, followed by optional arguments
 *
 * This function is used to print conditional debugging and error messages.
 * The output is directed to Zephyr logging subsystem.
 */
#define wpa_printf(level, fmt, ...)                                                                \
	wpa_debug_cond_run(wpa_debug_level_enabled(level),                                         \
			   wpa_printf_impl(level, fmt, ##__VA_ARGS__))

void wpa_printf_impl(int level, const char *fmt, ...) PRINTF_FORMAT(2, 3);

/**
 * wpa_hexdump - conditional hex dump
 *
 * @level: priority level (MSG_*) of the message
 * @title: title of the message
 * @buf: data buffer to be dumped
 * @len: length of the buffer
 *
 * This function is used to print conditional debugging and error messages.
 * The output is directed to Zephyr logging subsystem. The contents of buf is
 * printed out as hex dump.
 */
#define wpa_hexdump(level, title, buf, len)                                                        \
	wpa_debug_cond_run(wpa_debug_level_enabled(level), wpa_hexdump_impl(level, title, buf, len))

void wpa_hexdump_impl(int level, const char *title, const void *buf, size_t len);

static inline void wpa_hexdump_buf(int level, const char *title, const struct wpabuf *buf)
{
	wpa_hexdump(level, title, buf ? wpabuf_head(buf) : NULL, buf ? wpabuf_len(buf) : 0);
}

/**
 * wpa_hexdump_key - conditional hex dump, hide keys
 *
 * @level: priority level (MSG_*) of the message
 * @title: title of the message
 * @buf: data buffer to be dumped
 * @len: length of the buffer
 *
 * This function is used to print conditional debugging and error messages.
 * The output is directed to Zephyr logging subsystem. The contents of buf is
 * printed out as hex dump. This works like wpa_hexdump(), but by default, does
 * not include secret keys (passwords, etc.) in debug output.
 */
#define wpa_hexdump_key(level, title, buf, len)                                                    \
	wpa_debug_cond_run(wpa_debug_level_enabled(level),                                         \
			   wpa_hexdump_key_impl(level, title, buf, len))

void wpa_hexdump_key_impl(int level, const char *title, const void *buf, size_t len);

static inline void wpa_hexdump_buf_key(int level, const char *title, const struct wpabuf *buf)
{
	wpa_hexdump_key(level, title, buf ? wpabuf_head(buf) : NULL, buf ? wpabuf_len(buf) : 0);
}

/**
 * wpa_hexdump_ascii - conditional hex dump
 *
 * @level: priority level (MSG_*) of the message
 * @title: title of the message
 * @buf: data buffer to be dumped
 * @len: length of the buffer
 *
 * This function is used to print conditional debugging and error messages.
 * The output is directed to Zephyr logging subsystem. The contents of buf is
 * printed out as hex dump with both the hex numbers and ASCII characters (for
 * printable range) shown. 16 bytes per line will be shown.
 */
#define wpa_hexdump_ascii(level, title, buf, len)                                                  \
	wpa_debug_cond_run(wpa_debug_level_enabled(level),                                         \
			   wpa_hexdump_ascii_impl(level, title, buf, len))

void wpa_hexdump_ascii_impl(int level, const char *title, const void *buf, size_t len);

/**
 * wpa_hexdump_ascii_key - conditional hex dump, hide keys
 * @level: priority level (MSG_*) of the message
 * @title: title of the message
 * @buf: data buffer to be dumped
 * @len: length of the buffer
 *
 * This function is used to print conditional debugging and error messages.
 * The output is directed to Zephyr logging subsystem. The contents of buf is
 * printed out as hex dump with both the hex numbers and ASCII characters (for
 * printable range) shown. 16 bytes per line will be shown. This works like
 * wpa_hexdump_ascii(), but by default, does not include secret keys
 * (passwords, etc.) in debug output.
 */
#define wpa_hexdump_ascii_key(level, title, buf, len)                                              \
	wpa_debug_cond_run(wpa_debug_level_enabled(level),                                         \
			   wpa_hexdump_ascii_key_impl(level, title, buf, len));

void wpa_hexdump_ascii_key_impl(int level, const char *title, const void *buf, size_t len);

/*
 * wpa_dbg() behaves like wpa_msg(), but it can be removed from build to reduce
 * binary size. As such, it should be used with debugging messages that are not
 * needed in the control interface while wpa_msg() has to be used for anything
 * that needs to shown to control interface monitors.
 */
#define wpa_dbg(args...) wpa_msg(args)

#endif /* CONFIG_NO_STDOUT_DEBUG */


#ifdef CONFIG_NO_WPA_MSG

#define wpa_msg(args...) do { } while (0)
#define wpa_msg_ctrl(args...) do { } while (0)
#define wpa_msg_global(args...) do { } while (0)
#define wpa_msg_global_ctrl(args...) do { } while (0)
#define wpa_msg_no_global(args...) do { } while (0)
#define wpa_msg_global_only(args...) do { } while (0)
#define wpa_msg_register_cb(f) do { } while (0)
#define wpa_msg_register_ifname_cb(f) do { } while (0)

#else /* CONFIG_NO_WPA_MSG */

/**
 * wpa_msg - Conditional printf for default target and ctrl_iface monitors
 *
 * @ctx: Pointer to context data; this is the ctx variable registered
 *	with struct wpa_driver_ops::init()
 * @level: priority level (MSG_*) of the message
 * @fmt: printf format string, followed by optional arguments
 *
 * This function is used to print conditional debugging and error messages.
 * The output is directed to Zephyr logging subsystem. This function is like
 * wpa_printf(), but it also sends the same message to all attached ctrl_iface
 * monitors.
 */
#define wpa_msg(ctx, level, fmt, ...)                                                              \
	wpa_debug_cond_run(wpa_debug_level_enabled(level),                                         \
			   wpa_msg_impl(ctx, level, fmt, ##__VA_ARGS__))

void wpa_msg_impl(void *ctx, int level, const char *fmt, ...) PRINTF_FORMAT(3, 4);

/**
 * wpa_msg_ctrl - Conditional printf for ctrl_iface monitors
 *
 * @ctx: Pointer to context data; this is the ctx variable registered
 *	with struct wpa_driver_ops::init()
 * @level: priority level (MSG_*) of the message
 * @fmt: printf format string, followed by optional arguments
 *
 * This function is used to print conditional debugging and error messages.
 * This function is like wpa_msg(), but it sends the output only to the
 * attached ctrl_iface monitors. In other words, it can be used for frequent
 * events that do not need to be sent to syslog.
 */
#define wpa_msg_ctrl(ctx, level, fmt, ...)                                                         \
	wpa_debug_cond_run(wpa_debug_level_enabled(level),                                         \
			   wpa_msg_ctrl_impl(ctx, level, fmt, ##__VA_ARGS__))

void wpa_msg_ctrl_impl(void *ctx, int level, const char *fmt, ...) PRINTF_FORMAT(3, 4);

/**
 * wpa_msg_global - Global printf for ctrl_iface monitors
 *
 * @ctx: Pointer to context data; this is the ctx variable registered
 *	with struct wpa_driver_ops::init()
 * @level: priority level (MSG_*) of the message
 * @fmt: printf format string, followed by optional arguments
 *
 * This function is used to print conditional debugging and error messages.
 * This function is like wpa_msg(), but it sends the output as a global event,
 * i.e., without being specific to an interface. For backwards compatibility,
 * an old style event is also delivered on one of the interfaces (the one
 * specified by the context data).
 */
#define wpa_msg_global(ctx, level, fmt, ...)                                                       \
	wpa_debug_cond_run(wpa_debug_level_enabled(level),                                         \
			   wpa_msg_global_impl(ctx, level, fmt, ##__VA_ARGS__))

void wpa_msg_global_impl(void *ctx, int level, const char *fmt, ...) PRINTF_FORMAT(3, 4);

/**
 * wpa_msg_global_ctrl - Conditional global printf for ctrl_iface monitors
 *
 * @ctx: Pointer to context data; this is the ctx variable registered
 *	with struct wpa_driver_ops::init()
 * @level: priority level (MSG_*) of the message
 * @fmt: printf format string, followed by optional arguments
 *
 * This function is used to print conditional debugging and error messages.
 * This function is like wpa_msg_global(), but it sends the output only to the
 * attached global ctrl_iface monitors. In other words, it can be used for
 * frequent events that do not need to be sent to syslog.
 */
#define wpa_msg_global_ctrl(ctx, level, fmt, ...)                                                  \
	wpa_debug_cond_run(wpa_debug_level_enabled(level),                                         \
			   wpa_msg_global_ctrl_impl(ctx, level, fmt, ##__VA_ARGS__))

void wpa_msg_global_ctrl_impl(void *ctx, int level, const char *fmt, ...) PRINTF_FORMAT(3, 4);

/**
 * wpa_msg_no_global - Conditional printf for ctrl_iface monitors
 *
 * @ctx: Pointer to context data; this is the ctx variable registered
 *	with struct wpa_driver_ops::init()
 * @level: priority level (MSG_*) of the message
 * @fmt: printf format string, followed by optional arguments
 *
 * This function is used to print conditional debugging and error messages.
 * This function is like wpa_msg(), but it does not send the output as a global
 * event.
 */
#define wpa_msg_no_global(ctx, level, fmt, ...)                                                    \
	wpa_debug_cond_run(wpa_debug_level_enabled(level),                                         \
			   wpa_msg_no_global_impl(ctx, level, fmt, ##__VA_ARGS__))

void wpa_msg_no_global_impl(void *ctx, int level, const char *fmt, ...) PRINTF_FORMAT(3, 4);

/**
 * wpa_msg_global_only - Conditional printf for ctrl_iface monitors
 *
 * @ctx: Pointer to context data; this is the ctx variable registered
 *	with struct wpa_driver_ops::init()
 * @level: priority level (MSG_*) of the message
 * @fmt: printf format string, followed by optional arguments
 *
 * This function is used to print conditional debugging and error messages.
 * This function is like wpa_msg_global(), but it sends the output only as a
 * global event.
 */
#define wpa_msg_global_only(ctx, level, fmt, ...)                                                  \
	wpa_debug_cond_run(wpa_debug_level_enabled(level),                                         \
			   wpa_msg_global_only_impl(ctx, level, fmt, ##__VA_ARGS__))

void wpa_msg_global_only_impl(void *ctx, int level, const char *fmt, ...) PRINTF_FORMAT(3, 4);

enum wpa_msg_type {
	WPA_MSG_PER_INTERFACE,
	WPA_MSG_GLOBAL,
	WPA_MSG_NO_GLOBAL,
	WPA_MSG_ONLY_GLOBAL,
};

typedef void (*wpa_msg_cb_func)(void *ctx, int level, enum wpa_msg_type type, const char *txt,
				size_t len);

/**
 * wpa_msg_register_cb - Register callback function for wpa_msg() messages
 * @func: Callback function (%NULL to unregister)
 */
void wpa_msg_register_cb(wpa_msg_cb_func func);

typedef const char * (*wpa_msg_get_ifname_func)(void *ctx);
void wpa_msg_register_ifname_cb(wpa_msg_get_ifname_func func);

#endif /* CONFIG_NO_WPA_MSG */


#ifdef CONFIG_NO_HOSTAPD_LOGGER

#define hostapd_logger(args...) do { } while (0)
#define hostapd_logger_register_cb(f) do { } while (0)

#else /* CONFIG_NO_HOSTAPD_LOGGER */

#define HOSTAPD_MODULE_IEEE80211 0x00000001
#define HOSTAPD_MODULE_IEEE8021X 0x00000002
#define HOSTAPD_MODULE_RADIUS 0x00000004
#define HOSTAPD_MODULE_WPA 0x00000008
#define HOSTAPD_MODULE_DRIVER 0x00000010
#define HOSTAPD_MODULE_MLME 0x00000040

enum hostapd_logger_level {
	HOSTAPD_LEVEL_DEBUG_VERBOSE = 0,
	HOSTAPD_LEVEL_DEBUG = 1,
	HOSTAPD_LEVEL_INFO = 2,
	HOSTAPD_LEVEL_NOTICE = 3,
	HOSTAPD_LEVEL_WARNING = 4
};

void hostapd_logger(void *ctx, const u8 *addr, unsigned int module, int level, const char *fmt, ...)
	PRINTF_FORMAT(5, 6);

typedef void (*hostapd_logger_cb_func)(void *ctx, const u8 *addr, unsigned int module, int level,
				       const char *txt, size_t len);

/**
 * hostapd_logger_register_cb - Register callback function for hostapd_logger()
 * @func: Callback function (NULL to unregister)
 */
void hostapd_logger_register_cb(hostapd_logger_cb_func func);

#endif /* CONFIG_NO_HOSTAPD_LOGGER */


/* CONFIG_DEBUG_FILE is not supported by Zephyr */

static inline int wpa_debug_open_file(const char *path) { return 0; }
static inline int wpa_debug_reopen_file(void) { return 0; }
static inline void wpa_debug_close_file(void) {}
static inline void wpa_debug_setup_stdout(void) {}


/* CONFIG_DEBUG_SYSLOG is not supported by Zephyr */

static inline void wpa_debug_open_syslog(void) {}
static inline void wpa_debug_close_syslog(void) {}


/* CONFIG_DEBUG_LINUX_TRACING is not supported by Zephyr */

static inline int wpa_debug_open_linux_tracing(void) { return 0; }
static inline void wpa_debug_close_linux_tracing(void) {}


#ifdef EAPOL_TEST
#define WPA_ASSERT __ASSERT
#else
#define WPA_ASSERT(a) do {} while (0)
#endif


const char *debug_level_str(int level);
int str_to_debug_level(const char *s);

#endif /* WPA_DEBUG_H */
