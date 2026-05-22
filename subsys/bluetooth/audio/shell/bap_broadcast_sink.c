/**
 * @file
 * @brief Shell APIs for Bluetooth BAP Broadcast Sink
 *
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/data.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/net_buf.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#include "audio/shell/audio.h"
#include "common/bt_shell_private.h"
#include "host/shell/bt.h"

static struct shell_stream broadcast_sink_streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
static struct broadcast_sink default_broadcast_sink;

#define PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO 20U /* Set the timeout relative to interval */
#define PA_SYNC_SKIP                      5U

struct bt_broadcast_info {
	uint32_t broadcast_id;
	char broadcast_name[BT_AUDIO_BROADCAST_NAME_LEN_MAX + 1];
};

static struct broadcast_sink_auto_scan {
	struct broadcast_sink *broadcast_sink;
	struct bt_broadcast_info broadcast_info;
} auto_scan = {
	.broadcast_info = {
		.broadcast_id = BT_BAP_INVALID_BROADCAST_ID,
	},
};

void bap_broadcast_sink_foreach_stream(void (*func)(struct shell_stream *sh_stream, void *data),
				       void *data)
{
	ARRAY_FOR_EACH_PTR(broadcast_sink_streams, stream) {
		func(stream, data);
	}
}

static void clear_auto_scan(void)
{
	(void)memset(&auto_scan, 0, sizeof(auto_scan));
	auto_scan.broadcast_info.broadcast_id = BT_BAP_INVALID_BROADCAST_ID;
}

static uint16_t interval_to_sync_timeout(uint16_t interval)
{
	uint32_t interval_us;
	uint32_t timeout;

	/* Add retries and convert to unit in 10's of ms */
	interval_us = BT_GAP_PER_ADV_INTERVAL_TO_US(interval);
	timeout =
		BT_GAP_US_TO_PER_ADV_SYNC_TIMEOUT(interval_us) * PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO;

	/* Enforce restraints */
	timeout = CLAMP(timeout, BT_GAP_PER_ADV_MIN_TIMEOUT, BT_GAP_PER_ADV_MAX_TIMEOUT);

	return (uint16_t)timeout;
}

static bool scan_check_and_get_broadcast_values(struct bt_data *data, void *user_data)
{
	struct bt_broadcast_info *sr_info = (struct bt_broadcast_info *)user_data;
	struct bt_uuid_16 adv_uuid;

	switch (data->type) {
	case BT_DATA_SVC_DATA16:
		if (data->data_len < BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE) {
			return true;
		}

		if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
			return true;
		}

		if (bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO) != 0) {
			return true;
		}

		sr_info->broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);
		return true;
	case BT_DATA_BROADCAST_NAME:
		if (!IN_RANGE(data->data_len, BT_AUDIO_BROADCAST_NAME_LEN_MIN,
			      BT_AUDIO_BROADCAST_NAME_LEN_MAX)) {
			return false;
		}

		(void)memcpy(sr_info->broadcast_name, data->data, data->data_len);
		sr_info->broadcast_name[data->data_len] = '\0';
		return true;
	default:
		return true;
	}
}

static void pa_sync_sink(const struct bt_le_scan_recv_info *info)
{
	struct bt_le_per_adv_sync_param create_params = {0};
	int err;

	err = bt_le_scan_stop();
	if (err != 0) {
		bt_shell_error("Could not stop scan: %d", err);
	}

	bt_addr_le_copy(&create_params.addr, info->addr);
	create_params.options = BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
	create_params.sid = info->sid;
	create_params.skip = PA_SYNC_SKIP;
	create_params.timeout = interval_to_sync_timeout(info->interval);

	bt_shell_print("Attempting to PA sync to the broadcaster");
	err = bt_le_per_adv_sync_create(&create_params, &per_adv_syncs[selected_per_adv_sync]);
	if (err != 0) {
		bt_shell_error("Could not create Broadcast PA sync: %d", err);
	} else {
		struct bt_le_per_adv_sync *pa_sync = per_adv_syncs[selected_per_adv_sync];
		struct scan_delegator_sync_state *sync_state = NULL;

		default_broadcast_sink.pa_sync = pa_sync;

		sync_state = scan_delegator_sync_state_get_by_values(
			auto_scan.broadcast_info.broadcast_id, info->addr->type, info->sid);
		if (sync_state == NULL) {
			sync_state = scan_delegator_sync_state_new();

			if (sync_state == NULL) {
				bt_shell_error("Could not get new sync state");

				return;
			}
		}

		sync_state->pa_sync = pa_sync;
		sync_state->broadcast_id = auto_scan.broadcast_info.broadcast_id;
	}
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	struct bt_broadcast_info sr_info = {0};
	bool identified_broadcast = false;

	sr_info.broadcast_id = BT_BAP_INVALID_BROADCAST_ID;

	if (!passes_scan_filter(info, ad)) {
		return;
	}

	bt_data_parse(ad, scan_check_and_get_broadcast_values, (void *)&sr_info);

	/* Verify that it is a BAP broadcaster*/
	if (sr_info.broadcast_id == BT_BAP_INVALID_BROADCAST_ID) {
		return;
	}

	bt_shell_print("Found broadcaster with ID 0x%06X (%s) and addr %s and sid 0x%02X "
		       "(scanning for 0x%06X (%s))",
		       sr_info.broadcast_id, sr_info.broadcast_name, bt_addr_le_str(info->addr),
		       info->sid, auto_scan.broadcast_info.broadcast_id,
		       auto_scan.broadcast_info.broadcast_name);

	if ((auto_scan.broadcast_info.broadcast_id == BT_BAP_INVALID_BROADCAST_ID) &&
	    (strlen(auto_scan.broadcast_info.broadcast_name) == 0U)) {
		/* no op */
		return;
	}

	if (sr_info.broadcast_id == auto_scan.broadcast_info.broadcast_id) {
		identified_broadcast = true;
	} else if ((strlen(auto_scan.broadcast_info.broadcast_name) != 0U) &&
		   is_substring(auto_scan.broadcast_info.broadcast_name, sr_info.broadcast_name)) {
		auto_scan.broadcast_info.broadcast_id = sr_info.broadcast_id;
		identified_broadcast = true;
	} else {
		/* no op */
		return;
	}

	bt_shell_print("Found matched broadcast with address %s%s", bt_addr_le_str(info->addr),
		       info->interval > 0U ? "" : " but is not syncable");

	if (info->interval > 0U && identified_broadcast && auto_scan.broadcast_sink != NULL) {
		pa_sync_sink(info);
	}
}

static void broadcast_sink_base_recv_cb(struct bt_bap_broadcast_sink *sink,
					const struct bt_bap_base *base, size_t base_size)
{
	/* Don't print duplicates */
	if (!util_eq(base, base_size, default_broadcast_sink.received_base,
		     default_broadcast_sink.base_size)) {
		bt_shell_print("Received BASE from sink %p:", sink);
		(void)memcpy(&default_broadcast_sink.received_base, base, base_size);
		default_broadcast_sink.base_size = base_size;

		print_base(base);
	}
}

static void broadcast_sink_syncable_cb(struct bt_bap_broadcast_sink *sink,
				       const struct bt_iso_biginfo *biginfo)
{
	if (default_broadcast_sink.bap_sink == sink) {
		if (default_broadcast_sink.syncable) {
			return;
		}

		bt_shell_print("Sink %p is ready to sync %s encryption", sink,
			       biginfo->encryption ? "with" : "without");
		default_broadcast_sink.syncable = true;
	}
}

static void broadcast_sink_stopped_cb(struct bt_bap_broadcast_sink *sink, uint8_t reason)
{
	bt_shell_info("Broadcast sink %p stopped with reason 0x%02X %s", sink, reason,
		      bt_hci_err_to_str(reason));

	/* All streams in the broadcast sink have been terminated */
	(void)memset(&default_broadcast_sink.received_base, 0,
		     sizeof(default_broadcast_sink.received_base));
	default_broadcast_sink.syncable = false;
}

static void bap_pa_sync_synced_cb(struct bt_le_per_adv_sync *sync,
				  struct bt_le_per_adv_sync_synced_info *info)
{
	ARG_UNUSED(info);

	if (auto_scan.broadcast_sink != NULL && auto_scan.broadcast_sink->pa_sync == sync) {
		bt_shell_print("PA synced to broadcast with broadcast ID 0x%06x",
			       auto_scan.broadcast_info.broadcast_id);

		if (auto_scan.broadcast_sink->bap_sink == NULL) {
			bt_shell_print("Attempting to create the sink");
			int err;

			err = bt_bap_broadcast_sink_create(sync,
							   auto_scan.broadcast_info.broadcast_id,
							   &auto_scan.broadcast_sink->bap_sink);
			if (err != 0) {
				bt_shell_error("Could not create broadcast sink: %d", err);
			}
		} else {
			bt_shell_print("Sink is already created");
		}
	}

	clear_auto_scan();
}

static void bap_pa_sync_terminated_cb(struct bt_le_per_adv_sync *sync,
				      const struct bt_le_per_adv_sync_term_info *info)
{
	ARG_UNUSED(info);

	if (default_broadcast_sink.pa_sync == sync) {
		default_broadcast_sink.syncable = false;
		(void)memset(&default_broadcast_sink.received_base, 0,
			     sizeof(default_broadcast_sink.received_base));
	}

	clear_auto_scan();
}

static void broadcast_scan_timeout_cb(void)
{
	bt_shell_print("Scan timeout");

	clear_auto_scan();
}

static struct bt_bap_broadcast_sink_cb sink_cbs = {
	.base_recv = broadcast_sink_base_recv_cb,
	.syncable = broadcast_sink_syncable_cb,
	.stopped = broadcast_sink_stopped_cb,
};

static struct bt_le_per_adv_sync_cb bap_pa_sync_cb = {
	.synced = bap_pa_sync_synced_cb,
	.term = bap_pa_sync_terminated_cb,
};

static struct bt_le_scan_cb bap_scan_cb = {
	.timeout = broadcast_scan_timeout_cb,
	.recv = broadcast_scan_recv,
};

static struct bt_bap_stream_ops stream_ops = {
	.recv = bap_stream_recv_cb,
	.started = bap_stream_started_cb,
	.stopped = bap_stream_stopped_cb,
};

static int cmd_bap_broadcast_sink_init(const struct shell *sh, size_t argc, char **argv)
{
	static bool initialized;
	int err;

	if (initialized) {
		shell_print(sh, "Already initialized");
		return -ENOEXEC;
	}

	err = bt_bap_broadcast_sink_register_cb(&sink_cbs);
	__ASSERT_NO_MSG(err == 0);

	err = bt_le_per_adv_sync_cb_register(&bap_pa_sync_cb);
	__ASSERT_NO_MSG(err == 0);

	err = bt_le_scan_cb_register(&bap_scan_cb);
	__ASSERT_NO_MSG(err == 0);

	ARRAY_FOR_EACH_PTR(broadcast_sink_streams, sh_stream) {
		bt_bap_stream_cb_register(bap_stream_from_shell_stream(sh_stream), &stream_ops);
	}

	initialized = true;

	return 0;
}

static int create_broadcast_sink(const struct shell *sh, struct bt_le_per_adv_sync *per_adv_sync,
				 uint32_t broadcast_id)
{
	struct scan_delegator_sync_state *sync_state = NULL;
	int err;

	shell_print(sh, "Creating broadcast sink with broadcast ID 0x%06X", broadcast_id);

	err = bt_bap_broadcast_sink_create(per_adv_sync, broadcast_id,
					   &default_broadcast_sink.bap_sink);
	if (err != 0) {
		shell_error(sh, "Failed to create broadcast sink: %d", err);

		return -ENOEXEC;
	}

	default_broadcast_sink.pa_sync = per_adv_sync;

	/* Lookup sync_state by PA sync or by values */
	sync_state = scan_delegator_sync_state_get_by_pa(per_adv_sync);
	if (sync_state == NULL) {
		struct bt_le_per_adv_sync_info sync_info;

		err = bt_le_per_adv_sync_get_info(per_adv_sync, &sync_info);
		if (err != 0) {
			bt_shell_error("Failed to get sync info: %d", err);
			err = bt_bap_broadcast_sink_delete(default_broadcast_sink.bap_sink);
			if (err != 0) {
				bt_shell_error("Failed to delete broadcast sink: %d", err);
			} else {
				default_broadcast_sink.bap_sink = NULL;
			}

			return -ENOEXEC;
		}

		sync_state = scan_delegator_sync_state_get_by_values(
			broadcast_id, sync_info.addr.type, sync_info.sid);
	}

	if (sync_state == NULL) {
		sync_state = scan_delegator_sync_state_new();

		if (sync_state == NULL) {
			bt_shell_error("Could not get new sync state");
			err = bt_bap_broadcast_sink_delete(default_broadcast_sink.bap_sink);
			if (err != 0) {
				bt_shell_error("Failed to delete broadcast sink: %d", err);
			} else {
				default_broadcast_sink.bap_sink = NULL;
			}

			return -ENOEXEC;
		}
	}

	sync_state->pa_sync = per_adv_sync;
	sync_state->broadcast_id = (uint32_t)broadcast_id;

	return 0;
}

static int cmd_create(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_le_per_adv_sync *per_adv_sync = per_adv_syncs[selected_per_adv_sync];

	ARG_UNUSED(argc);

	unsigned long broadcast_id;
	int err = 0;

	broadcast_id = shell_strtoul(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Could not parse broadcast_id: %d", err);

		return -ENOEXEC;
	}

	if (broadcast_id > BT_AUDIO_BROADCAST_ID_MAX) {
		shell_error(sh, "Invalid broadcast_id: %lu", broadcast_id);

		return -ENOEXEC;
	}

	if (per_adv_sync == NULL) {
		const struct bt_le_scan_param param = {
			.type = BT_LE_SCAN_TYPE_PASSIVE,
			.options = BT_LE_SCAN_OPT_NONE,
			.interval = BT_GAP_SCAN_FAST_INTERVAL,
			.window = BT_GAP_SCAN_FAST_WINDOW,
			.timeout = 1000U, /* 10ms units -> 10 second timeout */
		};

		shell_print(sh, "No PA sync available, starting scanning for broadcast_id");

		err = bt_le_scan_start(&param, NULL);
		if (err != 0) {
			shell_print(sh, "Fail to start scanning: %d", err);

			return -ENOEXEC;
		}

		auto_scan.broadcast_sink = &default_broadcast_sink;
		auto_scan.broadcast_info.broadcast_id = broadcast_id;
	} else {
		return create_broadcast_sink(sh, per_adv_sync, (uint32_t)broadcast_id);
	}

	return 0;
}

static int cmd_create_by_name(const struct shell *sh, size_t argc, char *argv[])
{
	const struct bt_le_scan_param param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
		.timeout = 1000U, /* 10ms units -> 10 second timeout */
	};
	char *broadcast_name;
	int err = 0;

	ARG_UNUSED(argc);

	broadcast_name = argv[1];
	if (!IN_RANGE(strlen(broadcast_name), BT_AUDIO_BROADCAST_NAME_LEN_MIN,
		      BT_AUDIO_BROADCAST_NAME_LEN_MAX)) {
		shell_error(sh, "Broadcast name should be minimum %d and maximum %d characters",
			    BT_AUDIO_BROADCAST_NAME_LEN_MIN, BT_AUDIO_BROADCAST_NAME_LEN_MAX);

		return -ENOEXEC;
	}

	shell_print(sh, "Starting scanning for broadcast_name");

	err = bt_le_scan_start(&param, NULL);
	if (err != 0) {
		shell_print(sh, "Fail to start scanning: %d", err);

		return -ENOEXEC;
	}

	(void)memcpy(auto_scan.broadcast_info.broadcast_name, broadcast_name,
		     strlen(broadcast_name));
	auto_scan.broadcast_info.broadcast_name[strlen(broadcast_name)] = '\0';

	auto_scan.broadcast_info.broadcast_id = BT_BAP_INVALID_BROADCAST_ID;
	auto_scan.broadcast_sink = &default_broadcast_sink;

	return 0;
}

static int cmd_sync(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_bap_stream *streams[ARRAY_SIZE(broadcast_sink_streams)];
	uint8_t bcode[BT_ISO_BROADCAST_CODE_SIZE] = {0};
	bool bcode_set = false;
	uint32_t bis_bitfield;
	int err = 0;
	size_t argn;

	bis_bitfield = 0U;
	argn = 1;
	while (argn < argc) {
		const char *arg = argv[argn];

		if (strcmp(argv[argn], "bcode") == 0) {
			size_t len;

			if (++argn == argc) {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}

			arg = argv[argn];

			len = hex2bin(arg, strlen(arg), bcode, sizeof(bcode));
			if (len == 0) {
				shell_print(sh, "Invalid broadcast code: %s", arg);

				return -ENOEXEC;
			}

			bcode_set = true;
		} else if (strcmp(argv[argn], "bcode_str") == 0) {
			if (++argn == argc) {
				shell_help(sh);

				return SHELL_CMD_HELP_PRINTED;
			}

			arg = argv[argn];

			if (strlen(arg) == 0U || strlen(arg) > sizeof(bcode)) {
				shell_print(sh, "Invalid broadcast code: %s", arg);

				return -ENOEXEC;
			}

			(void)memcpy(bcode, arg, strlen(arg));
			bcode_set = true;
		} else {
			unsigned long val;

			val = shell_strtoul(arg, 0, &err);
			if (err != 0) {
				shell_error(sh, "Could not parse BIS index val: %d", err);

				return -ENOEXEC;
			}

			if (!IN_RANGE(val, BT_ISO_BIS_INDEX_MIN, BT_ISO_BIS_INDEX_MAX)) {
				shell_error(sh, "Invalid index: %lu", val);

				return -ENOEXEC;
			}

			bis_bitfield |= BT_ISO_BIS_INDEX_BIT(val);
		}
	}

	if (default_broadcast_sink.bap_sink == NULL) {
		shell_error(sh, "No sink available");
		return -ENOEXEC;
	}

	(void)memset(streams, 0, sizeof(streams));
	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		streams[i] = bap_stream_from_shell_stream(&broadcast_sink_streams[i]);
	}

	err = bt_bap_broadcast_sink_sync(default_broadcast_sink.bap_sink, bis_bitfield, streams,
					 bcode_set ? bcode : NULL);
	if (err != 0) {
		shell_error(sh, "Failed to sync to broadcast: %d", err);
		return err;
	}

	return 0;
}

static int cmd_stop(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (default_broadcast_sink.bap_sink == NULL) {
		shell_error(sh, "No sink available");
		return -ENOEXEC;
	}

	err = bt_bap_broadcast_sink_stop(default_broadcast_sink.bap_sink);
	if (err != 0) {
		shell_error(sh, "Failed to stop sink: %d", err);
		return err;
	}

	return err;
}

static int cmd_delete(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (default_broadcast_sink.bap_sink == NULL) {
		shell_error(sh, "No sink available");
		return -ENOEXEC;
	}

	err = bt_bap_broadcast_sink_delete(default_broadcast_sink.bap_sink);
	if (err != 0) {
		shell_error(sh, "Failed to term sink: %d", err);
		return err;
	}

	default_broadcast_sink.bap_sink = NULL;
	default_broadcast_sink.syncable = false;

	return err;
}

static int cmd_bap_broadcast_sink(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

		return -ENOEXEC;
	}

	shell_help(sh);

	return SHELL_CMD_HELP_PRINTED;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	bap_broadcast_sink_cmds,
	SHELL_CMD_ARG(init, NULL, "Initialize and register callbacks", cmd_bap_broadcast_sink_init,
		      1, 0),
	SHELL_CMD_ARG(create, NULL, "Create the broadcast sink by Broadcast ID: 0x<broadcast_id>",
		      cmd_create, 2, 0),
	SHELL_CMD_ARG(create_by_name, NULL,
		      "Create the broadcast sink by Broadcast Name: <broadcast_name>",
		      cmd_create_by_name, 2, 0),
	SHELL_CMD_ARG(sync, NULL,
		      "Sync to one of my BIS indexes: "
		      "0x<bis_index> [[[0x<bis_index>] 0x<bis_index>] ...] "
		      "[bcode <broadcast code> || bcode_str <broadcast code as string>]",
		      cmd_sync, 2, ARRAY_SIZE(broadcast_sink_streams) + 1),
	SHELL_CMD_ARG(stop, NULL, "Stop the broadcast sink", cmd_stop, 1, 0),
	SHELL_CMD_ARG(term, NULL, "Delete the broadcast sink", cmd_delete, 1, 0),
	SHELL_SUBCMD_SET_END,
);

SHELL_CMD_ARG_REGISTER(bap_broadcast_sink, &bap_broadcast_sink_cmds,
		       "Bluetooth BAP Broadcast Sink shell commands", cmd_bap_broadcast_sink, 1, 1);
