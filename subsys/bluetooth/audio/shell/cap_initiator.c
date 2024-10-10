/**
 * @file
 * @brief Shell APIs for Bluetooth CAP initiator
 *
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/net_buf.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/cap.h>

#include "host/shell/bt.h"
#include "audio.h"

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
#define UNICAST_SINK_SUPPORTED (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0)
#define UNICAST_SRC_SUPPORTED  (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0)

#define CAP_UNICAST_CLIENT_STREAM_COUNT ARRAY_SIZE(unicast_streams)

static void cap_discover_cb(struct bt_conn *conn, int err,
			    const struct bt_csip_set_coordinator_set_member *member,
			    const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (err != 0) {
		shell_error(ctx_shell, "discover failed (%d)", err);
		return;
	}

	shell_print(ctx_shell, "discovery completed%s",
		    csis_inst == NULL ? "" : " with CSIS");
}

static void cap_unicast_start_complete_cb(int err, struct bt_conn *conn)
{
	if (err == -ECANCELED) {
		shell_print(ctx_shell, "Unicast start was cancelled for conn %p", conn);
	} else if (err != 0) {
		shell_error(ctx_shell, "Unicast start failed for conn %p (%d)", conn, err);
	} else {
		shell_print(ctx_shell, "Unicast start completed");
	}
}

static void unicast_update_complete_cb(int err, struct bt_conn *conn)
{
	if (err == -ECANCELED) {
		shell_print(ctx_shell, "Unicast update was cancelled for conn %p", conn);
	} else if (err != 0) {
		shell_error(ctx_shell, "Unicast update failed for conn %p (%d)",
			    conn, err);
	} else {
		shell_print(ctx_shell, "Unicast updated completed");
	}
}

static void unicast_stop_complete_cb(int err, struct bt_conn *conn)
{
	if (err == -ECANCELED) {
		shell_print(ctx_shell, "Unicast stop was cancelled for conn %p", conn);
	} else if (err != 0) {
		shell_error(ctx_shell, "Unicast stop failed for conn %p (%d)", conn, err);
	} else {
		shell_print(ctx_shell, "Unicast stop completed");

		if (default_unicast_group != NULL) {
			err = bt_bap_unicast_group_delete(default_unicast_group);
			if (err != 0) {
				shell_error(ctx_shell, "Failed to delete unicast group %p: %d",
					    default_unicast_group, err);
			} else {
				default_unicast_group = NULL;
			}
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
		for (size_t j = 0U; j < sink_cnt; j++) {
			struct bt_cap_stream *stream =
				&unicast_streams[start_param.count].stream;
			struct shell_stream *uni_stream =
				CONTAINER_OF(stream, struct shell_stream, stream);
			struct bt_bap_ep *snk_ep = snks[bt_conn_index(conn)][j];

			if (snk_ep == NULL) {
				shell_info(sh, "Could only setup %zu/%zu sink endpoints",
					   j, sink_cnt);
				conn_snk_cnt = j;
				break;
			}

			stream_param[start_param.count].member.member = conn;
			stream_param[start_param.count].stream = stream;
			stream_param[start_param.count].ep = snk_ep;
			copy_unicast_stream_preset(uni_stream, &default_sink_preset);
			stream_param[start_param.count].codec_cfg = &uni_stream->codec_cfg;

			group_stream_params[start_param.count].stream =
				&stream_param[start_param.count].stream->bap_stream;
			group_stream_params[start_param.count].qos = &uni_stream->qos;
			pair_params[pair_cnt + j].tx_param =
				&group_stream_params[start_param.count];

			start_param.count++;
		}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
		conn_src_cnt = source_cnt;
		for (size_t j = 0U; j < source_cnt; j++) {
			struct bt_cap_stream *stream =
				&unicast_streams[start_param.count].stream;
			struct shell_stream *uni_stream =
				CONTAINER_OF(stream, struct shell_stream, stream);
			struct bt_bap_ep *src_ep = srcs[bt_conn_index(conn)][j];

			if (src_ep == NULL) {
				shell_info(sh, "Could only setup %zu/%zu source endpoints",
					   j, source_cnt);
				conn_src_cnt = j;
				break;
			}

			stream_param[start_param.count].member.member = conn;
			stream_param[start_param.count].stream = stream;
			stream_param[start_param.count].ep = src_ep;
			copy_unicast_stream_preset(uni_stream, &default_source_preset);
			stream_param[start_param.count].codec_cfg = &uni_stream->codec_cfg;
			group_stream_params[start_param.count].stream =
				&stream_param[start_param.count].stream->bap_stream;
			group_stream_params[start_param.count].qos = &uni_stream->qos;
			pair_params[pair_cnt + j].rx_param =
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

	err = bt_cap_initiator_unicast_audio_start(&start_param);
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
	struct bt_cap_unicast_audio_update_stream_param
		stream_params[CAP_UNICAST_CLIENT_STREAM_COUNT] = {0};
	struct bt_cap_unicast_audio_update_param param = {0};
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (argc == 2 && strcmp(argv[1], "all") == 0) {
		for (size_t i = 0U; i < ARRAY_SIZE(unicast_streams); i++) {
			struct bt_cap_stream *stream = &unicast_streams[i].stream;
			struct shell_stream *uni_stream =
				CONTAINER_OF(stream, struct shell_stream, stream);
			struct bt_bap_ep_info ep_info;

			if (stream->bap_stream.conn == NULL) {
				break;
			}

			err = bt_bap_ep_get_info(stream->bap_stream.ep, &ep_info);
			if (err != 0) {
				shell_error(sh, "Failed to get endpoint info: %d", err);

				return -ENOEXEC;
			}

			stream_params[param.count].stream = stream;

			if (ep_info.dir == BT_AUDIO_DIR_SINK) {
				copy_unicast_stream_preset(uni_stream, &default_sink_preset);
			} else {
				copy_unicast_stream_preset(uni_stream, &default_source_preset);
			}

			stream_params[param.count].meta = uni_stream->codec_cfg.meta;
			stream_params[param.count].meta_len = uni_stream->codec_cfg.meta_len;

			param.count++;
		}

	} else {
		for (size_t i = 1U; i < argc; i++) {
			struct bt_cap_stream *stream = (void *)shell_strtoul(argv[i], 16, &err);
			struct shell_stream *uni_stream =
				CONTAINER_OF(stream, struct shell_stream, stream);
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

			stream_params[param.count].stream = stream;

			if (ep_info.dir == BT_AUDIO_DIR_SINK) {
				copy_unicast_stream_preset(uni_stream, &default_sink_preset);
			} else {
				copy_unicast_stream_preset(uni_stream, &default_source_preset);
			}

			stream_params[param.count].meta = uni_stream->codec_cfg.meta;
			stream_params[param.count].meta_len = uni_stream->codec_cfg.meta_len;

			param.count++;
		}
	}

	if (param.count == 0) {
		shell_error(sh, "No streams to update");

		return -ENOEXEC;
	}

	param.stream_params = stream_params;
	param.type = BT_CAP_SET_TYPE_AD_HOC;

	shell_print(sh, "Updating %zu streams", param.count);

	err = bt_cap_initiator_unicast_audio_update(&param);
	if (err != 0) {
		shell_print(sh, "Failed to update unicast audio: %d", err);
	}

	return err;
}

static int cmd_cap_initiator_unicast_stop(const struct shell *sh, size_t argc,
					  char *argv[])
{
	struct bt_cap_stream *streams[CAP_UNICAST_CLIENT_STREAM_COUNT];
	struct bt_cap_unicast_audio_stop_param param = {0};
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (argc == 1) {
		for (size_t i = 0U; i < ARRAY_SIZE(unicast_streams); i++) {
			struct bt_cap_stream *stream = &unicast_streams[i].stream;
			struct bt_bap_ep_info ep_info;

			if (stream->bap_stream.conn == NULL) {
				break;
			}

			err = bt_bap_ep_get_info(stream->bap_stream.ep, &ep_info);
			if (err != 0) {
				shell_error(sh, "Failed to get endpoint info: %d", err);

				return -ENOEXEC;
			}

			streams[param.count] = stream;
			param.count++;
		}

	} else {
		for (size_t i = 1U; i < argc; i++) {
			struct bt_cap_stream *stream = (void *)shell_strtoul(argv[i], 16, &err);
			struct bt_bap_ep_info ep_info;

			if (err != 0) {
				shell_error(sh, "Failed to parse stream argument %s: %d", argv[i],
					    err);

				return err;
			}

			if (!PART_OF_ARRAY(unicast_streams, stream)) {
				shell_error(sh, "Pointer %p is not a CAP stream pointer", stream);

				return -ENOEXEC;
			}

			err = bt_bap_ep_get_info(stream->bap_stream.ep, &ep_info);
			if (err != 0) {
				shell_error(sh, "Failed to get endpoint info: %d", err);

				return -ENOEXEC;
			}

			streams[param.count] = stream;
			param.count++;
		}
	}

	if (param.count == 0) {
		shell_error(sh, "No streams to update");

		return -ENOEXEC;
	}

	param.streams = streams;
	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.release = true;

	err = bt_cap_initiator_unicast_audio_stop(&param);
	if (err != 0) {
		shell_print(sh, "Failed to update unicast audio: %d", err);
	}

	return err;
}

static int cmd_cap_initiator_unicast_cancel(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	err = bt_cap_initiator_unicast_audio_cancel();
	if (err != 0) {
		shell_print(sh, "Failed to cancel unicast audio procedure: %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cap_ac_unicast_start(const struct bap_unicast_ac_param *param,
				struct bt_conn *connected_conns[],
				struct shell_stream *snk_uni_streams[], size_t snk_cnt,
				struct shell_stream *src_uni_streams[], size_t src_cnt)
{
	struct bt_cap_unicast_audio_start_stream_param stream_params[BAP_UNICAST_AC_MAX_STREAM] = {
		0};
	struct bt_audio_codec_cfg *snk_codec_cfgs[BAP_UNICAST_AC_MAX_SNK] = {0};
	struct bt_audio_codec_cfg *src_codec_cfgs[BAP_UNICAST_AC_MAX_SRC] = {0};
	struct bt_cap_stream *snk_cap_streams[BAP_UNICAST_AC_MAX_SNK] = {0};
	struct bt_cap_stream *src_cap_streams[BAP_UNICAST_AC_MAX_SRC] = {0};
	struct bt_cap_unicast_audio_start_param start_param = {0};
	struct bt_bap_ep *snk_eps[BAP_UNICAST_AC_MAX_SNK] = {0};
	struct bt_bap_ep *src_eps[BAP_UNICAST_AC_MAX_SRC] = {0};
	size_t snk_stream_cnt = 0U;
	size_t src_stream_cnt = 0U;
	size_t stream_cnt = 0U;
	size_t snk_ep_cnt = 0U;
	size_t src_ep_cnt = 0U;

	for (size_t i = 0U; i < param->conn_cnt; i++) {
#if UNICAST_SINK_SUPPORTED
		for (size_t j = 0U; j < param->snk_cnt[i]; j++) {
			snk_eps[snk_ep_cnt] = snks[bt_conn_index(connected_conns[i])][j];
			if (snk_eps[snk_ep_cnt] == NULL) {
				shell_error(ctx_shell, "No sink[%zu][%zu] endpoint available", i,
					    j);

				return -ENOEXEC;
			}
			snk_ep_cnt++;
		}
#endif /* UNICAST_SINK_SUPPORTED */

#if UNICAST_SRC_SUPPORTED
		for (size_t j = 0U; j < param->src_cnt[i]; j++) {
			src_eps[src_ep_cnt] = srcs[bt_conn_index(connected_conns[i])][j];
			if (src_eps[src_ep_cnt] == NULL) {
				shell_error(ctx_shell, "No source[%zu][%zu] endpoint available", i,
					    j);

				return -ENOEXEC;
			}
			src_ep_cnt++;
		}
#endif /* UNICAST_SRC_SUPPORTED  */
	}

	if (snk_ep_cnt != snk_cnt) {
		shell_error(ctx_shell, "Sink endpoint and stream count mismatch: %zu != %zu",
			    snk_ep_cnt, snk_cnt);

		return -ENOEXEC;
	}

	if (src_ep_cnt != src_cnt) {
		shell_error(ctx_shell, "Source  endpoint and stream count mismatch: %zu != %zu",
			    src_ep_cnt, src_cnt);

		return -ENOEXEC;
	}

	/* Setup arrays of parameters based on the preset for easier access. This also copies the
	 * preset so that we can modify them (e.g. update the metadata)
	 */
	for (size_t i = 0U; i < snk_cnt; i++) {
		snk_cap_streams[i] = &snk_uni_streams[i]->stream;
		snk_codec_cfgs[i] = &snk_uni_streams[i]->codec_cfg;
	}

	for (size_t i = 0U; i < src_cnt; i++) {
		src_cap_streams[i] = &src_uni_streams[i]->stream;
		src_codec_cfgs[i] = &src_uni_streams[i]->codec_cfg;
	}

	/* CAP Start */
	for (size_t i = 0U; i < param->conn_cnt; i++) {
		for (size_t j = 0U; j < param->snk_cnt[i]; j++) {
			struct bt_cap_unicast_audio_start_stream_param *stream_param =
				&stream_params[stream_cnt];

			stream_param->member.member = connected_conns[i];
			stream_param->codec_cfg = snk_codec_cfgs[snk_stream_cnt];
			stream_param->ep = snk_eps[snk_stream_cnt];
			stream_param->stream = snk_cap_streams[snk_stream_cnt];

			snk_stream_cnt++;
			stream_cnt++;
		}

		for (size_t j = 0U; j < param->src_cnt[i]; j++) {
			struct bt_cap_unicast_audio_start_stream_param *stream_param =
				&stream_params[stream_cnt];

			stream_param->member.member = connected_conns[i];
			stream_param->codec_cfg = src_codec_cfgs[src_stream_cnt];
			stream_param->ep = src_eps[src_stream_cnt];
			stream_param->stream = src_cap_streams[src_stream_cnt];

			src_stream_cnt++;
			stream_cnt++;
		}
	}

	start_param.stream_params = stream_params;
	start_param.count = stream_cnt;
	start_param.type = BT_CAP_SET_TYPE_AD_HOC;

	return bt_cap_initiator_unicast_audio_start(&start_param);
}

static int set_codec_config(const struct shell *sh, struct shell_stream *sh_stream,
			    struct named_lc3_preset *preset, size_t conn_cnt, size_t ep_cnt,
			    size_t chan_cnt, size_t conn_index, size_t ep_index)
{
	enum bt_audio_location new_chan_alloc;
	enum bt_audio_location chan_alloc;
	int err;

	copy_unicast_stream_preset(sh_stream, preset);

	if (chan_cnt == 1U) {
		/* - When we have a single channel on a single connection then we make it mono
		 * - When we have a single channel on a multiple connections then we make it left on
		 *   the first connection and right on the second connection
		 * - When we have multiple channels streams for a connection, we make them either
		 *   left or right, regardless of the connection count
		 */
		if (ep_cnt == 1) {
			if (conn_cnt == 1) {
				new_chan_alloc = BT_AUDIO_LOCATION_MONO_AUDIO;
			} else if (conn_cnt == 2) {
				if (conn_index == 0) {
					new_chan_alloc = BT_AUDIO_LOCATION_FRONT_LEFT;
				} else if (conn_index == 1) {
					new_chan_alloc = BT_AUDIO_LOCATION_FRONT_RIGHT;
				} else {
					return 0;
				}
			} else {
				return 0;
			}
		} else if (ep_cnt == 2) {
			if (ep_index == 0) {
				new_chan_alloc = BT_AUDIO_LOCATION_FRONT_LEFT;
			} else if (ep_index == 1) {
				new_chan_alloc = BT_AUDIO_LOCATION_FRONT_RIGHT;
			} else {
				return 0;
			}
		} else {
			return 0;
		}
	} else if (chan_cnt == 2U) {
		/* Some audio configuration requires multiple sink channels,
		 * so multiply the SDU based on the channel count
		 */
		sh_stream->qos.sdu *= chan_cnt;

		/* If a stream has 2 channels, we make it stereo */
		new_chan_alloc = BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT;

	} else {
		return 0;
	}

	err = bt_audio_codec_cfg_get_chan_allocation(&sh_stream->codec_cfg, &chan_alloc, false);
	if (err != 0) {
		if (err == -ENODATA) {
			chan_alloc = BT_AUDIO_LOCATION_MONO_AUDIO;
		}
	}

	if (chan_alloc != new_chan_alloc) {
		shell_info(sh,
			   "[%zu][%zu]: Overwriting existing channel allocation 0x%08X with 0x%08X",
			   conn_index, ep_index, chan_alloc, new_chan_alloc);

		err = bt_audio_codec_cfg_set_chan_allocation(&sh_stream->codec_cfg, new_chan_alloc);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

int cap_ac_unicast(const struct shell *sh, const struct bap_unicast_ac_param *param)
{
	/* Allocate params large enough for any params, but only use what is required */
	struct bt_conn *connected_conns[BAP_UNICAST_AC_MAX_CONN] = {0};
	struct shell_stream *snk_uni_streams[BAP_UNICAST_AC_MAX_SNK];
	struct shell_stream *src_uni_streams[BAP_UNICAST_AC_MAX_SRC];
	size_t conn_avail_cnt;
	size_t snk_cnt = 0;
	size_t src_cnt = 0;
	int err;

	if (default_unicast_group != NULL) {
		shell_error(sh, "Unicast Group already exist, please delete first");
		return -ENOEXEC;
	}

	if (param->conn_cnt > BAP_UNICAST_AC_MAX_CONN) {
		shell_error(sh, "Invalid conn_cnt: %zu", param->conn_cnt);
		return -ENOEXEC;
	}

	for (size_t i = 0; i < param->conn_cnt; i++) {
		/* Verify conn values */
		if (param->snk_cnt[i] > BAP_UNICAST_AC_MAX_SNK) {
			shell_error(sh, "Invalid conn_snk_cnt[%zu]: %zu", i, param->snk_cnt[i]);
			return -ENOEXEC;
		}

		if (param->src_cnt[i] > BAP_UNICAST_AC_MAX_SRC) {
			shell_error(sh, "Invalid conn_src_cnt[%zu]: %zu", i, param->src_cnt[i]);
			return -ENOEXEC;
		}
	}

	/* Populate the array of connected connections */
	bt_conn_foreach(BT_CONN_TYPE_LE, populate_connected_conns, (void *)connected_conns);
	for (conn_avail_cnt = 0; conn_avail_cnt < ARRAY_SIZE(connected_conns); conn_avail_cnt++) {
		if (connected_conns[conn_avail_cnt] == NULL) {
			break;
		}
	}

	if (conn_avail_cnt < param->conn_cnt) {
		shell_error(sh,
			    "Only %zu/%u connected devices, please connect additional devices for "
			    "this audio configuration",
			    conn_avail_cnt, param->conn_cnt);
		return -ENOEXEC;
	}

	/* Set all endpoints from multiple connections in a single array, and verify that the known
	 * endpoints matches the audio configuration
	 */
	for (size_t i = 0U; i < param->conn_cnt; i++) {
		for (size_t j = 0U; j < param->snk_cnt[i]; j++) {
			struct shell_stream *snk_uni_stream;

			snk_uni_stream = snk_uni_streams[snk_cnt] = &unicast_streams[snk_cnt];

			err = set_codec_config(sh, snk_uni_stream, &default_sink_preset,
					       param->conn_cnt, param->snk_cnt[i],
					       param->snk_chan_cnt, i, j);
			if (err != 0) {
				shell_error(sh, "Failed to set codec configuration: %d", err);

				return -ENOEXEC;
			}

			snk_cnt++;
		}

		for (size_t j = 0U; j < param->src_cnt[i]; j++) {
			struct shell_stream *src_uni_stream;

			src_uni_stream = snk_uni_streams[src_cnt] = &unicast_streams[src_cnt];

			err = set_codec_config(sh, src_uni_stream, &default_source_preset,
					       param->conn_cnt, param->src_cnt[i],
					       param->src_chan_cnt, i, j);
			if (err != 0) {
				shell_error(sh, "Failed to set codec configuration: %d", err);

				return -ENOEXEC;
			}

			src_cnt++;
		}
	}

	if (!ctx_shell) {
		ctx_shell = sh;
	}

	err = bap_ac_create_unicast_group(param, snk_uni_streams, snk_cnt, src_uni_streams,
					  src_cnt);
	if (err != 0) {
		shell_error(sh, "Failed to create group: %d", err);

		return -ENOEXEC;
	}

	shell_print(sh, "Starting %zu streams for %s", snk_cnt + src_cnt, param->name);
	err = cap_ac_unicast_start(param, connected_conns, snk_uni_streams, snk_cnt,
				   src_uni_streams, src_cnt);
	if (err != 0) {
		shell_error(sh, "Failed to start unicast audio: %d", err);

		err = bt_bap_unicast_group_delete(default_unicast_group);
		if (err != 0) {
			shell_error(sh, "Failed to delete group: %d", err);
		} else {
			default_unicast_group = NULL;
		}

		return -ENOEXEC;
	}

	return 0;
}

#if UNICAST_SINK_SUPPORTED
static int cmd_cap_ac_1(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_1",
		.conn_cnt = 1U,
		.snk_cnt = {1U},
		.src_cnt = {0U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 0U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* UNICAST_SINK_SUPPORTED */

#if UNICAST_SRC_SUPPORTED
static int cmd_cap_ac_2(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_2",
		.conn_cnt = 1U,
		.snk_cnt = {0U},
		.src_cnt = {1U},
		.snk_chan_cnt = 0U,
		.src_chan_cnt = 1U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* UNICAST_SRC_SUPPORTED */

#if UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED
static int cmd_cap_ac_3(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_3",
		.conn_cnt = 1U,
		.snk_cnt = {1U},
		.src_cnt = {1U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 1U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED */

#if UNICAST_SINK_SUPPORTED
static int cmd_cap_ac_4(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_4",
		.conn_cnt = 1,
		.snk_cnt = {1U},
		.src_cnt = {0U},
		.snk_chan_cnt = 2U,
		.src_chan_cnt = 0U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* UNICAST_SINK_SUPPORTED */

#if UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED
static int cmd_cap_ac_5(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_5",
		.conn_cnt = 1U,
		.snk_cnt = {1U},
		.src_cnt = {1U},
		.snk_chan_cnt = 2U,
		.src_chan_cnt = 1U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED */

#if UNICAST_SINK_SUPPORTED
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1
static int cmd_cap_ac_6_i(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_6_I",
		.conn_cnt = 1U,
		.snk_cnt = {2U},
		.src_cnt = {0U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 0U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1 */

#if CONFIG_BT_MAX_CONN >= 2
static int cmd_cap_ac_6_ii(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_6_II",
		.conn_cnt = 2U,
		.snk_cnt = {1U, 1U},
		.src_cnt = {0U, 0U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 0U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* CONFIG_BT_MAX_CONN >= 2 */
#endif /* UNICAST_SINK_SUPPORTED */

#if UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED
static int cmd_cap_ac_7_i(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_7_I",
		.conn_cnt = 1U,
		.snk_cnt = {1U}, /* TODO: These should be separate CIS */
		.src_cnt = {1U}, /* TODO: These should be separate CIS */
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 1U,
	};

	return cap_ac_unicast(sh, &param);
}

#if CONFIG_BT_MAX_CONN >= 2
static int cmd_cap_ac_7_ii(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_7_II",
		.conn_cnt = 2U,
		.snk_cnt = {1U, 0U},
		.src_cnt = {0U, 1U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 1U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* CONFIG_BT_MAX_CONN >= 2 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1
static int cmd_cap_ac_8_i(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_8_I",
		.conn_cnt = 1U,
		.snk_cnt = {2U},
		.src_cnt = {1U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 1U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1 */

#if CONFIG_BT_MAX_CONN >= 2
static int cmd_cap_ac_8_ii(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_8_II",
		.conn_cnt = 2U,
		.snk_cnt = {1U, 1U},
		.src_cnt = {1U, 0U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 1U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* CONFIG_BT_MAX_CONN >= 2 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1
static int cmd_cap_ac_9_i(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_9_I",
		.conn_cnt = 1U,
		.snk_cnt = {0U},
		.src_cnt = {2U},
		.snk_chan_cnt = 0U,
		.src_chan_cnt = 1U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1 */

#if CONFIG_BT_MAX_CONN >= 2 && CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1
static int cmd_cap_ac_9_ii(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_9_II",
		.conn_cnt = 2U,
		.snk_cnt = {0U, 0U},
		.src_cnt = {1U, 1U},
		.snk_chan_cnt = 0U,
		.src_chan_cnt = 1U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* CONFIG_BT_MAX_CONN >= 2 && CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1
static int cmd_cap_ac_10(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_10",
		.conn_cnt = 1U,
		.snk_cnt = {0U},
		.src_cnt = {1U},
		.snk_chan_cnt = 0U,
		.src_chan_cnt = 2U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1 && CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1
static int cmd_cap_ac_11_i(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_11_I",
		.conn_cnt = 1U,
		.snk_cnt = {2U},
		.src_cnt = {2U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 1U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1 &&                                        \
	* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1                                           \
	*/

#if CONFIG_BT_MAX_CONN >= 2
static int cmd_cap_ac_11_ii(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_11_II",
		.conn_cnt = 2U,
		.snk_cnt = {1U, 1U},
		.src_cnt = {1U, 1U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 1U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* CONFIG_BT_MAX_CONN >= 2 */
#endif /* UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED */
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
static int cmd_broadcast_start(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_le_ext_adv *adv = adv_sets[selected_adv];
	int err;

	if (adv == NULL) {
		shell_info(sh, "Extended advertising set is NULL");

		return -ENOEXEC;
	}

	if (default_source.cap_source == NULL || !default_source.is_cap) {
		shell_info(sh, "CAP Broadcast source not created");

		return -ENOEXEC;
	}

	err = bt_cap_initiator_broadcast_audio_start(default_source.cap_source,
						     adv_sets[selected_adv]);
	if (err != 0) {
		shell_error(sh, "Unable to start broadcast source: %d", err);

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_broadcast_update(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t meta[CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE];
	size_t len;
	int err;

	if (default_source.cap_source == NULL || !default_source.is_cap) {
		shell_info(sh, "CAP Broadcast source not created");

		return -ENOEXEC;
	}

	len = hex2bin(argv[1], strlen(argv[1]), meta, sizeof(meta));
	if (len == 0) {
		shell_print(sh, "Unable to parse metadata (len was %zu, max len is %d)",
			    strlen(argv[1]) / 2U + strlen(argv[1]) % 2U,
			    CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE);

		return -ENOEXEC;
	}

	err = bt_cap_initiator_broadcast_audio_update(default_source.cap_source, meta, len);
	if (err != 0) {
		shell_error(sh, "Unable to update broadcast source: %d", err);

		return -ENOEXEC;
	}

	shell_print(sh, "CAP Broadcast source updated with new metadata. Update the advertised "
			"base via `bt per-adv-data`");

	return 0;
}

static int cmd_broadcast_stop(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_source.cap_source == NULL || !default_source.is_cap) {
		shell_info(sh, "CAP Broadcast source not created");

		return -ENOEXEC;
	}

	err = bt_cap_initiator_broadcast_audio_stop(default_source.cap_source);
	if (err != 0) {
		shell_error(sh, "Unable to stop broadcast source: %d", err);

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_broadcast_delete(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_source.cap_source == NULL || !default_source.is_cap) {
		shell_info(sh, "CAP Broadcast source not created");

		return -ENOEXEC;
	}

	err = bt_cap_initiator_broadcast_audio_delete(default_source.cap_source);
	if (err != 0) {
		shell_error(sh, "Unable to stop broadcast source: %d", err);

		return -ENOEXEC;
	}

	default_source.cap_source = NULL;
	default_source.is_cap = false;

	return 0;
}

int cap_ac_broadcast(const struct shell *sh, size_t argc, char **argv,
			    const struct bap_broadcast_ac_param *param)
{
	/* TODO: Use CAP API when the CAP shell has broadcast support */
	struct bt_cap_initiator_broadcast_stream_param stream_params[BAP_UNICAST_AC_MAX_SRC] = {0};
	uint8_t stereo_data[] = {
		BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
				    BT_AUDIO_LOCATION_FRONT_RIGHT | BT_AUDIO_LOCATION_FRONT_LEFT)};
	uint8_t right_data[] = {BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
						    BT_AUDIO_LOCATION_FRONT_RIGHT)};
	uint8_t left_data[] = {BT_AUDIO_CODEC_DATA(BT_AUDIO_CODEC_CFG_CHAN_ALLOC,
						   BT_AUDIO_LOCATION_FRONT_LEFT)};
	struct bt_cap_initiator_broadcast_subgroup_param subgroup_param = {0};
	struct bt_cap_initiator_broadcast_create_param create_param = {0};
	struct bt_le_ext_adv *adv;
	int err;

	if (default_source.cap_source != NULL) {
		shell_error(sh, "Broadcast Source already created, please delete first");
		return -ENOEXEC;
	}

	adv = adv_sets[selected_adv];
	if (adv == NULL) {
		shell_error(sh, "Extended advertising set is NULL");
		return -ENOEXEC;
	}

	copy_broadcast_source_preset(&default_source, &default_broadcast_source_preset);
	default_source.qos.sdu *= param->chan_cnt;

	for (size_t i = 0U; i < param->stream_cnt; i++) {
		stream_params[i].stream = &broadcast_source_streams[i].stream;

		if (param->stream_cnt == 1U) {
			stream_params[i].data_len = ARRAY_SIZE(stereo_data);
			stream_params[i].data = stereo_data;
		} else if (i == 0U) {
			stream_params[i].data_len = ARRAY_SIZE(left_data);
			stream_params[i].data = left_data;
		} else if (i == 1U) {
			stream_params[i].data_len = ARRAY_SIZE(right_data);
			stream_params[i].data = right_data;
		}
	}

	subgroup_param.stream_count = param->stream_cnt;
	subgroup_param.stream_params = stream_params;
	subgroup_param.codec_cfg = &default_source.codec_cfg;
	create_param.subgroup_count = 1U;
	create_param.subgroup_params = &subgroup_param;
	create_param.qos = &default_source.qos;

	err = bt_cap_initiator_broadcast_audio_create(&create_param, &default_source.cap_source);
	if (err != 0) {
		shell_error(sh, "Failed to create broadcast source: %d", err);
		return -ENOEXEC;
	}

	/* We don't start the broadcast source here, because in order to populate the BASE in the
	 * periodic advertising data, the broadcast source needs to be created but not started.
	 */
	shell_print(sh,
		    "CAP Broadcast source for %s created. "
		    "Start via `cap_initiator broadcast_start`, "
		    "and update / set the base via `bt per-adv data`",
		    param->name);
	default_source.is_cap = true;

	return 0;
}

static int cmd_cap_ac_12(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_broadcast_ac_param param = {
		.name = "AC_12",
		.stream_cnt = 1U,
		.chan_cnt = 1U,
	};

	return cap_ac_broadcast(sh, argc, argv, &param);
}

#if CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT > 1
static int cmd_cap_ac_13(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_broadcast_ac_param param = {
		.name = "AC_13",
		.stream_cnt = 2U,
		.chan_cnt = 1U,
	};

	return cap_ac_broadcast(sh, argc, argv, &param);
}
#endif /* CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT > 1 */

static int cmd_cap_ac_14(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_broadcast_ac_param param = {
		.name = "AC_14",
		.stream_cnt = 2U,
		.chan_cnt = 2U,
	};

	return cap_ac_broadcast(sh, argc, argv, &param);
}
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */

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

SHELL_STATIC_SUBCMD_SET_CREATE(
	cap_initiator_cmds,
#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
	SHELL_CMD_ARG(discover, NULL, "Discover CAS", cmd_cap_initiator_discover, 1, 0),
	SHELL_CMD_ARG(unicast_start, NULL,
		      "Unicast Start [csip] [sinks <cnt> (default 1)] "
		      "[sources <cnt> (default 1)] "
		      "[conns (<cnt> | all) (default 1)]",
		      cmd_cap_initiator_unicast_start, 1, 7),
	SHELL_CMD_ARG(unicast_list, NULL, "Unicast list streams", cmd_cap_initiator_unicast_list, 1,
		      0),
	SHELL_CMD_ARG(unicast_update, NULL, "Unicast Update <all | stream [stream [stream...]]>",
		      cmd_cap_initiator_unicast_update, 2, CAP_UNICAST_CLIENT_STREAM_COUNT),
	SHELL_CMD_ARG(unicast_stop, NULL,
		      "Unicast stop streams [stream [stream [stream...]]] (all by default)",
		      cmd_cap_initiator_unicast_stop, 1, CAP_UNICAST_CLIENT_STREAM_COUNT),
	SHELL_CMD_ARG(unicast_cancel, NULL, "Unicast cancel current procedure",
		      cmd_cap_initiator_unicast_cancel, 1, 0),
#if UNICAST_SINK_SUPPORTED
	SHELL_CMD_ARG(ac_1, NULL, "Unicast audio configuration 1", cmd_cap_ac_1, 1, 0),
#endif /* UNICAST_SINK_SUPPORTED */
#if UNICAST_SRC_SUPPORTED
	SHELL_CMD_ARG(ac_2, NULL, "Unicast audio configuration 2", cmd_cap_ac_2, 1, 0),
#endif /* UNICAST_SRC_SUPPORTED */
#if UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED
	SHELL_CMD_ARG(ac_3, NULL, "Unicast audio configuration 3", cmd_cap_ac_3, 1, 0),
#endif /* UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED */
#if UNICAST_SINK_SUPPORTED
	SHELL_CMD_ARG(ac_4, NULL, "Unicast audio configuration 4", cmd_cap_ac_4, 1, 0),
#endif /* UNICAST_SINK_SUPPORTED */
#if UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED
	SHELL_CMD_ARG(ac_5, NULL, "Unicast audio configuration 5", cmd_cap_ac_5, 1, 0),
#endif /* UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED */
#if UNICAST_SINK_SUPPORTED
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1
	SHELL_CMD_ARG(ac_6_i, NULL, "Unicast audio configuration 6(i)", cmd_cap_ac_6_i, 1, 0),
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1 */
#if CONFIG_BT_MAX_CONN >= 2
	SHELL_CMD_ARG(ac_6_ii, NULL, "Unicast audio configuration 6(ii)", cmd_cap_ac_6_ii, 1, 0),
#endif /* CONFIG_BT_MAX_CONN >= 2 */
#endif /* UNICAST_SINK_SUPPORTED */
#if UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED
	SHELL_CMD_ARG(ac_7_i, NULL, "Unicast audio configuration 7(i)", cmd_cap_ac_7_i, 1, 0),
#if CONFIG_BT_MAX_CONN >= 2
	SHELL_CMD_ARG(ac_7_ii, NULL, "Unicast audio configuration 7(ii)", cmd_cap_ac_7_ii, 1, 0),
#endif /* CONFIG_BT_MAX_CONN >= 2 */
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1
	SHELL_CMD_ARG(ac_8_i, NULL, "Unicast audio configuration 8(i)", cmd_cap_ac_8_i, 1, 0),
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1 */
#if CONFIG_BT_MAX_CONN >= 2
	SHELL_CMD_ARG(ac_8_ii, NULL, "Unicast audio configuration 8(ii)", cmd_cap_ac_8_ii, 1, 0),
#endif /* CONFIG_BT_MAX_CONN >= 2 */
#if UNICAST_SRC_SUPPORTED
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1
	SHELL_CMD_ARG(ac_9_i, NULL, "Unicast audio configuration 9(i)", cmd_cap_ac_9_i, 1, 0),
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1 */
#if CONFIG_BT_MAX_CONN >= 2
	SHELL_CMD_ARG(ac_9_ii, NULL, "Unicast audio configuration 9(ii)", cmd_cap_ac_9_ii, 1, 0),
#endif /* CONFIG_BT_MAX_CONN >= 2 */
	SHELL_CMD_ARG(ac_10, NULL, "Unicast audio configuration 10", cmd_cap_ac_10, 1, 0),
#endif /* UNICAST_SRC_SUPPORTED */
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1 && CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1
	SHELL_CMD_ARG(ac_11_i, NULL, "Unicast audio configuration 11(i)", cmd_cap_ac_11_i, 1, 0),
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1 &&                                        \
	* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1                                           \
	*/
#if CONFIG_BT_MAX_CONN >= 2
	SHELL_CMD_ARG(ac_11_ii, NULL, "Unicast audio configuration 11(ii)", cmd_cap_ac_11_ii, 1, 0),
#endif /* CONFIG_BT_MAX_CONN >= 2 */
#endif /* UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED */
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */
#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
	SHELL_CMD_ARG(broadcast_start, NULL, "", cmd_broadcast_start, 1, 0),
	SHELL_CMD_ARG(broadcast_update, NULL, "<meta>", cmd_broadcast_update, 2, 0),
	SHELL_CMD_ARG(broadcast_stop, NULL, "", cmd_broadcast_stop, 1, 0),
	SHELL_CMD_ARG(broadcast_delete, NULL, "", cmd_broadcast_delete, 1, 0),
	SHELL_CMD_ARG(ac_12, NULL, "Broadcast audio configuration 12", cmd_cap_ac_12, 1, 0),
#if CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT > 1
	SHELL_CMD_ARG(ac_13, NULL, "Broadcast audio configuration 13", cmd_cap_ac_13, 1, 0),
#endif /* CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT > 1 */
	SHELL_CMD_ARG(ac_14, NULL, "Broadcast audio configuration 14", cmd_cap_ac_14, 1, 0),
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(cap_initiator, &cap_initiator_cmds,
		       "Bluetooth CAP initiator shell commands",
		       cmd_cap_initiator, 1, 1);

static ssize_t nonconnectable_ad_data_add(struct bt_data *data_array, const size_t data_array_size)
{
#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
	if (default_source.cap_source != NULL && default_source.is_cap) {
		static uint8_t ad_cap_broadcast_announcement[5] = {
			BT_UUID_16_ENCODE(BT_UUID_BROADCAST_AUDIO_VAL),
		};
		uint32_t broadcast_id;
		int err;

		err = bt_cap_initiator_broadcast_get_id(default_source.cap_source, &broadcast_id);
		if (err != 0) {
			printk("Unable to get broadcast ID: %d\n", err);

			return -1;
		}

		sys_put_le24(broadcast_id, &ad_cap_broadcast_announcement[2]);
		data_array[0].type = BT_DATA_SVC_DATA16;
		data_array[0].data_len = ARRAY_SIZE(ad_cap_broadcast_announcement);
		data_array[0].data = ad_cap_broadcast_announcement;

		return 1;
	}
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */

	return 0;
}

ssize_t cap_initiator_ad_data_add(struct bt_data *data_array, const size_t data_array_size,
				  const bool discoverable, const bool connectable)
{
	if (!discoverable) {
		return 0;
	}

	if (!connectable) {
		return nonconnectable_ad_data_add(data_array, data_array_size);
	}

	return 0;
}

ssize_t cap_initiator_pa_data_add(struct bt_data *data_array, const size_t data_array_size)
{
#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
	if (default_source.cap_source != NULL && default_source.is_cap) {
		/* Required size of the buffer depends on what has been
		 * configured. We just use the maximum size possible.
		 */
		NET_BUF_SIMPLE_DEFINE_STATIC(base_buf, UINT8_MAX);
		int err;

		net_buf_simple_reset(&base_buf);

		err = bt_cap_initiator_broadcast_get_base(default_source.cap_source, &base_buf);
		if (err != 0) {
			printk("Unable to get BASE: %d\n", err);

			return -1;
		}

		data_array[0].type = BT_DATA_SVC_DATA16;
		data_array[0].data_len = base_buf.len;
		data_array[0].data = base_buf.data;

		return 1;
	}
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */

	return 0;
}
