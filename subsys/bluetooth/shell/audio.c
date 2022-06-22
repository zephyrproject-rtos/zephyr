/** @file
 *  @brief Bluetooth Basic Audio Profile shell
 *
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <zephyr/zephyr.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/capabilities.h>

#include "bt.h"

#define LOCATION BT_AUDIO_LOCATION_FRONT_LEFT
#define CONTEXT BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA

#if defined(CONFIG_BT_AUDIO_UNICAST)
#define MAX_PAC 2
static struct bt_audio_stream streams[MAX_PAC];

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)
static struct bt_audio_unicast_group *default_unicast_group;
static struct bt_codec *rcodecs[2][CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT];
static struct bt_audio_ep *snks[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_audio_ep *srcs[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT];
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */
#endif /* CONFIG_BT_AUDIO_UNICAST */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
static struct bt_audio_stream broadcast_source_streams[CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT];
static struct bt_audio_broadcast_source *default_source;
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */
#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
static struct bt_audio_stream broadcast_sink_streams[BROADCAST_SNK_STREAM_CNT];
static struct bt_audio_broadcast_sink *default_sink;
#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK */
static struct bt_audio_stream *default_stream;
static uint32_t seq_num;
static bool connecting;

struct named_lc3_preset {
	const char *name;
	struct bt_audio_lc3_preset preset;
};

static struct named_lc3_preset lc3_unicast_presets[] = {
	{"8_1_1",   BT_AUDIO_LC3_UNICAST_PRESET_8_1_1(LOCATION, CONTEXT)},
	{"8_2_1",   BT_AUDIO_LC3_UNICAST_PRESET_8_2_1(LOCATION, CONTEXT)},
	{"16_1_1",  BT_AUDIO_LC3_UNICAST_PRESET_16_1_1(LOCATION, CONTEXT)},
	{"16_2_1",  BT_AUDIO_LC3_UNICAST_PRESET_16_2_1(LOCATION, CONTEXT)},
	{"24_1_1",  BT_AUDIO_LC3_UNICAST_PRESET_24_1_1(LOCATION, CONTEXT)},
	{"24_2_1",  BT_AUDIO_LC3_UNICAST_PRESET_24_2_1(LOCATION, CONTEXT)},
	{"32_1_1",  BT_AUDIO_LC3_UNICAST_PRESET_32_1_1(LOCATION, CONTEXT)},
	{"32_2_1",  BT_AUDIO_LC3_UNICAST_PRESET_32_2_1(LOCATION, CONTEXT)},
	{"441_1_1", BT_AUDIO_LC3_UNICAST_PRESET_441_1_1(LOCATION, CONTEXT)},
	{"441_2_1", BT_AUDIO_LC3_UNICAST_PRESET_441_2_1(LOCATION, CONTEXT)},
	{"48_1_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_1_1(LOCATION, CONTEXT)},
	{"48_2_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_2_1(LOCATION, CONTEXT)},
	{"48_3_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_3_1(LOCATION, CONTEXT)},
	{"48_4_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_4_1(LOCATION, CONTEXT)},
	{"48_5_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_5_1(LOCATION, CONTEXT)},
	{"48_6_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_6_1(LOCATION, CONTEXT)},
	/* High-reliability presets */
	{"8_1_2",   BT_AUDIO_LC3_UNICAST_PRESET_8_1_2(LOCATION, CONTEXT)},
	{"8_2_2",   BT_AUDIO_LC3_UNICAST_PRESET_8_2_2(LOCATION, CONTEXT)},
	{"16_1_2",  BT_AUDIO_LC3_UNICAST_PRESET_16_1_2(LOCATION, CONTEXT)},
	{"16_2_2",  BT_AUDIO_LC3_UNICAST_PRESET_16_2_2(LOCATION, CONTEXT)},
	{"24_1_2",  BT_AUDIO_LC3_UNICAST_PRESET_24_1_2(LOCATION, CONTEXT)},
	{"24_2_2",  BT_AUDIO_LC3_UNICAST_PRESET_24_2_2(LOCATION, CONTEXT)},
	{"32_1_2",  BT_AUDIO_LC3_UNICAST_PRESET_32_1_2(LOCATION, CONTEXT)},
	{"32_2_2",  BT_AUDIO_LC3_UNICAST_PRESET_32_2_2(LOCATION, CONTEXT)},
	{"441_1_2", BT_AUDIO_LC3_UNICAST_PRESET_441_1_2(LOCATION, CONTEXT)},
	{"441_2_2", BT_AUDIO_LC3_UNICAST_PRESET_441_2_2(LOCATION, CONTEXT)},
	{"48_1_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_1_2(LOCATION, CONTEXT)},
	{"48_2_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_2_2(LOCATION, CONTEXT)},
	{"48_3_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_3_2(LOCATION, CONTEXT)},
	{"48_4_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_4_2(LOCATION, CONTEXT)},
	{"48_5_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_5_2(LOCATION, CONTEXT)},
	{"48_6_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_6_2(LOCATION, CONTEXT)},
};

static struct named_lc3_preset lc3_broadcast_presets[] = {
	{"8_1_1",   BT_AUDIO_LC3_BROADCAST_PRESET_8_1_1(LOCATION, CONTEXT)},
	{"8_2_1",   BT_AUDIO_LC3_BROADCAST_PRESET_8_2_1(LOCATION, CONTEXT)},
	{"16_1_1",  BT_AUDIO_LC3_BROADCAST_PRESET_16_1_1(LOCATION, CONTEXT)},
	{"16_2_1",  BT_AUDIO_LC3_BROADCAST_PRESET_16_2_1(LOCATION, CONTEXT)},
	{"24_1_1",  BT_AUDIO_LC3_BROADCAST_PRESET_24_1_1(LOCATION, CONTEXT)},
	{"24_2_1",  BT_AUDIO_LC3_BROADCAST_PRESET_24_2_1(LOCATION, CONTEXT)},
	{"32_1_1",  BT_AUDIO_LC3_BROADCAST_PRESET_32_1_1(LOCATION, CONTEXT)},
	{"32_2_1",  BT_AUDIO_LC3_BROADCAST_PRESET_32_2_1(LOCATION, CONTEXT)},
	{"441_1_1", BT_AUDIO_LC3_BROADCAST_PRESET_441_1_1(LOCATION, CONTEXT)},
	{"441_2_1", BT_AUDIO_LC3_BROADCAST_PRESET_441_2_1(LOCATION, CONTEXT)},
	{"48_1_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_1_1(LOCATION, CONTEXT)},
	{"48_2_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_2_1(LOCATION, CONTEXT)},
	{"48_3_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_3_1(LOCATION, CONTEXT)},
	{"48_4_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_4_1(LOCATION, CONTEXT)},
	{"48_5_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_5_1(LOCATION, CONTEXT)},
	{"48_6_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_6_1(LOCATION, CONTEXT)},
	/* High-reliability presets */
	{"8_1_2",   BT_AUDIO_LC3_BROADCAST_PRESET_8_1_2(LOCATION, CONTEXT)},
	{"8_2_2",   BT_AUDIO_LC3_BROADCAST_PRESET_8_2_2(LOCATION, CONTEXT)},
	{"16_1_2",  BT_AUDIO_LC3_BROADCAST_PRESET_16_1_2(LOCATION, CONTEXT)},
	{"16_2_2",  BT_AUDIO_LC3_BROADCAST_PRESET_16_2_2(LOCATION, CONTEXT)},
	{"24_1_2",  BT_AUDIO_LC3_BROADCAST_PRESET_24_1_2(LOCATION, CONTEXT)},
	{"24_2_2",  BT_AUDIO_LC3_BROADCAST_PRESET_24_2_2(LOCATION, CONTEXT)},
	{"32_1_2",  BT_AUDIO_LC3_BROADCAST_PRESET_32_1_2(LOCATION, CONTEXT)},
	{"32_2_2",  BT_AUDIO_LC3_BROADCAST_PRESET_32_2_2(LOCATION, CONTEXT)},
	{"441_1_2", BT_AUDIO_LC3_BROADCAST_PRESET_441_1_2(LOCATION, CONTEXT)},
	{"441_2_2", BT_AUDIO_LC3_BROADCAST_PRESET_441_2_2(LOCATION, CONTEXT)},
	{"48_1_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_1_2(LOCATION, CONTEXT)},
	{"48_2_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_2_2(LOCATION, CONTEXT)},
	{"48_3_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_3_2(LOCATION, CONTEXT)},
	{"48_4_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_4_2(LOCATION, CONTEXT)},
	{"48_5_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_5_2(LOCATION, CONTEXT)},
	{"48_6_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_6_2(LOCATION, CONTEXT)},
};

/* Default to 16_2_1 */
static struct named_lc3_preset *default_preset = &lc3_unicast_presets[3];

static void print_codec(const struct bt_codec *codec)
{
	int i;

	shell_print(ctx_shell, "codec 0x%02x cid 0x%04x vid 0x%04x count %u",
		    codec->id, codec->cid, codec->vid, codec->data_count);

	for (i = 0; i < codec->data_count; i++) {
		shell_print(ctx_shell, "data #%u: type 0x%02x len %u", i,
			    codec->data[i].data.type,
			    codec->data[i].data.data_len);
		shell_hexdump(ctx_shell, codec->data[i].data.data,
			      codec->data[i].data.data_len -
				sizeof(codec->data[i].data.type));
	}

	for (i = 0; i < codec->meta_count; i++) {
		shell_print(ctx_shell, "meta #%u: type 0x%02x len %u", i,
			    codec->meta[i].data.type,
			    codec->meta[i].data.data_len);
		shell_hexdump(ctx_shell, codec->meta[i].data.data,
			      codec->data[i].data.data_len -
				sizeof(codec->meta[i].data.type));
	}
}

static struct named_lc3_preset *set_preset(bool is_unicast, size_t argc,
					   char **argv)
{
	static struct named_lc3_preset named_preset;
	int i;

	if (is_unicast) {
		for (i = 0; i < ARRAY_SIZE(lc3_unicast_presets); i++) {
			if (!strcmp(argv[0], lc3_unicast_presets[i].name)) {
				default_preset = &lc3_unicast_presets[i];
				break;
			}
		}

		if (i == ARRAY_SIZE(lc3_unicast_presets)) {
			return NULL;
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(lc3_broadcast_presets); i++) {
			if (!strcmp(argv[0], lc3_broadcast_presets[i].name)) {
				default_preset = &lc3_broadcast_presets[i];
				break;
			}
		}

		if (i == ARRAY_SIZE(lc3_broadcast_presets)) {
			return NULL;
		}

	}

	if (argc == 1) {
		return default_preset;
	}

	named_preset = *default_preset;
	default_preset = &named_preset;

	if (argc > 1) {
		named_preset.preset.qos.interval = strtol(argv[1], NULL, 0);
	}

	if (argc > 2) {
		named_preset.preset.qos.framing = strtol(argv[2], NULL, 0);
	}

	if (argc > 3) {
		named_preset.preset.qos.latency = strtol(argv[3], NULL, 0);
	}

	if (argc > 4) {
		named_preset.preset.qos.pd = strtol(argv[4], NULL, 0);
	}

	if (argc > 5) {
		named_preset.preset.qos.sdu = strtol(argv[5], NULL, 0);
	}

	if (argc > 6) {
		named_preset.preset.qos.phy = strtol(argv[6], NULL, 0);
	}

	if (argc > 7) {
		named_preset.preset.qos.rtn = strtol(argv[7], NULL, 0);
	}

	return default_preset;
}

static void set_stream(struct bt_audio_stream *stream)
{
	int i;

	default_stream = stream;

	for (i = 0; i < ARRAY_SIZE(streams); i++) {
		if (stream == &streams[i]) {
			shell_print(ctx_shell, "Default stream: %u", i + 1);
		}
	}
}

#if defined(CONFIG_BT_AUDIO_UNICAST)
static void print_qos(struct bt_codec_qos *qos)
{
	shell_print(ctx_shell, "QoS: interval %u framing 0x%02x "
		    "phy 0x%02x sdu %u rtn %u latency %u pd %u",
		    qos->interval, qos->framing, qos->phy, qos->sdu,
		    qos->rtn, qos->latency, qos->pd);
}

static int cmd_select_unicast(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_audio_stream *stream;
	uint8_t index;

	index = strtol(argv[1], NULL, 0);
	if (index < 0 || index > ARRAY_SIZE(streams)) {
		shell_error(sh, "Invalid index: %d", index);
		return -ENOEXEC;
	}

	stream = &streams[index];
	if (stream->conn == NULL) {
		shell_error(sh, "Invalid index");
		return -ENOEXEC;
	}

	set_stream(stream);

	return 0;
}

#if defined(CONFIG_BT_AUDIO_UNICAST_SERVER)
static struct bt_audio_stream *lc3_config(struct bt_conn *conn,
					struct bt_audio_ep *ep,
					enum bt_audio_dir dir,
					struct bt_audio_capability *cap,
					struct bt_codec *codec)
{
	int i;

	shell_print(ctx_shell, "ASE Codec Config: conn %p ep %p dir %u, cap %p",
		    conn, ep, dir, cap);

	print_codec(codec);

	for (i = 0; i < ARRAY_SIZE(streams); i++) {
		struct bt_audio_stream *stream = &streams[i];

		if (stream->conn == NULL) {
			shell_print(ctx_shell, "ASE Codec Config stream %p",
				    stream);
			set_stream(stream);
			return stream;
		}
	}

	shell_print(ctx_shell, "No streams available");

	return NULL;
}

static int lc3_reconfig(struct bt_audio_stream *stream,
			struct bt_audio_capability *cap,
			struct bt_codec *codec)
{
	shell_print(ctx_shell, "ASE Codec Reconfig: stream %p cap %p", stream, cap);

	print_codec(codec);

	if (default_stream == NULL) {
		set_stream(stream);
	}

	return 0;
}

static int lc3_qos(struct bt_audio_stream *stream, struct bt_codec_qos *qos)
{
	shell_print(ctx_shell, "QoS: stream %p %p", stream, qos);

	print_qos(qos);

	return 0;
}

static int lc3_enable(struct bt_audio_stream *stream,
		      struct bt_codec_data *meta,
		      size_t meta_count)
{
	shell_print(ctx_shell, "Enable: stream %p meta_count %zu", stream,
		    meta_count);

	return 0;
}

static int lc3_start(struct bt_audio_stream *stream)
{
	shell_print(ctx_shell, "Start: stream %p", stream);

	seq_num = 0;

	return 0;
}

static int lc3_metadata(struct bt_audio_stream *stream,
			struct bt_codec_data *meta,
			size_t meta_count)
{
	shell_print(ctx_shell, "Metadata: stream %p meta_count %zu", stream,
		    meta_count);

	return 0;
}

static int lc3_disable(struct bt_audio_stream *stream)
{
	shell_print(ctx_shell, "Disable: stream %p", stream);

	return 0;
}

static int lc3_stop(struct bt_audio_stream *stream)
{
	shell_print(ctx_shell, "Stop: stream %p", stream);

	return 0;
}

static int lc3_release(struct bt_audio_stream *stream)
{
	shell_print(ctx_shell, "Release: stream %p", stream);

	if (stream == default_stream) {
		default_stream = NULL;
	}

	connecting = false;

	return 0;
}

static struct bt_codec lc3_codec = BT_CODEC_LC3(BT_CODEC_LC3_FREQ_ANY,
						BT_CODEC_LC3_DURATION_ANY,
						BT_CODEC_LC3_CHAN_COUNT_SUPPORT(1, 2), 30, 240, 2,
						(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL |
						BT_AUDIO_CONTEXT_TYPE_MEDIA));

static struct bt_audio_capability_ops lc3_ops = {
	.config = lc3_config,
	.reconfig = lc3_reconfig,
	.qos = lc3_qos,
	.enable = lc3_enable,
	.start = lc3_start,
	.metadata = lc3_metadata,
	.disable = lc3_disable,
	.stop = lc3_stop,
	.release = lc3_release,
};
#endif /* CONFIG_BT_AUDIO_UNICAST_SERVER */
#endif /* CONFIG_BT_AUDIO_UNICAST */

#if defined(CONFIG_BT_AUDIO_UNICAST_SERVER) || defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
static struct bt_audio_capability caps[MAX_PAC] = {
#if defined(CONFIG_BT_AUDIO_UNICAST_SERVER)
	{
		.dir = BT_AUDIO_DIR_SOURCE,
		.pref = BT_AUDIO_CAPABILITY_PREF(
				BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED,
				BT_GAP_LE_PHY_2M, 0u, 60u, 20000u, 40000u,
				20000u, 40000u),
		.codec = &lc3_codec,
		.ops = &lc3_ops,
	},
#endif /* CONFIG_BT_AUDIO_UNICAST_SERVER */
	{
		.dir = BT_AUDIO_DIR_SINK,
		.codec = &lc3_codec,
#if defined(CONFIG_BT_AUDIO_UNICAST_SERVER)
		.pref = BT_AUDIO_CAPABILITY_PREF(
				BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED,
				BT_GAP_LE_PHY_2M, 0u, 60u, 20000u, 40000u,
				20000u, 40000u),
		.ops = &lc3_ops,
#endif /* CONFIG_BT_AUDIO_UNICAST_SERVER */
	},
};
#endif /* CONFIG_BT_AUDIO_UNICAST_SERVER || CONFIG_BT_AUDIO_BROADCAST_SINK */

#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)
static uint8_t stream_dir(const struct bt_audio_stream *stream)
{
	for (size_t i = 0; i < ARRAY_SIZE(snks); i++) {
		if (snks[i] != NULL && stream->ep == snks[i]) {
			return BT_AUDIO_DIR_SINK;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(srcs); i++) {
		if (srcs[i] != NULL && stream->ep == srcs[i]) {
			return BT_AUDIO_DIR_SOURCE;
		}
	}

	__ASSERT(false, "Invalid stream");
	return 0;
}

static void add_codec(struct bt_codec *codec, uint8_t index, enum bt_audio_dir dir)
{
	shell_print(ctx_shell, "#%u: codec %p dir 0x%02x", index, codec, dir);

	print_codec(codec);

	if (dir != BT_AUDIO_DIR_SINK && dir != BT_AUDIO_DIR_SOURCE) {
		return;
	}

	if (index < CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT) {
		rcodecs[dir - 1][index] = codec;
	}
}

static void add_sink(struct bt_audio_ep *ep, uint8_t index)
{
	shell_print(ctx_shell, "Sink #%u: ep %p", index, ep);

	snks[index] = ep;
}

static void add_source(struct bt_audio_ep *ep, uint8_t index)
{
	shell_print(ctx_shell, "Source #%u: ep %p", index, ep);

	srcs[index] = ep;
}

static void discover_cb(struct bt_conn *conn, struct bt_codec *codec,
			struct bt_audio_ep *ep,
			struct bt_audio_discover_params *params)
{
	if (codec != NULL) {
		add_codec(codec, params->num_caps, params->dir);
		return;
	}

	if (ep) {
		if (params->dir == BT_AUDIO_DIR_SINK) {
			add_sink(ep, params->num_eps);
		} else if (params->dir == BT_AUDIO_DIR_SOURCE) {
			add_source(ep, params->num_eps);
		}

		return;
	}

	shell_print(ctx_shell, "Discover complete: err %d", params->err);

	memset(params, 0, sizeof(*params));
}

static void discover_all(struct bt_conn *conn, struct bt_codec *codec,
			struct bt_audio_ep *ep,
			struct bt_audio_discover_params *params)
{
	if (codec != NULL) {
		add_codec(codec, params->num_caps, params->dir);
		return;
	}

	if (ep) {
		if (params->dir == BT_AUDIO_DIR_SINK) {
			add_sink(ep, params->num_eps);
		} else if (params->dir == BT_AUDIO_DIR_SOURCE) {
			add_source(ep, params->num_eps);
		}

		return;
	}

	/* Sinks discovery complete, now discover sources */
	if (params->dir == BT_AUDIO_DIR_SINK) {
		int err;

		params->func = discover_cb;
		params->dir = BT_AUDIO_DIR_SOURCE;

		err = bt_audio_discover(default_conn, params);
		if (err) {
			shell_error(ctx_shell, "bt_audio_discover err %d", err);
			discover_cb(conn, NULL, NULL, params);
		}
	}
}

static int cmd_discover(const struct shell *sh, size_t argc, char *argv[])
{
	static struct bt_audio_discover_params params;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (params.func) {
		shell_error(sh, "Discover in progress");
		return -ENOEXEC;
	}

	params.func = discover_all;
	params.dir = BT_AUDIO_DIR_SINK;

	if (argc > 1) {
		if (!strcmp(argv[1], "sink")) {
			params.func = discover_cb;
		} else if (!strcmp(argv[1], "source")) {
			params.func = discover_cb;
			params.dir = BT_AUDIO_DIR_SOURCE;
		} else {
			shell_error(sh, "Unsupported dir: %s", argv[1]);
			return -ENOEXEC;
		}
	}

	return bt_audio_discover(default_conn, &params);
}

static int cmd_preset(const struct shell *sh, size_t argc, char *argv[])
{
	struct named_lc3_preset *named_preset;

	named_preset = default_preset;

	if (argc > 1) {
		named_preset = set_preset(true, argc - 1, argv + 1);
		if (named_preset == NULL) {
			shell_error(sh, "Unable to parse named_preset %s",
				    argv[1]);
			return -ENOEXEC;
		}
	}

	shell_print(sh, "%s", named_preset->name);

	print_codec(&named_preset->preset.codec);
	print_qos(&named_preset->preset.qos);

	return 0;
}

static int cmd_config(const struct shell *sh, size_t argc, char *argv[])
{
	int32_t index, dir;
	struct bt_audio_ep *ep = NULL;
	struct named_lc3_preset *named_preset;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = strtol(argv[2], NULL, 0);
	if (index < 0) {
		shell_error(sh, "Invalid index");
		return -ENOEXEC;
	}

	if (!strcmp(argv[1], "sink")) {
		dir = BT_AUDIO_DIR_SINK;
		ep = snks[index];
	} else if (!strcmp(argv[1], "source")) {
		dir = BT_AUDIO_DIR_SOURCE;
		ep = srcs[index];
	} else {
		shell_error(sh, "Unsupported dir: %s", argv[1]);
		return -ENOEXEC;
	}

	if (!ep) {
		shell_error(sh, "Unable to find endpoint");
		return -ENOEXEC;
	}

	named_preset = default_preset;

	if (argc > 3) {
		named_preset = set_preset(true, 1, argv + 3);
		if (named_preset == NULL) {
			shell_error(sh, "Unable to parse named_preset %s",
				    argv[4]);
			return -ENOEXEC;
		}
	}

	if (default_stream && default_stream->ep == ep) {
		if (bt_audio_stream_reconfig(default_stream,
					     &named_preset->preset.codec) < 0) {
			shell_error(sh, "Unable reconfig stream");
			return -ENOEXEC;
		}
	} else {
		struct bt_audio_stream *stream;
		int err;

		if (default_stream == NULL) {
			stream = &streams[0];
		} else {
			stream = default_stream;
		}

		err = bt_audio_stream_config(default_conn, stream, ep,
					     &named_preset->preset.codec);
		if (err != 0) {
			shell_error(sh, "Unable to config stream: %d", err);
			return err;
		}

		default_stream = stream;
	}

	shell_print(sh, "ASE config: preset %s", named_preset->name);

	return 0;
}

static int cmd_release(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_stream == NULL) {
		shell_print(sh, "Not connected");
		return -ENOEXEC;
	}

	err = bt_audio_stream_release(default_stream, false);
	if (err) {
		shell_error(sh, "Unable to release Channel");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_qos(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct named_lc3_preset *named_preset = NULL;

	if (default_stream == NULL) {
		shell_print(sh, "Not connected");
		return -ENOEXEC;
	}

	named_preset = default_preset;

	if (argc > 1) {
		named_preset = set_preset(true, argc - 1, argv + 1);
		if (named_preset == NULL) {
			shell_error(sh, "Unable to parse named_preset %s",
				    argv[1]);
			return -ENOEXEC;
		}
	}

	if (default_unicast_group == NULL) {
		err = bt_audio_unicast_group_create(&default_stream, 1, &default_unicast_group);
		if (err != 0) {
			shell_error(sh, "Unable to create default unicast group: %d", err);
			return -ENOEXEC;
		}
	}

	err = bt_audio_stream_qos(default_conn, default_unicast_group,
				  &named_preset->preset.qos);
	if (err) {
		shell_error(sh, "Unable to setup QoS");
		return -ENOEXEC;
	}

	shell_print(sh, "ASE config: preset %s", named_preset->name);

	return 0;
}

static int cmd_enable(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_stream == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	err = bt_audio_stream_enable(default_stream,
				     default_preset->preset.codec.meta,
				     default_preset->preset.codec.meta_count);
	if (err) {
		shell_error(sh, "Unable to enable Channel");
		return -ENOEXEC;
	}

	return 0;
}

#define MAX_META_DATA \
	(CONFIG_BT_CODEC_MAX_METADATA_COUNT * sizeof(struct bt_codec_data))

static uint16_t strmeta(const char *name)
{
	if (!strcmp(name, "Unspecified")) {
		return BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED;
	} else if (!strcmp(name, "Conversational")) {
		return BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL;
	} else if (!strcmp(name, "Media")) {
		return BT_AUDIO_CONTEXT_TYPE_MEDIA;
	} else if (!strcmp(name, "Game")) {
		return BT_AUDIO_CONTEXT_TYPE_GAME;
	} else if (!strcmp(name, "Instructional")) {
		return BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL;
	} else if (!strcmp(name, "VoiceAssistants")) {
		return BT_AUDIO_CONTEXT_TYPE_VOICE_ASSISTANTS;
	} else if (!strcmp(name, "Live")) {
		return BT_AUDIO_CONTEXT_TYPE_LIVE;
	} else if (!strcmp(name, "SoundEffects")) {
		return BT_AUDIO_CONTEXT_TYPE_SOUND_EFFECTS;
	} else if (!strcmp(name, "Notifications")) {
		return BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS;
	} else if (!strcmp(name, "Ringtone")) {
		return BT_AUDIO_CONTEXT_TYPE_RINGTONE;
	} else if (!strcmp(name, "Alerts")) {
		return BT_AUDIO_CONTEXT_TYPE_ALERTS;
	} else if (!strcmp(name, "EmergencyAlarm")) {
		return BT_AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM;
	}

	return 0u;
}

static int cmd_metadata(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_stream == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (argc > 1) {
		uint16_t context;

		context = strmeta(argv[1]);
		if (context == 0) {
			shell_error(sh, "Invalid context");
			return -ENOEXEC;
		}

		sys_put_le16(context,
			     default_preset->preset.codec.meta[0].value);
	}

	err = bt_audio_stream_metadata(default_stream,
				       default_preset->preset.codec.meta,
				       default_preset->preset.codec.meta_count);
	if (err) {
		shell_error(sh, "Unable to set Channel metadata");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_start(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_stream == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	err = bt_audio_stream_start(default_stream);
	if (err) {
		shell_error(sh, "Unable to start Channel");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_disable(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_stream == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	err = bt_audio_stream_disable(default_stream);
	if (err) {
		shell_error(sh, "Unable to disable Channel");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_stop(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_stream == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	err = bt_audio_stream_stop(default_stream);
	if (err) {
		shell_error(sh, "Unable to start Channel");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_list(const struct shell *sh, size_t argc, char *argv[])
{
	int i;

	shell_print(sh, "Configured Channels:");

	for (i = 0; i < ARRAY_SIZE(streams); i++) {
		struct bt_audio_stream *stream = &streams[i];

		if (stream->conn) {
			shell_print(sh, "  %s#%u: stream %p dir 0x%02x group %p",
				    stream == default_stream ? "*" : " ", i, stream,
				    stream_dir(stream), stream->group);
		}
	}

	shell_print(sh, "Sinks:");

	for (i = 0; i < ARRAY_SIZE(snks); i++) {
		struct bt_audio_ep *ep = snks[i];

		if (ep) {
			shell_print(sh, "  #%u: ep %p", i, ep);
		}
	}

	shell_print(sh, "Sources:");

	for (i = 0; i < ARRAY_SIZE(snks); i++) {
		struct bt_audio_ep *ep = srcs[i];

		if (ep) {
			shell_print(sh, "  #%u: ep %p", i, ep);
		}
	}

	return 0;
}

static int cmd_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = cmd_config(sh, argc, argv);
	if (err) {
		return err;
	}

	connecting = true;

	return 0;
}
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
static uint32_t accepted_broadcast_id;
static struct bt_audio_base received_base;
static bool sink_syncable;

static bool scan_recv(const struct bt_le_scan_recv_info *info,
		     uint32_t broadcast_id)
{
	shell_print(ctx_shell, "Found broadcaster with ID 0x%06X",
		    broadcast_id);

	if (broadcast_id == accepted_broadcast_id) {
		shell_print(ctx_shell, "PA syncing to broadcaster");
		accepted_broadcast_id = 0;
		return true;
	}

	return false;
}

static void pa_synced(struct bt_audio_broadcast_sink *sink,
		      struct bt_le_per_adv_sync *sync,
		      uint32_t broadcast_id)
{
	shell_print(ctx_shell,
		    "PA synced to broadcaster with ID 0x%06X as sink %p",
		    broadcast_id, sink);

	if (default_sink == NULL) {
		default_sink = sink;

		shell_print(ctx_shell, "Sink %p is set as default", sink);
	}
}

static void base_recv(struct bt_audio_broadcast_sink *sink,
		      const struct bt_audio_base *base)
{
	uint8_t bis_indexes[BROADCAST_SNK_STREAM_CNT] = { 0 };
	/* "0xXX " requires 5 characters */
	char bis_indexes_str[5 * ARRAY_SIZE(bis_indexes) + 1];
	size_t index_count = 0;

	if (memcmp(base, &received_base, sizeof(received_base)) == 0) {
		/* Don't print duplicates */
		return;
	}

	shell_print(ctx_shell, "Received BASE from sink %p:", sink);

	for (int i = 0; i < base->subgroup_count; i++) {
		const struct bt_audio_base_subgroup *subgroup;

		subgroup = &base->subgroups[i];

		shell_print(ctx_shell, "Subgroup[%d]:", i);
		print_codec(&subgroup->codec);

		for (int j = 0; j < subgroup->bis_count; j++) {
			const struct bt_audio_base_bis_data *bis_data;

			bis_data = &subgroup->bis_data[j];

			shell_print(ctx_shell, "BIS[%d] index 0x%02x",
				    i, bis_data->index);
			bis_indexes[index_count++] = bis_data->index;

			for (int k = 0; k < bis_data->data_count; k++) {
				const struct bt_codec_data *codec_data;

				codec_data = &bis_data->data[k];

				shell_print(ctx_shell,
					    "data #%u: type 0x%02x len %u",
					    i, codec_data->data.type,
					    codec_data->data.data_len);
				shell_hexdump(ctx_shell, codec_data->data.data,
					      codec_data->data.data_len -
						sizeof(codec_data->data.type));
			}
		}
	}

	memset(bis_indexes_str, 0, sizeof(bis_indexes_str));
	/* Create space separated list of indexes as hex values */
	for (int i = 0; i < index_count; i++) {
		char bis_index_str[6];

		sprintf(bis_index_str, "0x%02x ", bis_indexes[i]);

		strcat(bis_indexes_str, bis_index_str);
		shell_print(ctx_shell, "[%d]: %s", i, bis_index_str);
	}

	shell_print(ctx_shell, "Possible indexes: %s", bis_indexes_str);

	(void)memcpy(&received_base, base, sizeof(received_base));
}

static void syncable(struct bt_audio_broadcast_sink *sink, bool encrypted)
{
	if (sink_syncable) {
		return;
	}

	shell_print(ctx_shell, "Sink %p is ready to sync %s encryption",
		    sink, encrypted ? "with" : "without");
	sink_syncable = true;
}

static void scan_term(int err)
{
	shell_print(ctx_shell, "Broadcast scan was terminated: %d", err);

}

static void pa_sync_lost(struct bt_audio_broadcast_sink *sink)
{
	shell_warn(ctx_shell, "Sink %p disconnected", sink);

	if (default_sink == sink) {
		default_sink = NULL;
		sink_syncable = false;
	}
}
#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
static struct bt_audio_broadcast_sink_cb sink_cbs = {
	.scan_recv = scan_recv,
	.pa_synced = pa_synced,
	.base_recv = base_recv,
	.syncable = syncable,
	.scan_term = scan_term,
	.pa_sync_lost = pa_sync_lost,
};
#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK */

#if defined(CONFIG_BT_AUDIO_UNICAST) || defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
static void audio_recv(struct bt_audio_stream *stream,
		       const struct bt_iso_recv_info *info,
		       struct net_buf *buf)
{
	shell_print(ctx_shell, "Incoming audio on stream %p len %u\n", stream, buf->len);
}

static struct bt_audio_stream_ops stream_ops = {
	.recv = audio_recv
};
#endif /* CONFIG_BT_AUDIO_UNICAST || CONFIG_BT_AUDIO_BROADCAST_SINK */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
static int cmd_select_broadcast_source(const struct shell *sh, size_t argc,
				       char *argv[])
{
	struct bt_audio_stream *stream;
	uint8_t index;

	index = strtol(argv[1], NULL, 0);
	if (index < 0 || index > ARRAY_SIZE(broadcast_source_streams)) {
		shell_error(sh, "Invalid index: %d", index);
		return -ENOEXEC;
	}

	stream = &broadcast_source_streams[index];

	set_stream(stream);

	return 0;
}

static int cmd_create_broadcast(const struct shell *sh, size_t argc,
				char *argv[])
{
	static struct bt_audio_stream *streams[ARRAY_SIZE(broadcast_source_streams)];
	struct named_lc3_preset *named_preset;
	int err;

	if (default_source != NULL) {
		shell_info(sh, "Broadcast source already created");
		return -ENOEXEC;
	}

	named_preset = default_preset;

	if (argc > 1) {
		named_preset = set_preset(false, 1, &argv[1]);
		if (named_preset == NULL) {
			shell_error(sh, "Unable to parse named_preset %s",
				    argv[1]);
			return -ENOEXEC;
		}
	}

	(void)memset(streams, 0, sizeof(streams));
	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		streams[i] = &broadcast_source_streams[i];
	}

	err = bt_audio_broadcast_source_create(streams, ARRAY_SIZE(streams),
					       &named_preset->preset.codec,
					       &named_preset->preset.qos,
					       &default_source);
	if (err != 0) {
		shell_error(sh, "Unable to create broadcast source: %d", err);
		return err;
	}

	shell_print(sh, "Broadcast source created: preset %s",
		    named_preset->name);

	if (default_stream == NULL) {
		default_stream = streams[0];
	}

	return 0;
}

static int cmd_start_broadcast(const struct shell *sh, size_t argc,
			       char *argv[])
{
	int err;

	if (default_source == NULL) {
		shell_info(sh, "Broadcast source not created");
		return -ENOEXEC;
	}

	err = bt_audio_broadcast_source_start(default_source);
	if (err != 0) {
		shell_error(sh, "Unable to start broadcast source: %d", err);
		return err;
	}

	return 0;
}

static int cmd_stop_broadcast(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_source == NULL) {
		shell_info(sh, "Broadcast source not created");
		return -ENOEXEC;
	}

	err = bt_audio_broadcast_source_stop(default_source);
	if (err != 0) {
		shell_error(sh, "Unable to stop broadcast source: %d", err);
		return err;
	}

	return 0;
}

static int cmd_delete_broadcast(const struct shell *sh, size_t argc,
				char *argv[])
{
	int err;

	if (default_source == NULL) {
		shell_info(sh, "Broadcast source not created");
		return -ENOEXEC;
	}

	err = bt_audio_broadcast_source_delete(default_source);
	if (err != 0) {
		shell_error(sh, "Unable to delete broadcast source: %d", err);
		return err;
	}
	default_source = NULL;

	return 0;
}
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
static int cmd_broadcast_scan(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct bt_le_scan_param param = {
			.type       = BT_LE_SCAN_TYPE_ACTIVE,
			.options    = BT_LE_SCAN_OPT_NONE,
			.interval   = BT_GAP_SCAN_FAST_INTERVAL,
			.window     = BT_GAP_SCAN_FAST_WINDOW,
			.timeout    = 0 };

	if (strcmp(argv[1], "on") == 0) {
		err =  bt_audio_broadcast_sink_scan_start(&param);
		if (err != 0) {
			shell_error(sh, "Could not start scan: %d", err);
		}
	} else if (strcmp(argv[1], "off") == 0) {
		err = bt_audio_broadcast_sink_scan_stop();
		if (err != 0) {
			shell_error(sh, "Could not stop scan: %d", err);
		}
	} else {
		shell_help(sh);
		err = -ENOEXEC;
	}

	return err;
}

static int cmd_accept_broadcast(const struct shell *sh, size_t argc,
				char *argv[])
{
	accepted_broadcast_id = strtoul(argv[1], NULL, 16);

	return 0;
}

static int cmd_sync_broadcast(const struct shell *sh, size_t argc, char *argv[])
{
	static struct bt_audio_stream *streams[ARRAY_SIZE(broadcast_sink_streams)];
	uint32_t bis_bitfield;
	int err;

	bis_bitfield = 0;
	for (int i = 1; i < argc; i++) {
		unsigned long val = strtoul(argv[1], NULL, 16);

		if (val < 0x01 || val > 0x1F) {
			shell_error(sh, "Invalid index: %ld", val);
		}
		bis_bitfield |= BIT(val);
	}

	if (default_sink == NULL) {
		shell_error(sh, "No sink available");
		return -ENOEXEC;
	}

	(void)memset(streams, 0, sizeof(streams));
	for (size_t i = 0; i < ARRAY_SIZE(streams); i++) {
		streams[i] = &broadcast_sink_streams[i];
	}

	err = bt_audio_broadcast_sink_sync(default_sink, bis_bitfield,
					   streams, NULL);
	if (err != 0) {
		shell_error(sh, "Failed to sync to broadcast: %d", err);
		return err;
	}

	return 0;
}

static int cmd_stop_broadcast_sink(const struct shell *sh, size_t argc,
				   char *argv[])
{
	int err;

	if (default_sink == NULL) {
		shell_error(sh, "No sink available");
		return -ENOEXEC;
	}

	err = bt_audio_broadcast_sink_stop(default_sink);
	if (err != 0) {
		shell_error(sh, "Failed to stop sink: %d", err);
		return err;
	}

	return err;
}

static int cmd_term_broadcast_sink(const struct shell *sh, size_t argc,
				   char *argv[])
{
	int err;

	if (default_sink == NULL) {
		shell_error(sh, "No sink available");
		return -ENOEXEC;
	}

	err = bt_audio_broadcast_sink_delete(default_sink);
	if (err != 0) {
		shell_error(sh, "Failed to term sink: %d", err);
		return err;
	}

	default_sink = NULL;
	sink_syncable = false;

	return err;
}
#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK */

static int cmd_init(const struct shell *sh, size_t argc, char *argv[])
{
	int err, i;

	ctx_shell = sh;

	err = bt_enable(NULL);
	if (err && err != -EALREADY) {
		shell_error(sh, "Bluetooth init failed (err %d)", err);
		return err;
	}

#if defined(CONFIG_BT_AUDIO_UNICAST_SERVER) || defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
	for (i = 0; i < ARRAY_SIZE(caps); i++) {
		bt_audio_capability_register(&caps[i]);
	}
#endif /* CONFIG_BT_AUDIO_UNICAST || CONFIG_BT_AUDIO_BROADCAST_SOURCE */

	if (IS_ENABLED(CONFIG_BT_AUDIO_CAPABILITY)) {
		/* Mark all supported contexts as available */
		bt_audio_capability_set_available_contexts(BT_AUDIO_DIR_SINK,
							   BT_AUDIO_CONTEXT_TYPE_ANY);
		bt_audio_capability_set_available_contexts(BT_AUDIO_DIR_SOURCE,
							   BT_AUDIO_CONTEXT_TYPE_ANY);
	}

#if defined(CONFIG_BT_AUDIO_UNICAST)
	for (i = 0; i < ARRAY_SIZE(streams); i++) {
		bt_audio_stream_cb_register(&streams[i], &stream_ops);
	}
#endif /* CONFIG_BT_AUDIO_UNICAST */

#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
	bt_audio_broadcast_sink_register_cb(&sink_cbs);

	for (i = 0; i < ARRAY_SIZE(broadcast_sink_streams); i++) {
		bt_audio_stream_cb_register(&broadcast_sink_streams[i],
					    &stream_ops);
	}
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */
	return 0;
}

#define DATA_MTU CONFIG_BT_ISO_TX_MTU
NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, DATA_MTU, 8, NULL);

static uint32_t get_next_seq_num(uint32_t interval_us)
{
	static int64_t last_ticks;
	int64_t uptime_ticks, delta_ticks;
	uint64_t delta_us;
	uint64_t seq_num_incr;
	uint64_t next_seq_num;

	/* Note: This does not handle wrapping of ticks when they go above
	 * 2^(62-1)
	 */
	uptime_ticks = k_uptime_ticks();
	delta_ticks = uptime_ticks - last_ticks;
	last_ticks = uptime_ticks;

	delta_us = k_ticks_to_us_near64((uint64_t)delta_ticks);
	seq_num_incr = delta_us / interval_us;
	next_seq_num = (seq_num_incr + seq_num);

	return (uint32_t)next_seq_num;
}

static int cmd_send(const struct shell *sh, size_t argc, char *argv[])
{
	static uint8_t data[DATA_MTU - BT_ISO_CHAN_SEND_RESERVE];
	int ret, len;
	struct net_buf *buf;

	if (argc > 1) {
		len = hex2bin(argv[1], strlen(argv[1]), data, sizeof(data));
		if (len > default_stream->iso->qos->tx->sdu) {
			shell_print(sh, "Unable to send: len %d > %u MTU",
				    len, default_stream->iso->qos->tx->sdu);
			return -ENOEXEC;
		}
	} else {
		len = MIN(default_stream->iso->qos->tx->sdu, sizeof(data));
		memset(data, 0xff, len);
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

	net_buf_add_mem(buf, data, len);

	seq_num = get_next_seq_num(default_preset->preset.qos.interval);

	ret = bt_audio_stream_send(default_stream, buf, seq_num,
				   BT_ISO_TIMESTAMP_NONE);
	if (ret < 0) {
		shell_print(sh, "Unable to send: %d", -ret);
		net_buf_unref(buf);
		return -ENOEXEC;
	}
	shell_print(sh, "Sending:");
	shell_hexdump(sh, data, len);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(audio_cmds,
	SHELL_CMD_ARG(init, NULL, NULL, cmd_init, 1, 0),
#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
	SHELL_CMD_ARG(select_broadcast, NULL, "<stream>",
		      cmd_select_broadcast_source, 2, 0),
	SHELL_CMD_ARG(create_broadcast, NULL, "[codec] [preset]",
		      cmd_create_broadcast, 1, 2),
	SHELL_CMD_ARG(start_broadcast, NULL, "", cmd_start_broadcast, 1, 0),
	SHELL_CMD_ARG(stop_broadcast, NULL, "", cmd_stop_broadcast, 1, 0),
	SHELL_CMD_ARG(delete_broadcast, NULL, "", cmd_delete_broadcast, 1, 0),
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */
#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
	SHELL_CMD_ARG(broadcast_scan, NULL, "<on, off>",
		      cmd_broadcast_scan, 2, 0),
	SHELL_CMD_ARG(accept_broadcast, NULL, "0x<broadcast_id>",
		      cmd_accept_broadcast, 2, 0),
	SHELL_CMD_ARG(sync_broadcast, NULL, "0x<bis_bitfield>",
		      cmd_sync_broadcast, 2, 0),
	SHELL_CMD_ARG(stop_broadcast_sink, NULL, "Stops broadcast sink",
		      cmd_stop_broadcast_sink, 1, 0),
	SHELL_CMD_ARG(term_broadcast_sink, NULL, "",
		      cmd_term_broadcast_sink, 1, 0),
#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK */
#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)
	SHELL_CMD_ARG(discover, NULL, "[dir: sink, source]",
		      cmd_discover, 1, 1),
	SHELL_CMD_ARG(preset, NULL, "[preset]", cmd_preset, 1, 1),
	SHELL_CMD_ARG(config, NULL,
		      "<direction: sink, source> <index> [codec] [preset]",
		      cmd_config, 3, 2),
	SHELL_CMD_ARG(qos, NULL,
		      "[preset] [interval] [framing] [latency] [pd] [sdu] [phy]"
		      " [rtn]", cmd_qos, 1, 8),
	SHELL_CMD_ARG(enable, NULL, NULL, cmd_enable, 1, 1),
	SHELL_CMD_ARG(metadata, NULL, "[context]", cmd_metadata, 1, 1),
	SHELL_CMD_ARG(start, NULL, NULL, cmd_start, 1, 0),
	SHELL_CMD_ARG(disable, NULL, NULL, cmd_disable, 1, 0),
	SHELL_CMD_ARG(stop, NULL, NULL, cmd_stop, 1, 0),
	SHELL_CMD_ARG(release, NULL, NULL, cmd_release, 1, 0),
	SHELL_CMD_ARG(list, NULL, NULL, cmd_list, 1, 0),
	SHELL_CMD_ARG(connect, NULL,
		      "<direction: sink, source> <index>  [codec] [preset]",
		      cmd_connect, 3, 2),
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */
#if defined(CONFIG_BT_AUDIO_UNICAST)
	SHELL_CMD_ARG(select_unicast, NULL, "<stream>",
		      cmd_select_unicast, 2, 0),
#endif /* CONFIG_BT_AUDIO_UNICAST */
	SHELL_CMD_ARG(send, NULL, "Send to Audio Stream [data]",
		      cmd_send, 1, 1),
	SHELL_SUBCMD_SET_END
);

static int cmd_audio(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(audio, &audio_cmds, "Bluetooth audio shell commands",
		       cmd_audio, 1, 1);
