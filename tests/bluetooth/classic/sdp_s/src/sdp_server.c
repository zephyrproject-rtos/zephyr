/* sdp_server.c - Bluetooth classic SDP server smoke test */

/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>

#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include <zephyr/shell/shell.h>

static struct bt_sdp_attribute spp_attrs_large[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				BT_SDP_ARRAY_8(BT_RFCOMM_CHAN_SPP)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("==================================================="
			    "==================================================="
			    "large_sdp_record"
			    "==================================================="
			    "==================================================="),
};

static struct bt_sdp_record spp_rec_large = BT_SDP_RECORD(spp_attrs_large);

static struct bt_sdp_attribute spp_attrs_large_valid[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				BT_SDP_ARRAY_8(BT_RFCOMM_CHAN_SPP)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("====================================="
			    "====================================="
			    "large_sdp_record"
			    "====================================="
			    "====================================="),
};

static struct bt_sdp_record spp_rec_large_valid = BT_SDP_RECORD(spp_attrs_large_valid);

#define MAX_SDP_RECORD_COUNT 8

#define _SDP_ATTRS_DEFINE(index, sdp_attrs_name) \
static struct bt_sdp_attribute sdp_attrs_name##index[] = { \
	BT_SDP_NEW_SERVICE, \
	BT_SDP_LIST( \
		BT_SDP_ATTR_SVCLASS_ID_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
			BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS) \
		}, \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_PROTO_DESC_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) \
			}, \
			) \
		}, \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM) \
			}, \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8), \
				BT_SDP_ARRAY_8(BT_RFCOMM_CHAN_SPP + index + 1) \
			}, \
			) \
		}, \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_PROFILE_DESC_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8), \
		BT_SDP_DATA_ELEM_LIST( \
		{ \
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), \
			BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS) \
			}, \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), \
				BT_SDP_ARRAY_16(0x0102) \
			}, \
			) \
		}, \
		) \
	), \
	BT_SDP_SERVICE_NAME("Serial Port"), \
}

#define SDP_ATTRS_DEFINE(index, ...) _SDP_ATTRS_DEFINE(index, ##__VA_ARGS__)

LISTIFY(MAX_SDP_RECORD_COUNT, SDP_ATTRS_DEFINE, (;), spp_attrs);

#define _SDP_REC_DEFINE(index, sdp_attrs_name) BT_SDP_RECORD(sdp_attrs_name##index)
#define SDP_REC_DEFINE(index, ...)             _SDP_REC_DEFINE(index, ##__VA_ARGS__)

static struct bt_sdp_record spp_rec[MAX_SDP_RECORD_COUNT] = {
	LISTIFY(MAX_SDP_RECORD_COUNT, SDP_REC_DEFINE, (,), spp_attrs),
};

static bool sdp_rec_reg[MAX_SDP_RECORD_COUNT];

static int cmd_register_sdp(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t index;

	index = strtoul(argv[1], NULL, 16);
	if (sdp_rec_reg[index]) {
		shell_error(sh, "The SDP record %d has been installed", index);
	}

	err = bt_sdp_register_service(&spp_rec[index]);
	if (err) {
		shell_error(sh, "Register SDP record failed (err %d)", err);
	} else {
		sdp_rec_reg[index] = true;
	}
	return err;
}

static int cmd_register_sdp_all(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	sh = sh;

	for (size_t i = 0; i < ARRAY_SIZE(spp_rec); i++) {
		if (!sdp_rec_reg[i]) {
			err = bt_sdp_register_service(&spp_rec[i]);
			if (err) {
				shell_error(sh, "Register SDP record failed (err %d)", err);
			} else {
				sdp_rec_reg[i] = true;
			}
		}
	}
	return 0;
}

static int cmd_register_sdp_large(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	sh = sh;

	err = bt_sdp_register_service(&spp_rec_large);
	if (err) {
		shell_error(sh, "Register SDP large record failed (err %d)", err);
	}
	return 0;
}

static int cmd_register_sdp_large_valid(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	sh = sh;

	err = bt_sdp_register_service(&spp_rec_large_valid);
	if (err) {
		shell_error(sh, "Register SDP large record failed (err %d)", err);
	}
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sdp_server_cmds,
	SHELL_CMD_ARG(register_sdp, NULL, "<SDP Record Index>", cmd_register_sdp, 2, 0),
	SHELL_CMD_ARG(register_sdp_all, NULL, "", cmd_register_sdp_all, 1, 0),
	SHELL_CMD_ARG(register_sdp_large, NULL, "", cmd_register_sdp_large, 1, 0),
	SHELL_CMD_ARG(register_sdp_large_valid, NULL, "", cmd_register_sdp_large_valid, 1, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_default_handler(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(sdp_server, &sdp_server_cmds, "Bluetooth classic SDP server shell commands",
		   cmd_default_handler);
