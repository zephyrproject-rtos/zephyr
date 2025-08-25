/**
 * @file
 * @brief Shell APIs for Bluetooth CAP handover
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/net_buf.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/cap.h>

#include "common/bt_shell_private.h"
#include "host/shell/bt.h"
#include "audio.h"

static void unicast_to_broadcast_complete_cb(int err, struct bt_conn *conn,
					     struct bt_cap_unicast_group *unicast_group,
					     struct bt_cap_broadcast_source *broadcast_source)
{
	if (err == -ECANCELED) {
		bt_shell_print("Unicast to broadcast handover was cancelled for conn %p", conn);
	} else if (err != 0) {
		bt_shell_error("Unicast to broadcast handover failed for conn %p (%d)", conn, err);
	} else {
		bt_shell_print(
			"Unicast to broadcast handover completed with new broadcast source %p",
			(void *)broadcast_source);
	}
}

static void broadcast_to_unicast_complete_cb(int err, struct bt_conn *conn,
					     struct bt_cap_broadcast_source *broadcast_source,
					     struct bt_cap_unicast_group *unicast_group)
{
	if (err == -ECANCELED) {
		bt_shell_print("Broadcast to unicast handover was cancelled for conn %p", conn);
	} else if (err != 0) {
		bt_shell_error("Broadcast to unicast handover failed for conn %p (%d)", conn, err);
	} else {
		bt_shell_print("Broadcast to unicast handover completed with new group %p",
			       (void *)unicast_group);
	}
}

static struct bt_cap_handover_cb cbs = {
	.unicast_to_broadcast_complete = unicast_to_broadcast_complete_cb,
	.broadcast_to_unicast_complete = broadcast_to_unicast_complete_cb,
};

struct cap_unicast_group_stream_lookup {
	struct bt_cap_stream *active_sink_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	size_t active_sink_streams_cnt;
};

static bool unicast_group_foreach_stream_cb(struct bt_cap_stream *cap_stream, void *user_data)
{
	struct cap_unicast_group_stream_lookup *data = user_data;
	const struct bt_bap_stream *bap_stream = &cap_stream->bap_stream;

	__ASSERT_NO_MSG(data->active_sink_streams_cnt < ARRAY_SIZE(data->active_sink_streams));

	if (bap_stream->ep != NULL) {
		struct bt_bap_ep_info ep_info;
		int err;

		err = bt_bap_ep_get_info(bap_stream->ep, &ep_info);
		__ASSERT_NO_MSG(err == 0);

		/* Only consider sink streams for handover to broadcast */
		if (ep_info.state == BT_BAP_EP_STATE_STREAMING &&
		    ep_info.dir == BT_AUDIO_DIR_SINK) {
			data->active_sink_streams[data->active_sink_streams_cnt++] = cap_stream;
		}
	}

	return false;
}

static int cmd_cap_handover_unicast_to_broadcast(const struct shell *sh, size_t argc, char *argv[])
{
	static struct bt_cap_initiator_broadcast_stream_param
		stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	static struct bt_cap_initiator_broadcast_subgroup_param subgroup_param = {0};
	struct bt_cap_initiator_broadcast_create_param broadcast_create_param;
	/* Struct containing the converted unicast group configuration */
	struct bt_cap_handover_unicast_to_broadcast_param param = {0};
	struct cap_unicast_group_stream_lookup lookup_data = {0};
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	const struct named_lc3_preset *named_preset;
	int err;

	if (adv == NULL) {
		shell_error(sh, "Extended advertising set is NULL");

		return -ENOEXEC;
	}

	if (default_unicast_group->cap_group == NULL) {
		shell_error(sh, "CAP unicast group not created");

		return -ENOEXEC;
	}

	if (!default_unicast_group->is_cap) {
		shell_error(sh, "Unicast group is not CAP");

		return -ENOEXEC;
	}

	if (default_source.cap_source != NULL) {
		shell_error(sh, "CAP Broadcast source already created");

		return -ENOEXEC;
	}

	for (size_t i = 1U; i < argc; i++) {
		char *arg = argv[i];

		if (strcmp(arg, "enc") == 0) {
			if (argc > i) {
				size_t bcode_len;

				i++;
				arg = argv[i];

				bcode_len = hex2bin(arg, strlen(arg),
						    broadcast_create_param.broadcast_code,
						    sizeof(broadcast_create_param.broadcast_code));

				if (bcode_len != sizeof(broadcast_create_param.broadcast_code)) {
					shell_error(sh, "Invalid Broadcast Code Length: %zu",
						    bcode_len);

					return -ENOEXEC;
				}

				broadcast_create_param.encryption = true;
			} else {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}
		} else if (strcmp(arg, "preset") == 0) {
			if (argc > i) {

				i++;
				arg = argv[i];

				named_preset =
					bap_get_named_preset(false, BT_AUDIO_DIR_SOURCE, arg);
				if (named_preset == NULL) {
					shell_error(sh, "Unable to parse named_preset %s", arg);

					return -ENOEXEC;
				}
			} else {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}
		}
	}

	err = bt_cap_unicast_group_foreach_stream(default_unicast_group->cap_group,
						  unicast_group_foreach_stream_cb, &lookup_data);
	__ASSERT_NO_MSG(err == 0);

	if (lookup_data.active_sink_streams_cnt == 0U) {
		shell_error(sh, "No active sink streams in group, cannot handover");

		return -ENOEXEC;
	}

	copy_broadcast_source_preset(&default_source, named_preset);

	subgroup_param.stream_count = lookup_data.active_sink_streams_cnt;
	subgroup_param.stream_params = stream_params;
	subgroup_param.codec_cfg = &default_source.codec_cfg;
	for (size_t i = 0U; i < lookup_data.active_sink_streams_cnt; i++) {
		subgroup_param.stream_params[i].stream = lookup_data.active_sink_streams[i];
		stream_params[i].data_len = 0U;
		stream_params[i].data = NULL;
	}

	broadcast_create_param.subgroup_count = 1U;
	broadcast_create_param.subgroup_params = &subgroup_param;
	broadcast_create_param.qos = &default_source.qos;
	broadcast_create_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	broadcast_create_param.encryption = false;

	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.unicast_group = default_unicast_group->cap_group;
	param.broadcast_create_param = &broadcast_create_param;
	param.ext_adv = adv;
	param.sid = adv_sid;
	param.pa_interval = BT_BAP_PA_INTERVAL_UNKNOWN;
	param.broadcast_id = broadcast_id;

	err = bt_cap_handover_unicast_to_broadcast(&param);
	if (err != 0) {
		shell_error("Failed to handover unicast audio to broadcast: %d", err);

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_cap_handover_broadcast_to_unicast(const struct shell *sh, size_t argc, char *argv[])
{
	return 0;
}

static int cmd_cap_handover(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	cap_initiator_cmds,
	SHELL_CMD_ARG(unicast_to_broadcast, NULL,
		      "Handover current unicast group to broadcast (unicast group will be deleted)",
		      cmd_cap_handover_unicast_to_broadcast, 1, 0),
	SHELL_CMD_ARG(
		broadcast_to_unicast, NULL,
		"Handover current broadcast source to unicast (broadcast source will be deleted)",
		cmd_cap_handover_broadcast_to_unicast, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(cap_initiator, &cap_initiator_cmds, "Bluetooth CAP initiator shell commands",
		       cmd_cap_handover, 1, 1);
