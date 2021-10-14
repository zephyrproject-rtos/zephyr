/** @file
 *  @brief Bluetooth Basic Audio Profile shell
 *
 */

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <zephyr.h>
#include <shell/shell.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include <bluetooth/gatt.h>
#include <bluetooth/audio.h>

#include "bt.h"

#define MAX_PAC 2
#define MAX_ASE (CONFIG_BT_BAP_ASE_SNK_COUNT + CONFIG_BT_BAP_ASE_SRC_COUNT)

static struct bt_audio_chan chans[MAX_PAC];
#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
static struct bt_audio_chan broadcast_source_chans[CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT];
static struct bt_audio_broadcast_source *default_source;
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */
#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
static struct bt_audio_chan broadcast_sink_chans[BROADCAST_SNK_STREAM_CNT];
static struct bt_audio_broadcast_sink *default_sink;
#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK */
static struct bt_audio_capability *rcaps[2][CONFIG_BT_BAP_PAC_COUNT];
static struct bt_audio_ep *snks[CONFIG_BT_BAP_ASE_SNK_COUNT];
static struct bt_audio_ep *srcs[CONFIG_BT_BAP_ASE_SNK_COUNT];
static struct bt_audio_chan *default_chan;
static bool connecting;

struct named_lc3_preset {
	const char *name;
	struct bt_audio_lc3_preset preset;
};

static struct named_lc3_preset lc3_unicast_presets[] = {
	{"8_1_1",   BT_AUDIO_LC3_UNICAST_PRESET_8_1_1},
	{"8_2_1",   BT_AUDIO_LC3_UNICAST_PRESET_8_2_1},
	{"16_1_1",  BT_AUDIO_LC3_UNICAST_PRESET_16_1_1},
	{"16_2_1",  BT_AUDIO_LC3_UNICAST_PRESET_16_2_1},
	{"24_1_1",  BT_AUDIO_LC3_UNICAST_PRESET_24_1_1},
	{"24_2_1",  BT_AUDIO_LC3_UNICAST_PRESET_24_2_1},
	{"32_1_1",  BT_AUDIO_LC3_UNICAST_PRESET_32_1_1},
	{"32_2_1",  BT_AUDIO_LC3_UNICAST_PRESET_32_2_1},
	{"441_1_1", BT_AUDIO_LC3_UNICAST_PRESET_441_1_1},
	{"441_2_1", BT_AUDIO_LC3_UNICAST_PRESET_441_2_1},
	{"48_1_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_1_1},
	{"48_2_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_2_1},
	{"48_3_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_3_1},
	{"48_4_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_4_1},
	{"48_5_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_5_1},
	{"48_6_1",  BT_AUDIO_LC3_UNICAST_PRESET_48_6_1},
	/* High-reliability presets */
	{"8_1_2",   BT_AUDIO_LC3_UNICAST_PRESET_8_1_2},
	{"8_2_2",   BT_AUDIO_LC3_UNICAST_PRESET_8_2_2},
	{"16_1_2",  BT_AUDIO_LC3_UNICAST_PRESET_16_1_2},
	{"16_2_2",  BT_AUDIO_LC3_UNICAST_PRESET_16_2_2},
	{"24_1_2",  BT_AUDIO_LC3_UNICAST_PRESET_24_1_2},
	{"24_2_2",  BT_AUDIO_LC3_UNICAST_PRESET_24_2_2},
	{"32_1_2",  BT_AUDIO_LC3_UNICAST_PRESET_32_1_2},
	{"32_2_2",  BT_AUDIO_LC3_UNICAST_PRESET_32_2_2},
	{"441_1_2", BT_AUDIO_LC3_UNICAST_PRESET_441_1_2},
	{"441_2_2", BT_AUDIO_LC3_UNICAST_PRESET_441_2_2},
	{"48_1_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_1_2},
	{"48_2_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_2_2},
	{"48_3_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_3_2},
	{"48_4_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_4_2},
	{"48_5_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_5_2},
	{"48_6_2",  BT_AUDIO_LC3_UNICAST_PRESET_48_6_2},
};

static struct named_lc3_preset lc3_broadcast_presets[] = {
	{"8_1_1",   BT_AUDIO_LC3_BROADCAST_PRESET_8_1_1},
	{"8_2_1",   BT_AUDIO_LC3_BROADCAST_PRESET_8_2_1},
	{"16_1_1",  BT_AUDIO_LC3_BROADCAST_PRESET_16_1_1},
	{"16_2_1",  BT_AUDIO_LC3_BROADCAST_PRESET_16_2_1},
	{"24_1_1",  BT_AUDIO_LC3_BROADCAST_PRESET_24_1_1},
	{"24_2_1",  BT_AUDIO_LC3_BROADCAST_PRESET_24_2_1},
	{"32_1_1",  BT_AUDIO_LC3_BROADCAST_PRESET_32_1_1},
	{"32_2_1",  BT_AUDIO_LC3_BROADCAST_PRESET_32_2_1},
	{"441_1_1", BT_AUDIO_LC3_BROADCAST_PRESET_441_1_1},
	{"441_2_1", BT_AUDIO_LC3_BROADCAST_PRESET_441_2_1},
	{"48_1_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_1_1},
	{"48_2_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_2_1},
	{"48_3_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_3_1},
	{"48_4_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_4_1},
	{"48_5_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_5_1},
	{"48_6_1",  BT_AUDIO_LC3_BROADCAST_PRESET_48_6_1},
	/* High-reliability presets */
	{"8_1_2",   BT_AUDIO_LC3_BROADCAST_PRESET_8_1_2},
	{"8_2_2",   BT_AUDIO_LC3_BROADCAST_PRESET_8_2_2},
	{"16_1_2",  BT_AUDIO_LC3_BROADCAST_PRESET_16_1_2},
	{"16_2_2",  BT_AUDIO_LC3_BROADCAST_PRESET_16_2_2},
	{"24_1_2",  BT_AUDIO_LC3_BROADCAST_PRESET_24_1_2},
	{"24_2_2",  BT_AUDIO_LC3_BROADCAST_PRESET_24_2_2},
	{"32_1_2",  BT_AUDIO_LC3_BROADCAST_PRESET_32_1_2},
	{"32_2_2",  BT_AUDIO_LC3_BROADCAST_PRESET_32_2_2},
	{"441_1_2", BT_AUDIO_LC3_BROADCAST_PRESET_441_1_2},
	{"441_2_2", BT_AUDIO_LC3_BROADCAST_PRESET_441_2_2},
	{"48_1_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_1_2},
	{"48_2_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_2_2},
	{"48_3_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_3_2},
	{"48_4_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_4_2},
	{"48_5_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_5_2},
	{"48_6_2",  BT_AUDIO_LC3_BROADCAST_PRESET_48_6_2},
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

static void add_capability(struct bt_audio_capability *cap, int index)
{
	shell_print(ctx_shell, "#%u: cap %p type 0x%02x", index, cap,
		    cap->type);

	print_codec(cap->codec);

	if (cap->type != BT_AUDIO_SINK && cap->type != BT_AUDIO_SOURCE) {
		return;
	}

	if (index < CONFIG_BT_BAP_PAC_COUNT) {
		rcaps[cap->type - 1][index] = cap;
	}
}

static void add_sink(struct bt_audio_ep *ep, int index)
{
	shell_print(ctx_shell, "Sink #%u: ep %p index 0x%02x", index, ep);

	snks[index] = ep;
}

static void add_source(struct bt_audio_ep *ep, int index)
{
	shell_print(ctx_shell, "Source #%u: ep %p index 0x%02x", index, ep);

	srcs[index] = ep;
}

static void discover_cb(struct bt_conn *conn, struct bt_audio_capability *cap,
			struct bt_audio_ep *ep,
			struct bt_audio_discover_params *params)
{
	if (cap) {
		add_capability(cap, params->num_caps);
		return;
	}

	if (ep) {
		if (params->type == BT_AUDIO_SINK) {
			add_sink(ep, params->num_eps);
		} else if (params->type == BT_AUDIO_SOURCE) {
			add_source(ep, params->num_eps);
		}

		return;
	}

	shell_print(ctx_shell, "Discover complete: err %d", params->err);

	memset(params, 0, sizeof(*params));
}

static void discover_all(struct bt_conn *conn, struct bt_audio_capability *cap,
			struct bt_audio_ep *ep,
			struct bt_audio_discover_params *params)
{
	if (cap) {
		add_capability(cap, params->num_caps);
		return;
	}

	if (ep) {
		if (params->type == BT_AUDIO_SINK) {
			add_sink(ep, params->num_eps);
		} else if (params->type == BT_AUDIO_SOURCE) {
			add_source(ep, params->num_eps);
		}

		return;
	}

	/* Sinks discovery complete, now discover sources */
	if (params->type == BT_AUDIO_SINK) {
		int err;

		params->func = discover_cb;
		params->type = BT_AUDIO_SOURCE;

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
	params.type = BT_AUDIO_SINK;

	if (argc > 1) {
		if (!strcmp(argv[1], "sink")) {
			params.func = discover_cb;
		} else if (!strcmp(argv[1], "source")) {
			params.func = discover_cb;
			params.type = BT_AUDIO_SOURCE;
		} else {
			shell_error(sh, "Unsupported type: %s", argv[1]);
			return -ENOEXEC;
		}
	}

	return bt_audio_discover(default_conn, &params);
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

static void set_channel(struct bt_audio_chan *chan)
{
	int i;

	default_chan = chan;

	for (i = 0; i < ARRAY_SIZE(chans); i++) {
		if (chan == &chans[i]) {
			shell_print(ctx_shell, "Default channel: %u", i + 1);
		}
	}
}

static void print_qos(struct bt_codec_qos *qos)
{
	shell_print(ctx_shell, "QoS: dir 0x%02x interval %u framing 0x%02x "
		    "phy 0x%02x sdu %u rtn %u latency %u pd %u",
		    qos->dir, qos->interval, qos->framing, qos->phy, qos->sdu,
		    qos->rtn, qos->latency, qos->pd);
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
	struct bt_audio_capability *cap = NULL;
	struct bt_audio_ep *ep = NULL;
	struct named_lc3_preset *named_preset;
	int i;

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
		dir = BT_AUDIO_SINK;
		ep = snks[index];
	} else if (!strcmp(argv[1], "source")) {
		dir = BT_AUDIO_SOURCE;
		ep = srcs[index];
	} else {
		shell_error(sh, "Unsupported type: %s", argv[1]);
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

	for (i = 0; i < ARRAY_SIZE(rcaps); i++) {
		if (rcaps[dir - 1][i]) {
			cap = rcaps[dir - 1][i];
			break;
		}
	}

	if (!cap) {
		shell_error(sh, "Unable to find matching capabilities");
		return -ENOEXEC;
	}

	if (default_chan && default_chan->ep == ep) {
		if (bt_audio_chan_reconfig(default_chan, cap,
					   &named_preset->preset.codec) < 0) {
			shell_error(sh, "Unable reconfig channel");
			return -ENOEXEC;
		}
	} else {
		struct bt_audio_chan *chan;

		chan = bt_audio_chan_config(default_conn, ep, cap,
					    &named_preset->preset.codec);
		if (!chan) {
			shell_error(sh, "Unable to config channel");
			return -ENOEXEC;
		}
	}

	shell_print(sh, "ASE config: preset %s", named_preset->name);

	return 0;
}

static int cmd_release(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_chan) {
		shell_print(sh, "Not connected");
		return -ENOEXEC;
	}

	err = bt_audio_chan_release(default_chan, false);
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

	if (!default_chan) {
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

	err = bt_audio_chan_qos(default_chan, &named_preset->preset.qos);
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

	if (!default_chan) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	err = bt_audio_chan_enable(default_chan,
				   default_preset->preset.codec.meta_count,
				   default_preset->preset.codec.meta);
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
	if (!strcmp(name, "None")) {
		return BT_CODEC_META_CONTEXT_NONE;
	} else if (!strcmp(name, "Voice")) {
		return BT_CODEC_META_CONTEXT_VOICE;
	} else if (!strcmp(name, "Media")) {
		return BT_CODEC_META_CONTEXT_MEDIA;
	} else if (!strcmp(name, "Instruction")) {
		return BT_CODEC_META_CONTEXT_INSTRUCTION;
	} else if (!strcmp(name, "Attention")) {
		return BT_CODEC_META_CONTEXT_ATTENTION;
	} else if (!strcmp(name, "Alert")) {
		return BT_CODEC_META_CONTEXT_ALERT;
	} else if (!strcmp(name, "ManMachine")) {
		return BT_CODEC_META_CONTEXT_MAN_MACHINE;
	} else if (!strcmp(name, "Emergency")) {
		return BT_CODEC_META_CONTEXT_EMERGENCY;
	} else if (!strcmp(name, "Ringtone")) {
		return BT_CODEC_META_CONTEXT_RINGTONE;
	} else if (!strcmp(name, "TV")) {
		return BT_CODEC_META_CONTEXT_TV;
	}

	return 0u;
}

static int cmd_metadata(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_chan) {
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

	err = bt_audio_chan_metadata(default_chan,
				     default_preset->preset.codec.meta_count,
				     default_preset->preset.codec.meta);
	if (err) {
		shell_error(sh, "Unable to set Channel metadata");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_start(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_chan) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	err = bt_audio_chan_start(default_chan);
	if (err) {
		shell_error(sh, "Unable to start Channel");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_disable(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_chan) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	err = bt_audio_chan_disable(default_chan);
	if (err) {
		shell_error(sh, "Unable to disable Channel");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_stop(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (!default_chan) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	err = bt_audio_chan_stop(default_chan);
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

	for (i = 0; i < ARRAY_SIZE(chans); i++) {
		struct bt_audio_chan *chan = &chans[i];

		if (chan->conn) {
			shell_print(sh, "  %s#%u: chan %p dir 0x%02x "
				    "group %p linked %s",
				    chan == default_chan ? "*" : " ", i, chan,
				    chan->cap->type, chan->group,
				    sys_slist_is_empty(&chan->links) ? "no" :
				    "yes");
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

static int cmd_select(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_audio_chan *chan;
	int index;
	bool broadcast = false;

	index = strtol(argv[1], NULL, 0);
	if (index < 0 || index > ARRAY_SIZE(chans)) {
		shell_error(sh, "Invalid index: %d", index);
		return -ENOEXEC;
	}

	if (argc > 2) {
#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
		if (strcmp(argv[2], "broadcast") == 0) {
			broadcast = true;
			if (index > ARRAY_SIZE(broadcast_source_chans)) {
				shell_error(sh,
					    "Invalid index for broadcast: %d",
					    index);
			}
		} else {
			shell_error(sh, "Invalid argument %s", argv[2]);
		}
#else
		shell_error(sh, "Invalid argument %s", argv[2]);
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */
	}

	if (broadcast) {
#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
		chan = &broadcast_source_chans[index];
#else
		shell_error(sh, "Invalid choice");
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */
	} else {
		chan = &chans[index];
		if (!chan->conn) {
			shell_error(sh, "Invalid index");
			return -ENOEXEC;
		}
	}

	set_channel(chan);

	return 0;
}

static int cmd_link(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_audio_chan *chan1, *chan2;
	int index1, index2, err;

	index1 = strtol(argv[1], NULL, 0);
	if (index1 < 0 || index1 > ARRAY_SIZE(chans)) {
		shell_error(sh, "Invalid index1");
		return -ENOEXEC;
	}

	index2 = strtol(argv[2], NULL, 0);
	if (index2 < 0 || index2 > ARRAY_SIZE(chans)) {
		shell_error(sh, "Invalid index2");
		return -ENOEXEC;
	}

	chan1 = &chans[index1 - 1];
	if (!chan1->conn) {
		shell_error(sh, "Invalid index1");
		return -ENOEXEC;
	}

	chan2 = &chans[index2 - 1];
	if (!chan2->conn) {
		shell_error(sh, "Invalid index2");
		return -ENOEXEC;
	}

	err = bt_audio_chan_link(chan1, chan2);
	if (err) {
		shell_error(sh, "Unable to bind: %d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "channnels %d:%d linked", index1, index2);

	return 0;
}

static int cmd_unlink(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_audio_chan *chan1, *chan2;
	int index1, index2, err;

	index1 = strtol(argv[1], NULL, 0);
	if (index1 < 0 || index1 > ARRAY_SIZE(chans)) {
		shell_error(sh, "Invalid index1");
		return -ENOEXEC;
	}

	index2 = strtol(argv[2], NULL, 0);
	if (index2 < 0 || index2 > ARRAY_SIZE(chans)) {
		shell_error(sh, "Invalid index2");
		return -ENOEXEC;
	}

	chan1 = &chans[index1 - 1];
	if (!chan1->conn) {
		shell_error(sh, "Invalid index1");
		return -ENOEXEC;
	}

	chan2 = &chans[index2 - 1];
	if (!chan2->conn) {
		shell_error(sh, "Invalid index2");
		return -ENOEXEC;
	}

	err = bt_audio_chan_unlink(chan1, chan2);
	if (err) {
		shell_error(sh, "Unable to bind: %d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "ases %d:%d unbound", index1, index2);

	return 0;
}

static struct bt_audio_chan *lc3_config(struct bt_conn *conn,
					struct bt_audio_ep *ep,
					struct bt_audio_capability *cap,
					struct bt_codec *codec)
{
	int i;

	shell_print(ctx_shell, "ASE Codec Config: conn %p ep %p cap %p", conn,
		    ep, cap);

	print_codec(codec);

	for (i = 0; i < ARRAY_SIZE(chans); i++) {
		struct bt_audio_chan *chan = &chans[i];

		if (!chan->conn) {
			shell_print(ctx_shell, "ASE Codec Config chan %p",
				    chan);
			set_channel(chan);
			return chan;
		}
	}

	shell_print(ctx_shell, "No channels available");

	return NULL;
}

static int lc3_reconfig(struct bt_audio_chan *chan,
			struct bt_audio_capability *cap,
			struct bt_codec *codec)
{
	shell_print(ctx_shell, "ASE Codec Reconfig: chan %p cap %p", chan, cap);

	print_codec(codec);

	if (!default_chan) {
		set_channel(chan);
	}

	if (connecting) {
		int err;

		err = bt_audio_chan_qos(chan, &default_preset->preset.qos);
		if (err) {
			shell_error(ctx_shell, "Unable to setup QoS");
			connecting = false;
			return -ENOEXEC;
		}
	}

	return 0;
}

static int lc3_qos(struct bt_audio_chan *chan, struct bt_codec_qos *qos)
{
	shell_print(ctx_shell, "QoS: chan %p", chan, qos);

	print_qos(qos);

	if (connecting) {
		int err;

		connecting = false;

		err = bt_audio_chan_enable(chan,
					   default_preset->preset.codec.meta_count,
					   default_preset->preset.codec.meta);
		if (err) {
			shell_error(ctx_shell, "Unable to enable Channel");
			return -ENOEXEC;
		}
	}

	return 0;
}

static int lc3_enable(struct bt_audio_chan *chan,
		      uint8_t meta_count, struct bt_codec_data *meta)
{
	shell_print(ctx_shell, "Enable: chan %p meta_count %u", chan,
		    meta_count);

	return 0;
}

static int lc3_start(struct bt_audio_chan *chan)
{
	shell_print(ctx_shell, "Start: chan %p", chan);

	return 0;
}

static int lc3_metadata(struct bt_audio_chan *chan,
			uint8_t meta_count, struct bt_codec_data *meta)
{
	shell_print(ctx_shell, "Metadata: chan %p meta_count %u", chan,
		    meta_count);

	return 0;
}

static int lc3_disable(struct bt_audio_chan *chan)
{
	shell_print(ctx_shell, "Disable: chan %p", chan);

	return 0;
}

static int lc3_stop(struct bt_audio_chan *chan)
{
	shell_print(ctx_shell, "Stop: chan %p", chan);

	return 0;
}

static int lc3_release(struct bt_audio_chan *chan)
{
	shell_print(ctx_shell, "Release: chan %p", chan);

	if (chan == default_chan) {
		default_chan = NULL;
	}

	connecting = false;

	return 0;
}

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

static struct bt_codec lc3_codec = BT_CODEC_LC3(BT_CODEC_LC3_FREQ_ANY,
						BT_CODEC_LC3_DURATION_ANY,
						0x03, 30, 240, 2,
						(BT_CODEC_META_CONTEXT_VOICE |
						BT_CODEC_META_CONTEXT_MEDIA),
						BT_CODEC_META_CONTEXT_ANY);

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
#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
	.scan_recv = scan_recv,
	.pa_synced = pa_synced,
	.base_recv = base_recv,
	.syncable = syncable,
	.scan_term = scan_term,
	.pa_sync_lost = pa_sync_lost,
#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK */
};

static void audio_connected(struct bt_audio_chan *chan)
{
	shell_print(ctx_shell, "Channel %p connected\n", chan);
}

static void audio_disconnected(struct bt_audio_chan *chan, uint8_t reason)
{
	shell_print(ctx_shell, "Channel %p disconnected with reason 0x%2x\n",
		    chan, reason);
}

static void audio_recv(struct bt_audio_chan *chan, struct net_buf *buf)
{
	shell_print(ctx_shell, "Incoming audio on channel %p len %u\n", chan, buf->len);
}

static struct bt_audio_chan_ops chan_ops = {
	.connected = audio_connected,
	.disconnected = audio_disconnected,
	.recv = audio_recv
};

static struct bt_audio_capability caps[MAX_PAC] = {
	{
		.type = BT_AUDIO_SOURCE,
		.pref = BT_AUDIO_CAPABILITY_PREF(
				BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED,
				BT_GAP_LE_PHY_2M, 0u, 60u, 20000u, 40000u,
				20000u, 40000u),
		.codec = &lc3_codec,
		.ops = &lc3_ops,
	},
	{
		.type = BT_AUDIO_SINK,
		.pref = BT_AUDIO_CAPABILITY_PREF(
				BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED,
				BT_GAP_LE_PHY_2M, 0u, 60u, 20000u, 40000u,
				20000u, 40000u),
		.codec = &lc3_codec,
		.ops = &lc3_ops,
	},
};

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
static int cmd_create_broadcast(const struct shell *sh, size_t argc,
				char *argv[])
{
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

	err = bt_audio_broadcast_source_create(broadcast_source_chans,
					       ARRAY_SIZE(broadcast_source_chans),
					       &named_preset->preset.codec,
					       &named_preset->preset.qos,
					       &default_source);
	if (err != 0) {
		shell_error(sh, "Unable to create broadcast source: %d", err);
		return err;
	}

	shell_print(sh, "Broadcast source created: preset %s",
		    named_preset->name);

	if (default_chan == NULL) {
		default_chan = &broadcast_source_chans[0];
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

	err = bt_audio_broadcast_sink_sync(default_sink, bis_bitfield,
					   broadcast_sink_chans, NULL);
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

	for (i = 0; i < ARRAY_SIZE(caps); i++) {
		bt_audio_capability_register(&caps[i]);
	}

	for (i = 0; i < ARRAY_SIZE(chans); i++) {
		bt_audio_chan_cb_register(&chans[i], &chan_ops);
	}

#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
	for (i = 0; i < ARRAY_SIZE(broadcast_source_chans); i++) {
		bt_audio_chan_cb_register(&broadcast_source_chans[i], &chan_ops);
	}
#endif /* CONFIG_BT_AUDIO_BROADCAST_SOURCE */

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

#define DATA_MTU CONFIG_BT_ISO_TX_MTU
NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, DATA_MTU, NULL);

static int cmd_send(const struct shell *sh, size_t argc, char *argv[])
{
	static uint8_t data[DATA_MTU - BT_ISO_CHAN_SEND_RESERVE];
	int ret, len;
	struct net_buf *buf;

	if (argc > 1) {
		len = hex2bin(argv[1], strlen(argv[1]), data, sizeof(data));
		if (len > default_chan->iso->qos->tx->sdu) {
			shell_print(sh, "Unable to send: len %d > %u MTU",
				    len, default_chan->iso->qos->tx->sdu);
			return -ENOEXEC;
		}
	} else {
		len = MIN(default_chan->iso->qos->tx->sdu, sizeof(data));
		memset(data, 0xff, len);
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

	net_buf_add_mem(buf, data, len);
	ret = bt_audio_chan_send(default_chan, buf);
	if (ret < 0) {
		shell_print(sh, "Unable to send: %d", -ret);
		net_buf_unref(buf);
		return -ENOEXEC;
	}
	shell_print(sh, "Sending:");
	shell_hexdump(sh, data, len);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(bap_cmds,
	SHELL_CMD_ARG(init, NULL, NULL, cmd_init, 1, 0),
	SHELL_CMD_ARG(discover, NULL, "[type: sink, source]",
		      cmd_discover, 1, 1),
	SHELL_CMD_ARG(preset, NULL, "[preset]", cmd_preset, 1, 1),
	SHELL_CMD_ARG(config, NULL,
		      "<direction: sink, source> <index> [codec] [preset]",
		      cmd_config, 3, 2),
#if defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
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
	SHELL_CMD_ARG(select, NULL, "<chan> [broadcast]", cmd_select, 2, 0),
	SHELL_CMD_ARG(link, NULL, "<chan1> <chan2>", cmd_link, 3, 0),
	SHELL_CMD_ARG(unlink, NULL, "<chan1> <chan2>", cmd_unlink, 3, 0),
	SHELL_CMD_ARG(connect, NULL,
		      "<direction: sink, source> <index>  [codec] [preset]",
		      cmd_connect, 3, 2),
	SHELL_CMD_ARG(send, NULL, "Send to Audio Channel [data]",
		      cmd_send, 1, 1),
	SHELL_SUBCMD_SET_END
);

static int cmd_bap(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(bap, &bap_cmds, "Bluetooth BAP shell commands",
		       cmd_bap, 1, 1);
