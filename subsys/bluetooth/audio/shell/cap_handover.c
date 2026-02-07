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
#include <stdint.h>
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

		default_source.broadcast_id = BT_BAP_INVALID_BROADCAST_ID;
		default_source.adv_sid = BT_GAP_SID_INVALID;
	} else if (err != 0) {
		bt_shell_error("Unicast to broadcast handover failed for conn %p (%d)", conn, err);

		default_source.broadcast_id = BT_BAP_INVALID_BROADCAST_ID;
		default_source.adv_sid = BT_GAP_SID_INVALID;
	} else {
		bt_shell_print(
			"Unicast to broadcast handover completed with new broadcast source %p",
			(void *)broadcast_source);
	}

	if (unicast_group == NULL) {
		default_unicast_group.is_cap = false;
		default_unicast_group.cap_group = NULL;
	}

	if (broadcast_source != NULL) {
		default_source.cap_source = broadcast_source;
		default_source.is_cap = true;
	}

	default_source.handover_in_progress = false;
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

	if (broadcast_source == NULL) {
		default_source.cap_source = NULL;
		default_source.is_cap = false;
		default_source.broadcast_id = BT_BAP_INVALID_BROADCAST_ID;
		default_source.adv_sid = BT_GAP_SID_INVALID;
	}

	if (unicast_group != NULL) {
		default_unicast_group.is_cap = true;
		default_unicast_group.cap_group = unicast_group;
	}

	default_source.handover_in_progress = false;
}

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

		if (broadcast_assistant_recv_states[bt_conn_index(bap_stream->conn)]
			    .recv_state_count == 0U) {
			return true; /* Stop iterating as it does not have a (discovered) BASS */
		}

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

static int register_callbacks(void)
{
	static bool registered;

	if (!registered) {
		static struct bt_cap_handover_cb cbs = {
			.unicast_to_broadcast_complete = unicast_to_broadcast_complete_cb,
			.broadcast_to_unicast_complete = broadcast_to_unicast_complete_cb,
		};
		int err;

		err = bt_cap_handover_register_cb(&cbs);
		if (err != 0) {
			bt_shell_error("Failed to register CAP handover callbacks: %d", err);
			return err;
		}

		registered = true;
	}

	return 0;
}

static int validate_and_parse_cmd_cap_handover_unicast_to_broadcast_args(
	const struct shell *sh, size_t argc, char *argv[],
	const struct named_lc3_preset **named_preset,
	uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE], bool *encrypted)
{

	for (size_t i = 1U; i < argc; i++) {
		char *arg = argv[i];

		if (strcmp(arg, "enc") == 0) {
			if (argc > i) {
				size_t bcode_len;

				i++;
				arg = argv[i];

				bcode_len = hex2bin(arg, strlen(arg), broadcast_code,
						    BT_ISO_BROADCAST_CODE_SIZE);

				if (bcode_len != BT_ISO_BROADCAST_CODE_SIZE) {
					shell_error(sh, "Invalid Broadcast Code Length: %zu",
						    bcode_len);

					return -ENOEXEC;
				}

				*encrypted = true;
			} else {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}
		} else if (strcmp(arg, "preset") == 0) {
			if (argc > i) {

				i++;
				arg = argv[i];

				*named_preset = bap_get_named_preset(false, BT_AUDIO_DIR_SINK, arg);
				if (*named_preset == NULL) {
					shell_error(sh, "Unable to parse named_preset %s", arg);

					return -ENOEXEC;
				}
			} else {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}
		} else {
			shell_help(sh);

			return SHELL_CMD_HELP_PRINTED;
		}
	}

	return 0;
}

static int cmd_cap_handover_unicast_to_broadcast(const struct shell *sh, size_t argc, char *argv[])
{
	const struct named_lc3_preset *named_preset = &default_broadcast_source_preset;
	struct bt_cap_handover_unicast_to_broadcast_param param = {0};
	struct cap_unicast_group_stream_lookup lookup_data = {0};
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	struct bt_le_ext_adv_info adv_info;
	uint32_t broadcast_id = 0U;
	int err;

	if (adv == NULL) {
		shell_error(sh, "Extended advertising set is NULL");

		return -ENOEXEC;
	}

	err = bt_le_ext_adv_get_info(adv, &adv_info);
	if (err != 0) {
		shell_error(sh, "Failed to get adv info: %d", err);
		return -ENOEXEC;
	}

	if (default_unicast_group.cap_group == NULL) {
		shell_error(sh, "CAP unicast group not created");

		return -ENOEXEC;
	}

	if (!default_unicast_group.is_cap) {
		shell_error(sh, "Unicast group is not CAP");

		return -ENOEXEC;
	}

	if (default_source.cap_source != NULL) {
		shell_error(sh, "CAP Broadcast source already created");

		return -ENOEXEC;
	}

	if (default_source.handover_in_progress) {
		shell_info(sh, "CAP Handover in progress");

		return -ENOEXEC;
	}

	(void)memset(cap_initiator_broadcast_stream_params, 0,
		     sizeof(cap_initiator_broadcast_stream_params));
	(void)memset(&cap_initiator_broadcast_subgroup_param, 0,
		     sizeof(cap_initiator_broadcast_subgroup_param));
	(void)memset(&cap_initiator_broadcast_create_param, 0,
		     sizeof(cap_initiator_broadcast_create_param));

	err = validate_and_parse_cmd_cap_handover_unicast_to_broadcast_args(
		sh, argc, argv, &named_preset, cap_initiator_broadcast_create_param.broadcast_code,
		&cap_initiator_broadcast_create_param.encryption);
	if (err != 0) {
		return err;
	}

	err = register_callbacks();
	if (err != 0) {
		return -ENOEXEC;
	}

	/* Lookup all active sink streams */
	err = bt_cap_unicast_group_foreach_stream(default_unicast_group.cap_group,
						  unicast_group_foreach_stream_cb, &lookup_data);
	if (err == -ECANCELED) {
		shell_error(sh, "BASS not discovered on all streams");

		return -ENOEXEC;
	} else if (err != 0) {
		shell_error(sh, "Failed to iterate on all streams: %d", err);

		return -ENOEXEC;
	}

	if (lookup_data.active_sink_streams_cnt == 0U) {
		shell_error(sh, "No active sink streams in group, cannot handover");

		return -ENOEXEC;
	}

	/* Generate the broadcast_id here even though the broadcast source is created later -
	 * We need to provide the broadcast ID that we expect to use when it is started
	 */
	err = bt_rand(&broadcast_id, BT_AUDIO_BROADCAST_ID_SIZE);
	if (err != 0) {
		shell_error(sh, "Unable to generate broadcast ID: %d", err);

		return -ENOEXEC;
	}

	shell_print(sh, "Generated broadcast_id 0x%06X", broadcast_id);

	copy_broadcast_source_preset(&default_source, named_preset);

	cap_initiator_broadcast_subgroup_param.stream_count = lookup_data.active_sink_streams_cnt;
	cap_initiator_broadcast_subgroup_param.stream_params =
		cap_initiator_broadcast_stream_params;
	cap_initiator_broadcast_subgroup_param.codec_cfg = &default_source.codec_cfg;
	for (size_t i = 0U; i < lookup_data.active_sink_streams_cnt; i++) {
		struct bt_cap_initiator_broadcast_stream_param *stream_param =
			&cap_initiator_broadcast_stream_params[i];

		cap_initiator_broadcast_subgroup_param.stream_params[i].stream =
			lookup_data.active_sink_streams[i];
		stream_param->data_len = 0U;
		stream_param->data = NULL;
	}

	cap_initiator_broadcast_create_param.subgroup_count = 1U;
	cap_initiator_broadcast_create_param.subgroup_params =
		&cap_initiator_broadcast_subgroup_param;
	cap_initiator_broadcast_create_param.qos = &default_source.qos;
	cap_initiator_broadcast_create_param.packing = BT_ISO_PACKING_SEQUENTIAL;

	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.unicast_group = default_unicast_group.cap_group;
	param.broadcast_create_param = &cap_initiator_broadcast_create_param;
	param.ext_adv = adv;
	param.pa_interval = BT_BAP_PA_INTERVAL_UNKNOWN;
	param.broadcast_id = broadcast_id;

	err = bt_cap_handover_unicast_to_broadcast(&param);
	if (err != 0) {
		shell_error(sh, "Failed to handover unicast audio to broadcast: %d", err);

		return -ENOEXEC;
	}

	default_source.handover_in_progress = true;
	default_source.broadcast_id = broadcast_id;
	default_source.addr_type = adv_info.addr->type;
	default_source.adv_sid = adv_info.sid;

	return 0;
}

struct cap_broadcast_source_stream_lookup {
	struct bt_cap_stream *streams[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
	size_t cnt;
};

static bool cap_initiator_broadcast_foreach_stream_cb(struct bt_cap_stream *cap_stream,
						      void *user_data)
{
	struct cap_broadcast_source_stream_lookup *data = user_data;
	const struct bt_bap_stream *bap_stream = &cap_stream->bap_stream;

	if (bap_stream->ep != NULL) {
		struct bt_bap_ep_info ep_info;
		int err;

		err = bt_bap_ep_get_info(bap_stream->ep, &ep_info);
		__ASSERT_NO_MSG(err == 0);

		/* Abort if broadcast source is not in the streaming state */
		if (ep_info.state != BT_BAP_EP_STATE_STREAMING) {
			return true;
		}
	}

	data->streams[data->cnt++] = cap_stream;

	return false;
}

static void populate_active_stream_conns(struct bt_conn *conn, void *data)
{
	struct bt_conn **connected_conns = (struct bt_conn **)data;
	size_t conn_cnt = 0U;

	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		const struct bt_bap_ep *snk_ep = snks[bt_conn_index(conn)][0];

		if (snk_ep == NULL) {
			bt_shell_info("Conn %p does not have any sink endpoints", conn);
			continue;
		}

		connected_conns[conn_cnt] = conn;
		conn_cnt++;
	}
}

static int validate_and_parse_cmd_cap_handover_broadcast_to_unicast(
	const struct shell *sh, size_t argc, char *argv[],
	const struct named_lc3_preset **named_preset, bool *all_conn, size_t *conn_cnt)
{
	for (size_t i = 1U; i < argc; i++) {
		const char *arg = argv[i];
		int err;

		if (strcmp(arg, "csip") == 0) {
			cap_initiator_unicast_audio_start_param.type = BT_CAP_SET_TYPE_CSIP;
		} else if (strcmp(arg, "conns") == 0) {
			if (++i == argc) {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}

			if (strcmp(argv[i], "all") == 0) {
				*all_conn = true;
				*conn_cnt = 0;
			} else {
				unsigned long conn_cnt_arg = shell_strtoul(argv[i], 10, &err);

				if (err != 0) {
					shell_error(
						sh,
						"Failed to parse conn_cnt argument: %s: %s (%d)",
						arg, argv[i], err);

					return err;
				}

				if (conn_cnt_arg > CONFIG_BT_MAX_CONN) {
					shell_error(sh, "Invalid number of connections: %lu",
						    conn_cnt_arg);

					return -ENOEXEC;
				}

				*conn_cnt = (size_t)conn_cnt_arg;
			}
		} else if (strcmp(arg, "preset") == 0) {
			if (++i == argc) {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}

			arg = argv[i];

			*named_preset = bap_get_named_preset(true, BT_AUDIO_DIR_SINK, arg);
			if (*named_preset == NULL) {
				shell_error(sh, "Unable to parse named_preset %s", arg);

				return -ENOEXEC;
			}
		} else {
			shell_help(sh);

			return SHELL_CMD_HELP_PRINTED;
		}
	}

	return 0;
}

static int cap_handover_broadcast_to_unicast_init_stream_params(
	const struct shell *sh, struct bt_conn *active_stream_conns[CONFIG_BT_MAX_CONN],
	size_t conn_cnt, const struct named_lc3_preset *named_preset,
	struct bt_cap_unicast_audio_start_param *start_param, size_t *stream_cnt)

{
	struct cap_broadcast_source_stream_lookup lookup_data = {0};
	int err;

	err = bt_cap_initiator_broadcast_foreach_stream(
		default_source.cap_source, cap_initiator_broadcast_foreach_stream_cb, &lookup_data);
	if (err != 0) {
		shell_error(sh, "Broadcast source not actively streaming");

		return -ENOEXEC;
	}

	if (MAX(lookup_data.cnt, conn_cnt) > CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT) {
		shell_error(sh, "Cannot setup %zu unicast streams in a single group (max %d)",
			    MAX(lookup_data.cnt, conn_cnt),
			    CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT);

		return -ENOEXEC;
	}

	start_param->stream_params = cap_initiator_audio_start_stream_params;
	if (lookup_data.cnt == 1U || conn_cnt == 1U) {
		/* If there is a single BIS, we attempt to setup a sink stream to all connected
		 * devices
		 *
		 * If there is a multiple BIS and a single ACL, we attempt to setup as many sink
		 * streams to that connection as possible
		 *
		 * conn_0 will use stream_params[0..count - 1],
		 * conn_1 will use stream_params[count..count * 2 - 1],
		 * conn_2 will use stream_params[count * 2..count * 3 - 1], etc.
		 */

		for (size_t i = 0U; i < conn_cnt; i++) {
			for (size_t j = 0U; j < lookup_data.cnt; j++) {
				struct bt_cap_unicast_audio_start_stream_param *stream_param =
					&cap_initiator_audio_start_stream_params[*stream_cnt];
				struct bt_cap_unicast_group_stream_param *group_stream_param =
					&cap_initiator_unicast_group_stream_params[*stream_cnt];
				struct bt_cap_unicast_group_stream_pair_param *stream_pair_param =
					&cap_initiator_unicast_group_pair_params[*stream_cnt];
				struct shell_stream *sh_stream = CONTAINER_OF(
					lookup_data.streams[j], struct shell_stream, stream);

				struct bt_bap_ep *snk_ep =
					snks[bt_conn_index(active_stream_conns[i])][j];

				if (snk_ep == NULL) {
					shell_info(sh,
						   "Could only setup %zu/%zu sink endpoints on %p",
						   j, lookup_data.cnt, active_stream_conns[i]);
					break;
				}

				stream_param->member.member = active_stream_conns[i];
				stream_param->stream = lookup_data.streams[j];
				stream_param->ep = snk_ep;
				copy_unicast_stream_preset(sh_stream, named_preset);
				stream_param->codec_cfg = &sh_stream->codec_cfg;

				group_stream_param->stream = stream_param->stream;
				group_stream_param->qos_cfg = &sh_stream->qos;
				stream_pair_param->tx_param = group_stream_param;

				(*stream_cnt)++;
			}
		}
	} else if (lookup_data.cnt == conn_cnt) {
		/* If there is the same amount of streams as connection, we set up one stream per
		 * device
		 *
		 * conn_0 will use stream_params[0],
		 * conn_1 will use stream_params[1],
		 * conn_2 will use stream_params[2], etc.
		 */

		for (size_t i = 0U; i < conn_cnt; i++) {
			struct bt_cap_unicast_audio_start_stream_param *stream_param =
				&cap_initiator_audio_start_stream_params[*stream_cnt];
			struct bt_cap_unicast_group_stream_param *group_stream_param =
				&cap_initiator_unicast_group_stream_params[*stream_cnt];
			struct bt_cap_unicast_group_stream_pair_param *stream_pair_param =
				&cap_initiator_unicast_group_pair_params[*stream_cnt];
			struct shell_stream *sh_stream =
				CONTAINER_OF(lookup_data.streams[i], struct shell_stream, stream);

			struct bt_bap_ep *snk_ep = snks[bt_conn_index(active_stream_conns[i])][0];

			if (snk_ep == NULL) {
				shell_info(sh, "Could not setup stream on %p",
					   active_stream_conns[i]);
				break;
			}

			stream_param->member.member = active_stream_conns[i];
			stream_param->stream = lookup_data.streams[i];
			stream_param->ep = snk_ep;
			copy_unicast_stream_preset(sh_stream, named_preset);
			stream_param->codec_cfg = &sh_stream->codec_cfg;

			group_stream_param->stream = stream_param->stream;
			group_stream_param->qos_cfg = &sh_stream->qos;
			stream_pair_param->tx_param = group_stream_param;

			(*stream_cnt)++;
		}
	} else {
		shell_error(sh, "Complex scenarios not supported; please use either\n"
				"\t * one connection and multiple streams\n"
				"\t * multiple connections and one stream\n"
				"\t * an identical amount of connections and streams");

		/* TODO: Support these cases*/

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_cap_handover_broadcast_to_unicast(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_cap_handover_broadcast_to_unicast_param param = {0};
	struct bt_conn *active_stream_conns[CONFIG_BT_MAX_CONN] = {0};
	const struct named_lc3_preset *named_preset;
	bool all_conn = false;
	size_t conn_cnt = 1U;
	int err;

	if (default_source.cap_source == NULL || !default_source.is_cap) {
		shell_error(sh, "No CAP broadcast source");

		return -ENOEXEC;
	}

	if (default_source.handover_in_progress) {
		shell_error(sh, "Handover already in progress");

		return -ENOEXEC;
	}

	(void)memset(&cap_commander_reception_stop_param, 0,
		     sizeof(cap_commander_reception_stop_param));
	(void)memset(&cap_commander_reception_stop_member_params, 0,
		     sizeof(cap_commander_reception_stop_member_params));
	(void)memset(&cap_initiator_audio_start_stream_params, 0,
		     sizeof(cap_initiator_audio_start_stream_params));
	(void)memset(&cap_initiator_unicast_group_stream_params, 0,
		     sizeof(cap_initiator_unicast_group_stream_params));
	(void)memset(&cap_initiator_unicast_group_pair_params, 0,
		     sizeof(cap_initiator_unicast_group_pair_params));
	(void)memset(&cap_initiator_unicast_group_param, 0,
		     sizeof(cap_initiator_unicast_group_param));
	(void)memset(&cap_initiator_unicast_audio_start_param, 0,
		     sizeof(cap_initiator_unicast_audio_start_param));
	cap_initiator_unicast_audio_start_param.type = BT_CAP_SET_TYPE_AD_HOC;

	named_preset = &default_sink_preset;

	err = validate_and_parse_cmd_cap_handover_broadcast_to_unicast(
		sh, argc, argv, &named_preset, &all_conn, &conn_cnt);
	if (err != 0) {
		return err;
	}

	/* Populate the array of connected connections and verify conn_cnt */
	(void)memset(active_stream_conns, 0, sizeof(active_stream_conns));
	bt_conn_foreach(BT_CONN_TYPE_LE, populate_active_stream_conns, (void *)active_stream_conns);
	for (size_t i = 0U; i < ARRAY_SIZE(active_stream_conns); i++) {
		if (active_stream_conns[i] == NULL) {
			if (conn_cnt > i) {
				shell_error(sh,
					    "Cannot perform action on %zu connections, only %zu "
					    "connected",
					    conn_cnt, i);

				return -ENOEXEC;
			}

			break;
		}

		if (broadcast_assistant_recv_states[bt_conn_index(active_stream_conns[i])]
			    .recv_state_count == 0U) {
			shell_error(sh, "Conn %p has not discovered BASS, cannot perform handover",
				    active_stream_conns[i]);

			return -ENOEXEC;
		}

		if (all_conn) {
			conn_cnt = i + 1;
		}
	}

	if (conn_cnt == 0U) {
		shell_error(sh, "No connections for action");

		return -ENOEXEC;
	}

	err = register_callbacks();
	if (err != 0) {
		shell_error(sh, "Failed to register callbacks: %d", err);

		return -ENOEXEC;
	}

	err = cap_handover_broadcast_to_unicast_init_stream_params(
		sh, active_stream_conns, conn_cnt, named_preset,
		&cap_initiator_unicast_audio_start_param,
		&cap_initiator_unicast_audio_start_param.count);
	if (err != 0) {
		return -ENOEXEC;
	}

	cap_initiator_unicast_group_param.params = cap_initiator_unicast_group_pair_params;
	cap_initiator_unicast_group_param.params_count =
		cap_initiator_unicast_audio_start_param.count;
	cap_initiator_unicast_group_param.packing = BT_ISO_PACKING_SEQUENTIAL;

	cap_commander_reception_stop_param.type = BT_CAP_SET_TYPE_AD_HOC;
	cap_commander_reception_stop_param.param = cap_commander_reception_stop_member_params;
	cap_commander_reception_stop_param.count = conn_cnt;
	for (size_t i = 0; i < conn_cnt; i++) {
		const struct broadcast_assistant_recv_state *recv_state =
			&broadcast_assistant_recv_states[bt_conn_index(active_stream_conns[i])];

		if (!recv_state->default_source_big_synced) {
			shell_error(sh, "Conn %p is not BIG synced to us, cannot perform handover",
				    active_stream_conns[i]);

			return -ENOEXEC;
		}

		cap_commander_reception_stop_param.param[i].member.member = active_stream_conns[i];
		cap_commander_reception_stop_param.param[i].src_id =
			recv_state->default_source_src_id;
		cap_commander_reception_stop_param.param[i].num_subgroups =
			recv_state->default_source_subgroup_count;
	}

	param.adv_type = default_source.addr_type;
	param.adv_sid = default_source.adv_sid;
	param.broadcast_id = default_source.broadcast_id;
	param.broadcast_source = default_source.cap_source;
	param.unicast_group_param = &cap_initiator_unicast_group_param;
	param.unicast_start_param = &cap_initiator_unicast_audio_start_param;
	param.reception_stop_param = &cap_commander_reception_stop_param;

	err = bt_cap_handover_broadcast_to_unicast(&param);
	if (err != 0) {
		shell_error(sh, "Failed to handover broadcast audio to unicast: %d", err);

		return -ENOEXEC;
	}

	default_source.handover_in_progress = true;

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
	cap_handover_cmds,
	SHELL_CMD_ARG(unicast_to_broadcast, NULL,
		      "Handover current unicast group to broadcast (unicast group will be deleted) "
		      "[enc <broadcast_code>] [preset <preset_name>]",
		      cmd_cap_handover_unicast_to_broadcast, 1, 4),
	SHELL_CMD_ARG(broadcast_to_unicast, NULL,
		      "Handover current broadcast source to unicast (broadcast source will be "
		      "deleted) [csip] [conns <count>|all] [preset <preset_name>]",
		      cmd_cap_handover_broadcast_to_unicast, 1, 4),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(cap_handover, &cap_handover_cmds, "Bluetooth CAP handover shell commands",
		       cmd_cap_handover, 1, 1);
