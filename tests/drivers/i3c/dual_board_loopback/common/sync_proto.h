/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Header-only UART line protocol used by the dual_board_loopback test pair.
 *
 * The protocol is line-oriented ASCII so it is human-readable in a
 * serial capture and easy to debug.  All messages are terminated by a
 * single '\n'.  The UART is opened polling-mode (no IRQ requirement),
 * which keeps the implementation usable from any context including
 * before kernel multithreading is fully up.
 *
 * Wire commands
 * -------------
 * A -> B (requests):
 *   READY <id>\n             - arm B for test <id>; B replies ACK <id>
 *   STAGE_READ <hex_bytes>\n - pre-load the data B returns on the next read
 *   RESET\n                  - reset B's harness state; B replies READY reset ...
 *   SOFT_RST\n               - soft-reset B's I3C controller; B replies
 *                              READY soft_rst ...
 *   DUMP\n                   - request B's I3C register state; B replies DUMP ...
 *   RAISE_TIR <hex_bytes>\n  - raise a target interrupt request carrying <hex_bytes>
 *   RAISE_HJ\n               - raise a hot-join request
 *   RAISE_MR\n               - raise a controller-role request
 *
 * B -> A (responses and notifications):
 *   HELLO\n                  - handshake, sent on boot
 *   ACK   <id>\n             - request accepted
 *   READY <tag> <regs...>\n  - RESET / SOFT_RST completed, with register state
 *   DUMP  <regs...>\n        - register state, in reply to DUMP
 *   PASS  <id>\n             - check passed
 *   FAIL  <id> <reason>\n    - check failed
 *   IBI   <type> rc=<n>\n    - an IBI was raised, with the driver's return code
 *
 * The RAISE_* requests and the IBI notification exist only when both sides
 * are built with CONFIG_I3C_USE_IBI=y.
 *
 * Reserved: VERIFY <id> <hex>\n asks B to confirm the last write it received.
 * B implements it and answers PASS/FAIL, but no test sends it today.
 *
 * Addresses are decimal or hex (0x-prefixed); helpers are stringly typed
 * to keep the header tiny and dependency-free.
 *
 * Use the alias ``zephyr,sync-uart`` in DT to pick the channel.
 */

#ifndef I3C_DUAL_BOARD_LOOPBACK_COMMON_SYNC_PROTO_H_
#define I3C_DUAL_BOARD_LOOPBACK_COMMON_SYNC_PROTO_H_

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define SYNC_LINE_MAX 256U

/**
 * @brief Send a printf-formatted line over the sync UART.
 *
 * A trailing '\n' is appended automatically if the caller did not
 * include one.  Polling-mode TX so this is safe from any context.
 */
static inline void sync_send(const struct device *uart, const char *fmt, ...)
{
	char buf[SYNC_LINE_MAX];
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (n < 0) {
		return;
	}
	if ((size_t)n >= sizeof(buf)) {
		n = (int)sizeof(buf) - 1;
	}

	for (int i = 0; i < n; i++) {
		uart_poll_out(uart, (unsigned char)buf[i]);
	}
	if (n == 0 || buf[n - 1] != '\n') {
		uart_poll_out(uart, (unsigned char)'\n');
	}
}

/**
 * @brief Receive a single '\n'-terminated line over the sync UART.
 *
 * Returns the number of bytes written to @p buf (excluding NUL),
 * -EAGAIN if @p timeout elapses with no full line received, or other
 * negative on bus error.  The trailing '\n' is stripped.
 */
static inline int sync_recv_line(const struct device *uart, char *buf, size_t buflen,
				 k_timeout_t timeout)
{
	if (buflen < 2U) {
		return -EINVAL;
	}

	const k_timepoint_t deadline = sys_timepoint_calc(timeout);
	size_t pos = 0;

	while (1) {
		unsigned char c;
		int rc = uart_poll_in(uart, &c);

		if (rc == 0) {
			if (c == '\r') {
				continue; /* tolerate CRLF */
			}
			if (c == '\n') {
				buf[pos] = '\0';
				return (int)pos;
			}
			if (pos < (buflen - 1U)) {
				buf[pos++] = (char)c;
			}
			/* else: silently drop overflow bytes until '\n' */
			continue;
		}

		if (sys_timepoint_expired(deadline)) {
			return -EAGAIN;
		}
		k_msleep(1);
	}
}

/**
 * @brief Discard any bytes currently sitting in the UART RX FIFO.
 *
 * Per-test setup uses this to drop stale ACK/IBI/READY/FAIL lines from
 * the previous test before issuing the next sync command.
 */
static inline void sync_drain(const struct device *uart)
{
	unsigned char c;

	while (uart_poll_in(uart, &c) == 0) {
		/* drop */
	}
}

/**
 * @brief Wait until a line arrives whose first token equals @p prefix.
 *
 * Returns 0 on match, -EAGAIN on timeout, other negative on bus error.
 * Mismatching lines are silently dropped (so spurious log messages on
 * the sync channel do not break the protocol).
 */
static inline int sync_expect(const struct device *uart, const char *prefix, k_timeout_t timeout)
{
	const k_timepoint_t deadline = sys_timepoint_calc(timeout);
	const size_t plen = strlen(prefix);
	char line[SYNC_LINE_MAX];

	while (1) {
		k_timeout_t remaining;

		if (sys_timepoint_expired(deadline)) {
			return -EAGAIN;
		}
		remaining = sys_timepoint_timeout(deadline);

		int rc = sync_recv_line(uart, line, sizeof(line), remaining);

		if (rc == -EAGAIN) {
			return -EAGAIN;
		}
		if (rc < 0) {
			return rc;
		}
		if (strncmp(line, prefix, plen) == 0 && (line[plen] == '\0' || line[plen] == ' ')) {
			return 0;
		}
		/* unrelated line — keep waiting */
	}
}

/**
 * @brief Like sync_expect, but copies the full matching line into out_buf.
 *
 * Returns 0 on success, with the matching line (NUL-terminated, no
 * trailing newline) copied into out_buf (truncated if larger than
 * out_len).  -EAGAIN on timeout.  Mismatching lines are silently
 * dropped, same as sync_expect.
 */
static inline int sync_expect_line(const struct device *uart, const char *prefix, char *out_buf,
				   size_t out_len, k_timeout_t timeout)
{
	const k_timepoint_t deadline = sys_timepoint_calc(timeout);
	const size_t plen = strlen(prefix);
	char line[SYNC_LINE_MAX];

	while (1) {
		if (sys_timepoint_expired(deadline)) {
			return -EAGAIN;
		}
		k_timeout_t remaining = sys_timepoint_timeout(deadline);
		int rc = sync_recv_line(uart, line, sizeof(line), remaining);

		if (rc == -EAGAIN) {
			return -EAGAIN;
		}
		if (rc < 0) {
			return rc;
		}
		if (strncmp(line, prefix, plen) == 0 && (line[plen] == '\0' || line[plen] == ' ')) {
			if (out_buf != NULL && out_len > 0U) {
				size_t n = strlen(line);

				if (n >= out_len) {
					n = out_len - 1U;
				}
				memcpy(out_buf, line, n);
				out_buf[n] = '\0';
			}
			return 0;
		}
		/* unrelated line — keep waiting */
	}
}

/**
 * @brief Initial three-way HELLO handshake.
 *
 * A repeatedly sends HELLO until B replies HELLO; B waits for an
 * inbound HELLO and replies with HELLO.  Eliminates boot-order races.
 */
static inline int sync_handshake(const struct device *uart, k_timeout_t to)
{
	const k_timepoint_t deadline = sys_timepoint_calc(to);

	while (!sys_timepoint_expired(deadline)) {
		sync_send(uart, "HELLO");
		int rc = sync_expect(uart, "HELLO", K_MSEC(200));

		if (rc == 0) {
			return 0;
		}
	}
	return -EAGAIN;
}

#endif /* I3C_DUAL_BOARD_LOOPBACK_COMMON_SYNC_PROTO_H_ */
