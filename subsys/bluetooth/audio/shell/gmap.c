/** @file
 *  @brief Bluetooth Gaming Audio Profile shell
 *
 */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/gmap_lc3_preset.h>
#include <zephyr/bluetooth/audio/gmap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "shell/bt.h"
#include "audio.h"

#define UNICAST_SINK_SUPPORTED (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 0)
#define UNICAST_SRC_SUPPORTED  (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0)

#define GMAP_AC_MAX_CONN   2U
#define GMAP_AC_MAX_SRC    (2U * GMAP_AC_MAX_CONN)

static enum bt_gmap_role gmap_role;

size_t gmap_ad_data_add(struct bt_data data[], size_t data_size)
{
	static uint8_t ad_gmap[3] = {
		BT_UUID_16_ENCODE(BT_UUID_GMAS_VAL),
	};

	if (gmap_role == 0) {
		/* Not initialized*/

		return 0U;
	}

	ad_gmap[2] = (uint8_t)gmap_role;

	__ASSERT(data > 0, "No space for ad_gmap");
	data[0].type = BT_DATA_SVC_DATA16;
	data[0].data_len = ARRAY_SIZE(ad_gmap);
	data[0].data = &ad_gmap[0];

	return 1U;
}

static void gmap_discover_cb(struct bt_conn *conn, int err, enum bt_gmap_role role,
			     struct bt_gmap_feat features)
{
	if (err != 0) {
		shell_error(ctx_shell, "gmap discovery (err %d)", err);
		return;
	}

	shell_print(ctx_shell,
		    "gmap discovered for conn %p:\n\trole 0x%02x\n\tugg_feat 0x%02x\n\tugt_feat "
		    "0x%02x\n\tbgs_feat 0x%02x\n\tbgr_feat 0x%02x",
		    conn, role, features.ugg_feat, features.ugt_feat, features.bgs_feat,
		    features.bgr_feat);
}

static const struct bt_gmap_cb gmap_cb = {
	.discover = gmap_discover_cb,
};

static void set_gmap_features(struct bt_gmap_feat *features)
{
	memset(features, 0, sizeof(*features));

	if (IS_ENABLED(CONFIG_BT_GMAP_UGG_SUPPORTED)) {
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0
		features->ugg_feat |= (BT_GMAP_UGG_FEAT_MULTIPLEX | BT_GMAP_UGG_FEAT_96KBPS_SOURCE);
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1 &&                                              \
	CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT > 1
		features->ugg_feat |= BT_GMAP_UGG_FEAT_MULTISINK;
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1 && > 1 */
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 0 */
	}

	if (IS_ENABLED(CONFIG_BT_GMAP_UGT_SUPPORTED)) {
#if CONFIG_BT_ASCS_ASE_SRC_COUNT > 0
		features->ugt_feat |= (BT_GMAP_UGT_FEAT_SOURCE | BT_GMAP_UGT_FEAT_80KBPS_SOURCE);
#if CONFIG_BT_ASCS_ASE_SRC_COUNT > 1
		features->ugt_feat |= BT_GMAP_UGT_FEAT_MULTISOURCE;
#endif /* CONFIG_BT_ASCS_ASE_SRC_COUNT > 1 */
#endif /* CONFIG_BT_ASCS_ASE_SRC_COUNT > 0 */
#if CONFIG_BT_ASCS_ASE_SNK_COUNT > 0
		features->ugt_feat |= (BT_GMAP_UGT_FEAT_SINK | BT_GMAP_UGT_FEAT_64KBPS_SINK);
#if CONFIG_BT_ASCS_ASE_SNK_COUNT > 1
		features->ugt_feat |= BT_GMAP_UGT_FEAT_MULTISINK;
#endif /* CONFIG_BT_ASCS_ASE_SNK_COUNT > 1 */
#endif /* CONFIG_BT_ASCS_ASE_SNK_COUNT > 0 */
	}

	if (IS_ENABLED(CONFIG_BT_GMAP_BGS_SUPPORTED)) {
		features->bgs_feat |= BT_GMAP_BGS_FEAT_96KBPS;
	}

	if (IS_ENABLED(CONFIG_BT_GMAP_BGR_SUPPORTED)) {
		features->bgr_feat |= BT_GMAP_BGR_FEAT_MULTISINK;
#if CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT > 1
		features->bgr_feat |= BT_GMAP_BGR_FEAT_MULTIPLEX;
#endif /* CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT > 1 */
	}
}

static int cmd_gmap_init(const struct shell *sh, size_t argc, char **argv)
{
	struct bt_gmap_feat features;
	int err;

	gmap_role = (IS_ENABLED(CONFIG_BT_GMAP_UGG_SUPPORTED) ? BT_GMAP_ROLE_UGG : 0U) |
		    (IS_ENABLED(CONFIG_BT_GMAP_UGT_SUPPORTED) ? BT_GMAP_ROLE_UGT : 0U) |
		    (IS_ENABLED(CONFIG_BT_GMAP_BGS_SUPPORTED) ? BT_GMAP_ROLE_BGS : 0U) |
		    (IS_ENABLED(CONFIG_BT_GMAP_BGR_SUPPORTED) ? BT_GMAP_ROLE_BGR : 0U);

	set_gmap_features(&features);

	err = bt_gmap_register(gmap_role, features);
	if (err != 0) {
		shell_error(sh, "Failed to register GMAS (err %d)", err);

		return -ENOEXEC;
	}

	err = bt_gmap_cb_register(&gmap_cb);
	if (err != 0) {
		shell_error(sh, "Failed to register callbacks (err %d)", err);

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_gmap_set_role(const struct shell *sh, size_t argc, char **argv)
{
	enum bt_gmap_role role = 0;
	struct bt_gmap_feat features;
	int err;

	for (size_t i = 1U; i < argc; i++) {
		const char *arg = argv[i];

		if (strcmp(arg, "ugg") == 0) {
			role |= BT_GMAP_ROLE_UGG;
		} else if (strcmp(arg, "ugt") == 0) {
			role |= BT_GMAP_ROLE_UGT;
		} else if (strcmp(arg, "bgs") == 0) {
			role |= BT_GMAP_ROLE_BGS;
		} else if (strcmp(arg, "bgr") == 0) {
			role |= BT_GMAP_ROLE_BGR;
		} else {
			shell_error(sh, "Invalid arg: %s", arg);
			shell_help(sh);

			return SHELL_CMD_HELP_PRINTED;
		}
	}

	set_gmap_features(&features);

	err = bt_gmap_set_role(role, features);
	if (err != 0) {
		shell_error(sh, "Failed to set new role (err %d)", err);

		return -ENOEXEC;
	}

	gmap_role = role;

	return 0;
}

static int cmd_gmap_discover(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!ctx_shell) {
		ctx_shell = sh;
	}

	err = bt_gmap_discover(default_conn);
	if (err != 0) {
		shell_error(sh, "bt_gmap_discover (err %d)", err);
	}

	return err;
}

static struct named_lc3_preset gmap_unicast_snk_presets[] = {
	{"32_1_gr", BT_GMAP_LC3_PRESET_32_1_GR(LOCATION, CONTEXT)},
	{"32_2_gr", BT_GMAP_LC3_PRESET_32_2_GR(LOCATION, CONTEXT)},
	{"48_1_gr", BT_GMAP_LC3_PRESET_48_1_GR(LOCATION, CONTEXT)},
	{"48_2_gr", BT_GMAP_LC3_PRESET_48_2_GR(LOCATION, CONTEXT)},
	{"48_3_gr", BT_GMAP_LC3_PRESET_48_3_GR(LOCATION, CONTEXT)},
	{"48_4_gr", BT_GMAP_LC3_PRESET_48_4_GR(LOCATION, CONTEXT)},
};

static struct named_lc3_preset gmap_unicast_src_presets[] = {
	{"16_1_gs", BT_GMAP_LC3_PRESET_16_1_GS(LOCATION, CONTEXT)},
	{"16_2_gs", BT_GMAP_LC3_PRESET_16_2_GS(LOCATION, CONTEXT)},
	{"32_1_gs", BT_GMAP_LC3_PRESET_32_1_GS(LOCATION, CONTEXT)},
	{"32_2_gs", BT_GMAP_LC3_PRESET_32_2_GS(LOCATION, CONTEXT)},
	{"48_1_gs", BT_GMAP_LC3_PRESET_48_1_GS(LOCATION, CONTEXT)},
	{"48_2_gs", BT_GMAP_LC3_PRESET_48_2_GS(LOCATION, CONTEXT)},
};

static struct named_lc3_preset gmap_broadcast_presets[] = {
	{"48_1_g", BT_GMAP_LC3_PRESET_48_1_G(LOCATION, CONTEXT)},
	{"48_2_g", BT_GMAP_LC3_PRESET_48_2_G(LOCATION, CONTEXT)},
	{"48_3_g", BT_GMAP_LC3_PRESET_48_3_G(LOCATION, CONTEXT)},
	{"48_4_g", BT_GMAP_LC3_PRESET_48_4_G(LOCATION, CONTEXT)},
};

const struct named_lc3_preset *gmap_get_named_preset(bool is_unicast, enum bt_audio_dir dir,
						     const char *preset_arg)
{
	if (is_unicast) {
		if (dir == BT_AUDIO_DIR_SINK) {
			for (size_t i = 0U; i < ARRAY_SIZE(gmap_unicast_snk_presets); i++) {
				if (!strcmp(preset_arg, gmap_unicast_snk_presets[i].name)) {
					return &gmap_unicast_snk_presets[i];
				}
			}
		} else if (dir == BT_AUDIO_DIR_SOURCE) {
			for (size_t i = 0U; i < ARRAY_SIZE(gmap_unicast_src_presets); i++) {
				if (!strcmp(preset_arg, gmap_unicast_src_presets[i].name)) {
					return &gmap_unicast_src_presets[i];
				}
			}
		}
	} else {
		for (size_t i = 0U; i < ARRAY_SIZE(gmap_broadcast_presets); i++) {
			if (!strcmp(preset_arg, gmap_broadcast_presets[i].name)) {
				return &gmap_broadcast_presets[i];
			}
		}
	}

	return NULL;
}

#if defined(CONFIG_BT_GMAP_UGG_SUPPORTED)
#if UNICAST_SINK_SUPPORTED
static int cmd_gmap_ac_1(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_1",
		.conn_cnt = 1,
		.snk_cnt = {1U},
		.src_cnt = {0U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 0U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* UNICAST_SINK_SUPPORTED */

#if UNICAST_SRC_SUPPORTED
static int cmd_gmap_ac_2(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_2",
		.conn_cnt = 1,
		.snk_cnt = {0U},
		.src_cnt = {1U},
		.snk_chan_cnt = 0U,
		.src_chan_cnt = 1U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* UNICAST_SRC_SUPPORTED */

#if UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED
static int cmd_gmap_ac_3(const struct shell *sh, size_t argc, char **argv)
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
static int cmd_gmap_ac_4(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_4",
		.conn_cnt = 1U,
		.snk_cnt = {1U},
		.src_cnt = {0U},
		.snk_chan_cnt = 2U,
		.src_chan_cnt = 0U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* UNICAST_SINK_SUPPORTED */

#if UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED
static int cmd_gmap_ac_5(const struct shell *sh, size_t argc, char **argv)
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
static int cmd_gmap_ac_6_i(const struct shell *sh, size_t argc, char **argv)
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
static int cmd_gmap_ac_6_ii(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_6_II",
		.conn_cnt = 2,
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
#if CONFIG_BT_MAX_CONN >= 2
static int cmd_gmap_ac_7_ii(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_unicast_ac_param param = {
		.name = "AC_7_II",
		.conn_cnt = 2,
		.snk_cnt = {1U, 0U},
		.src_cnt = {0U, 1U},
		.snk_chan_cnt = 1U,
		.src_chan_cnt = 1U,
	};

	return cap_ac_unicast(sh, &param);
}
#endif /* CONFIG_BT_MAX_CONN >= 2 */

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1
static int cmd_gmap_ac_8_i(const struct shell *sh, size_t argc, char **argv)
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
static int cmd_gmap_ac_8_ii(const struct shell *sh, size_t argc, char **argv)
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

#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1 && CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1
static int cmd_gmap_ac_11_i(const struct shell *sh, size_t argc, char **argv)
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
static int cmd_gmap_ac_11_ii(const struct shell *sh, size_t argc, char **argv)
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
#endif /* CONFIG_BT_GMAP_UGG_SUPPORTED */

#if defined(CONFIG_BT_GMAP_BGS_SUPPORTED)
static int cmd_gmap_ac_12(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_broadcast_ac_param param = {
		.name = "AC_12",
		.stream_cnt = 1U,
		.chan_cnt = 1U,
	};

	return cap_ac_broadcast(sh, argc, argv, &param);
}

#if CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT > 1
static int cmd_gmap_ac_13(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_broadcast_ac_param param = {
		.name = "AC_13",
		.stream_cnt = 2U,
		.chan_cnt = 1U,
	};

	return cap_ac_broadcast(sh, argc, argv, &param);
}
#endif /* CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT > 1 */

static int cmd_gmap_ac_14(const struct shell *sh, size_t argc, char **argv)
{
	const struct bap_broadcast_ac_param param = {
		.name = "AC_14",
		.stream_cnt = 1U,
		.chan_cnt = 2U,
	};

	return cap_ac_broadcast(sh, argc, argv, &param);
}
#endif /* CONFIG_BT_GMAP_BGS_SUPPORTED */

static int cmd_gmap(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	} else {
		shell_error(sh, "%s missing subcomand", argv[0]);
	}

	return -ENOEXEC;
}

#define HELP_NONE "[none]"

SHELL_STATIC_SUBCMD_SET_CREATE(
	gmap_cmds, SHELL_CMD_ARG(init, NULL, HELP_NONE, cmd_gmap_init, 1, 0),
	SHELL_CMD_ARG(set_role, NULL, "[ugt | ugg | bgr | bgs]", cmd_gmap_set_role, 2, 3),
	SHELL_CMD_ARG(discover, NULL, HELP_NONE, cmd_gmap_discover, 1, 0),
#if defined(CONFIG_BT_GMAP_UGG_SUPPORTED)
#if UNICAST_SINK_SUPPORTED
	SHELL_CMD_ARG(ac_1, NULL, "Unicast audio configuration 1", cmd_gmap_ac_1, 1, 0),
#endif /* UNICAST_SINK_SUPPORTED */
#if UNICAST_SRC_SUPPORTED
	SHELL_CMD_ARG(ac_2, NULL, "Unicast audio configuration 2", cmd_gmap_ac_2, 1, 0),
#endif /* UNICAST_SRC_SUPPORTED */
#if UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED
	SHELL_CMD_ARG(ac_3, NULL, "Unicast audio configuration 3", cmd_gmap_ac_3, 1, 0),
#endif /* UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED */
#if UNICAST_SINK_SUPPORTED
	SHELL_CMD_ARG(ac_4, NULL, "Unicast audio configuration 4", cmd_gmap_ac_4, 1, 0),
#endif /* UNICAST_SINK_SUPPORTED */
#if UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED
	SHELL_CMD_ARG(ac_5, NULL, "Unicast audio configuration 5", cmd_gmap_ac_5, 1, 0),
#endif /* UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED */
#if UNICAST_SINK_SUPPORTED
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1
	SHELL_CMD_ARG(ac_6_i, NULL, "Unicast audio configuration 6(i)", cmd_gmap_ac_6_i, 1, 0),
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1 */
#if CONFIG_BT_MAX_CONN >= 2
	SHELL_CMD_ARG(ac_6_ii, NULL, "Unicast audio configuration 6(ii)", cmd_gmap_ac_6_ii, 1, 0),
#endif /* CONFIG_BT_MAX_CONN >= 2 */
#endif /* UNICAST_SINK_SUPPORTED */
#if UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED
#if CONFIG_BT_MAX_CONN >= 2
	SHELL_CMD_ARG(ac_7_ii, NULL, "Unicast audio configuration 7(ii)", cmd_gmap_ac_7_ii, 1, 0),
#endif /* CONFIG_BT_MAX_CONN >= 2 */
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1
	SHELL_CMD_ARG(ac_8_i, NULL, "Unicast audio configuration 8(i)", cmd_gmap_ac_8_i, 1, 0),
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1 */
#if CONFIG_BT_MAX_CONN >= 2
	SHELL_CMD_ARG(ac_8_ii, NULL, "Unicast audio configuration 8(ii)", cmd_gmap_ac_8_ii, 1, 0),
#endif /* CONFIG_BT_MAX_CONN >= 2 */
#if CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1 && CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1
	SHELL_CMD_ARG(ac_11_i, NULL, "Unicast audio configuration 11(i)", cmd_gmap_ac_11_i, 1, 0),
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT > 1 &&                                        \
	* CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT > 1                                           \
	*/
#if CONFIG_BT_MAX_CONN >= 2
	SHELL_CMD_ARG(ac_11_ii, NULL, "Unicast audio configuration 11(ii)", cmd_gmap_ac_11_ii, 1,
		      0),
#endif /* CONFIG_BT_MAX_CONN >= 2 */
#endif /* UNICAST_SINK_SUPPORTED && UNICAST_SRC_SUPPORTED */
#endif /* CONFIG_BT_GMAP_UGG_SUPPORTED */

#if defined(CONFIG_BT_GMAP_BGS_SUPPORTED)
	SHELL_CMD_ARG(ac_12, NULL, "Broadcast audio configuration 12", cmd_gmap_ac_12, 1, 0),
#if CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT > 1
	SHELL_CMD_ARG(ac_13, NULL, "Broadcast audio configuration 13", cmd_gmap_ac_13, 1, 0),
#endif /* CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT > 1 */
	SHELL_CMD_ARG(ac_14, NULL, "Broadcast audio configuration 14", cmd_gmap_ac_14, 1, 0),
#endif /* CONFIG_BT_GMAP_BGS_SUPPORTED*/
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(gmap, &gmap_cmds, "Bluetooth GMAP shell commands", cmd_gmap, 1, 1);
