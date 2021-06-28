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
static struct bt_audio_chan broadcast_chans[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
static uint64_t broadcast_chan_index_bits;
static struct bt_audio_capability *rcaps[2][CONFIG_BT_BAP_PAC_COUNT];
static struct bt_audio_ep *snks[CONFIG_BT_BAP_ASE_SNK_COUNT];
static struct bt_audio_ep *srcs[CONFIG_BT_BAP_ASE_SNK_COUNT];
static struct bt_audio_chan *default_chan;
static bool connecting;

#define LC3_PRESET(_name, _codec, _qos) \
	{ \
		.name = _name, \
		.codec = _codec, \
		.qos = _qos, \
	}

static struct lc3_preset {
	const char *name;
	struct bt_codec codec;
	struct bt_codec_qos qos;
} lc3_presets[] = {
	/* Table 4.43: QoS configuration support setting requirements */
	LC3_PRESET("8_1_1",
		   BT_CODEC_LC3_CONFIG_8_1,
		   BT_CODEC_LC3_QOS_7_5_INOUT_UNFRAMED(26u, 2u, 8u, 40000u)),
	LC3_PRESET("8_2_1",
		   BT_CODEC_LC3_CONFIG_8_2,
		   BT_CODEC_LC3_QOS_10_INOUT_UNFRAMED(30u, 2u, 10u, 40000u)),
	LC3_PRESET("16_1_1",
		   BT_CODEC_LC3_CONFIG_16_1,
		   BT_CODEC_LC3_QOS_7_5_INOUT_UNFRAMED(30u, 2u, 8u, 40000u)),
	LC3_PRESET("16_2_1",
		   BT_CODEC_LC3_CONFIG_16_2,
		   BT_CODEC_LC3_QOS_10_INOUT_UNFRAMED(40u, 2u, 10u, 40000u)),
	LC3_PRESET("2_16_2_1",
		   BT_CODEC_LC3_CONFIG_2_16_2,
		   BT_CODEC_LC3_QOS_10_INOUT_UNFRAMED(80u, 2u, 10u, 40000u)),
	LC3_PRESET("24_1_1",
		   BT_CODEC_LC3_CONFIG_24_1,
		   BT_CODEC_LC3_QOS_7_5_INOUT_UNFRAMED(45u, 2u, 8u, 40000u)),
	LC3_PRESET("24_2_1",
		   BT_CODEC_LC3_CONFIG_24_2,
		   BT_CODEC_LC3_QOS_10_INOUT_UNFRAMED(60u, 2u, 10u, 40000u)),
	LC3_PRESET("32_1_1",
		   BT_CODEC_LC3_CONFIG_32_1,
		   BT_CODEC_LC3_QOS_7_5_INOUT_UNFRAMED(60u, 2u, 8u, 40000u)),
	LC3_PRESET("32_2_1",
		   BT_CODEC_LC3_CONFIG_32_2,
		   BT_CODEC_LC3_QOS_10_INOUT_UNFRAMED(80u, 2u, 10u, 40000u)),
	LC3_PRESET("44_1_1",
		   BT_CODEC_LC3_CONFIG_44_1,
		   BT_CODEC_QOS(BT_CODEC_QOS_OUT,
				8163u, BT_CODEC_QOS_FRAMED, BT_CODEC_QOS_2M,
				98u, 5u, 24u, 40000u)),
	LC3_PRESET("44_1_1",
		   BT_CODEC_LC3_CONFIG_44_2,
		   BT_CODEC_QOS(BT_CODEC_QOS_OUT,
				10884u, BT_CODEC_QOS_FRAMED, BT_CODEC_QOS_2M,
				130u, 5u, 31u, 40000u)),
	LC3_PRESET("48_1_1",
		   BT_CODEC_LC3_CONFIG_48_1,
		   BT_CODEC_LC3_QOS_7_5_OUT_UNFRAMED(75u, 5u, 15u, 40000u)),
	LC3_PRESET("48_2_1",
		   BT_CODEC_LC3_CONFIG_48_2,
		   BT_CODEC_LC3_QOS_10_OUT_UNFRAMED(100u, 5u, 20u, 40000u)),
	LC3_PRESET("48_3_1",
		   BT_CODEC_LC3_CONFIG_48_3,
		   BT_CODEC_LC3_QOS_7_5_OUT_UNFRAMED(90u, 5u, 15u, 40000u)),
	LC3_PRESET("48_4_1",
		   BT_CODEC_LC3_CONFIG_48_4,
		   BT_CODEC_LC3_QOS_10_OUT_UNFRAMED(120u, 5u, 20u, 40000u)),
	LC3_PRESET("48_5_1",
		   BT_CODEC_LC3_CONFIG_48_5,
		   BT_CODEC_LC3_QOS_7_5_OUT_UNFRAMED(117u, 5u, 15u, 40000u)),
	LC3_PRESET("48_6_1",
		   BT_CODEC_LC3_CONFIG_48_6,
		   BT_CODEC_LC3_QOS_10_OUT_UNFRAMED(155u, 5u, 20u, 40000u)),
	/* QoS Configuration settings for high reliability audio data */
	LC3_PRESET("44_1_2",
		   BT_CODEC_LC3_CONFIG_44_2,
		   BT_CODEC_QOS(BT_CODEC_QOS_OUT,
				8163u, BT_CODEC_QOS_FRAMED, BT_CODEC_QOS_2M,
				98u, 23u, 54u, 40000u)),
	LC3_PRESET("44_1_2",
		   BT_CODEC_LC3_CONFIG_44_2,
		   BT_CODEC_QOS(BT_CODEC_QOS_OUT,
				10884u, BT_CODEC_QOS_FRAMED, BT_CODEC_QOS_2M,
				130u, 23u, 71u, 40000u)),
	LC3_PRESET("48_1_2",
		   BT_CODEC_LC3_CONFIG_48_1,
		   BT_CODEC_LC3_QOS_7_5_OUT_UNFRAMED(75u, 23u, 45u, 40000u)),
	LC3_PRESET("48_2_2",
		   BT_CODEC_LC3_CONFIG_48_2,
		   BT_CODEC_LC3_QOS_10_OUT_UNFRAMED(100u, 23u, 60u, 40000u)),
	LC3_PRESET("48_3_2",
		   BT_CODEC_LC3_CONFIG_48_1,
		   BT_CODEC_LC3_QOS_7_5_OUT_UNFRAMED(90u, 23u, 45u, 40000u)),
	LC3_PRESET("48_4_2",
		   BT_CODEC_LC3_CONFIG_48_2,
		   BT_CODEC_LC3_QOS_10_OUT_UNFRAMED(120u, 23u, 60u, 40000u)),
	LC3_PRESET("48_5_2",
		   BT_CODEC_LC3_CONFIG_48_1,
		   BT_CODEC_LC3_QOS_7_5_OUT_UNFRAMED(117u, 23u, 45u, 40000u)),
	LC3_PRESET("48_6_2",
		   BT_CODEC_LC3_CONFIG_48_2,
		   BT_CODEC_LC3_QOS_10_OUT_UNFRAMED(155u, 23u, 60u, 40000u)),
};

/* Default to 16_2_1 */
static struct lc3_preset *default_preset = &lc3_presets[3];

static void print_codec(struct bt_codec *codec)
{
	int i;

	shell_print(ctx_shell, "codec 0x%02x cid 0x%04x vid 0x%04x count %u",
		    codec->id, codec->cid, codec->vid, codec->data_count);

	for (i = 0; i < codec->data_count; i++) {
		shell_print(ctx_shell, "data #%u: type 0x%02x len %u", i,
			    codec->data[i].data.type,
			    codec->data[i].data.data_len);
		shell_hexdump(ctx_shell, codec->data[i].data.data,
			      codec->data[i].data.data_len);
	}

	for (i = 0; i < codec->meta_count; i++) {
		shell_print(ctx_shell, "meta #%u: type 0x%02x len %u", i,
			    codec->meta[i].data.type,
			    codec->meta[i].data.data_len);
		shell_hexdump(ctx_shell, codec->meta[i].data.data,
			      codec->meta[i].data.data_len);
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

static struct lc3_preset *set_preset(size_t argc, char **argv)
{
	static struct lc3_preset preset;
	int i;

	for (i = 0; i < ARRAY_SIZE(lc3_presets); i++) {
		if (!strcmp(argv[0], lc3_presets[i].name)) {
			default_preset = &lc3_presets[i];
			break;
		}
	}

	if (i == ARRAY_SIZE(lc3_presets)) {
		return NULL;
	}

	if (argc == 1) {
		return default_preset;
	}

	preset = *default_preset;
	default_preset = &preset;

	if (argc > 1) {
		preset.qos.interval = strtol(argv[1], NULL, 0);
	}

	if (argc > 2) {
		preset.qos.framing = strtol(argv[2], NULL, 0);
	}

	if (argc > 3) {
		preset.qos.latency = strtol(argv[3], NULL, 0);
	}

	if (argc > 4) {
		preset.qos.pd = strtol(argv[4], NULL, 0);
	}

	if (argc > 5) {
		preset.qos.sdu = strtol(argv[5], NULL, 0);
	}

	if (argc > 6) {
		preset.qos.phy = strtol(argv[6], NULL, 0);
	}

	if (argc > 7) {
		preset.qos.rtn = strtol(argv[7], NULL, 0);
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
	struct lc3_preset *preset;

	preset = default_preset;

	if (argc > 1) {
		preset = set_preset(argc - 1, argv + 1);
		if (!preset) {
			shell_error(sh, "Unable to parse preset %s",
				    argv[1]);
			return -ENOEXEC;
		}
	}

	shell_print(sh, "%s", preset->name);

	print_codec(&preset->codec);
	print_qos(&preset->qos);

	return 0;
}

static int cmd_config(const struct shell *sh, size_t argc, char *argv[])
{
	int32_t index, dir;
	struct bt_audio_capability *cap = NULL;
	struct bt_audio_ep *ep = NULL;
	struct lc3_preset *preset;
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

	preset = default_preset;

	if (argc > 3) {
		preset = set_preset(1, argv + 3);
		if (!preset) {
			shell_error(sh, "Unable to parse preset %s",
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
					   &preset->codec) < 0) {
			shell_error(sh, "Unable reconfig channel");
			return -ENOEXEC;
		}
	} else {
		struct bt_audio_chan *chan;

		chan = bt_audio_chan_config(default_conn, ep, cap,
					    &preset->codec);
		if (!chan) {
			shell_error(sh, "Unable to config channel");
			return -ENOEXEC;
		}
	}

	shell_print(sh, "ASE config: preset %s", preset->name);

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
	struct lc3_preset *preset = NULL;

	if (!default_chan) {
		shell_print(sh, "Not connected");
		return -ENOEXEC;
	}

	preset = default_preset;

	if (argc > 1) {
		preset = set_preset(argc - 1, argv + 1);
		if (!preset) {
			shell_error(sh, "Unable to parse preset %s",
				    argv[1]);
			return -ENOEXEC;
		}
	}

	err = bt_audio_chan_qos(default_chan, &preset->qos);
	if (err) {
		shell_error(sh, "Unable to setup QoS");
		return -ENOEXEC;
	}

	shell_print(sh, "ASE config: preset %s", preset->name);

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
				   default_preset->codec.meta_count,
				   default_preset->codec.meta);
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

		sys_put_le16(context, default_preset->codec.meta[0].value);
	}

	err = bt_audio_chan_metadata(default_chan,
				     default_preset->codec.meta_count,
				     default_preset->codec.meta);
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
				    "state 0x%02x linked %s",
				    chan == default_chan ? "*" : " ", i, chan,
				    chan->cap->type, chan->state,
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

	index = strtol(argv[1], NULL, 0);
	if (index < 0 || index > ARRAY_SIZE(chans)) {
		shell_error(sh, "Invalid index");
		return -ENOEXEC;
	}

	chan = &chans[index];
	if (!chan->conn) {
		shell_error(sh, "Invalid index");
		return -ENOEXEC;
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

		err = bt_audio_chan_qos(chan, &default_preset->qos);
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
					   default_preset->codec.meta_count,
					   default_preset->codec.meta);
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
};

static void audio_recv(struct bt_audio_chan *chan, struct net_buf *buf)
{
	shell_print(ctx_shell, "Incoming audio on channel %p len %u\n", chan, buf->len);
}

static struct bt_audio_chan_ops chan_ops = {
	.recv = audio_recv
};

static struct bt_audio_capability caps[MAX_PAC] = {
	{
		.type = BT_AUDIO_SOURCE,
		.qos = BT_AUDIO_QOS(0x00, 0x02, 0u, 60u, 20000u, 40000u),
		.codec = &lc3_codec,
		.ops = &lc3_ops,
	},
	{
		.type = BT_AUDIO_SINK,
		.qos = BT_AUDIO_QOS(0x00, 0x02, 0u, 60u, 20000u, 40000u),
		.codec = &lc3_codec,
		.ops = &lc3_ops,
	},
};

static int cmd_create_broadcast(const struct shell *sh, size_t argc,
				char *argv[])
{
	struct lc3_preset *preset;
	int err;
	struct bt_audio_chan *free_chan;
	int i;

	/* TODO: Support multiple channels for the broadcaster */
	for (i = 0; i < ARRAY_SIZE(broadcast_chans); i++) {
		if ((BIT(i) & broadcast_chan_index_bits) == 0) {
			free_chan = &broadcast_chans[i];
			break;
		}
	}

	preset = default_preset;

	if (argc > 1) {
		preset = set_preset(1, &argv[1]);
		if (!preset) {
			shell_error(sh, "Unable to parse preset %s", argv[1]);
			return -ENOEXEC;
		}
	}

	err = bt_audio_broadcaster_create(free_chan, &preset->codec,
					  &preset->qos);
	if (err != 0) {
		shell_error(sh, "Unable to create broadcaster: %d", err);
		return err;
	}

	broadcast_chan_index_bits |= BIT(i);

	shell_print(sh, "Broadcaster created: preset %s", preset->name);

	return 0;
}

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
	SHELL_CMD_ARG(create_broadcast, NULL, "[codec] [preset]",
		      cmd_create_broadcast, 1, 2),
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
	SHELL_CMD_ARG(select, NULL, "<chan>", cmd_select, 2, 0),
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
