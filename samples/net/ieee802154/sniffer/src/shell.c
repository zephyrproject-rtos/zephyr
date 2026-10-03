/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include "sniffer.h"

static int cmd_sniffer_start(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err = sniffer_start();

	if (err == -EALREADY) {
		shell_print(sh, "already started");
		return 0;
	}

	if (err != 0) {
		shell_error(sh, "start failed (%d)", err);
		return err;
	}

	shell_print(sh, "started");
	return 0;
}

static int cmd_sniffer_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err = sniffer_stop();

	if (err == -EALREADY) {
		shell_print(sh, "already stopped");
		return 0;
	}

	if (err != 0) {
		shell_error(sh, "stop failed (%d)", err);
		return err;
	}

	shell_print(sh, "stopped");
	return 0;
}

static int cmd_sniffer_channel(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_print(sh, "%u", (unsigned int)sniffer_get_channel());
		return 0;
	}

	char *endp = NULL;
	long ch = strtol(argv[1], &endp, 0);

	if (!endp || *endp != '\0') {
		shell_error(sh, "invalid channel: %s", argv[1]);
		return -EINVAL;
	}

	if (ch < 0 || ch > UINT16_MAX) {
		shell_error(sh, "channel out of range: %ld", ch);
		return -ERANGE;
	}

	int err = sniffer_set_channel((uint16_t)ch);

	if (err == -EBUSY) {
		shell_error(sh, "cannot change channel while running; run 'sniffer stop' first");
		return err;
	}

	if (err != 0) {
		shell_error(sh, "set channel failed (%d)", err);
		return err;
	}

	shell_print(sh, "channel %u", (unsigned int)sniffer_get_channel());
	return 0;
}

/* The reply has to reach the host before the rate changes: the ESP32 driver
 * resets the transmit FIFO in uart_configure(), which would drop whatever is
 * still queued, and the host waits for this line at the old rate.
 */
#define SNIFFER_SPEED_DRAIN_MS 20

static int cmd_sniffer_speed(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_print(sh, "speed %u", (unsigned int)sniffer_get_console_speed());
		return 0;
	}

	char *endp = NULL;
	unsigned long baudrate = strtoul(argv[1], &endp, 0);

	if (!endp || *endp != '\0') {
		shell_error(sh, "invalid baud rate: %s", argv[1]);
		return -EINVAL;
	}

	if (baudrate == 0UL || baudrate > UINT32_MAX) {
		shell_error(sh, "baud rate out of range: %s", argv[1]);
		return -ERANGE;
	}

	shell_print(sh, "speed %lu", baudrate);

	k_msleep(SNIFFER_SPEED_DRAIN_MS);

	int err = sniffer_set_console_speed((uint32_t)baudrate);

	if (err != 0) {
		/* Reported at the rate the console had before, which is still the
		 * one in use when the change failed.
		 */
		shell_error(sh, "set speed failed (%d)", err);
		return err;
	}

	return 0;
}

static int cmd_sniffer_stats(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct sniffer_stream_stats stats;

	sniffer_stream_get_stats(&stats);
	shell_print(sh,
		    "enq=%u drop=%u tx_rec=%u tx_bytes=%u q_hwm=%u q_used=%u/%u",
		    stats.rx_enqueued, stats.rx_dropped, stats.tx_records, stats.tx_bytes,
		    stats.q_high_wm, sniffer_stream_queue_used(), sniffer_stream_queue_capacity());
	return 0;
}

static int cmd_sniffer_stats_reset(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	sniffer_stream_reset_stats();
	shell_print(sh, "stats reset");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_sniffer,
	SHELL_CMD(start, NULL, "Start sniffing (enable RX).", cmd_sniffer_start),
	SHELL_CMD(stop, NULL, "Stop sniffing (disable RX).", cmd_sniffer_stop),
	SHELL_CMD(channel, NULL, "Get/set channel: channel [11..26].", cmd_sniffer_channel),
	SHELL_CMD(speed, NULL, "Get/set console baud rate: speed [baud].", cmd_sniffer_speed),
	SHELL_CMD(stats, NULL, "Show stream queue and traffic stats.", cmd_sniffer_stats),
	SHELL_CMD(stats_reset, NULL, "Reset stream stats counters.", cmd_sniffer_stats_reset),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sniffer, &sub_sniffer, "IEEE 802.15.4 sniffer control.", NULL);
