/*
 * Copyright 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/hid_device.h>
#include <zephyr/bluetooth/classic/device_id.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/settings/settings.h>
#include <zephyr/shell/shell.h>

#include "bt.h"

#define BT_HID_DEVICE_VERSION      0x0101
#define BT_HID_PARSER_VERSION      0x0111
#define BT_HID_DEVICE_SUBCLASS     0xc0
#define BT_HID_DEVICE_COUNTRY_CODE 0x21
#define BT_HID_PROTO_INTERRUPT     0x0013

#define BT_HID_LANG_ID_ENGLISH 0x0409
#define BT_HID_LANG_ID_OFFSET  0x0100

#define BT_HID_SUPERVISION_TIMEOUT 1000
#define BT_HID_MAX_LATENCY         240
#define BT_HID_MIN_LATENCY         0

static uint8_t hid_descriptor[] = {
	0x05, 0x01, /* USAGE_PAGE (Generic Desktop Controls) */
	0x09, 0x02, /* USAGE (Mouse) */
	0xa1, 0x01, /* COLLECTION (Application (mouse, keyboard)) */
	0x85, 0x02, /* REPORT_ID (2) */
	0x09, 0x01, /* USAGE (Pointer) */
	0xa1, 0x00, /* COLLECTION (Physical (group of axes)) */
	0x05, 0x09, /* usage page(Button) */
	0x19, 0x01, /* Usage Minimum */
	0x29, 0x08, /* Usage Maximum */
	0x15, 0x00, /* Logical Minimum */
	0x25, 0x01, /* Logical Maximum */
	0x95, 0x08, /* Report Count */
	0x75, 0x01, /* Report size */
	0x81, 0x02, /* input() */
	0x05, 0x01, /* usage page() */
	0x09, 0x30, /* usage() */
	0x09, 0x31, /* usage() */
	0x09, 0x38, /* usage() */
	0x15, 0x81, /* logical minimum */
	0x25, 0x7f, /* logical maximum */
	0x75, 0x08, /* report size */
	0x95, 0x03, /* report count */
	0x81, 0x06, /* input */
	0xc0, 0xc0  /* END_COLLECTION */
};

static struct bt_sdp_attribute hid_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(BT_SDP_ATTR_SVCLASS_ID_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		    BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
					   BT_SDP_ARRAY_16(BT_SDP_HID_SVCLASS)})),
	BT_SDP_LIST(BT_SDP_ATTR_PROTO_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 13),
		    BT_SDP_DATA_ELEM_LIST(
			    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			     BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						    BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)},
						   {BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
						    BT_SDP_ARRAY_16(BT_SDP_PROTO_HID)})},
			    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			     BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						    BT_SDP_ARRAY_16(BT_SDP_PROTO_HID)})})),
	BT_SDP_LIST(
		BT_SDP_ATTR_LANG_BASE_ATTR_ID_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 9),
		BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_8('n', 'e')},
				      {BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(106)},
				      {BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				       BT_SDP_ARRAY_16(BT_SDP_PRIMARY_LANG_BASE)})),
	BT_SDP_LIST(BT_SDP_ATTR_PROFILE_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		    BT_SDP_DATA_ELEM_LIST(
			    {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			     BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						    BT_SDP_ARRAY_16(BT_SDP_HID_SVCLASS)},
						   {BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
						    BT_SDP_ARRAY_16(BT_HID_DEVICE_VERSION)})})),
	BT_SDP_LIST(
		BT_SDP_ATTR_ADD_PROTO_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 15),
		BT_SDP_DATA_ELEM_LIST(
			{BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 13),
			 BT_SDP_DATA_ELEM_LIST(
				 {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
				  BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
							 BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)},
							{BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
							 BT_SDP_ARRAY_16(BT_HID_PROTO_INTERRUPT)})

				 },
				 {BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
				  BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
							 BT_SDP_ARRAY_16(BT_SDP_PROTO_HID)})})})),
	BT_SDP_SERVICE_NAME("HID CONTROL"),
	{BT_SDP_ATTR_HID_PARSER_VERSION,
	 {BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(BT_HID_PARSER_VERSION)}},
	{BT_SDP_ATTR_HID_DEVICE_SUBCLASS,
	 {BT_SDP_TYPE_SIZE(BT_SDP_UINT8), BT_SDP_ARRAY_16(BT_HID_DEVICE_SUBCLASS)}},
	{BT_SDP_ATTR_HID_COUNTRY_CODE,
	 {BT_SDP_TYPE_SIZE(BT_SDP_UINT8), BT_SDP_ARRAY_16(BT_HID_DEVICE_COUNTRY_CODE)}},
	{BT_SDP_ATTR_HID_VIRTUAL_CABLE, {BT_SDP_TYPE_SIZE(BT_SDP_BOOL), BT_SDP_ARRAY_8(0x01)}},
	{BT_SDP_ATTR_HID_RECONNECT_INITIATE, {BT_SDP_TYPE_SIZE(BT_SDP_BOOL), BT_SDP_ARRAY_8(0x01)}},
	BT_SDP_LIST(BT_SDP_ATTR_HID_DESCRIPTOR_LIST,
		    BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ16, sizeof(hid_descriptor) + 8),
		    BT_SDP_DATA_ELEM_LIST({
			    BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ16, sizeof(hid_descriptor) + 5),
			    BT_SDP_DATA_ELEM_LIST(
				    {
					    BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
					    BT_SDP_ARRAY_8(0x22),
				    },
				    {
					    BT_SDP_TYPE_SIZE_VAR(BT_SDP_TEXT_STR16,
								 sizeof(hid_descriptor)),
					    hid_descriptor,
				    }),
		    })),
	BT_SDP_LIST(BT_SDP_ATTR_HID_LANG_ID_BASE_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		    BT_SDP_DATA_ELEM_LIST({
			    BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			    BT_SDP_DATA_ELEM_LIST(
				    {
					    BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
					    BT_SDP_ARRAY_16(BT_HID_LANG_ID_ENGLISH),
				    },
				    {
					    BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
					    BT_SDP_ARRAY_16(BT_HID_LANG_ID_OFFSET),
				    }),
		    })),
	{BT_SDP_ATTR_HID_BOOT_DEVICE, {BT_SDP_TYPE_SIZE(BT_SDP_BOOL), BT_SDP_ARRAY_8(0x01)}},
	{BT_SDP_ATTR_HID_SUPERVISION_TIMEOUT,
	 {BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(BT_HID_SUPERVISION_TIMEOUT)}},
	{BT_SDP_ATTR_HID_MAX_LATENCY,
	 {BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(BT_HID_MAX_LATENCY)}},
	{BT_SDP_ATTR_HID_MIN_LATENCY,
	 {BT_SDP_TYPE_SIZE(BT_SDP_UINT16), BT_SDP_ARRAY_16(BT_HID_MIN_LATENCY)}},
};

static int hid_inited;
static struct bt_sdp_record hid_rec = BT_SDP_RECORD(hid_attrs);

static struct bt_hid_device_cb hid_cb = {
	.connected = NULL,
	.disconnected = NULL,
	.set_report = NULL,
	.get_report = NULL,
	.set_protocol = NULL,
	.get_protocol = NULL,
	.intr_data = NULL,
	.vc_unplug = NULL,
};

static int cmd_hid_register(const struct shell *sh, int32_t argc, char *argv[])
{
	int ret;

	ret = bt_hid_device_register(&hid_cb);
	if (ret != 0) {
		shell_print(sh, "register cb fail");
		return ret;
	}

	ret = bt_sdp_register_service(&hid_rec);
	if (ret != 0) {
		shell_print(sh, "register desc fail");
		return ret;
	}

	hid_inited = 1;
	shell_print(sh, "success");
	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(hid_device_cmds,
			       SHELL_CMD_ARG(register, NULL, "register hid device",
					     cmd_hid_register, 1, 0),
			       SHELL_SUBCMD_SET_END);

static int cmd_hid_device(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(hid_device, &hid_device_cmds, "Bluetooth HID Device sh commands",
		       cmd_hid_device, 1, 1);
