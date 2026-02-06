/*
 * SPDX-FileCopyrightText: Copyright tinyVision.ai Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/drivers/video-controls.h>
#include <zephyr/drivers/video.h>
#include <zephyr/sys/byteorder.h>

#include "uvc.h"

static const struct uvc_guid_quirk uvc_guid_quirks[] = {
	{
		.fourcc = VIDEO_PIX_FMT_YUYV,
		.guid = UVC_FORMAT_GUID("YUY2"),
	},
	{
		.fourcc = VIDEO_PIX_FMT_GREY,
		.guid = UVC_FORMAT_GUID("Y800"),
	},
};

static const struct uvc_control_map uvc_control_map_ct[] = {
	{
		.size = 1,
		.bit = 1,
		.selector = UVC_CT_AE_MODE_CONTROL,
		.cid = VIDEO_CID_EXPOSURE_AUTO,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 1,
		.bit = 2,
		.selector = UVC_CT_AE_PRIORITY_CONTROL,
		.cid = VIDEO_CID_EXPOSURE_AUTO_PRIORITY,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 4,
		.bit = 3,
		.selector = UVC_CT_EXPOSURE_TIME_ABS_CONTROL,
		.cid = VIDEO_CID_EXPOSURE,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 2,
		.bit = 5,
		.selector = UVC_CT_FOCUS_ABS_CONTROL,
		.cid = VIDEO_CID_FOCUS_ABSOLUTE,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 2,
		.bit = 6,
		.selector = UVC_CT_FOCUS_REL_CONTROL,
		.cid = VIDEO_CID_FOCUS_RELATIVE,
		.type = UVC_CONTROL_SIGNED,
	},
	{
		.size = 2,
		.bit = 7,
		.selector = UVC_CT_IRIS_ABS_CONTROL,
		.cid = VIDEO_CID_IRIS_ABSOLUTE,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 1,
		.bit = 8,
		.selector = UVC_CT_IRIS_REL_CONTROL,
		.cid = VIDEO_CID_IRIS_RELATIVE,
		.type = UVC_CONTROL_SIGNED,
	},
	{
		.size = 2,
		.bit = 9,
		.selector = UVC_CT_ZOOM_ABS_CONTROL,
		.cid = VIDEO_CID_ZOOM_ABSOLUTE,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 3,
		.bit = 10,
		.selector = UVC_CT_ZOOM_REL_CONTROL,
		.cid = VIDEO_CID_ZOOM_RELATIVE,
		.type = UVC_CONTROL_SIGNED,
	},
};

static const struct uvc_control_map uvc_control_map_pu[] = {
	{
		.size = 2,
		.bit = 0,
		.selector = UVC_PU_BRIGHTNESS_CONTROL,
		.cid = VIDEO_CID_BRIGHTNESS,
		.type = UVC_CONTROL_SIGNED,
	},
	{
		.size = 1,
		.bit = 1,
		.selector = UVC_PU_CONTRAST_CONTROL,
		.cid = VIDEO_CID_CONTRAST,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 2,
		.bit = 9,
		.selector = UVC_PU_GAIN_CONTROL,
		.cid = VIDEO_CID_GAIN,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 2,
		.bit = 3,
		.selector = UVC_PU_SATURATION_CONTROL,
		.cid = VIDEO_CID_SATURATION,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 2,
		.bit = 6,
		.selector = UVC_PU_WHITE_BALANCE_TEMP_CONTROL,
		.cid = VIDEO_CID_WHITE_BALANCE_TEMPERATURE,
		.type = UVC_CONTROL_UNSIGNED,
	},
};

static const struct uvc_control_map uvc_control_map_su[] = {
	{
		.size = 1,
		.bit = 0,
		.selector = UVC_SU_INPUT_SELECT_CONTROL,
		.cid = VIDEO_CID_TEST_PATTERN,
		.type = UVC_CONTROL_UNSIGNED,
	},
};

static const struct uvc_control_map uvc_control_map_xu[] = {
	{
		.size = 4,
		.bit = 0,
		.selector = UVC_XU_BASE_CONTROL + 0,
		.cid = VIDEO_CID_PRIVATE_BASE + 0,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 4,
		.bit = 1,
		.selector = UVC_XU_BASE_CONTROL + 1,
		.cid = VIDEO_CID_PRIVATE_BASE + 1,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 4,
		.bit = 2,
		.selector = UVC_XU_BASE_CONTROL + 2,
		.cid = VIDEO_CID_PRIVATE_BASE + 2,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 4,
		.bit = 3,
		.selector = UVC_XU_BASE_CONTROL + 3,
		.cid = VIDEO_CID_PRIVATE_BASE + 3,
		.type = UVC_CONTROL_UNSIGNED,
	},
};

int uvc_get_control_map(uint8_t subtype, const struct uvc_control_map **map, size_t *length)
{
	switch (subtype) {
	case UVC_VC_INPUT_TERMINAL:
		*map = uvc_control_map_ct;
		*length = ARRAY_SIZE(uvc_control_map_ct);
		break;
	case UVC_VC_SELECTOR_UNIT:
		*map = uvc_control_map_su;
		*length = ARRAY_SIZE(uvc_control_map_su);
		break;
	case UVC_VC_PROCESSING_UNIT:
		*map = uvc_control_map_pu;
		*length = ARRAY_SIZE(uvc_control_map_pu);
		break;
	case UVC_VC_EXTENSION_UNIT:
		*map = uvc_control_map_xu;
		*length = ARRAY_SIZE(uvc_control_map_xu);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

void uvc_fourcc_to_guid(uint8_t guid[16], const uint32_t fourcc)
{
	uint32_t fourcc_le;

	/* Lookup in the "quirk table" if the UVC format GUID is custom */
	for (int i = 0; i < ARRAY_SIZE(uvc_guid_quirks); i++) {
		if (uvc_guid_quirks[i].fourcc == fourcc) {
			memcpy(guid, uvc_guid_quirks[i].guid, 16);
			return;
		}
	}

	/* By default, UVC GUIDs are the four character code followed by a common suffix */
	fourcc_le = sys_cpu_to_le32(fourcc);
	/* Copy the common suffix with the GUID set to 'XXXX' */
	memcpy(guid, UVC_FORMAT_GUID("XXXX"), 16);
	/* Replace the 'XXXX' by the actual GUID of the format */
	memcpy(guid, &fourcc_le, 4);
}

uint32_t uvc_guid_to_fourcc(const uint8_t guid[16])
{
	uint32_t fourcc;

	/* Lookup in the "quirk table" if the UVC format GUID is custom */
	for (int i = 0; i < ARRAY_SIZE(uvc_guid_quirks); i++) {
		if (memcmp(guid, uvc_guid_quirks[i].guid, 16) == 0) {
			return uvc_guid_quirks[i].fourcc;
		}
	}

	/* Extract the four character code out of the leading 4 bytes of the GUID */
	memcpy(&fourcc, guid, 4);
	fourcc = sys_le32_to_cpu(fourcc);

	return fourcc;
}
