/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

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
	if (err != 0) {
		shell_error(sh, "set channel failed (%d)", err);
		return err;
	}

	shell_print(sh, "channel %u", (unsigned int)sniffer_get_channel());
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
	SHELL_CMD(stats, NULL, "Show stream queue and traffic stats.", cmd_sniffer_stats),
	SHELL_CMD(stats_reset, NULL, "Reset stream stats counters.", cmd_sniffer_stats_reset),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sniffer, &sub_sniffer, "IEEE 802.15.4 sniffer control.", NULL);

