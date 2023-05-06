/**
 * @file
 * @brief Shell APIs for Bluetooth CAP initiator
 *
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>

#include <zephyr/types.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/cap.h>

#include "shell/bt.h"
#include "audio.h"

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
#define CAP_UNICAST_CLIENT_STREAM_COUNT ARRAY_SIZE(unicast_streams)

static void cap_discover_cb(struct bt_conn *conn, int err,
			    const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (err != 0) {
		shell_error(ctx_shell, "discover failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "discovery completed%s",
		    csis_inst == NULL ? "" : " with CSIS");
}

static void cap_unicast_start_complete_cb(struct bt_bap_unicast_group *unicast_group,
					  int err, struct bt_conn *conn)
{
	if (err != 0) {
		shell_error(ctx_shell, "Unicast start failed for conn %p (%d)",
			    conn, err);
	} else {
		shell_print(ctx_shell, "Unicast start completed");
	}
}

static void unicast_update_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		shell_error(ctx_shell, "Unicast update failed for conn %p (%d)",
			    conn, err);
	} else {
		shell_print(ctx_shell, "Unicast updated completed");
	}
}

static void unicast_stop_complete_cb(struct bt_bap_unicast_group *unicast_group, int err,
				     struct bt_conn *conn)
{
	if (default_unicast_group != unicast_group) {
		/* ignore */
		return;
	}

	if (err != 0) {
		shell_error(ctx_shell,
			    "Unicast stop failed for group %p and conn %p (%d)",
			    unicast_group, conn, err);
	} else {
		shell_print(ctx_shell,
			    "Unicast stopped for group %p completed",
			    default_unicast_group);

		err = bt_bap_unicast_group_delete(unicast_group);
		if (err != 0) {
			shell_error(ctx_shell, "Failed to delete unicast group %p: %d",
				    unicast_group, err);
		} else {
			default_unicast_group = NULL;
		}
	}
}

static struct bt_cap_initiator_cb cbs = {
	.unicast_discovery_complete = cap_discover_cb,
	.unicast_start_complete = cap_unicast_start_complete_cb,
	.unicast_update_complete = unicast_update_complete_cb,
	.unicast_stop_complete = unicast_stop_complete_cb,
};

static int cmd_cap_initiator_discover(const struct shell *sh, size_t argc,
				    char *argv[])
{
	static bool cbs_registered;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (ctx_shell == NULL) {
		ctx_shell = sh;
	}

	if (!cbs_registered) {
		bt_cap_initiator_register_cb(&cbs);
		cbs_registered = true;
	}

	err = bt_cap_initiator_unicast_discover(default_conn);
	if (err != 0) {
		shell_error(sh, "Fail: %d", err);
	}

	return err;
}

static void populate_connected_conns(struct bt_conn *conn, void *data)
{
	struct bt_conn **connected_conns = (struct bt_conn **)data;

	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (connected_conns[i] == NULL) {
			connected_conns[i] = conn;
			return;
		}
	}
}

static void cap_copy_preset(struct unicast_stream *uni_stream,
			    const struct named_lc3_preset *name_preset)
{
	memcpy(&uni_stream->qos, &name_preset->preset.qos, sizeof(uni_stream->qos));
	memcpy(&uni_stream->codec, &name_preset->preset.codec,
	       sizeof(uni_stream->codec));

	/* Need to update the `bt_data.data` pointer to the new value after copying the codec */
	for (size_t i = 0U; i < ARRAY_SIZE(uni_stream->codec.data); i++) {
		struct bt_codec_data *data = &uni_stream->codec.data[i];

		data->data.data = data->value;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(uni_stream->codec.meta); i++) {
		struct bt_codec_data *data = &uni_stream->codec.meta[i];

		data->data.data = data->value;
	}
}

static int cmd_cap_initiator_unicast_start(const struct shell *sh, size_t argc,
					   char *argv[])
{
	struct bt_bap_unicast_group_stream_param
		group_stream_params[CAP_UNICAST_CLIENT_STREAM_COUNT] = {0};
	struct bt_bap_unicast_group_stream_pair_param
		pair_params[CAP_UNICAST_CLIENT_STREAM_COUNT] = {0};
	struct bt_cap_unicast_audio_start_stream_param
		stream_param[CAP_UNICAST_CLIENT_STREAM_COUNT] = {0};
	struct bt_conn *connected_conns[CONFIG_BT_MAX_CONN] = {0};
	struct bt_cap_unicast_audio_start_param start_param = {0};
	struct bt_bap_unicast_group_param group_param = {0};
	size_t source_cnt = 1U;
	ssize_t conn_cnt = 1U;
	size_t sink_cnt = 1U;
	size_t pair_cnt = 0U;
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	start_param.type = BT_CAP_SET_TYPE_AD_HOC;

	for (size_t argn = 1; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (strcmp(arg, "csip") == 0) {
			start_param.type = BT_CAP_SET_TYPE_CSIP;
		} else if (strcmp(arg, "sinks") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}

			sink_cnt = shell_strtoul(argv[argn], 10, &err);
		} else if (strcmp(arg, "sources") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}

			source_cnt = shell_strtoul(argv[argn], 10, &err);
		} else if (strcmp(arg, "conns") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}

			if (strcmp(argv[argn], "all") == 0) {
				conn_cnt = CONFIG_BT_MAX_CONN;
			} else {
				conn_cnt = shell_strtoul(argv[argn], 10, &err);

				/* Ensure that we do not iterate over more conns
				 * than the array supports
				 */
				conn_cnt = MIN(conn_cnt, ARRAY_SIZE(connected_conns));
			}
		} else {
			shell_help(sh);

			return SHELL_CMD_HELP_PRINTED;
		}

		if (err != 0) {
			shell_error(sh, "Failed to parse argument: %s: %s (%d)",
				    arg, argv[argn], err);

			return err;
		}
	}

	shell_print(sh, "Setting up %u sinks and %u sources on each (%u) conn", sink_cnt,
		    source_cnt, conn_cnt);

	/* Populate the array of connected connections */
	(void)memset(connected_conns, 0, sizeof(connected_conns));
	bt_conn_foreach(BT_CONN_TYPE_LE, populate_connected_conns,
			(void *)connected_conns);

	start_param.count = 0U;
	start_param.stream_params = stream_param;
	for (size_t i = 0; i < conn_cnt; i++) {
		struct bt_conn *conn = connected_conns[i];
		size_t conn_src_cnt = 0U;
		size_t conn_snk_cnt = 0U;

		if (conn == NULL) {
			break;
		}

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0
		conn_snk_cnt = sink_cnt;
		for (size_t i = 0U; i < sink_cnt; i++) {
			struct bt_cap_stream *stream =
				&unicast_streams[start_param.count].stream;
			struct unicast_stream *uni_stream =
				CONTAINER_OF(stream, struct unicast_stream, stream);
			struct bt_bap_ep *snk_ep = snks[bt_conn_index(conn)][i];

			if (snk_ep == NULL) {
				shell_info(sh, "Could only setup %zu/%zu sink endpoints",
					   i, sink_cnt);
				conn_snk_cnt = i;
				break;
			}

			stream_param[start_param.count].member.member = conn;
			stream_param[start_param.count].stream = stream;
			stream_param[start_param.count].ep = snk_ep;
			cap_copy_preset(uni_stream, default_sink_preset);
			stream_param[start_param.count].codec = &uni_stream->codec;
			stream_param[start_param.count].qos = &uni_stream->qos;

			group_stream_params[start_param.count].qos =
				stream_param[start_param.count].qos;
			group_stream_params[start_param.count].stream =
				&stream_param[start_param.count].stream->bap_stream;
			pair_params[pair_cnt + i].tx_param =
				&group_stream_params[start_param.count];

			start_param.count++;
		}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
		conn_src_cnt = source_cnt;
		for (size_t i = 0U; i < source_cnt; i++) {
			struct bt_cap_stream *stream =
				&unicast_streams[start_param.count].stream;
			struct unicast_stream *uni_stream =
				CONTAINER_OF(stream, struct unicast_stream, stream);
			struct bt_bap_ep *src_ep = srcs[bt_conn_index(conn)][i];

			if (src_ep == NULL) {
				shell_info(sh, "Could only setup %zu/%zu source endpoints",
					   i, source_cnt);
				conn_src_cnt = i;
				break;
			}

			stream_param[start_param.count].member.member = conn;
			stream_param[start_param.count].stream = stream;
			stream_param[start_param.count].ep = src_ep;
			cap_copy_preset(uni_stream, default_source_preset);
			stream_param[start_param.count].codec = &uni_stream->codec;
			stream_param[start_param.count].qos = &uni_stream->qos;

			group_stream_params[start_param.count].qos =
				stream_param[start_param.count].qos;
			group_stream_params[start_param.count].stream =
				&stream_param[start_param.count].stream->bap_stream;
			pair_params[pair_cnt + i].rx_param =
				&group_stream_params[start_param.count];

			start_param.count++;
		}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */

		/* Increment pair count with the max of sink and source for this connection, as
		 * we cannot have pairs across connections
		 */
		pair_cnt += MAX(conn_snk_cnt, conn_src_cnt);
	}

	if (pair_cnt == 0U) {
		shell_error(sh, "No streams to setup");

		return -ENOEXEC;
	}

	group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	group_param.params_count = pair_cnt;
	group_param.params = pair_params;

	if (default_unicast_group == NULL) {
		err = bt_bap_unicast_group_create(&group_param, &default_unicast_group);
		if (err != 0) {
			shell_print(sh, "Failed to create group: %d", err);

			return -ENOEXEC;
		}
	}

	shell_print(sh, "Starting %zu streams", start_param.count);

	err = bt_cap_initiator_unicast_audio_start(&start_param, default_unicast_group);
	if (err != 0) {
		shell_print(sh, "Failed to start unicast audio: %d", err);

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_cap_initiator_unicast_list(const struct shell *sh, size_t argc,
					  char *argv[])
{
	for (size_t i = 0U; i < ARRAY_SIZE(unicast_streams); i++) {
		if (unicast_streams[i].stream.bap_stream.conn == NULL) {
			break;
		}

		shell_print(sh, "Stream #%zu: %p", i, &unicast_streams[i].stream);
	}
	return 0;
}

static int cmd_cap_initiator_unicast_update(const struct shell *sh, size_t argc,
					    char *argv[])
{
	struct bt_cap_unicast_audio_update_param params[CAP_UNICAST_CLIENT_STREAM_COUNT];
	size_t count;
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	count = 0;

	if (argc == 2 && strcmp(argv[1], "all") == 0) {
		for (size_t i = 0U; i < ARRAY_SIZE(unicast_streams); i++) {
			struct bt_cap_stream *stream = &unicast_streams[i].stream;
			struct unicast_stream *uni_stream =
				CONTAINER_OF(stream, struct unicast_stream, stream);
			struct bt_bap_ep_info ep_info;

			if (stream->bap_stream.conn == NULL) {
				break;
			}

			err = bt_bap_ep_get_info(stream->bap_stream.ep, &ep_info);
			if (err != 0) {
				shell_error(sh, "Failed to get endpoint info: %d", err);

				return -ENOEXEC;
			}

			params[count].stream = stream;


			if (ep_info.dir == BT_AUDIO_DIR_SINK) {
				cap_copy_preset(uni_stream, default_sink_preset);
			} else {
				cap_copy_preset(uni_stream, default_source_preset);
			}

			params[count].meta = uni_stream->codec.meta;
			params[count].meta_count = uni_stream->codec.meta_count;

			count++;
		}

	} else {
		for (size_t i = 1U; i < argc; i++) {
			struct bt_cap_stream *stream = (void *)shell_strtoul(argv[i], 16, &err);
			struct unicast_stream *uni_stream =
				CONTAINER_OF(stream, struct unicast_stream, stream);
			struct bt_bap_ep_info ep_info;

			if (err != 0) {
				shell_error(sh, "Failed to parse stream argument %s: %d",
					argv[i], err);

				return err;
			}

			if (!PART_OF_ARRAY(unicast_streams, stream)) {
				shell_error(sh, "Pointer %p is not a CAP stream pointer",
					stream);

				return -ENOEXEC;
			}

			err = bt_bap_ep_get_info(stream->bap_stream.ep, &ep_info);
			if (err != 0) {
				shell_error(sh, "Failed to get endpoint info: %d", err);

				return -ENOEXEC;
			}

			params[count].stream = stream;

			if (ep_info.dir == BT_AUDIO_DIR_SINK) {
				cap_copy_preset(uni_stream, default_sink_preset);
			} else {
				cap_copy_preset(uni_stream, default_source_preset);
			}

			params[count].meta = uni_stream->codec.meta;
			params[count].meta_count = uni_stream->codec.meta_count;

			count++;
		}
	}

	if (count == 0) {
		shell_error(sh, "No streams to update");

		return -ENOEXEC;
	}

	shell_print(sh, "Updating %zu streams", count);

	err = bt_cap_initiator_unicast_audio_update(params, count);
	if (err != 0) {
		shell_print(sh, "Failed to update unicast audio: %d", err);
	}

	return err;
}

static int cmd_cap_initiator_unicast_stop(const struct shell *sh, size_t argc,
					  char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	} else if (default_unicast_group == NULL) {
		shell_error(sh, "No unicast group starteds");
		return -ENOEXEC;
	}

	err = bt_cap_initiator_unicast_audio_stop(default_unicast_group);
	if (err != 0) {
		shell_print(sh, "Failed to update unicast audio: %d", err);
	}

	return err;
}

#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */

static int cmd_cap_initiator(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(cap_initiator_cmds,
#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
	SHELL_CMD_ARG(discover, NULL,
		      "Discover CAS", cmd_cap_initiator_discover, 1, 0),
	SHELL_CMD_ARG(unicast-start, NULL,
		      "Unicast Start [csip] [sinks <cnt> (default 1)] "
		      "[sources <cnt> (default 1)] "
		      "[conns (<cnt> | all) (default 1)]",
		      cmd_cap_initiator_unicast_start, 1, 7),
	SHELL_CMD_ARG(unicast-list, NULL, "Unicast list streams",
		      cmd_cap_initiator_unicast_list, 1, 0),
	SHELL_CMD_ARG(unicast-update, NULL, "Unicast Update <all | stream [stream [stream...]]>",
		      cmd_cap_initiator_unicast_update, 2,
		      CAP_UNICAST_CLIENT_STREAM_COUNT),
	SHELL_CMD_ARG(unicast-stop, NULL, "Unicast stop all streams",
		      cmd_cap_initiator_unicast_stop, 1, 0),
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(cap_initiator, &cap_initiator_cmds,
		       "Bluetooth CAP initiator shell commands",
		       cmd_cap_initiator, 1, 1);
