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

static struct bt_audio_chan chans[MAX_PAC];
static struct bt_audio_cap *rcaps[2][CONFIG_BT_BAP_PAC_COUNT];
static struct bt_audio_ep *reps[CONFIG_BT_BAP_ASE_COUNT];
static struct bt_audio_chan *default_chan;

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

static void discover_cb(struct bt_conn *conn, struct bt_audio_cap *cap,
			struct bt_audio_ep *ep,
			struct bt_audio_discover_params *params)
{
	if (cap) {
		shell_print(ctx_shell, "cap %p type 0x%02x", cap, cap->type);

		print_codec(cap->codec);

		if (cap->type != BT_AUDIO_SINK &&
		    cap->type != BT_AUDIO_SOURCE) {
			return;
		}

		if (params->num_caps < CONFIG_BT_BAP_PAC_COUNT) {
			rcaps[cap->type - 1][params->num_caps] = cap;
		}

		return;
	}

	if (ep) {
		shell_print(ctx_shell, "ep %p", ep);
		reps[params->num_eps] = ep;
		return;
	}

	shell_print(ctx_shell, "Discover complete: err %d", params->err);

	memset(params, 0, sizeof(*params));
}

static int cmd_discover(const struct shell *shell, size_t argc, char *argv[])
{
	static struct bt_audio_discover_params params;

	if (!default_conn) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
	}

	if (params.func) {
		shell_error(shell, "Discover in progress");
		return -ENOEXEC;
	}

	params.type = strtol(argv[1], NULL, 0);
	if (params.type != BT_AUDIO_SINK && params.type != BT_AUDIO_SOURCE) {
		shell_error(shell, "Invalid type");
		return -ENOEXEC;
	}

	params.func = discover_cb;

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
			shell_print(ctx_shell, "Default ase: %u", i + 1);
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

static int cmd_preset(const struct shell *shell, size_t argc, char *argv[])
{
	struct lc3_preset *preset;

	preset = default_preset;

	if (argc > 1) {
		preset = set_preset(argc - 1, argv + 1);
		if (!preset) {
			shell_error(shell, "Unable to parse preset %s",
				    argv[1]);
			return -ENOEXEC;
		}
	}

	shell_print(shell, "%s", preset->name);

	print_codec(&preset->codec);
	print_qos(&preset->qos);

	return 0;
}

static int cmd_config(const struct shell *shell, size_t argc, char *argv[])
{
	int32_t ase, dir;
	struct bt_audio_cap *cap = NULL;
	struct bt_audio_ep *ep = NULL;
	struct lc3_preset *preset;
	int i;

	if (!default_conn) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
	}

	ase = strtol(argv[1], NULL, 0);
	if (ase < 0 || ase > ARRAY_SIZE(reps)) {
		shell_error(shell, "Invalid ase");
		return -ENOEXEC;
	}

	dir = strtol(argv[2], NULL, 0);
	if (dir != BT_AUDIO_SOURCE && dir != BT_AUDIO_SINK) {
		shell_error(shell, "Invalid direction");
		return -ENOEXEC;
	}

	preset = default_preset;

	if (argc > 3) {
		preset = set_preset(1, argv + 3);
		if (!preset) {
			shell_error(shell, "Unable to parse preset %s",
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
		shell_error(shell, "Unable to find matching capabilities");
		return -ENOEXEC;
	}

	ep = reps[ase - 1];
	if (!ep) {
		shell_error(shell, "Unable to find endpoint");
		return -ENOEXEC;
	}

	if (default_chan && default_chan->ep == ep) {
		if (bt_audio_chan_reconfig(default_chan, cap,
					   &preset->codec) < 0) {
			shell_error(shell, "Unable reconfig channel");
			return -ENOEXEC;
		}
	} else {
		struct bt_audio_chan *chan;

		chan = bt_audio_chan_config(default_conn, ep, cap,
					    &preset->codec);
		if (!chan) {
			shell_error(shell, "Unable to config channel");
			return -ENOEXEC;
		}
	}

	shell_print(shell, "ASE config: preset %s", preset->name);

	return 0;
}

static int cmd_release(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (!default_chan) {
		shell_print(shell, "Not connected");
		return -ENOEXEC;
	}

	err = bt_audio_chan_release(default_chan, false);
	if (err) {
		shell_error(shell, "Unable to release Channel");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_qos(const struct shell *shell, size_t argc, char *argv[])
{
	int err;
	struct lc3_preset *preset = NULL;

	if (!default_chan) {
		shell_print(shell, "Not connected");
		return -ENOEXEC;
	}

	preset = default_preset;

	if (argc > 1) {
		preset = set_preset(argc - 1, argv + 1);
		if (!preset) {
			shell_error(shell, "Unable to parse preset %s",
				    argv[1]);
			return -ENOEXEC;
		}
	}

	err = bt_audio_chan_qos(default_chan, &preset->qos);
	if (err) {
		shell_error(shell, "Unable to setup QoS");
		return -ENOEXEC;
	}

	shell_print(shell, "ASE config: preset %s", preset->name);

	return 0;
}

static int cmd_enable(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (!default_chan) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
	}

	err = bt_audio_chan_enable(default_chan,
				   default_preset->codec.meta_count,
				   default_preset->codec.meta);
	if (err) {
		shell_error(shell, "Unable to enable Channel");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_start(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (!default_chan) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
	}

	err = bt_audio_chan_start(default_chan);
	if (err) {
		shell_error(shell, "Unable to start Channel");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_disable(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (!default_chan) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
	}

	err = bt_audio_chan_disable(default_chan);
	if (err) {
		shell_error(shell, "Unable to disable Channel");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_stop(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (!default_chan) {
		shell_error(shell, "Not connected");
		return -ENOEXEC;
	}

	err = bt_audio_chan_stop(default_chan);
	if (err) {
		shell_error(shell, "Unable to start Channel");
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_list(const struct shell *shell, size_t argc, char *argv[])
{
	int i;

	for (i = 0; i < ARRAY_SIZE(chans); i++) {
		struct bt_audio_chan *chan = &chans[i];

		if (chan->conn) {
			shell_print(shell, "%s%u: ase 0x%02x dir 0x%02x "
				    "state 0x%02x linked %s",
				    chan == default_chan ? "*" : " ", i, i + 1,
				    chan->cap->type, chan->state,
				    sys_slist_is_empty(&chan->links) ? "no" :
				    "yes");
		}
	}

	return 0;
}

static int cmd_select(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_audio_chan *chan;
	int ase;

	ase = strtol(argv[1], NULL, 0);
	if (ase < 0 || ase > ARRAY_SIZE(chans)) {
		shell_error(shell, "Invalid ase");
		return -ENOEXEC;
	}

	chan = &chans[ase - 1];
	if (!chan->conn) {
		shell_error(shell, "Invalid ase");
		return -ENOEXEC;
	}

	set_channel(chan);

	return 0;
}

static int cmd_link(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_audio_chan *chan1, *chan2;
	int ase1, ase2, err;

	ase1 = strtol(argv[1], NULL, 0);
	if (ase1 < 0 || ase1 > ARRAY_SIZE(chans)) {
		shell_error(shell, "Invalid ase1");
		return -ENOEXEC;
	}

	ase2 = strtol(argv[2], NULL, 0);
	if (ase2 < 0 || ase2 > ARRAY_SIZE(chans)) {
		shell_error(shell, "Invalid ase2");
		return -ENOEXEC;
	}

	chan1 = &chans[ase1 - 1];
	if (!chan1->conn) {
		shell_error(shell, "Invalid ase1");
		return -ENOEXEC;
	}

	chan2 = &chans[ase2 - 1];
	if (!chan2->conn) {
		shell_error(shell, "Invalid ase2");
		return -ENOEXEC;
	}

	err = bt_audio_chan_link(chan1, chan2);
	if (err) {
		shell_error(shell, "Unable to bind: %d", err);
		return -ENOEXEC;
	}

	shell_print(shell, "ases %d:%d linked", ase1, ase2);

	return 0;
}

static int cmd_unlink(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_audio_chan *chan1, *chan2;
	int ase1, ase2, err;

	ase1 = strtol(argv[1], NULL, 0);
	if (ase1 < 0 || ase1 > ARRAY_SIZE(chans)) {
		shell_error(shell, "Invalid ase1");
		return -ENOEXEC;
	}

	ase2 = strtol(argv[2], NULL, 0);
	if (ase2 < 0 || ase2 > ARRAY_SIZE(chans)) {
		shell_error(shell, "Invalid ase2");
		return -ENOEXEC;
	}

	chan1 = &chans[ase1 - 1];
	if (!chan1->conn) {
		shell_error(shell, "Invalid ase1");
		return -ENOEXEC;
	}

	chan2 = &chans[ase2 - 1];
	if (!chan2->conn) {
		shell_error(shell, "Invalid ase2");
		return -ENOEXEC;
	}

	err = bt_audio_chan_unlink(chan1, chan2);
	if (err) {
		shell_error(shell, "Unable to bind: %d", err);
		return -ENOEXEC;
	}

	shell_print(shell, "ases %d:%d unbound", ase1, ase2);

	return 0;
}

static struct bt_audio_chan *lc3_config(struct bt_conn *conn,
					struct bt_audio_ep *ep,
					struct bt_audio_cap *cap,
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

static int lc3_qos(struct bt_audio_chan *chan, struct bt_codec_qos *qos)
{
	shell_print(ctx_shell, "QoS: chan %p", chan, qos);

	print_qos(qos);

	return 0;
}

static int lc3_enable(struct bt_audio_chan *chan,
		      uint8_t meta_count, struct bt_codec_data *meta)
{
	shell_print(ctx_shell, "Enable: chan %p meta_count %u", chan,
		    meta_count);

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

static int lc3_release(struct bt_audio_chan *chan)
{
	shell_print(ctx_shell, "Release: chan %p", chan);

	if (chan == default_chan) {
		default_chan = NULL;
	}

	return 0;
}

static struct bt_codec lc3_codec = BT_CODEC_LC3(BT_CODEC_LC3_FREQ_ANY,
						BT_CODEC_LC3_DURATION_ANY,
						0x03, 30, 240, 1,
						(BT_CODEC_META_CONTEXT_VOICE |
						BT_CODEC_META_CONTEXT_MEDIA),
						BT_CODEC_META_CONTEXT_ANY);

static struct bt_audio_cap_ops lc3_ops = {
	.config = lc3_config,
	.qos = lc3_qos,
	.enable = lc3_enable,
	.metadata = lc3_metadata,
	.disable = lc3_disable,
	.release = lc3_release,
};

static struct bt_audio_cap caps[MAX_PAC] = {
	{
		.type = BT_AUDIO_SOURCE,
		.qos = BT_AUDIO_QOS(0x00, 0x02, 2u, 60u, 20000u, 40000u),
		.codec = &lc3_codec,
		.ops = &lc3_ops,
	},
	{
		.type = BT_AUDIO_SINK,
		.qos = BT_AUDIO_QOS(0x00, 0x02, 2u, 60u, 20000u, 40000u),
		.codec = &lc3_codec,
		.ops = &lc3_ops,
	},
};

static int cmd_init(const struct shell *shell, size_t argc, char *argv[])
{
	int err, i;

	ctx_shell = shell;

	err = bt_enable(NULL);
	if (err && err != -EALREADY) {
		shell_error(shell, "Bluetooth init failed (err %d)", err);
		return err;
	}

	for (i = 0; i < ARRAY_SIZE(caps); i++) {
		bt_audio_register(&caps[i]);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(bap_cmds,
	SHELL_CMD_ARG(init, NULL, NULL, cmd_init, 1, 0),
	SHELL_CMD_ARG(discover, NULL, "<type: sink, source>",
		      cmd_discover, 2, 0),
	SHELL_CMD_ARG(preset, NULL, "[preset]", cmd_preset, 1, 1),
	SHELL_CMD_ARG(config, NULL,
		      "<ase> <direction: sink, source> [codec] [preset]",
		      cmd_config, 3, 2),
	SHELL_CMD_ARG(qos, NULL,
		      "[preset] [interval] [framing] [latency] [pd] [sdu] [phy]"
		      " [rtn]", cmd_qos, 1, 8),
	SHELL_CMD_ARG(enable, NULL, NULL, cmd_enable, 1, 1),
	SHELL_CMD_ARG(start, NULL, NULL, cmd_start, 1, 0),
	SHELL_CMD_ARG(disable, NULL, NULL, cmd_disable, 1, 0),
	SHELL_CMD_ARG(stop, NULL, NULL, cmd_stop, 1, 0),
	SHELL_CMD_ARG(release, NULL, NULL, cmd_release, 1, 0),
	SHELL_CMD_ARG(list, NULL, NULL, cmd_list, 1, 0),
	SHELL_CMD_ARG(select, NULL, "<ase>", cmd_select, 2, 0),
	SHELL_CMD_ARG(link, NULL, "<ase1> <ase2>", cmd_link, 3, 0),
	SHELL_CMD_ARG(unlink, NULL, "<ase1> <ase2>", cmd_unlink, 3, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_bap(const struct shell *shell, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(shell, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(shell, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(bap, &bap_cmds, "Bluetooth BAP shell commands",
		       cmd_bap, 1, 1);
