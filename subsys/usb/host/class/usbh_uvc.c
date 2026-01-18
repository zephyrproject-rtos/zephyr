/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_uvc_host

#include <stdlib.h>

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_device.h"
#include "usb_uvc.h"

#include "../../common/include/usb_common_uvc.h"
#include "../../../drivers/video/video_ctrls.h"
#include "../../../drivers/video/video_device.h"

LOG_MODULE_REGISTER(usbh_uvc, CONFIG_USBH_VIDEO_LOG_LEVEL);

#define UVC_FRAME_ID_INVALID                    0xFF
#define UVC_PAYLOAD_HEADER_MIN_SIZE             11
#define UVC_DEFAULT_FRAME_INTERVAL              333333
#define UVC_FRAME_DESC_MIN_SIZE_WITH_INTERVAL   26
#define UVC_FRAME_DESC_MIN_SIZE_STEPWISE        38
#define UVC_BANDWIDTH_MARGIN_PERCENT            110
#define UVC_MJPEG_COMPRESSION_RATIO             6

/**
 * @brief USB UVC camera control parameters structure
 */
struct usb_camera_ctrls {
	struct video_ctrl auto_gain;
	struct video_ctrl gain;
	struct video_ctrl auto_exposure;
	struct video_ctrl exposure;
	struct video_ctrl exposure_absolute;
	struct video_ctrl brightness;
	struct video_ctrl contrast;
	struct video_ctrl hue;
	struct video_ctrl saturation;
	struct video_ctrl sharpness;
	struct video_ctrl gamma;
	struct video_ctrl white_balance_temperature;
	struct video_ctrl auto_white_balance_temperature;
	struct video_ctrl backlight_compensation;
	struct video_ctrl auto_focus;
	struct video_ctrl focus_absolute;
	struct video_ctrl zoom_absolute;
	struct video_ctrl zoom_relative;
	struct video_ctrl light_freq;
	struct video_ctrl test_pattern;
	struct video_ctrl pixel_rate;
};

/**
 * @brief UVC device descriptors collection
 */
struct uvc_device_descriptors {
	struct uvc_control_header_descriptor *vc_header;
	struct uvc_input_terminal_descriptor *vc_itd;
	struct uvc_output_terminal_descriptor *vc_otd;
	struct uvc_camera_terminal_descriptor *vc_ctd;
	struct uvc_selector_unit_descriptor *vc_sud;
	struct uvc_processing_unit_descriptor *vc_pud;
	struct uvc_encoding_unit_descriptor *vc_encoding_unit;
	struct uvc_extension_unit_descriptor *vc_extension_unit;
	struct uvc_stream_header_descriptor *vs_input_header;
};

struct uvc_stream_iface_info {
	const struct usb_if_descriptor *current_stream_iface;
	const struct usb_ep_descriptor *current_stream_ep;
	uint32_t cur_ep_mps_mult;
};

struct uvc_format_info {
	uint32_t pixelformat;
	uint16_t width;
	uint16_t height;
	uint32_t fps;
	uint32_t frame_interval;
	uint32_t pitch;
	uint8_t format_index;
	uint8_t frame_index;
	struct uvc_format_descriptor *format_ptr;
	struct uvc_frame_descriptor *frame_ptr;
};

struct uvc_vs_format_mjpeg_info {
	struct uvc_format_mjpeg_descriptor *vs_mjpeg_format[CONFIG_USBH_VIDEO_MAX_FORMATS];
	uint8_t num_mjpeg_formats;
};

struct uvc_vs_format_uncompressed_info {
	struct uvc_format_uncomp_descriptor *uncompressed_format[CONFIG_USBH_VIDEO_MAX_FORMATS];
	uint8_t num_uncompressed_formats;
};

struct uvc_format_info_all {
	struct uvc_vs_format_mjpeg_info format_mjpeg;
	struct uvc_vs_format_uncompressed_info format_uncompressed;
};

struct uvc_device {
	struct usb_device *udev;
	struct k_mutex lock;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct k_poll_signal *sig;
	
	bool connected;
	bool streaming;
	uint8_t expect_frame_id;
	uint8_t discard_first_frame;
	uint8_t save_picture;
	uint8_t video_transfer_count;
	uint8_t multi_prime_cnt;
	
	size_t vbuf_offset;
	size_t transfer_count;
	uint32_t discard_frame_cnt;
	uint32_t current_frame_timestamp;
	
	struct video_buffer *current_vbuf;
	struct uhc_transfer *video_transfer[CONFIG_USBH_VIDEO_MULTIPLE_PRIME_COUNT];

	const struct usb_if_descriptor *current_control_interface;
	struct usb_if_descriptor *stream_ifaces[CONFIG_USBH_VIDEO_MAX_STREAM_INTERFACE];
	struct uvc_stream_iface_info current_stream_iface_info;
	struct uvc_device_descriptors uvc_descriptors;
	
	struct uvc_format_info_all formats;
	struct uvc_format_info current_format;
	struct video_format_cap *video_format_caps;
	struct uvc_probe video_probe;
	struct usb_camera_ctrls ctrls;
};

static int uvc_host_stream_iso_req_cb(struct usb_device *const dev,
				      struct uhc_transfer *const xfer);

/**
 * @brief Select default video format for UVC device
 */
static int uvc_host_select_default_format(struct uvc_device *uvc_dev)
{
	struct uvc_vs_format_uncompressed_info *uncompressed_info = NULL;
	struct uvc_vs_format_mjpeg_info *mjpeg_info = NULL;
	struct uvc_format_uncomp_descriptor *uncomp_format = NULL;
	struct uvc_format_mjpeg_descriptor *mjpeg_format = NULL;
	void *format_ptr;
	uint8_t *desc_buf;
	uint32_t pixelformat;
	uint8_t format_index;
	uint8_t frame_subtype;
	bool is_mjpeg;

	if (!uvc_dev) {
		return -EINVAL;
	}

	uncompressed_info = &uvc_dev->formats.format_uncompressed;
	mjpeg_info = &uvc_dev->formats.format_mjpeg;

	/* Try uncompressed formats first */
	if (uncompressed_info->num_uncompressed_formats > 0 &&
	    uncompressed_info->uncompressed_format[0]) {
		uncomp_format = uncompressed_info->uncompressed_format[0];
		pixelformat = uvc_guid_to_fourcc(uncomp_format->guidFormat);
		if (pixelformat != 0) {
			format_ptr = uncomp_format;
			format_index = uncomp_format->bFormatIndex;
			frame_subtype = UVC_VS_FRAME_UNCOMPRESSED;
			is_mjpeg = false;
			goto find_frame;
		}
		LOG_WRN("First uncompressed format has unsupported GUID");
	}

	/* Try MJPEG format */
	if (mjpeg_info->num_mjpeg_formats > 0 && mjpeg_info->vs_mjpeg_format[0]) {
		mjpeg_format = mjpeg_info->vs_mjpeg_format[0];
		format_ptr = mjpeg_format;
		format_index = mjpeg_format->bFormatIndex;
		frame_subtype = UVC_VS_FRAME_MJPEG;
		pixelformat = VIDEO_PIX_FMT_JPEG;
		is_mjpeg = true;
		goto find_frame;
	}

	LOG_ERR("No valid format/frame descriptors found");
	return -ENOTSUP;

find_frame:
	/* Find first frame descriptor */
	desc_buf = (uint8_t *)format_ptr +
			((struct uvc_format_descriptor *)format_ptr)->bLength;

	while (desc_buf) {
		struct uvc_frame_descriptor *frame = (struct uvc_frame_descriptor *)desc_buf;

		if (frame->bLength == 0) {
			break;
		}

		if (frame->bDescriptorType == USB_DESC_CS_INTERFACE &&
		    frame->bDescriptorSubtype == frame_subtype &&
		    frame->bLength >= sizeof(struct uvc_frame_descriptor)) {

			uint16_t width = sys_le16_to_cpu(frame->wWidth);
			uint16_t height = sys_le16_to_cpu(frame->wHeight);
			uint32_t frame_interval = sys_le32_to_cpu(frame->dwDefaultFrameInterval);

			/* Configure format */
			uvc_dev->current_format.pixelformat = pixelformat;
			uvc_dev->current_format.width = width;
			uvc_dev->current_format.height = height;
			uvc_dev->current_format.format_index = format_index;
			uvc_dev->current_format.frame_index = frame->bFrameIndex;
			uvc_dev->current_format.frame_interval = frame_interval;
			uvc_dev->current_format.format_ptr =
				(struct uvc_format_descriptor *)format_ptr;
			uvc_dev->current_format.frame_ptr = frame;
			uvc_dev->current_format.fps = 10000000 / frame_interval;
			uvc_dev->current_format.pitch = is_mjpeg ? width :
				width * video_bits_per_pixel(pixelformat) / 8;

			LOG_INF("Set default format: %s %ux%u@%ufps (format_idx=%u, frame_idx=%u)",
				is_mjpeg ? "MJPEG" : VIDEO_FOURCC_TO_STR(pixelformat),
				width, height, uvc_dev->current_format.fps, format_index,
				frame->bFrameIndex);
			return 0;
		}

		desc_buf += frame->bLength;
	}

	LOG_ERR("No valid format/frame descriptors found");
	return -ENOTSUP;
}

/**
 * @brief Configure UVC device interfaces
 */
static int uvc_host_configure_device(struct uvc_device *uvc_dev)
{
	struct usb_device *udev = uvc_dev->udev;
	int ret;

	if (!uvc_dev || !udev) {
		LOG_ERR("Invalid UVC device or USB device");
		return -EINVAL;
	}

	if (!uvc_dev->current_control_interface) {
		LOG_ERR("No control interface found");
		return -ENODEV;
	}

	if (!uvc_dev->current_stream_iface_info.current_stream_iface) {
		LOG_ERR("No streaming interface found");
		return -ENODEV;
	}

	/* Set control interface to default alternate setting (0) */
	ret = usbh_device_interface_set(uvc_dev->udev,
					uvc_dev->current_control_interface->bInterfaceNumber,
					0,
					false);
	if (ret != 0) {
		LOG_ERR("Failed to set control interface alternate setting: %d", ret);
		return ret;
	}

	/* Set streaming interface to idle state (alternate 0) */
	ret = usbh_device_interface_set(uvc_dev->udev,
					uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber,
					0,
					false);

	if (ret != 0) {
		LOG_ERR("Failed to set streaming interface alternate setting: %d", ret);
		return ret;
	}

	LOG_INF("UVC device configured successfully (control: interface %u, streaming: interface %u)",
		uvc_dev->current_control_interface->bInterfaceNumber,
		uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber);

	return 0;
}

static bool usbh_uvc_desc_is_valid_vs_header(const void *const desc,
					      const void *const desc_end)
{
	const struct uvc_stream_header_descriptor *const header_desc = desc;

	return usbh_desc_is_valid(desc, desc_end, sizeof(struct uvc_stream_header_descriptor),
				  USB_DESC_CS_INTERFACE) &&
	       header_desc->bDescriptorSubtype == UVC_VS_INPUT_HEADER;
}

static bool usbh_uvc_desc_is_valid_vc_header(const void *const desc,
					      const void *const desc_end)
{
	const struct uvc_control_header_descriptor *const header_desc = desc;

	return usbh_desc_is_valid(desc, desc_end, sizeof(struct uvc_control_header_descriptor),
				  USB_DESC_CS_INTERFACE) &&
	       header_desc->bDescriptorSubtype == UVC_VC_HEADER;
}

static const void *usbh_uvc_desc_get_vs_end(const struct usb_if_descriptor *if_desc,
					     const void *const desc_end)
{
	const struct uvc_stream_header_descriptor *const header_desc =
		usbh_desc_get_next(if_desc, desc_end);
	const void *vs_end;

	if (!usbh_uvc_desc_is_valid_vs_header(header_desc, desc_end)) {
		return NULL;
	}

	vs_end = (uint8_t *)header_desc + sys_le16_to_cpu(header_desc->wTotalLength);
	if (vs_end > desc_end) {
		return NULL;
	}

	return vs_end;
}

static const void *usbh_uvc_desc_get_vc_end(const struct usb_if_descriptor *if_desc,
					     const void *const desc_end)
{
	const struct uvc_control_header_descriptor *const header_desc =
		usbh_desc_get_next(if_desc, desc_end);
	const void *vc_end;

	if (!usbh_uvc_desc_is_valid_vc_header(header_desc, desc_end)) {
		return NULL;
	}

	vc_end = (uint8_t *)header_desc + sys_le16_to_cpu(header_desc->wTotalLength);
	if (vc_end > desc_end) {
		LOG_WRN("vc_end %p > desc_end %p", vc_end, desc_end);
		return NULL;
	}

	return vc_end;
}

static int usbh_uvc_parse_vc_desc(struct uvc_device *uvc_dev,
				   const void *const desc_beg,
				   const void *const desc_end)
{
	const struct usb_desc_header *desc = usbh_desc_get_next(desc_beg, desc_end);

	for (; desc != NULL && (uint8_t *)desc < (uint8_t *)desc_end;
	     desc = usbh_desc_get_next(desc, desc_end)) {
		if (desc->bDescriptorType == USB_DESC_INTERFACE ||
		    desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
			break;
		}

		if (desc->bDescriptorType != USB_DESC_CS_INTERFACE) {
			LOG_WRN("VideoControl descriptor: unknown type %u, skipping",
				desc->bDescriptorType);
			continue;
		}

		const struct uvc_cs_descriptor_header *const cs_desc = (const void *)desc;

		switch (cs_desc->bDescriptorSubtype) {
		case UVC_VC_HEADER: {
			struct uvc_control_header_descriptor *header_desc =
				(struct uvc_control_header_descriptor *)desc;

			if (desc->bLength < sizeof(struct uvc_control_header_descriptor)) {
				LOG_ERR("Invalid VC header descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			uvc_dev->uvc_descriptors.vc_header = header_desc;
			LOG_DBG("Found VideoControl Header: UVC v%u.%u",
				(sys_le16_to_cpu(header_desc->bcdUVC) >> 8) & 0xFF,
				sys_le16_to_cpu(header_desc->bcdUVC) & 0xFF);
			break;
		}

		case UVC_VC_INPUT_TERMINAL: {
			struct uvc_input_terminal_descriptor *it_desc =
				(struct uvc_input_terminal_descriptor *)desc;

			if (desc->bLength < sizeof(struct uvc_input_terminal_descriptor)) {
				LOG_ERR("Invalid input terminal descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			if (sys_le16_to_cpu(it_desc->wTerminalType) == UVC_ITT_CAMERA) {
				struct uvc_camera_terminal_descriptor *ct_desc =
					(struct uvc_camera_terminal_descriptor *)desc;
				uvc_dev->uvc_descriptors.vc_ctd = ct_desc;
				LOG_DBG("Found Camera Terminal: ID=%u", ct_desc->bTerminalID);
			} else {
				uvc_dev->uvc_descriptors.vc_itd = it_desc;
				LOG_DBG("Found Input Terminal: ID=%u", it_desc->bTerminalID);
			}
			break;
		}

		case UVC_VC_OUTPUT_TERMINAL: {
			struct uvc_output_terminal_descriptor *ot_desc =
				(struct uvc_output_terminal_descriptor *)desc;

			if (desc->bLength < sizeof(struct uvc_output_terminal_descriptor)) {
				LOG_ERR("Invalid output terminal descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			uvc_dev->uvc_descriptors.vc_otd = ot_desc;
			LOG_DBG("Found Output Terminal: ID=%u", ot_desc->bTerminalID);
			break;
		}

		case UVC_VC_SELECTOR_UNIT: {
			struct uvc_selector_unit_descriptor *su_desc =
				(struct uvc_selector_unit_descriptor *)desc;

			if (desc->bLength < 5) {
				LOG_ERR("Invalid selector unit descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			uvc_dev->uvc_descriptors.vc_sud = su_desc;
			LOG_DBG("Found Selector Unit: ID=%u", su_desc->bUnitID);
			break;
		}

		case UVC_VC_PROCESSING_UNIT: {
			struct uvc_processing_unit_descriptor *pu_desc =
				(struct uvc_processing_unit_descriptor *)desc;

			if (desc->bLength < 8) {
				LOG_ERR("Invalid processing unit descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			uvc_dev->uvc_descriptors.vc_pud = pu_desc;
			LOG_DBG("Found Processing Unit: ID=%u", pu_desc->bUnitID);
			break;
		}

		case UVC_VC_ENCODING_UNIT: {
			struct uvc_encoding_unit_descriptor *enc_desc =
				(struct uvc_encoding_unit_descriptor *)desc;

			if (desc->bLength < 8) {
				LOG_ERR("Invalid encoding unit descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			uvc_dev->uvc_descriptors.vc_encoding_unit = enc_desc;
			LOG_DBG("Found Encoding Unit: ID=%u", enc_desc->bUnitID);
			break;
		}

		case UVC_VC_EXTENSION_UNIT: {
			struct uvc_extension_unit_descriptor *eu_desc =
				(struct uvc_extension_unit_descriptor *)desc;

			if (desc->bLength < 24) {
				LOG_ERR("Invalid extension unit descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			uvc_dev->uvc_descriptors.vc_extension_unit = eu_desc;
			LOG_DBG("Found Extension Unit: ID=%u", eu_desc->bUnitID);
			break;
		}

		default:
			LOG_DBG("VideoControl interface: unknown subtype %u, skipping",
				cs_desc->bDescriptorSubtype);
			break;
		}
	}

	return 0;
}

static int usbh_uvc_parse_vs_desc(struct uvc_device *uvc_dev,
				   const void *const desc_beg,
				   const void *const desc_end)
{
	const struct usb_desc_header *desc = usbh_desc_get_next(desc_beg, desc_end);

	for (; desc != NULL && (uint8_t *)desc < (uint8_t *)desc_end;
	     desc = usbh_desc_get_next(desc, desc_end)) {
		if (desc->bDescriptorType == USB_DESC_INTERFACE ||
		    desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
			break;
		}

		if (desc->bDescriptorType == USB_DESC_ENDPOINT) {
			const struct usb_ep_descriptor *const ep_desc = (const void *)desc;
			LOG_DBG("VideoStreaming Endpoint: 0x%02x", ep_desc->bEndpointAddress);
			continue;
		}

		if (desc->bDescriptorType != USB_DESC_CS_INTERFACE) {
			LOG_WRN("VideoStreaming descriptor: unknown type %u, skipping",
				desc->bDescriptorType);
			continue;
		}

		const struct uvc_cs_descriptor_header *const cs_desc = (const void *)desc;

		switch (cs_desc->bDescriptorSubtype) {
		case UVC_VS_INPUT_HEADER: {
			struct uvc_stream_header_descriptor *header_desc =
				(struct uvc_stream_header_descriptor *)desc;

			if (desc->bLength < sizeof(struct uvc_stream_header_descriptor)) {
				LOG_ERR("Invalid VS input header descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			uvc_dev->uvc_descriptors.vs_input_header = header_desc;
			LOG_DBG("Found VS input header: formats=%u", header_desc->bNumFormats);
			break;
		}

		case UVC_VS_FORMAT_UNCOMPRESSED: {
			struct uvc_format_uncomp_descriptor *format_desc =
				(struct uvc_format_uncomp_descriptor *)desc;
			struct uvc_vs_format_uncompressed_info *info =
				&uvc_dev->formats.format_uncompressed;

			if (desc->bLength < sizeof(struct uvc_format_uncomp_descriptor)) {
				LOG_ERR("Invalid uncompressed format descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			if (info->num_uncompressed_formats >= CONFIG_USBH_VIDEO_MAX_FORMATS) {
				LOG_WRN("Too many uncompressed formats, ignoring format index %u",
					format_desc->bFormatIndex);
				break;
			}

			info->uncompressed_format[info->num_uncompressed_formats] = format_desc;
			info->num_uncompressed_formats++;
			LOG_DBG("Added uncompressed format[%u]: index=%u",
				info->num_uncompressed_formats - 1, format_desc->bFormatIndex);
			break;
		}

		case UVC_VS_FORMAT_MJPEG: {
			struct uvc_format_mjpeg_descriptor *format_desc =
				(struct uvc_format_mjpeg_descriptor *)desc;
			struct uvc_vs_format_mjpeg_info *info = &uvc_dev->formats.format_mjpeg;

			if (desc->bLength < sizeof(struct uvc_format_mjpeg_descriptor)) {
				LOG_ERR("Invalid MJPEG format descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			if (info->num_mjpeg_formats >= CONFIG_USBH_VIDEO_MAX_FORMATS) {
				LOG_WRN("Too many MJPEG formats, ignoring format index %u",
					format_desc->bFormatIndex);
				break;
			}

			info->vs_mjpeg_format[info->num_mjpeg_formats] = format_desc;
			info->num_mjpeg_formats++;
			LOG_DBG("Added MJPEG format[%u]: index=%u",
				info->num_mjpeg_formats - 1, format_desc->bFormatIndex);
			break;
		}

		case UVC_VS_FRAME_UNCOMPRESSED:
			LOG_DBG("VideoStreaming interface: Uncompressed Frame");
			break;

		case UVC_VS_FRAME_MJPEG:
			LOG_DBG("VideoStreaming interface: MJPEG Frame");
			break;

		case UVC_VS_COLORFORMAT:
			LOG_DBG("VideoStreaming interface: Color Format");
			break;

		default:
			LOG_DBG("VideoStreaming descriptor: unknown subtype %u, skipping",
				cs_desc->bDescriptorSubtype);
			break;
		}
	}

	return 0;
}

static void usbh_uvc_parse_vs_interface_alt_desc(struct uvc_device *uvc_dev,
						  uint8_t iface, const void *const desc_beg,
						  const void *const desc_end)
{
	const struct usb_desc_header *desc = (const struct usb_desc_header *)desc_beg;
	const struct usb_if_descriptor *if_desc;
	int i;

	for (; desc != NULL && (uint8_t *)desc < (uint8_t *)desc_end;
	     desc = usbh_desc_get_next(desc, desc_end)) {

		if (desc->bDescriptorType != USB_DESC_INTERFACE) {
			continue;
		}

		if_desc = (const struct usb_if_descriptor *)desc;

		if (if_desc->bInterfaceNumber != iface ||
		    if_desc->bInterfaceClass != USB_BCC_VIDEO ||
		    if_desc->bInterfaceSubClass != UVC_SC_VIDEOSTREAMING) {
			return;
		}

		for (i = 0; i < CONFIG_USBH_VIDEO_MAX_STREAM_INTERFACE; i++) {
			if (uvc_dev->stream_ifaces[i] != NULL) {
				continue;
			}

			uvc_dev->stream_ifaces[i] = (struct usb_if_descriptor *)if_desc;
			LOG_DBG("Added VideoStreaming interface %u alt %u to stream_ifaces[%d]",
				if_desc->bInterfaceNumber, if_desc->bAlternateSetting, i);
			break;
		}
	}
}

/**
 * @brief Parse all UVC descriptors from device
 */
static int uvc_host_parse_descriptors(struct usbh_class_data *const c_data,
				      struct uvc_device *uvc_dev, uint8_t iface)
{
	const void *const desc_beg = usbh_desc_get_cfg(uvc_dev->udev);
	const void *const desc_end = usbh_desc_get_cfg_end(uvc_dev->udev);
	const struct usb_association_descriptor *const iad_desc =
		usbh_desc_get_by_iface(desc_beg, desc_end, iface);
	const struct usb_if_descriptor *if_desc;
	const struct usb_desc_header *desc;
	bool has_vc_if = false;
	bool has_vs_if = false;
	int ret;

	if (iad_desc == NULL) {
		LOG_ERR("failed to find interface or interface association number %u", iface);
		return -ENOSYS;
	}

	if (iad_desc->bDescriptorType != USB_DESC_INTERFACE_ASSOC) {
		LOG_WRN("Interface %u is not a valid %s, skipping", iface, "interface assoc");
		return -ENOTSUP;
	}

	desc = (struct usb_desc_header *)usbh_desc_get_next(iad_desc, desc_end);
	if (desc == NULL) {
		return -EBADMSG;
	}

	for (int i = 0; i < iad_desc->bInterfaceCount; i++) {
		if_desc = (const void *)usbh_desc_get_by_iface(desc, desc_end,
								c_data->iface + i);
		if (if_desc == NULL) {
			LOG_ERR("Not as many interfaces (%u) as announced (%u)",
				i, iad_desc->bInterfaceCount);
			return -EBADMSG;
		}

		if (if_desc->bInterfaceClass == USB_BCC_VIDEO &&
		    if_desc->bInterfaceSubClass == UVC_SC_VIDEOCONTROL) {
			const void *vc_end;

			if (has_vc_if) {
				LOG_WRN("Skipping extra VideoControl interface");
				continue;
			}

			uvc_dev->current_control_interface = if_desc;

			vc_end = usbh_uvc_desc_get_vc_end(if_desc, desc_end);
			if (vc_end == NULL) {
				LOG_ERR("Invalid VideoControl interface descriptor");
				return -EBADMSG;
			}

			ret = usbh_uvc_parse_vc_desc(uvc_dev, if_desc, vc_end);
			if (ret != 0) {
				LOG_ERR("Failed to parse VC descriptor");
				return ret;
			}

			has_vc_if = true;
		}

		if (if_desc->bInterfaceClass == USB_BCC_VIDEO &&
		    if_desc->bInterfaceSubClass == UVC_SC_VIDEOSTREAMING) {
			const void *vs_end;

			if (has_vs_if) {
				LOG_WRN("Skipping extra VideoStreaming interface");
				continue;
			}

			uvc_dev->current_stream_iface_info.current_stream_iface = if_desc;

			vs_end = usbh_uvc_desc_get_vs_end(if_desc, desc_end);
			if (vs_end == NULL) {
				LOG_ERR("Invalid VideoStreaming interface descriptor");
				return -EBADMSG;
			}

			ret = usbh_uvc_parse_vs_desc(uvc_dev, if_desc, vs_end);
			if (ret != 0) {
				return ret;
			}

			/* vs_end is the start of alternate stream interfaces descriptors */
			usbh_uvc_parse_vs_interface_alt_desc(uvc_dev,
							     if_desc->bInterfaceNumber,
							     vs_end, desc_end);

			has_vs_if = true;
		}
	}

	if (!has_vs_if) {
		LOG_ERR("No VideoStreaming interface found");
		return -EINVAL;
	}

	if (!has_vc_if) {
		LOG_DBG("No VideoControl interface found");
		return -EINVAL;
	}

	LOG_INF("Interface %u associated with UVC class", iface);

	return 0;
}

/**
 * @brief Parse default frame interval from descriptor
 */
static uint32_t uvc_host_parse_frame_default_intervals(struct uvc_frame_descriptor *frame_desc)
{
	uint32_t default_interval = sys_le32_to_cpu(frame_desc->dwDefaultFrameInterval);
	uint8_t interval_type = frame_desc->bFrameIntervalType;

	if (default_interval != 0) {
		return default_interval;
	}

	/* Default interval is invalid, find maximum supported interval */
	uint32_t max_interval = UVC_DEFAULT_FRAME_INTERVAL;

	if (interval_type == 0) {
		/* Continuous frame intervals */
		struct uvc_frame_continuous_descriptor *continuous_desc =
			(struct uvc_frame_continuous_descriptor *)frame_desc;
		max_interval = sys_le32_to_cpu(continuous_desc->dwMaxFrameInterval);
	} else {
		/* Discrete frame intervals */
		struct uvc_frame_discrete_descriptor *discrete_desc =
			(struct uvc_frame_discrete_descriptor *)frame_desc;
		max_interval = sys_le32_to_cpu(discrete_desc->dwFrameInterval[discrete_desc->bFrameIntervalType - 1]);
	}

	return max_interval;
}

/**
 * @brief Find matching frame in specific format type
 */
static int uvc_host_find_frame_in_format(struct uvc_format_descriptor *format_header,
					  uint16_t target_width,
					  uint16_t target_height,
					  uint8_t expected_frame_subtype,
					  struct uvc_frame_descriptor **found_frame,
					  uint32_t *found_interval)
{
	uint8_t *desc_buf = (uint8_t *)format_header + format_header->bLength;
	int frames_found = 0;

	while (frames_found < format_header->bNumFrameDescriptors) {
		struct uvc_frame_descriptor *frame_header =
			(struct uvc_frame_descriptor *)desc_buf;

		if (frame_header->bLength == 0) {
			break;
		}

		if (frame_header->bDescriptorType == USB_DESC_CS_INTERFACE &&
		    frame_header->bDescriptorSubtype == expected_frame_subtype) {

			uint16_t frame_width = sys_le16_to_cpu(frame_header->wWidth);
			uint16_t frame_height = sys_le16_to_cpu(frame_header->wHeight);

			if (frame_width == target_width && frame_height == target_height) {
				*found_frame = frame_header;
				*found_interval =
					uvc_host_parse_frame_default_intervals(frame_header);
				return 0;
			}
			frames_found++;
		}

		if (frame_header->bDescriptorType == USB_DESC_CS_INTERFACE &&
		    (frame_header->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED ||
		     frame_header->bDescriptorSubtype == UVC_VS_FORMAT_MJPEG)) {
			break;
		}

		desc_buf += frame_header->bLength;
	}

	return -ENOTSUP;
}

/**
 * @brief Find format and frame matching specifications
 */
static int uvc_host_find_format(struct uvc_device *uvc_dev,
				uint32_t pixelformat,
				uint16_t width,
				uint16_t height,
				struct uvc_format_descriptor **format,
				struct uvc_frame_descriptor **frame,
				uint32_t *frame_interval)
{
	if (!uvc_dev || !format || !frame || !frame_interval) {
		return -EINVAL;
	}

	LOG_DBG("Looking for format: %s %ux%u",
		VIDEO_FOURCC_TO_STR(pixelformat), width, height);

	/* Search uncompressed formats */
	struct uvc_vs_format_uncompressed_info *uncompressed_info =
		&uvc_dev->formats.format_uncompressed;

	for (int i = 0; i < uncompressed_info->num_uncompressed_formats; i++) {
		struct uvc_format_uncomp_descriptor *format_desc =
			uncompressed_info->uncompressed_format[i];

		if (!format_desc) {
			continue;
		}

		uint32_t desc_pixelformat = uvc_guid_to_fourcc(format_desc->guidFormat);

		if (desc_pixelformat == pixelformat) {
			LOG_DBG("Found matching uncompressed format: index=%u",
				format_desc->bFormatIndex);

			if (uvc_host_find_frame_in_format(
				    (struct uvc_format_descriptor *)format_desc,
				    width, height, UVC_VS_FRAME_UNCOMPRESSED,
				    frame, frame_interval) == 0) {
				*format = (struct uvc_format_descriptor *)format_desc;
				LOG_DBG("Found matching frame: format_addr=%p, frame_addr=%p, interval=%u",
					*format, *frame, *frame_interval);
				return 0;
			}
		}
	}

	/* Search MJPEG formats */
	if (pixelformat == VIDEO_PIX_FMT_JPEG) {
		struct uvc_vs_format_mjpeg_info *mjpeg_info =
			&uvc_dev->formats.format_mjpeg;

		for (int i = 0; i < mjpeg_info->num_mjpeg_formats; i++) {
			struct uvc_format_mjpeg_descriptor *format_desc =
				mjpeg_info->vs_mjpeg_format[i];

			if (!format_desc) {
				continue;
			}

			LOG_DBG("Checking MJPEG format: index=%u", format_desc->bFormatIndex);

			if (uvc_host_find_frame_in_format(
				    (struct uvc_format_descriptor *)format_desc,
				    width, height, UVC_VS_FRAME_MJPEG,
				    frame, frame_interval) == 0) {
				*format = (struct uvc_format_descriptor *)format_desc;
				LOG_DBG("Found matching MJPEG frame: format_addr=%p, frame_addr=%p, interval=%u",
					*format, *frame, *frame_interval);
				return 0;
			}
		}
	}

	LOG_ERR("Format %s %ux%u not supported by device",
		VIDEO_FOURCC_TO_STR(pixelformat), width, height);
	return -ENOTSUP;
}

/**
 * @brief Select streaming alternate setting based on bandwidth
 */
static int uvc_host_select_streaming_alternate(struct uvc_device *uvc_dev,
						uint32_t required_bandwidth)
{
	struct usb_if_descriptor *selected_interface = NULL;
	struct usb_ep_descriptor *selected_endpoint = NULL;
	uint32_t optimal_bandwidth = UINT32_MAX;
	uint32_t selected_payload_size = 0;
	bool found_suitable = false;
	enum usb_device_speed device_speed = uvc_dev->udev->speed;
	uint32_t max_payload_transfer_size =
		sys_le32_to_cpu(uvc_dev->video_probe.dwMaxPayloadTransferSize);

	LOG_DBG("Required bandwidth: %u bytes/sec, Max payload: %u bytes (device speed: %s)",
		required_bandwidth, max_payload_transfer_size,
		(device_speed == USB_SPEED_SPEED_HS) ? "High Speed" : "Full Speed");

	for (int i = 0; i < CONFIG_USBH_VIDEO_MAX_STREAM_INTERFACE &&
			  uvc_dev->stream_ifaces[i]; i++) {
		struct usb_if_descriptor *if_desc = uvc_dev->stream_ifaces[i];

		/* Skip Alt 0 (idle state) */
		if (if_desc->bAlternateSetting == 0) {
			continue;
		}

		LOG_DBG("Checking interface %u alt %u (%u endpoints)",
			if_desc->bInterfaceNumber, if_desc->bAlternateSetting,
			if_desc->bNumEndpoints);

		uint8_t *ep_buf = (uint8_t *)if_desc + if_desc->bLength;

		for (int ep = 0; ep < if_desc->bNumEndpoints; ep++) {
			struct usb_ep_descriptor *ep_desc = (struct usb_ep_descriptor *)ep_buf;

			if (ep_desc->bDescriptorType == USB_DESC_ENDPOINT &&
			    (ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) ==
				    USB_EP_TYPE_ISO &&
			    (ep_desc->bEndpointAddress & USB_EP_DIR_MASK) == USB_EP_DIR_IN) {

				uint8_t interval = 1 << (ep_desc->bInterval - 1);
				uint16_t max_packet_size =
					USB_MPS_EP_SIZE(ep_desc->wMaxPacketSize);
				uint32_t ep_payload_size;
				uint32_t ep_bandwidth;

				if (device_speed == USB_SPEED_SPEED_HS) {
					ep_payload_size = USB_MPS_TO_TPL(ep_desc->wMaxPacketSize);
					ep_bandwidth = (ep_payload_size * 8000) / interval;
				} else {
					ep_payload_size = max_packet_size;
					ep_bandwidth = (max_packet_size * 1000) / interval;
				}

				LOG_DBG("Interface %u Alt %u EP[%d]: addr=0x%02x, mps=%u, payload=%u, interval=%u (%s), bandwidth=%u B/s",
					if_desc->bInterfaceNumber, if_desc->bAlternateSetting, ep,
					ep_desc->bEndpointAddress, max_packet_size, ep_payload_size,
					interval, (device_speed == USB_SPEED_SPEED_HS) ? "uframes" : "frames",
					ep_bandwidth);

				if (ep_bandwidth >= required_bandwidth &&
				    ep_payload_size >= max_payload_transfer_size &&
				    ep_bandwidth < optimal_bandwidth) {

					optimal_bandwidth = ep_bandwidth;
					selected_interface = if_desc;
					selected_endpoint = ep_desc;
					selected_payload_size = ep_payload_size;
					found_suitable = true;

					LOG_DBG("Selected optimal endpoint: interface %u alt %u EP 0x%02x, bandwidth=%u, payload=%u",
						if_desc->bInterfaceNumber,
						if_desc->bAlternateSetting,
						ep_desc->bEndpointAddress, ep_bandwidth,
						ep_payload_size);
				}
			}

			ep_buf += ep_desc->bLength;
		}
	}

	if (!found_suitable) {
		LOG_ERR("No endpoint satisfies bandwidth requirement %u and payload size %u",
			required_bandwidth, max_payload_transfer_size);
		return -ENOTSUP;
	}

	uvc_dev->current_stream_iface_info.current_stream_iface = selected_interface;
	uvc_dev->current_stream_iface_info.current_stream_ep = selected_endpoint;
	uvc_dev->current_stream_iface_info.cur_ep_mps_mult = selected_payload_size;

	LOG_DBG("Selected interface %u alternate %u endpoint 0x%02x (bandwidth=%u, payload=%u)",
		selected_interface->bInterfaceNumber, selected_interface->bAlternateSetting,
		selected_endpoint->bEndpointAddress, optimal_bandwidth, selected_payload_size);

	return 0;
}

/**
 * @brief Calculate required bandwidth for current video format
 */
static uint32_t uvc_host_calculate_required_bandwidth(const struct uvc_format_info *current_format)
{
	uint32_t bandwidth;
	uint32_t bytes_per_pixel;

	if (current_format->width == 0 || current_format->height == 0 || current_format->fps == 0) {
		LOG_ERR("Invalid format parameters: %ux%u@%ufps", 
			current_format->width, current_format->height, current_format->fps);
		return 0;
	}

	switch (current_format->pixelformat) {
	case VIDEO_PIX_FMT_JPEG:
		bytes_per_pixel = 3;
		bandwidth = (current_format->width * current_format->height * 
			     current_format->fps * bytes_per_pixel) / UVC_MJPEG_COMPRESSION_RATIO;
		break;

	case VIDEO_PIX_FMT_YUYV:
	case VIDEO_PIX_FMT_RGB565:
		bytes_per_pixel = 2;
		bandwidth = current_format->width * current_format->height * 
			    current_format->fps * bytes_per_pixel;
		break;

	case VIDEO_PIX_FMT_GREY:
		bytes_per_pixel = 1;
		bandwidth = current_format->width * current_format->height * 
			    current_format->fps * bytes_per_pixel;
		break;

	default:
		bandwidth = current_format->width * current_format->height * 
			    current_format->fps * 3;
		LOG_WRN("Unknown pixel format 0x%08x", current_format->pixelformat);
		break;
	}

	/* Add safety margin */
	bandwidth = (bandwidth * UVC_BANDWIDTH_MARGIN_PERCENT + 99) / 100;

	LOG_DBG("Calculated bandwidth: %u bytes/sec for %s %ux%u@%ufps",
		bandwidth, VIDEO_FOURCC_TO_STR(current_format->pixelformat), 
		current_format->width, current_format->height, current_format->fps);

	return bandwidth;
}

/**
 * @brief Send UVC streaming interface control request
 */
static int uvc_host_stream_interface_control(struct uvc_device *uvc_dev, uint8_t request,
					      uint8_t control_selector, void *data,
					      uint8_t data_len)
{
	uint8_t bmRequestType;
	uint16_t wValue, wIndex;
	struct net_buf *buf = NULL;
	int ret;

	if (!uvc_dev || !uvc_dev->udev) {
		LOG_ERR("Invalid UVC device");
		return -EINVAL;
	}

	if (data_len == 0) {
		LOG_ERR("Invalid data length: %u", data_len);
		return -EINVAL;
	}

	buf = usbh_xfer_buf_alloc(uvc_dev->udev, data_len);
	if (!buf) {
		LOG_ERR("Failed to allocate transfer buffer of size %u", data_len);
		return -ENOMEM;
	}

	switch (request) {
	/* SET requests */
	case UVC_SET_CUR:
		bmRequestType = (USB_REQTYPE_DIR_TO_DEVICE << 7) |
				(USB_REQTYPE_TYPE_CLASS << 5) |
				(USB_REQTYPE_RECIPIENT_INTERFACE << 0);
		if (data) {
			memcpy(buf->data, data, data_len);
			net_buf_add(buf, data_len);
		}
		break;

	/* GET requests */
	case UVC_GET_CUR:
	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_RES:
	case UVC_GET_LEN:
	case UVC_GET_INFO:
	case UVC_GET_DEF:
		bmRequestType = (USB_REQTYPE_DIR_TO_HOST << 7) |
				(USB_REQTYPE_TYPE_CLASS << 5) |
				(USB_REQTYPE_RECIPIENT_INTERFACE << 0);
		break;

	default:
		LOG_ERR("Unsupported UVC request: 0x%02x", request);
		ret = -ENOTSUP;
		goto cleanup;
	}

	wValue = control_selector << 8;
	wIndex = uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber;

	LOG_DBG("UVC control request: req=0x%02x, cs=0x%02x, len=%u",
		request, control_selector, data_len);

	ret = usbh_req_setup(uvc_dev->udev, bmRequestType, request, wValue, wIndex,
			     data_len, buf);
	if (ret != 0) {
		LOG_ERR("Failed to send UVC control request 0x%02x: %d", request, ret);
		goto cleanup;
	}

	/* For GET requests, copy received data */
	if ((bmRequestType & 0x80) && data && buf && buf->len > 0) {
		size_t copy_len = MIN(buf->len, data_len);
		memcpy(data, buf->data, copy_len);

		if (buf->len != data_len) {
			LOG_WRN("UVC GET request: expected %u bytes, got %zu bytes",
				data_len, buf->len);
		}

		LOG_DBG("GET request received %zu bytes", buf->len);
	}

	LOG_DBG("Successfully completed UVC control request 0x%02x", request);
	ret = 0;

cleanup:
	if (buf) {
		usbh_xfer_buf_free(uvc_dev->udev, buf);
	}

	return ret;
}

/**
 * @brief Get current UVC format
 */
int uvc_host_get_format(struct uvc_device *uvc_dev, struct video_format *fmt)
{
	if (!uvc_dev || !fmt) {
		return -EINVAL;
	}

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);

	fmt->pixelformat = uvc_dev->current_format.pixelformat;
	fmt->width = uvc_dev->current_format.width;
	fmt->height = uvc_dev->current_format.height;
	fmt->pitch = uvc_dev->current_format.pitch;

	k_mutex_unlock(&uvc_dev->lock);

	return 0;
}

/**
 * @brief Set UVC video format and configure streaming
 */
static int uvc_host_set_format(struct uvc_device *uvc_dev, uint32_t pixelformat,
			       uint32_t width, uint32_t height)
{
	uint32_t frame_interval;
	uint32_t required_bandwidth;
	struct uvc_format_descriptor *format;
	struct uvc_frame_descriptor *frame;
	int ret;

	/* Find matching format and frame descriptors */
	ret = uvc_host_find_format(uvc_dev, pixelformat, width, height,
				   &format, &frame, &frame_interval);
	if (ret != 0) {
		LOG_ERR("Unsupported format: %s %ux%u",
			VIDEO_FOURCC_TO_STR(pixelformat), width, height);
		return ret;
	}

	/* Prepare probe/commit structure */
	memset(&uvc_dev->video_probe, 0, sizeof(uvc_dev->video_probe));
	uvc_dev->video_probe.bmHint = sys_cpu_to_le16(0x0001);
	uvc_dev->video_probe.bFormatIndex = format->bFormatIndex;
	uvc_dev->video_probe.bFrameIndex = frame->bFrameIndex;
	uvc_dev->video_probe.dwFrameInterval = sys_cpu_to_le32(frame_interval);

	LOG_INF("Setting format: %s %ux%u (format_index=%u, frame_index=%u, interval=%u)",
		VIDEO_FOURCC_TO_STR(pixelformat), width, height,
		format->bFormatIndex, frame->bFrameIndex, frame_interval);

	/* PROBE SET */
	ret = uvc_host_stream_interface_control(uvc_dev, UVC_SET_CUR, UVC_VS_PROBE_CONTROL,
						&uvc_dev->video_probe,
						sizeof(uvc_dev->video_probe));
	if (ret == -EPIPE) {
		LOG_WRN("Request 0x%02x not supported by device, control selector 0x%02x",
			UVC_SET_CUR, UVC_VS_PROBE_CONTROL);
		return ret;
	} else if (ret != 0) {
		LOG_ERR("PROBE SET request failed: %d", ret);
		return ret;
	}

	/* PROBE GET */
	memset(&uvc_dev->video_probe, 0, sizeof(uvc_dev->video_probe));
	ret = uvc_host_stream_interface_control(uvc_dev, UVC_GET_CUR, UVC_VS_PROBE_CONTROL,
						&uvc_dev->video_probe,
						sizeof(uvc_dev->video_probe));
	if (ret == -EPIPE) {
		LOG_WRN("Request 0x%02x not supported by device, control selector 0x%02x",
			UVC_GET_CUR, UVC_VS_PROBE_CONTROL);
		return ret;
	} else if (ret != 0) {
		LOG_ERR("PROBE GET request failed: %d", ret);
		return ret;
	}

	/* COMMIT */
	ret = uvc_host_stream_interface_control(uvc_dev, UVC_SET_CUR, UVC_VS_COMMIT_CONTROL,
						&uvc_dev->video_probe,
						sizeof(uvc_dev->video_probe));
	if (ret == -EPIPE) {
		LOG_WRN("Request 0x%02x not supported by device, control selector 0x%02x",
			UVC_SET_CUR, UVC_VS_COMMIT_CONTROL);
		return ret;
	} else if (ret != 0) {
		LOG_ERR("COMMIT request failed: %d", ret);
		return ret;
	}

	/* Update device current format */
	k_mutex_lock(&uvc_dev->lock, K_FOREVER);
	uvc_dev->current_format.pixelformat = pixelformat;
	uvc_dev->current_format.width = width;
	uvc_dev->current_format.height = height;
	uvc_dev->current_format.format_index = format->bFormatIndex;
	uvc_dev->current_format.frame_index = frame->bFrameIndex;
	uvc_dev->current_format.frame_interval = frame_interval;
	uvc_dev->current_format.format_ptr = format;
	uvc_dev->current_format.frame_ptr = frame;
	uvc_dev->current_format.fps = 10000000 / frame_interval;
	uvc_dev->current_format.pitch = width * video_bits_per_pixel(pixelformat) / 8;
	k_mutex_unlock(&uvc_dev->lock);

	/* Calculate required bandwidth */
	required_bandwidth = uvc_host_calculate_required_bandwidth(&uvc_dev->current_format);
	if (required_bandwidth == 0) {
		LOG_WRN("Cannot calculate required bandwidth");
		return -EINVAL;
	}

	/* Select streaming interface alternate setting */
	ret = uvc_host_select_streaming_alternate(uvc_dev, required_bandwidth);
	if (ret != 0) {
		LOG_ERR("Select stream alternate failed: %d", ret);
		return ret;
	}

	/* Configure streaming interface */
	ret = usbh_device_interface_set(uvc_dev->udev,
					uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber,
					uvc_dev->current_stream_iface_info.current_stream_iface->bAlternateSetting,
					false);
	if (ret != 0) {
		LOG_ERR("Failed to set streaming interface %u alternate %u: %d",
			uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber,
			uvc_dev->current_stream_iface_info.current_stream_iface->bAlternateSetting,
			ret);
		return ret;
	}

	LOG_INF("Set streaming interface %u alternate %u successfully",
		uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber,
		uvc_dev->current_stream_iface_info.current_stream_iface->bAlternateSetting);

	LOG_INF("UVC format set successfully: %s %ux%u@%ufps",
		VIDEO_FOURCC_TO_STR(pixelformat), width, height,
		uvc_dev->current_format.fps);

	return 0;
}

/**
 * @brief Set UVC device frame rate
 */
static int uvc_host_set_frame_rate(struct uvc_device *uvc_dev, uint32_t fps)
{
	uint32_t target_frame_interval =0;
	uint32_t best_frame_interval = 0;
	uint32_t min_diff = UINT32_MAX;
	uint32_t required_bandwidth = 0;
	int ret;

	if (!uvc_dev || fps == 0) {
		return -EINVAL;
	}

	target_frame_interval = 10000000 / fps;

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);

	if (uvc_dev->current_format.frame_interval == target_frame_interval) {
		LOG_DBG("Frame rate already set to %u fps", fps);
		k_mutex_unlock(&uvc_dev->lock);
		return 0;
	}

	struct uvc_frame_descriptor *frame_desc = uvc_dev->current_format.frame_ptr;
	if (!frame_desc) {
		LOG_ERR("No current frame descriptor available");
		k_mutex_unlock(&uvc_dev->lock);
		return -EINVAL;
	}

	/* Parse frame intervals */
	if (frame_desc->bFrameIntervalType == 0) {
		/* Continuous frame intervals */
		struct uvc_frame_continuous_descriptor *continuous_desc =
			(struct uvc_frame_continuous_descriptor *)frame_desc;
		uint32_t min_interval = sys_le32_to_cpu(continuous_desc->dwMinFrameInterval);
		uint32_t max_interval = sys_le32_to_cpu(continuous_desc->dwMaxFrameInterval);
		uint32_t step_interval =
			sys_le32_to_cpu(continuous_desc->dwFrameIntervalStep);

		if (target_frame_interval < min_interval) {
			best_frame_interval = min_interval;
		} else if (target_frame_interval > max_interval) {
			best_frame_interval = max_interval;
		} else {
			uint32_t steps = (target_frame_interval - min_interval) / step_interval;
			best_frame_interval = min_interval + (steps * step_interval);
		}
	} else {
		/* Discrete frame intervals */
		struct uvc_frame_discrete_descriptor *discrete_desc =
			(struct uvc_frame_discrete_descriptor *)frame_desc;

		for (int i = 0; i < discrete_desc->bFrameIntervalType; i++) {
			uint32_t interval = sys_le32_to_cpu(discrete_desc->dwFrameInterval[i]);
			uint32_t diff = (interval > target_frame_interval) ?
				(interval - target_frame_interval) :
				(target_frame_interval - interval);

			if (diff < min_diff) {
				min_diff = diff;
				best_frame_interval = interval;
			}
		}
	}

	memset(&uvc_dev->video_probe, 0, sizeof(uvc_dev->video_probe));
	uvc_dev->video_probe.bmHint = sys_cpu_to_le16(0x0001);
	uvc_dev->video_probe.bFormatIndex = uvc_dev->current_format.format_index;
	uvc_dev->video_probe.bFrameIndex = uvc_dev->current_format.frame_index;
	uvc_dev->video_probe.dwFrameInterval = sys_cpu_to_le32(best_frame_interval);

	k_mutex_unlock(&uvc_dev->lock);

	LOG_INF("Setting frame rate: %u fps -> %u fps",
		fps, 10000000 / best_frame_interval);

	/* Send PROBE request */
	ret = uvc_host_stream_interface_control(uvc_dev, UVC_SET_CUR, UVC_VS_PROBE_CONTROL,
						&uvc_dev->video_probe,
						sizeof(uvc_dev->video_probe));
	if (ret != 0) {
		LOG_ERR("PROBE SET request failed: %d", ret);
		return ret;
	}

	/* Send PROBE GET request to read negotiated parameters */
	memset(&uvc_dev->video_probe, 0, sizeof(uvc_dev->video_probe));
	ret = uvc_host_stream_interface_control(uvc_dev, UVC_GET_CUR, UVC_VS_PROBE_CONTROL,
						&uvc_dev->video_probe,
						sizeof(uvc_dev->video_probe));
	if (ret != 0) {
		LOG_ERR("PROBE GET request failed: %d", ret);
		return ret;
	}

	/* Send COMMIT request */
	ret = uvc_host_stream_interface_control(uvc_dev, UVC_SET_CUR, UVC_VS_COMMIT_CONTROL,
						&uvc_dev->video_probe,
						sizeof(uvc_dev->video_probe));
	if (ret != 0) {
		LOG_ERR("COMMIT request failed: %d", ret);
		return ret;
	}

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);
	uvc_dev->current_format.frame_interval = best_frame_interval;
	uvc_dev->current_format.fps = 10000000 / best_frame_interval;
	k_mutex_unlock(&uvc_dev->lock);

	LOG_INF("Frame rate successfully set to %u fps", uvc_dev->current_format.fps);

	/* Calculate required bandwidth */
	required_bandwidth = uvc_host_calculate_required_bandwidth(&uvc_dev->current_format);
	if (required_bandwidth == 0) {
		LOG_ERR("Cannot calculate required bandwidth");
		return -EINVAL;
	}

	/* Select streaming interface alternate setting */
	ret = uvc_host_select_streaming_alternate(uvc_dev, required_bandwidth);
	if (ret != 0) {
		LOG_ERR("Failed to select streaming alternate: %d", ret);
		return ret;
	}

	/* Configure streaming interface */
	ret = usbh_device_interface_set(uvc_dev->udev,
					uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber,
					uvc_dev->current_stream_iface_info.current_stream_iface->bAlternateSetting,
					false);
	if (ret != 0) {
		LOG_ERR("Failed to set streaming interface %u alternate %u: %d",
			uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber,
			uvc_dev->current_stream_iface_info.current_stream_iface->bAlternateSetting,
			ret);
		return ret;
	}

	LOG_INF("Set streaming interface %u alternate %u successfully",
		uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber,
		uvc_dev->current_stream_iface_info.current_stream_iface->bAlternateSetting);

	return 0;
}

/**
 * @brief Parse frame descriptors for a specific format
 */
static int uvc_host_parse_format_frames(uint8_t *format_ptr, uint8_t format_length,
					uint8_t num_frames, uint32_t pixelformat,
					uint8_t frame_subtype, struct video_format_cap *caps_array,
					int start_index)
{
	uint8_t *desc_buf = format_ptr + format_length;
	int frames_found = 0;
	int cap_index = start_index;

	while (frames_found < num_frames) {
		struct uvc_frame_descriptor *frame_header =
			(struct uvc_frame_descriptor *)desc_buf;

		if (frame_header->bLength == 0) {
			break;
		}

		if (frame_header->bDescriptorType == USB_DESC_CS_INTERFACE &&
		    frame_header->bDescriptorSubtype == frame_subtype) {

			if (frame_header->bLength >= sizeof(struct uvc_frame_descriptor)) {
				uint16_t width = sys_le16_to_cpu(frame_header->wWidth);
				uint16_t height = sys_le16_to_cpu(frame_header->wHeight);

				caps_array[cap_index].pixelformat = pixelformat;
				caps_array[cap_index].width_min = width;
				caps_array[cap_index].width_max = width;
				caps_array[cap_index].height_min = height;
				caps_array[cap_index].height_max = height;
				caps_array[cap_index].width_step = 1;
				caps_array[cap_index].height_step = 1;

				cap_index++;
				frames_found++;
			}
		} else if (frame_header->bDescriptorType == USB_DESC_CS_INTERFACE &&
			   (frame_header->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED ||
			    frame_header->bDescriptorSubtype == UVC_VS_FORMAT_MJPEG)) {
			break;
		}

		desc_buf += frame_header->bLength;
	}

	return cap_index;
}

/**
 * @brief Create video format capabilities from UVC descriptors
 */
static struct video_format_cap *uvc_host_create_format_caps(struct uvc_device *uvc_dev)
{
	struct video_format_cap *caps_array = NULL;
	int total_caps = 0;
	int cap_index = 0;

	if (!uvc_dev) {
		return NULL;
	}

	struct uvc_vs_format_uncompressed_info *uncompressed_info =
		&uvc_dev->formats.format_uncompressed;
	struct uvc_vs_format_mjpeg_info *mjpeg_info = &uvc_dev->formats.format_mjpeg;

	/* Count frames in uncompressed formats */
	for (int i = 0; i < uncompressed_info->num_uncompressed_formats; i++) {
		struct uvc_format_uncomp_descriptor *format =
			uncompressed_info->uncompressed_format[i];
		if (format) {
			total_caps += format->bNumFrameDescriptors;
		}
	}

	/* Count frames in MJPEG formats */
	for (int i = 0; i < mjpeg_info->num_mjpeg_formats; i++) {
		struct uvc_format_mjpeg_descriptor *format = mjpeg_info->vs_mjpeg_format[i];
		if (format) {
			total_caps += format->bNumFrameDescriptors;
		}
	}

	if (total_caps == 0) {
		LOG_WRN("No supported formats found");
		return NULL;
	}

	caps_array = k_malloc(sizeof(struct video_format_cap) * (total_caps + 1));
	if (!caps_array) {
		LOG_ERR("Failed to allocate format caps array");
		return NULL;
	}

	memset(caps_array, 0, sizeof(struct video_format_cap) * (total_caps + 1));

	/* Process uncompressed formats */
	for (int i = 0; i < uncompressed_info->num_uncompressed_formats; i++) {
		struct uvc_format_uncomp_descriptor *format =
			uncompressed_info->uncompressed_format[i];
		if (!format) {
			continue;
		}

		uint32_t pixelformat = uvc_guid_to_fourcc(format->guidFormat);
		if (pixelformat == 0) {
			LOG_WRN("Unsupported GUID format in format index %u",
				format->bFormatIndex);
			continue;
		}

		cap_index = uvc_host_parse_format_frames((uint8_t *)format, format->bLength,
							 format->bNumFrameDescriptors,
							 pixelformat,
							 UVC_VS_FRAME_UNCOMPRESSED, caps_array,
							 cap_index);
	}

	/* Process MJPEG formats */
	for (int i = 0; i < mjpeg_info->num_mjpeg_formats; i++) {
		struct uvc_format_mjpeg_descriptor *format = mjpeg_info->vs_mjpeg_format[i];
		if (!format) {
			continue;
		}

		cap_index = uvc_host_parse_format_frames((uint8_t *)format, format->bLength,
							 format->bNumFrameDescriptors,
							 VIDEO_PIX_FMT_JPEG,
							 UVC_VS_FRAME_MJPEG, caps_array,
							 cap_index);
	}

	LOG_DBG("Created %d format capabilities from UVC descriptors", cap_index);
	return caps_array;
}

/**
 * @brief Get UVC device capabilities
 */
static int uvc_host_get_device_caps(struct uvc_device *uvc_dev, struct video_caps *caps)
{
	int ret = 0;

	if (!uvc_dev || !caps) {
		return -EINVAL;
	}

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);

	caps->min_vbuf_count = 1;

	if (uvc_dev->video_format_caps) {
		caps->format_caps = uvc_dev->video_format_caps;
	} else {
		uvc_dev->video_format_caps = uvc_host_create_format_caps(uvc_dev);
		if (!uvc_dev->video_format_caps) {
			LOG_ERR("Failed to create format capabilities");
			ret = -ENOMEM;
			goto unlock;
		}
		caps->format_caps = uvc_dev->video_format_caps;
	}

unlock:
	k_mutex_unlock(&uvc_dev->lock);
	return ret;
}

/**
 * @brief UVC host pre-initialization
 */
void uvc_host_preinit(const struct device *dev)
{
	struct uvc_device *uvc_dev = dev->data;

	k_fifo_init(&uvc_dev->fifo_in);
	k_fifo_init(&uvc_dev->fifo_out);
	k_mutex_init(&uvc_dev->lock);
}

/**
 * @brief Initiate new video transfer
 */
static int uvc_host_initiate_transfer(struct uvc_device *uvc_dev,
				      struct video_buffer *const vbuf)
{
	struct net_buf *buf;
	struct uhc_transfer *xfer;
	int ret;

	if (!vbuf || !uvc_dev || !uvc_dev->current_stream_iface_info.current_stream_ep) {
		LOG_ERR("Invalid parameters for transfer initiation");
		return -EINVAL;
	}

	LOG_DBG("Initiating transfer: ep=0x%02x, vbuf=%p",
		uvc_dev->current_stream_iface_info.current_stream_ep->bEndpointAddress, vbuf);

	xfer = usbh_xfer_alloc(uvc_dev->udev,
			       uvc_dev->current_stream_iface_info.current_stream_ep->bEndpointAddress,
			       uvc_host_stream_iso_req_cb, uvc_dev);
	if (!xfer) {
		LOG_ERR("Failed to allocate transfer");
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(uvc_dev->udev,
				  uvc_dev->current_stream_iface_info.cur_ep_mps_mult);
	if (!buf) {
		LOG_ERR("Failed to allocate buffer");
		usbh_xfer_free(uvc_dev->udev, xfer);
		return -ENOMEM;
	}

	buf->len = 0;
	uvc_dev->vbuf_offset = 0;
	xfer->buf = buf;

	uvc_dev->video_transfer[uvc_dev->video_transfer_count++] = xfer;

	ret = usbh_xfer_enqueue(uvc_dev->udev, xfer);
	if (ret != 0) {
		LOG_ERR("Enqueue failed: ret=%d", ret);
		net_buf_unref(buf);
		usbh_xfer_free(uvc_dev->udev, xfer);
		return ret;
	}

	return 0;
}

/**
 * @brief Continue existing video transfer
 */
static int uvc_host_continue_transfer(struct uvc_device *uvc_dev,
				      struct uhc_transfer *const xfer,
				      struct video_buffer *vbuf)
{
	struct net_buf *buf;
	int ret;

	if (xfer == NULL) {
		return -EINVAL;
	}

	buf = usbh_xfer_buf_alloc(uvc_dev->udev,
				  uvc_dev->current_stream_iface_info.cur_ep_mps_mult);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOMEM;
	}

	buf->len = 0;
	xfer->buf = buf;

	ret = usbh_xfer_enqueue(uvc_dev->udev, xfer);
	if (ret != 0) {
		LOG_ERR("Enqueue failed: ret=%d", ret);
		net_buf_unref(buf);
		return ret;
	}

	return 0;
}

/**
 * @brief ISO transfer completion callback
 */
static int uvc_host_stream_iso_req_cb(struct usb_device *const dev,
				      struct uhc_transfer *const xfer)
{
	struct uvc_device *uvc_dev;
	struct net_buf *buf;
	struct video_buffer *vbuf;
	struct uvc_payload_header *payload_header;
	uint32_t header_length;
	uint32_t data_size;
	uint32_t presentation_time;
	uint8_t end_frame;
	uint8_t frame_id;

	if (!xfer || !xfer->priv || !xfer->buf) {
		LOG_ERR("Invalid callback parameters");
		return -EINVAL;
	}

	uvc_dev = (struct uvc_device *)xfer->priv;
	buf = xfer->buf;
	vbuf = uvc_dev->current_vbuf;

	if (!vbuf) {
		LOG_DBG("No current buffer available, ignoring callback");
		goto cleanup;
	}

	if (!uvc_dev->streaming) {
		LOG_DBG("Device not streaming, ignoring callback");
		goto cleanup;
	}

	if (xfer->err == -ECONNRESET) {
		LOG_INF("ISO transfer canceled");
		goto cleanup;
	} else if (xfer->err) {
		LOG_WRN("ISO request failed, err %d", xfer->err);
		goto cleanup;
	}

	payload_header = (struct uvc_payload_header *)buf->data;
	end_frame = payload_header->bmHeaderInfo & UVC_BMHEADERINFO_END_OF_FRAME;
	frame_id = payload_header->bmHeaderInfo & UVC_BMHEADERINFO_FRAMEID;
	presentation_time = sys_le32_to_cpu(payload_header->dwPresentationTime);
	header_length = payload_header->bHeaderLength;

	if (buf->len <= UVC_PAYLOAD_HEADER_MIN_SIZE) {
		goto cleanup;
	}

	data_size = buf->len - header_length;

	if (data_size > 0) {
		if (vbuf->bytesused == 0U) {
			if (uvc_dev->current_frame_timestamp != presentation_time) {
				uvc_dev->current_frame_timestamp = presentation_time;
			}
		} else if (presentation_time != uvc_dev->current_frame_timestamp) {
			uvc_dev->save_picture = 0;
		}

		if (uvc_dev->save_picture && vbuf) {
			if (data_size > (vbuf->size - vbuf->bytesused)) {
				LOG_WRN("Buffer overflow: used=%u, payload=%u, capacity=%u",
									vbuf->bytesused, data_size, vbuf->size);
				uvc_dev->save_picture = 0;
				vbuf->bytesused = 0U;
				uvc_dev->vbuf_offset = 0;
			} else if (frame_id == uvc_dev->expect_frame_id) {
				memcpy((void *)(((uint8_t *)vbuf->buffer) + vbuf->bytesused),
				       (void *)(((uint8_t *)buf->data) + header_length),
				       data_size);
				vbuf->bytesused += data_size;
				uvc_dev->vbuf_offset = vbuf->bytesused;

				LOG_DBG("Processed %u payload bytes (FID:%u), total: %u, EOF: %u",
					data_size, frame_id, vbuf->bytesused, end_frame);
			} else {
				uvc_dev->save_picture = 0;
				LOG_DBG("Frame ID mismatch: expected %u, got %u - discarding",
					uvc_dev->expect_frame_id, frame_id);
			}
		}
	}

	/* Handle end of frame */
	if (end_frame) {
		if (uvc_dev->save_picture) {
			if (vbuf && vbuf->bytesused != 0) {
				LOG_DBG("Frame completed: %u bytes (FID: %u)",
					vbuf->bytesused, frame_id);

				k_mutex_lock(&uvc_dev->lock, K_FOREVER);

				if (!uvc_dev->streaming) {
					k_mutex_unlock(&uvc_dev->lock);
					goto cleanup;
				}

				if (uvc_dev->current_vbuf != vbuf) {
					LOG_DBG("Buffer %p already processed by another callback", vbuf);
					k_mutex_unlock(&uvc_dev->lock);
					goto cleanup;
				}

				k_fifo_get(&uvc_dev->fifo_in, K_NO_WAIT);
				k_fifo_put(&uvc_dev->fifo_out, vbuf);

				uvc_dev->expect_frame_id = uvc_dev->expect_frame_id ^ 1;
				uvc_dev->save_picture = 1;

				uvc_dev->vbuf_offset = 0;
				uvc_dev->transfer_count = 0;

				LOG_DBG("Raising VIDEO_BUF_DONE signal");
				k_poll_signal_raise(uvc_dev->sig, VIDEO_BUF_DONE);

				vbuf = k_fifo_peek_head(&uvc_dev->fifo_in);
				if (vbuf != NULL) {
					vbuf->bytesused = 0;
					memset(vbuf->buffer, 0, vbuf->size);
					uvc_dev->current_vbuf = vbuf;
				}
				k_mutex_unlock(&uvc_dev->lock);
			}
		} else {
			/* discard the first frame */
			if (uvc_dev->discard_first_frame) {
				uvc_dev->discard_first_frame = 0;
			}

			if (vbuf) {
				if (vbuf->bytesused != 0) {
					uvc_dev->discard_frame_cnt++;
				}
				vbuf->bytesused = 0U;
				uvc_dev->vbuf_offset = 0;
			}

			uvc_dev->expect_frame_id = frame_id ^ 1;
			uvc_dev->save_picture = 1;
		}
	}

cleanup:
	net_buf_unref(buf);
	if (uvc_dev->streaming && vbuf) {
		uvc_host_continue_transfer(uvc_dev, xfer, vbuf);
	}
	return 0;
}

/**
 * @brief Enumerate frame intervals for a given frame
 */
static int uvc_host_enum_frame_intervals(struct uvc_frame_descriptor *frame_ptr,
					 struct video_frmival_enum *frmival_enum)
{
	uint8_t interval_type;
	uint8_t *interval_data;

	if (!frame_ptr || !frmival_enum) {
		return -EINVAL;
	}

	if (frame_ptr->bLength < UVC_FRAME_DESC_MIN_SIZE_WITH_INTERVAL) {
		LOG_ERR("Frame descriptor too short for interval data");
		return -EINVAL;
	}

	interval_type = frame_ptr->bFrameIntervalType;
	interval_data = (uint8_t *)&frame_ptr->bFrameIntervalType + 1;

	LOG_DBG("Enumerating frame intervals: frame_index=%u, interval_type=%u, fie_index=%u",
		frame_ptr->bFrameIndex, interval_type, frmival_enum->index);

	if (interval_type == 0) {
		/* continuous/stepwise frame intervals */
		if (frmival_enum->index > 0) {
			return -EINVAL;
		}

		frmival_enum->type = VIDEO_FRMIVAL_TYPE_STEPWISE;
		frmival_enum->stepwise.min.numerator = sys_le32_to_cpu(*(uint32_t *)(interval_data + 0));
		frmival_enum->stepwise.min.denominator = 10000000;
		frmival_enum->stepwise.max.numerator = sys_le32_to_cpu(*(uint32_t *)(interval_data + 4));
		frmival_enum->stepwise.max.denominator = 10000000;
		frmival_enum->stepwise.step.numerator = sys_le32_to_cpu(*(uint32_t *)(interval_data + 8));
		frmival_enum->stepwise.step.denominator = 10000000;

		LOG_DBG("Stepwise intervals: min=%u (100ns), max=%u (100ns), step=%u (100ns)",
			frmival_enum->stepwise.min.numerator, 
			frmival_enum->stepwise.max.numerator,
			frmival_enum->stepwise.step.numerator);

	} else if (interval_type > 0) {
		/* discrete frame intervals */
		uint8_t num_intervals = interval_type;

		if (frmival_enum->index >= num_intervals) {
			return -EINVAL;
		}

		if (frame_ptr->bLength < (UVC_FRAME_DESC_MIN_SIZE_WITH_INTERVAL + num_intervals * 4)) {
			LOG_ERR("Frame descriptor too short for %u discrete intervals",
				num_intervals);
			return -EINVAL;
		}

		frmival_enum->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
		frmival_enum->discrete.numerator =
			sys_le32_to_cpu(*(uint32_t *)(interval_data + frmival_enum->index * 4));
		frmival_enum->discrete.denominator = 10000000;

		LOG_DBG("Discrete interval[%u]: %u/10000000 (%u ns)",
			frmival_enum->index, frmival_enum->discrete.numerator, frmival_enum->discrete.numerator * 100);
	} else {
		LOG_ERR("Invalid frame interval type: %u", interval_type);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Send UVC control request to unit or terminal
 */
static int uvc_host_control_unit_and_terminal_request(struct uvc_device *uvc_dev,
						      uint8_t request,
						      uint8_t control_selector,
						      uint8_t entity_id,
						      void *data, uint8_t data_len)
{
	uint8_t bmRequestType;
	uint16_t wValue, wIndex;
	struct net_buf *buf;
	int ret;

	if (!uvc_dev || !uvc_dev->udev) {
		LOG_ERR("Invalid UVC device");
		return -EINVAL;
	}

	if (data_len == 0) {
		LOG_ERR("Invalid data length: %u", data_len);
		return -EINVAL;
	}

	buf = usbh_xfer_buf_alloc(uvc_dev->udev, data_len);
	if (!buf) {
		LOG_ERR("Failed to allocate transfer buffer of size %u", data_len);
		return -ENOMEM;
	}

	switch (request) {
	/* SET requests */
	case UVC_SET_CUR:
	case UVC_SET_CUR_ALL:
		bmRequestType = (USB_REQTYPE_DIR_TO_DEVICE << 7) |
				(USB_REQTYPE_TYPE_CLASS << 5) |
				(USB_REQTYPE_RECIPIENT_INTERFACE << 0);
		wValue = (request == UVC_SET_CUR_ALL) ? 0x0000 : (control_selector << 8);
		wIndex = (entity_id << 8) | uvc_dev->current_control_interface->bInterfaceNumber;

		if (data) {
			memcpy(buf->data, data, data_len);
			net_buf_add(buf, data_len);
		}
		break;

	/* GET requests */
	case UVC_GET_CUR:
	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_RES:
	case UVC_GET_LEN:
	case UVC_GET_INFO:
	case UVC_GET_DEF:
	case UVC_GET_CUR_ALL:
	case UVC_GET_MIN_ALL:
	case UVC_GET_MAX_ALL:
	case UVC_GET_RES_ALL:
	case UVC_GET_DEF_ALL:
		bmRequestType = (USB_REQTYPE_DIR_TO_HOST << 7) |
				(USB_REQTYPE_TYPE_CLASS << 5) |
				(USB_REQTYPE_RECIPIENT_INTERFACE << 0);
		wValue = (request >= UVC_GET_CUR_ALL) ? 0x0000 : (control_selector << 8);
		wIndex = (entity_id << 8) | uvc_dev->current_control_interface->bInterfaceNumber;
		break;

	default:
		LOG_ERR("Unsupported UVC request: 0x%02x", request);
		ret = -ENOTSUP;
		goto cleanup;
	}

	LOG_DBG("UVC control request: req=0x%02x, cs=0x%02x, entity=0x%02x, len=%u",
		request, control_selector, entity_id, data_len);

	ret = usbh_req_setup(uvc_dev->udev, bmRequestType, request, wValue, wIndex,
			     data_len, buf);
	if (ret != 0) {
		goto cleanup;
	}

	/* For GET requests, copy received data */
	if ((bmRequestType & 0x80) && data && buf && buf->len > 0) {
		size_t copy_len = MIN(buf->len, data_len);
		memcpy(data, buf->data, copy_len);

		if (buf->len != data_len) {
			LOG_WRN("UVC GET request: expected %u bytes, got %zu bytes",
				data_len, buf->len);
		}

		LOG_DBG("GET request received %zu bytes", buf->len);
	}

	LOG_DBG("Successfully completed UVC control request 0x%02x to entity %d",
		request, entity_id);

	ret = 0;

cleanup:
	if (buf) {
		usbh_xfer_buf_free(uvc_dev->udev, buf);
	}

	return ret;
}

/**
 * @brief Check if Processing Unit supports specific control
 */
static bool uvc_host_pu_supports_control(struct uvc_device *uvc_dev, uint32_t bmcontrol_bit)
{
	if (!uvc_dev || !uvc_dev->uvc_descriptors.vc_pud) {
		return false;
	}

	struct uvc_processing_unit_descriptor *pud = uvc_dev->uvc_descriptors.vc_pud;

	if (pud->bControlSize == 0) {
		return false;
	}

	uint32_t controls = 0;
	for (int i = 0; i < pud->bControlSize && i < 3; i++) {
		controls |= ((uint32_t)pud->bmControls[i]) << (i * 8);
	}

	return (controls & bmcontrol_bit) != 0;
}

/**
 * @brief Check if Camera Terminal supports specific control
 */
static bool uvc_host_ct_supports_control(struct uvc_device *uvc_dev, uint32_t bmcontrol_bit)
{
	if (!uvc_dev || !uvc_dev->uvc_descriptors.vc_ctd) {
		return false;
	}

	struct uvc_camera_terminal_descriptor *vc_ctd = uvc_dev->uvc_descriptors.vc_ctd;

	if (vc_ctd->bControlSize == 0) {
		return false;
	}

	uint32_t controls = 0;
	for (int i = 0; i < vc_ctd->bControlSize && i < 3; i++) {
		controls |= ((uint32_t)vc_ctd->bmControls[i]) << (i * 8);
	}

	return (controls & bmcontrol_bit) != 0;
}

/**
 * @brief Initialize individual control based on CID
 */
static int uvc_host_init_control_by_cid(const struct device *dev, uint32_t cid,
					const struct video_ctrl_range *range)
{
	struct uvc_device *uvc_dev = dev->data;
	struct usb_camera_ctrls *ctrls = &uvc_dev->ctrls;
	int ret = -ENOTSUP;

	switch (cid) {
	/* Processing Unit Controls */
	case VIDEO_CID_BRIGHTNESS:
		ret = video_init_ctrl(&ctrls->brightness, dev, cid, *range);
		break;

	case VIDEO_CID_CONTRAST:
		ret = video_init_ctrl(&ctrls->contrast, dev, cid, *range);
		break;

	case VIDEO_CID_HUE:
		ret = video_init_ctrl(&ctrls->hue, dev, cid, *range);
		break;

	case VIDEO_CID_SATURATION:
		ret = video_init_ctrl(&ctrls->saturation, dev, cid, *range);
		break;

	case VIDEO_CID_SHARPNESS:
		ret = video_init_ctrl(&ctrls->sharpness, dev, cid, *range);
		break;

	case VIDEO_CID_GAMMA:
		ret = video_init_ctrl(&ctrls->gamma, dev, cid, *range);
		break;

	case VIDEO_CID_GAIN:
		ret = video_init_ctrl(&ctrls->gain, dev, cid, *range);
		break;

	case VIDEO_CID_AUTOGAIN:
		ret = video_init_ctrl(&ctrls->auto_gain, dev, cid, *range);
		if (ret != 0) {
			break;
		}

		video_auto_cluster_ctrl(&ctrls->auto_gain, 2, true);
		LOG_DBG("Created auto-gain cluster");
		break;

	case VIDEO_CID_WHITE_BALANCE_TEMPERATURE:
		ret = video_init_ctrl(&ctrls->white_balance_temperature, dev, cid, *range);
		break;

	case VIDEO_CID_AUTO_WHITE_BALANCE:
		ret = video_init_ctrl(&ctrls->auto_white_balance_temperature, dev, cid, *range);
		break;

	case VIDEO_CID_BACKLIGHT_COMPENSATION:
		ret = video_init_ctrl(&ctrls->backlight_compensation, dev, cid, *range);
		break;

	case VIDEO_CID_POWER_LINE_FREQUENCY:
		ret = video_init_menu_ctrl(&ctrls->light_freq, dev, cid,
					   VIDEO_CID_POWER_LINE_FREQUENCY_AUTO, NULL);
		break;

	/* Camera Terminal Controls */
	case VIDEO_CID_EXPOSURE_AUTO:
		ret = video_init_ctrl(&ctrls->auto_exposure, dev, cid, *range);
		break;

	case VIDEO_CID_EXPOSURE:
		ret = video_init_ctrl(&ctrls->exposure, dev, cid, *range);
		break;

	case VIDEO_CID_EXPOSURE_ABSOLUTE:
		ret = video_init_ctrl(&ctrls->exposure_absolute, dev, cid, *range);
		if (ret != 0) {
			break;
		}

		video_auto_cluster_ctrl(&ctrls->auto_exposure, 2, true);
		LOG_DBG("Created auto-exposure cluster");
		break;

	case VIDEO_CID_FOCUS_AUTO:
		ret = video_init_ctrl(&ctrls->auto_focus, dev, cid, *range);
		break;

	case VIDEO_CID_FOCUS_ABSOLUTE:
		ret = video_init_ctrl(&ctrls->focus_absolute, dev, cid, *range);
		break;

	case VIDEO_CID_ZOOM_ABSOLUTE:
		ret = video_init_ctrl(&ctrls->zoom_absolute, dev, cid, *range);
		break;

	case VIDEO_CID_ZOOM_RELATIVE:
		ret = video_init_ctrl(&ctrls->zoom_relative, dev, cid, *range);
		break;

	case VIDEO_CID_TEST_PATTERN:
		ret = video_init_ctrl(&ctrls->test_pattern, dev, cid, *range);
		break;

	default:
		LOG_DBG("Control CID 0x%08x not implemented", cid);
		break;
	}

	return ret;
}

/**
 * @brief Extract control bitmap from bmControls array
 */
static uint32_t uvc_host_extract_control_bitmap(const uint8_t *bmControls,
						 uint8_t control_size)
{
	uint32_t controls = 0;

	for (int i = 0; i < control_size; i++) {
		controls |= ((uint32_t)bmControls[i]) << (i * 8);
	}

	return controls;
}

/**
 * @brief Extract control value from data buffer based on size and type
 */
static int32_t uvc_host_extract_control_value(const uint8_t *data, uint8_t size,
					       uint8_t type)
{
	switch (size) {
	case 1:
		return (type == UVC_CONTROL_SIGNED) ? (int8_t)data[0] : (uint8_t)data[0];

	case 2:
		if (type == UVC_CONTROL_SIGNED) {
			return (int16_t)sys_le16_to_cpu(*(uint16_t *)data);
		} else {
			return (uint16_t)sys_le16_to_cpu(*(uint16_t *)data);
		}

	case 4:
		return (int32_t)sys_le32_to_cpu(*(uint32_t *)data);

	default:
		LOG_ERR("Unsupported control data size: %u", size);
		return 0;
	}
}

/**
 * @brief Query actual control range from UVC device
 */
static int uvc_host_query_control_range(struct uvc_device *uvc_dev,
					uint8_t entity_id,
					const struct uvc_control_map *map,
					struct video_ctrl_range *range)
{
	uint8_t data[4];
	int ret;

	/* Query minimum value */
	ret = uvc_host_control_unit_and_terminal_request(uvc_dev, UVC_GET_MIN,
							 map->selector, entity_id, data,
							 map->size);
	if (ret != 0) {
		LOG_DBG("Failed to get min value for selector 0x%02x: %d", map->selector, ret);
		return ret;
	}
	range->min = uvc_host_extract_control_value(data, map->size, map->type);

	/* Query maximum value */
	ret = uvc_host_control_unit_and_terminal_request(uvc_dev, UVC_GET_MAX,
							 map->selector, entity_id, data,
							 map->size);
	if (ret == -EPIPE) {
		LOG_WRN("Request 0x%02x not supported by device for entity %d, control selector 0x%02x",
			UVC_GET_MAX, entity_id, map->selector);
		return ret;
	} else if (ret != 0) {
		LOG_DBG("Failed to get max value for selector 0x%02x: %d", map->selector, ret);
		return ret;
	}
	range->max = uvc_host_extract_control_value(data, map->size, map->type);

	/* Query step/resolution value */
	ret = uvc_host_control_unit_and_terminal_request(uvc_dev, UVC_GET_RES,
							 map->selector, entity_id, data,
							 map->size);
	if (ret == -EPIPE) {
		LOG_WRN("Request 0x%02x not supported by device for entity %d, control selector 0x%02x",
			UVC_GET_RES, entity_id, map->selector);
		return ret;
	} else if (ret != 0) {
		range->step = 1;
		LOG_DBG("Using default step=1 for selector 0x%02x", map->selector);
	} else {
		int32_t step_val = uvc_host_extract_control_value(data, map->size, map->type);
		range->step = (step_val > 0) ? step_val : 1;
	}

	/* Query default value */
	ret = uvc_host_control_unit_and_terminal_request(uvc_dev, UVC_GET_DEF,
							 map->selector, entity_id, data,
							 map->size);
	if (ret == -EPIPE) {
		LOG_WRN("Request 0x%02x not supported by device for entity %d, control selector 0x%02x",
			UVC_GET_DEF, entity_id, map->selector);
		return ret;
	} else if (ret != 0) {
		range->def = (range->min + range->max) / 2;
		LOG_DBG("Using middle value as default for selector 0x%02x", map->selector);
	} else {
		range->def = uvc_host_extract_control_value(data, map->size, map->type);
	}

	LOG_DBG("Control 0x%02x range: [%d, %d, %d, %d]", map->selector,
		range->min, range->max, range->step, range->def);

	return 0;
}

/**
 * @brief Initialize controls for a specific UVC unit/terminal
 */
static int uvc_host_init_unit_controls(struct uvc_device *uvc_dev,
				       const struct device *dev,
				       uint8_t unit_subtype,
				       uint8_t entity_id,
				       uint32_t supported_controls,
				       int *initialized_count)
{
	const struct uvc_control_map *map;
	size_t map_length;
	int ret;

	ret = uvc_get_control_map(unit_subtype, &map, &map_length);
	if (ret != 0) {
		LOG_ERR("Failed to get control map for unit subtype %u: %d",
			unit_subtype, ret);
		return ret;
	}

	LOG_DBG("Processing %zu controls for unit subtype %u, entity ID %u",
		map_length, unit_subtype, entity_id);

	for (size_t i = 0; i < map_length; i++) {
		const struct uvc_control_map *ctrl_map = &map[i];
		struct video_ctrl_range range;

		if (!(supported_controls & BIT(ctrl_map->bit))) {
			LOG_DBG("Control bit %u not supported by device", ctrl_map->bit);
			continue;
		}

		ret = uvc_host_query_control_range(uvc_dev, entity_id, ctrl_map, &range);
		if (ret != 0) {
			continue;
		}

		ret = uvc_host_init_control_by_cid(dev, ctrl_map->cid, &range);
		if (ret == -ENOTSUP)
		{
			LOG_WRN("Control cid %u not supported by device", ctrl_map->cid);
		} else if (ret != 0) {
			LOG_WRN("Failed to initialize control CID 0x%08x: %d",
				ctrl_map->cid, ret);
			continue;
		}

		(*initialized_count)++;
		LOG_DBG("Successfully initialized control CID 0x%08x", ctrl_map->cid);
	}

	return 0;
}

/**
 * @brief Initialize USB camera controls based on device capabilities
 */
static int usb_host_camera_init_controls(const struct device *dev)
{
	int ret;
	struct uvc_device *uvc_dev = dev->data;
	uint32_t control_bitmap;
	int initialized_count = 0;

	if (!uvc_dev->uvc_descriptors.vc_pud) {
		LOG_WRN("No processing unit found, skipping control initialization");
		return 0;
	}

	/* Initialize Processing Unit controls */
	if (uvc_dev->uvc_descriptors.vc_pud) {
		LOG_DBG("Found Processing Unit (ID: %u)",
			uvc_dev->uvc_descriptors.vc_pud->bUnitID);

		control_bitmap = uvc_host_extract_control_bitmap(
			uvc_dev->uvc_descriptors.vc_pud->bmControls,
			uvc_dev->uvc_descriptors.vc_pud->bControlSize);

		ret = uvc_host_init_unit_controls(uvc_dev, dev, UVC_VC_PROCESSING_UNIT,
						  uvc_dev->uvc_descriptors.vc_pud->bUnitID,
						  control_bitmap,
						  &initialized_count);
		if (ret != 0) {
			LOG_ERR("Failed to initialize Processing Unit controls: %d", ret);
		}
	} else {
		LOG_WRN("No Processing Unit found, skipping PU controls");
	}

	/* Initialize Camera Terminal controls */
	if (uvc_dev->uvc_descriptors.vc_ctd) {
		LOG_DBG("Found Camera Terminal (ID: %u)",
			uvc_dev->uvc_descriptors.vc_ctd->bTerminalID);

		control_bitmap = uvc_host_extract_control_bitmap(
			uvc_dev->uvc_descriptors.vc_ctd->bmControls,
			uvc_dev->uvc_descriptors.vc_ctd->bControlSize);

		ret = uvc_host_init_unit_controls(uvc_dev, dev, UVC_VC_INPUT_TERMINAL,
						  uvc_dev->uvc_descriptors.vc_ctd->bTerminalID,
						  control_bitmap,
						  &initialized_count);
		if (ret != 0) {
			LOG_ERR("Failed to initialize Camera Terminal controls: %d", ret);
		}
	} else {
		LOG_DBG("No Camera Terminal found, skipping CT controls");
	}

	/* Initialize Selector Unit controls */
	if (uvc_dev->uvc_descriptors.vc_sud) {
		LOG_DBG("Found Selector Unit (ID: %u)",
			uvc_dev->uvc_descriptors.vc_sud->bUnitID);

		control_bitmap = BIT(0);

		ret = uvc_host_init_unit_controls(uvc_dev, dev, UVC_VC_SELECTOR_UNIT,
						  uvc_dev->uvc_descriptors.vc_sud->bUnitID,
						  control_bitmap,
						  &initialized_count);
		if (ret != 0) {
			LOG_ERR("Failed to initialize Selector Unit controls: %d", ret);
		}
	}

	/* Initialize Extension Unit controls */
	if (uvc_dev->uvc_descriptors.vc_extension_unit) {
		LOG_DBG("Found Extension Unit (ID: %u)",
			uvc_dev->uvc_descriptors.vc_extension_unit->bUnitID);

		control_bitmap = uvc_host_extract_control_bitmap(
			uvc_dev->uvc_descriptors.vc_extension_unit->bmControls,
			uvc_dev->uvc_descriptors.vc_extension_unit->bControlSize);

		ret = uvc_host_init_unit_controls(uvc_dev, dev, UVC_VC_EXTENSION_UNIT,
						  uvc_dev->uvc_descriptors.vc_extension_unit->bUnitID,
						  control_bitmap,
						  &initialized_count);
		if (ret != 0) {
			LOG_ERR("Failed to initialize Extension Unit controls: %d", ret);
		}
	}

	LOG_INF("Successfully initialized %d camera controls", initialized_count);
	return 0;
}

/**
 * @brief Initialize UVC host class
 */
static int uvc_host_init(struct usbh_class_data *const c_data,
			 struct usbh_context *const uhs_ctx)
{
	const struct device *dev = c_data->priv;
	struct uvc_device *uvc_dev = (struct uvc_device *)dev->data;

	if (!c_data || !dev || !uvc_dev) {
		LOG_ERR("Invalid parameters for UVC host init");
		return -EINVAL;
	}
	c_data->uhs_ctx = uhs_ctx;

	LOG_INF("Initializing UVC device structure");

	uvc_dev->udev = NULL;
	uvc_dev->connected = false;
	uvc_dev->streaming = false;
	uvc_dev->vbuf_offset = 0;
	uvc_dev->transfer_count = 0;
	uvc_dev->current_vbuf = NULL;		
	uvc_dev->save_picture = 0;
	uvc_dev->current_frame_timestamp = 0;

	k_fifo_init(&uvc_dev->fifo_in);
	k_fifo_init(&uvc_dev->fifo_out);

	k_mutex_init(&uvc_dev->lock);

	memset(&uvc_dev->ctrls, 0, sizeof(struct usb_camera_ctrls));

	for (int i = 0; i < CONFIG_USBH_VIDEO_MAX_STREAM_INTERFACE; i++) {
		uvc_dev->stream_ifaces[i] = NULL;
	}

	uvc_dev->current_control_interface = NULL;
	memset(&uvc_dev->current_stream_iface_info, 0, sizeof(struct uvc_stream_iface_info));

	uvc_dev->uvc_descriptors.vc_header = NULL;
	uvc_dev->uvc_descriptors.vc_itd = NULL;
	uvc_dev->uvc_descriptors.vc_otd = NULL;
	uvc_dev->uvc_descriptors.vc_ctd = NULL;
	uvc_dev->uvc_descriptors.vc_sud = NULL;
	uvc_dev->uvc_descriptors.vc_pud = NULL;
	uvc_dev->uvc_descriptors.vc_encoding_unit = NULL;
	uvc_dev->uvc_descriptors.vc_extension_unit = NULL;

	uvc_dev->uvc_descriptors.vs_input_header = NULL;

	memset(&uvc_dev->formats, 0, sizeof(struct uvc_format_info_all));
	if (uvc_dev->video_format_caps) {
		k_free(uvc_dev->video_format_caps);
		uvc_dev->video_format_caps = NULL;
	}

	memset(&uvc_dev->current_format, 0, sizeof(struct uvc_format_info));

	uvc_dev->expect_frame_id = UVC_FRAME_ID_INVALID;
	uvc_dev->discard_first_frame = 1;
	uvc_dev->discard_frame_cnt = 0;
	uvc_dev->multi_prime_cnt = CONFIG_USBH_VIDEO_MULTIPLE_PRIME_COUNT;

	LOG_INF("UVC device structure initialized successfully");
	return 0;
}

/**
 * @brief Handle UVC device connection
 */
static int usbh_uvc_probe(struct usbh_class_data *const c_data,
			  struct usb_device *const udev, uint8_t iface)
{
	const struct device *dev = c_data->priv;
	struct uvc_device *uvc_dev = (struct uvc_device *)dev->data;
	int ret;

	LOG_INF("UVC device connected");

	if (!udev || udev->state != USB_STATE_CONFIGURED) {
		LOG_ERR("USB device not properly configured");
		return -ENODEV;
	}

	if (!uvc_dev) {
		LOG_ERR("No UVC device instance available");
		return -ENODEV;
	}

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);

	uvc_dev->udev = udev;

	ret = uvc_host_parse_descriptors(c_data, uvc_dev, iface);
	if (ret != 0) {
		LOG_ERR("Failed to parse UVC descriptors: %d", ret);
		goto error_cleanup;
	}

	ret = uvc_host_configure_device(uvc_dev);
	if (ret != 0) {
		LOG_ERR("Failed to configure UVC device: %d", ret);
		goto error_cleanup;
	}

	ret = uvc_host_select_default_format(uvc_dev);
	if (ret != 0) {
		LOG_ERR("Failed to set UVC default format : %d", ret);
		goto error_cleanup;
	}

	ret = usb_host_camera_init_controls(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize USB camera controls: %d", ret);
		goto error_cleanup;
	}

	uvc_dev->connected = true;

#ifdef CONFIG_POLL
	if (uvc_dev->sig) {
		k_poll_signal_raise(uvc_dev->sig, USBH_DEVICE_CONNECTED);
		LOG_DBG("UVC device connected signal raised");
	}
#endif
	k_mutex_unlock(&uvc_dev->lock);

	LOG_INF("UVC device (addr=%d) initialization completed",
		uvc_dev->udev->addr);
	return 0;

error_cleanup:
	uvc_dev->udev = NULL;
	k_mutex_unlock(&uvc_dev->lock);
	return ret;
}

/**
 * @brief Handle UVC device disconnection
 */
static int uvc_host_removed(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct uvc_device *uvc_dev = (struct uvc_device *)dev->data;
	struct video_buffer *vbuf;
	int ret = 0;

	if (!uvc_dev) {
		LOG_ERR("No UVC device instance available");
		return -ENODEV;
	}

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);
	uvc_dev->streaming = false;
	uvc_dev->connected = false;	

	/* Dequeue all active video transfers */
	if (uvc_dev->video_transfer_count > 0) {
		LOG_DBG("Cancelling %u active video transfers", uvc_dev->video_transfer_count);
		
		for (int i = 0; i < uvc_dev->video_transfer_count; i++) {
			if (uvc_dev->video_transfer[i]) {
				ret = usbh_xfer_dequeue(uvc_dev->udev, uvc_dev->video_transfer[i]);
				if (ret != 0) {
					LOG_ERR("Failed to dequeue video transfer[%d]: %d", i, ret);
				}
				uvc_dev->video_transfer[i] = NULL;
				LOG_DBG("Video transfer[%d] cancelled", i);
			}
		}
		uvc_dev->video_transfer_count = 0;
	}

	while ((vbuf = k_fifo_get(&uvc_dev->fifo_in, K_NO_WAIT)) != NULL) {
		vbuf->bytesused = 0;
		k_fifo_put(&uvc_dev->fifo_out, vbuf);
	}

	uvc_dev->current_vbuf = NULL;
	uvc_dev->vbuf_offset = 0;
	uvc_dev->transfer_count = 0;

	LOG_DBG("Cleaning up camera controls");
	memset(&uvc_dev->ctrls, 0, sizeof(uvc_dev->ctrls));

	memset(uvc_dev->stream_ifaces, 0, sizeof(uvc_dev->stream_ifaces));
	uvc_dev->current_control_interface = NULL;
	memset(&uvc_dev->current_stream_iface_info, 0,
	       sizeof(uvc_dev->current_stream_iface_info));

	LOG_DBG("Clearing Video Control descriptors");
	uvc_dev->uvc_descriptors.vc_header = NULL;
	uvc_dev->uvc_descriptors.vc_itd = NULL;
	uvc_dev->uvc_descriptors.vc_otd = NULL;
	uvc_dev->uvc_descriptors.vc_ctd = NULL;
	uvc_dev->uvc_descriptors.vc_sud = NULL;
	uvc_dev->uvc_descriptors.vc_pud = NULL;
	uvc_dev->uvc_descriptors.vc_encoding_unit = NULL;
	uvc_dev->uvc_descriptors.vc_extension_unit = NULL;

	LOG_DBG("Clearing Video Streaming descriptors");
	uvc_dev->uvc_descriptors.vs_input_header = NULL;

	memset(&uvc_dev->formats, 0, sizeof(uvc_dev->formats));
	memset(&uvc_dev->current_format, 0, sizeof(uvc_dev->current_format));

	if (uvc_dev->video_format_caps) {
		LOG_DBG("Freeing video format capabilities");
		k_free(uvc_dev->video_format_caps);
		uvc_dev->video_format_caps = NULL;
	}

	memset(&uvc_dev->video_probe, 0, sizeof(uvc_dev->video_probe));

	uvc_dev->udev = NULL;

#ifdef CONFIG_POLL
	if (uvc_dev->sig) {
		k_poll_signal_raise(uvc_dev->sig, USBH_DEVICE_DISCONNECTED);
		LOG_DBG("UVC device disconnected signal raised");
	}
#endif

	k_mutex_unlock(&uvc_dev->lock);

	LOG_INF("UVC device removal completed");

	return 0;
}

static int uvc_host_suspended(struct usbh_context *const uhs_ctx)
{
	return 0;
}

static int uvc_host_resumed(struct usbh_context *const uhs_ctx)
{
	return 0;
}

/**
 * @brief Set video format
 */
static int usbh_uvc_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct uvc_device *uvc_dev = dev->data;
	int ret;

	if (!uvc_dev || !uvc_dev->udev) {
		LOG_ERR("No UVC device connected");
		return -ENODEV;
	}

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);

	ret = uvc_host_set_format(uvc_dev, fmt->pixelformat, fmt->width, fmt->height);
	if (ret != 0) {
		LOG_ERR("Failed to set UVC format: %d", ret);
		goto unlock;
	}

unlock:
	k_mutex_unlock(&uvc_dev->lock);
	return ret;
}

/**
 * @brief Get current video format
 */
static int usbh_uvc_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct uvc_device *uvc_dev = dev->data;

	if (!uvc_dev) {
		return -ENODEV;
	}

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);
	fmt->pixelformat = uvc_dev->current_format.pixelformat;
	fmt->width = uvc_dev->current_format.width;
	fmt->height = uvc_dev->current_format.height;
	k_mutex_unlock(&uvc_dev->lock);

	return 0;
}

/**
 * @brief Get device capabilities
 */
static int usbh_uvc_get_caps(const struct device *dev, struct video_caps *caps)
{
	struct uvc_device *uvc_dev = dev->data;

	if (!uvc_dev) {
		return -ENODEV;
	}

	return uvc_host_get_device_caps(uvc_dev, caps);
}

/**
 * @brief Set frame interval (frame rate)
 */
static int usbh_uvc_set_frmival(const struct device *dev,
					   struct video_frmival *frmival)
{
	struct uvc_device *uvc_dev = dev->data;
	uint32_t fps;
	int ret;

	if (!uvc_dev->connected || !uvc_dev) {
		return -ENODEV;
	}

	if (frmival->numerator == 0 || frmival->denominator == 0) {
		return -EINVAL;
	}

	fps = frmival->denominator / frmival->numerator;

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);

	ret = uvc_host_set_frame_rate(uvc_dev, fps);
	if (ret != 0) {
		LOG_ERR("Failed to set UVC frame rate: %d", ret);
	}

	k_mutex_unlock(&uvc_dev->lock);
	return ret;
}

/**
 * @brief Get current frame interval
 */
static int usbh_uvc_get_frmival(const struct device *dev,
					   struct video_frmival *frmival)
{
	struct uvc_device *uvc_dev = dev->data;

	if (!uvc_dev || !frmival) {
		LOG_ERR("Invalid parameters: dev=%p, frmival=%p", dev, frmival);
		return -EINVAL;
	}

	if (!uvc_dev->connected) {
		LOG_ERR("UVC device not connected");
		return -ENODEV;
	}

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);

	if (uvc_dev->current_format.fps == 0) {
		LOG_ERR("Invalid current format fps: %u", uvc_dev->current_format.fps);
		k_mutex_unlock(&uvc_dev->lock);
		return -EINVAL;
	}

	frmival->numerator = 1;
	frmival->denominator = uvc_dev->current_format.fps;

	k_mutex_unlock(&uvc_dev->lock);

	LOG_DBG("Current frame interval: %u/%u (fps=%u)",
		frmival->numerator, frmival->denominator, uvc_dev->current_format.fps);

	return 0;
}

/**
 * @brief Enumerate supported frame intervals
 */
static int usbh_uvc_enum_frmival(const struct device *dev,
					    struct video_frmival_enum *frmival_enum)
{
	struct uvc_device *uvc_dev = dev->data;
	int ret;

	if (!uvc_dev->connected || !uvc_dev) {
		return -ENODEV;
	}

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);

	ret = uvc_host_enum_frame_intervals(uvc_dev->current_format.frame_ptr, frmival_enum);
	if (ret != 0) {
		LOG_DBG("Failed to enumerate frame intervals: %d", ret);
	}

	k_mutex_unlock(&uvc_dev->lock);
	return ret;
}

#ifdef CONFIG_POLL
/**
 * @brief Set poll signal for UVC device events
 */
static int usbh_uvc_set_signal(const struct device *dev, struct k_poll_signal *sig)
{
	struct uvc_device *uvc_dev = dev->data;

	if (!uvc_dev) {
		LOG_ERR("No UVC device data");
		return -ENODEV;
	}

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);
	uvc_dev->sig = sig;
	k_mutex_unlock(&uvc_dev->lock);

	LOG_DBG("Signal set for UVC device %p", uvc_dev);

	return 0;
}
#endif

/**
 * @brief Search for control in a specific UVC entity type
 */
static int uvc_host_search_control_in_entity(uint8_t subtype, uint32_t cid,
					      uint8_t *found_type,
					      const struct uvc_control_map **map)
{
	const struct uvc_control_map *map_table;
	size_t map_length;
	int ret;

	ret = uvc_get_control_map(subtype, &map_table, &map_length);
	if (ret != 0) {
		return ret;
	}

	for (size_t i = 0; i < map_length; i++) {
		if (map_table[i].cid == cid) {
			*found_type = subtype;
			*map = &map_table[i];
			return 0;
		}
	}

	return -ENOENT;
}
/**
 * @brief Find control mapping by CID based on available entities
 */
static int uvc_host_find_control_mapping(struct uvc_device *uvc_dev, uint32_t cid,
					 uint8_t *entity_subtype,
					 const struct uvc_control_map **map)
{
	int ret;

	/* Search in Processing Unit if it exists */
	if (uvc_dev->uvc_descriptors.vc_pud) {
		ret = uvc_host_search_control_in_entity(UVC_VC_PROCESSING_UNIT, cid,
							entity_subtype, map);
		if (ret == 0) {
			goto exit_find;
		}
		LOG_DBG("Not found control CID 0x%08x in Processing Unit", cid);
	}

	/* Search in Camera Terminal if it exists */
	if (uvc_dev->uvc_descriptors.vc_ctd) {
		ret = uvc_host_search_control_in_entity(UVC_VC_INPUT_TERMINAL, cid,
							entity_subtype, map);
		if (ret == 0) {
			goto exit_find;
		}
		LOG_DBG("Not found control CID 0x%08x in Camera Terminal", cid);
	}

	/* Search in Selector Unit if it exists */
	if (uvc_dev->uvc_descriptors.vc_sud) {
		ret = uvc_host_search_control_in_entity(UVC_VC_SELECTOR_UNIT, cid,
							entity_subtype, map);
		if (ret == 0) {
			goto exit_find;
		}
		LOG_DBG("Not found control CID 0x%08x in Selector Unit", cid);
	}

	/* Search in Extension Unit if it exists */
	if (uvc_dev->uvc_descriptors.vc_extension_unit) {
		ret = uvc_host_search_control_in_entity(UVC_VC_EXTENSION_UNIT, cid,
							entity_subtype, map);
		if (ret == 0) {
			goto exit_find;
		}
		LOG_DBG("Not found control CID 0x%08x in Extension Unit", cid);
	}

	return -ENOENT;

exit_find:
	LOG_DBG("Found control CID 0x%08x in available entity", cid);
	return 0;
}

/**
 * @brief Get entity ID for a specific unit subtype
 */
static uint8_t uvc_host_get_entity_id(struct uvc_device *uvc_dev, uint8_t unit_subtype)
{
	switch (unit_subtype) {
	case UVC_VC_PROCESSING_UNIT:
		return uvc_dev->uvc_descriptors.vc_pud ?
		       uvc_dev->uvc_descriptors.vc_pud->bUnitID : 0;

	case UVC_VC_INPUT_TERMINAL:
		return uvc_dev->uvc_descriptors.vc_ctd ?
		       uvc_dev->uvc_descriptors.vc_ctd->bTerminalID : 0;

	case UVC_VC_SELECTOR_UNIT:
		return uvc_dev->uvc_descriptors.vc_sud ?
		       uvc_dev->uvc_descriptors.vc_sud->bUnitID : 0;

	case UVC_VC_EXTENSION_UNIT:
		return uvc_dev->uvc_descriptors.vc_extension_unit ?
		       uvc_dev->uvc_descriptors.vc_extension_unit->bUnitID : 0;

	default:
		LOG_ERR("Unknown unit subtype: %u", unit_subtype);
		return 0;
	}
}

/**
 * @brief Encode control value into data buffer
 */
static int uvc_host_encode_control_value(struct video_control *ctrl,
					 const struct uvc_control_map *map,
					 uint8_t *data)
{
	switch (ctrl->id) {
	/* Signed 16-bit controls */
	case VIDEO_CID_BRIGHTNESS:
	case VIDEO_CID_HUE:
		sys_put_le16((int16_t)ctrl->val, data);
		break;

	/* Unsigned 16-bit controls */
	case VIDEO_CID_CONTRAST:
	case VIDEO_CID_SATURATION:
	case VIDEO_CID_SHARPNESS:
	case VIDEO_CID_GAMMA:
	case VIDEO_CID_GAIN:
	case VIDEO_CID_WHITE_BALANCE_TEMPERATURE:
	case VIDEO_CID_BACKLIGHT_COMPENSATION:
	case VIDEO_CID_FOCUS_ABSOLUTE:
	case VIDEO_CID_IRIS_ABSOLUTE:
	case VIDEO_CID_ZOOM_ABSOLUTE:
		sys_put_le16((uint16_t)ctrl->val, data);
		break;

	/* Boolean controls */
	case VIDEO_CID_AUTO_WHITE_BALANCE:
	case VIDEO_CID_FOCUS_AUTO:
	case VIDEO_CID_EXPOSURE_AUTO_PRIORITY:
		data[0] = ctrl->val ? 1 : 0;
		break;

	/* Direct 8-bit controls */
	case VIDEO_CID_EXPOSURE_AUTO:
	case VIDEO_CID_IRIS_RELATIVE:
		data[0] = (uint8_t)ctrl->val;
		break;

	/* Special auto gain */
	case VIDEO_CID_AUTOGAIN:
		data[0] = ctrl->val ? 0xFF : 0x00;
		break;

	/* Power line frequency */
	case VIDEO_CID_POWER_LINE_FREQUENCY:
		if (ctrl->val < 0 || ctrl->val > 3) {
			return -EINVAL;
		}
		data[0] = (uint8_t)ctrl->val;
		break;

	/* 32-bit controls */
	case VIDEO_CID_EXPOSURE_ABSOLUTE:
		sys_put_le32((uint32_t)ctrl->val, data);
		break;

	/* Multi-byte relative controls */
	case VIDEO_CID_FOCUS_RELATIVE:
		data[0] = (int8_t)ctrl->val;
		data[1] = 0x01;
		break;

	case VIDEO_CID_ZOOM_RELATIVE:
		data[0] = (int8_t)ctrl->val;
		data[1] = 0x00;
		data[2] = 0x01;
		break;

	case VIDEO_CID_TILT_RELATIVE:
		data[0] = 0x00;
		data[1] = 0x00;
		sys_put_le16((int16_t)ctrl->val, &data[2]);
		break;

	default:
		/* Standard encoding based on mapping table */
		switch (map->size) {
		case 1:
			data[0] = (uint8_t)ctrl->val;
			break;

		case 2:
			if (map->type == UVC_CONTROL_SIGNED) {
				sys_put_le16((int16_t)ctrl->val, data);
			} else {
				sys_put_le16((uint16_t)ctrl->val, data);
			}
			break;

		case 3:
			data[0] = (uint8_t)(ctrl->val & 0xFF);
			data[1] = (uint8_t)((ctrl->val >> 8) & 0xFF);
			data[2] = (uint8_t)((ctrl->val >> 16) & 0xFF);
			break;

		case 4:
			if (map->type == UVC_CONTROL_SIGNED) {
				sys_put_le32((int32_t)ctrl->val, data);
			} else {
				sys_put_le32((uint32_t)ctrl->val, data);
			}
			break;

		default:
			return -EINVAL;
		}
		break;
	}

	return 0;
}

/**
 * @brief Check if control is supported
 */
static bool uvc_host_is_control_supported(struct uvc_device *uvc_dev, uint8_t unit_subtype,
					  uint8_t entity_id, uint32_t control_bit)
{
	switch (unit_subtype) {
	case UVC_VC_PROCESSING_UNIT:
		return uvc_host_pu_supports_control(uvc_dev, control_bit);

	case UVC_VC_INPUT_TERMINAL:
		return uvc_host_ct_supports_control(uvc_dev, control_bit);

	case UVC_VC_SELECTOR_UNIT:
		return true;

	case UVC_VC_EXTENSION_UNIT:
		return true;

	default:
		return false;
	}
}

/**
 * @brief Set control value
 */
static int usbh_uvc_set_ctrl(const struct device *dev, uint32_t id)
{
	struct uvc_device *uvc_dev = dev->data;
	const struct uvc_control_map *map;
	struct video_control control = {.id = id};
	uint8_t unit_subtype;
	uint8_t entity_id;
	uint8_t data[4] = {0};
	int ret;

	if (!uvc_dev || !dev) {
		LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	if (!uvc_dev->connected) {
		LOG_ERR("Device not connected");
		return -ENODEV;
	}

	ret = video_get_ctrl(dev, &control);
	if (ret != 0) {
		LOG_ERR("Failed to get control 0x%08x: %d", id, ret);
		return ret;
	}

	ret = uvc_host_find_control_mapping(uvc_dev, id, &unit_subtype, &map);
	if (ret != 0) {
		LOG_ERR("Control 0x%08x not found in mapping", id);
		return -EINVAL;
	}

	entity_id = uvc_host_get_entity_id(uvc_dev, unit_subtype);
	if (entity_id == 0) {
		LOG_ERR("Entity not found for subtype %u", unit_subtype);
		return -ENODEV;
	}

	if (!uvc_host_is_control_supported(uvc_dev, unit_subtype, entity_id,
					   BIT(map->bit))) {
		LOG_WRN("Control 0x%08x not supported", id);
		return -ENOTSUP;
	}

	ret = uvc_host_encode_control_value(&control, map, data);
	if (ret != 0) {
		LOG_ERR("Failed to encode control value: %d", ret);
		return ret;
	}

	ret = uvc_host_control_unit_and_terminal_request(uvc_dev, UVC_SET_CUR,
							 map->selector, entity_id,
							 data, map->size);
	if (ret != 0) {
		LOG_ERR("Failed to set control 0x%08x: %d", id, ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Generic getter for UVC control values
 */
static int uvc_host_get_control_value(struct uvc_device *uvc_dev,
				      uint32_t cid,
				      int32_t *value)
{
	const struct uvc_control_map *map;
	uint8_t unit_subtype;
	uint8_t entity_id;
	uint8_t data[4] = {0};
	int ret;

	if (!uvc_dev || !value) {
		LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	ret = uvc_host_find_control_mapping(uvc_dev, cid, &unit_subtype, &map);
	if (ret != 0) {
		LOG_DBG("Control 0x%08x mapping not found", cid);
		return ret;
	}

	entity_id = uvc_host_get_entity_id(uvc_dev, unit_subtype);
	if (entity_id == 0) {
		LOG_ERR("Entity not found for control 0x%08x", cid);
		return -ENODEV;
	}

	if (!uvc_host_is_control_supported(uvc_dev, unit_subtype, entity_id,
					   BIT(map->bit))) {
		LOG_DBG("Control 0x%08x not supported", cid);
		return -ENOTSUP;
	}

	ret = uvc_host_control_unit_and_terminal_request(uvc_dev,
							 UVC_GET_CUR,
							 map->selector,
							 entity_id,
							 data, map->size);
	if (ret != 0) {
		LOG_ERR("Failed to get control 0x%08x: %d", cid, ret);
		return ret;
	}

	switch (map->size) {
	case 1:
		*value = (map->type == UVC_CONTROL_SIGNED) ?
			 (int32_t)(int8_t)data[0] :
			 (int32_t)(uint8_t)data[0];
		break;

	case 2:
		if (map->type == UVC_CONTROL_SIGNED) {
			*value = (int32_t)(int16_t)sys_le16_to_cpu(*(uint16_t *)data);
		} else {
			*value = (int32_t)(uint16_t)sys_le16_to_cpu(*(uint16_t *)data);
		}
		break;

	case 4:
		*value = (int32_t)sys_le32_to_cpu(*(uint32_t *)data);
		break;

	default:
		LOG_ERR("Unsupported control size: %u", map->size);
		return -EINVAL;
	}

	LOG_DBG("Got control 0x%08x: %d", cid, *value);
	return 0;
}

/**
 * @brief Get volatile control values
 */
static int usbh_uvc_get_volatile_ctrl(const struct device *dev, uint32_t id)
{
	struct uvc_device *uvc_dev = dev->data;
	struct usb_camera_ctrls *ctrls = &uvc_dev->ctrls;
	int ret;

	switch (id) {
	case VIDEO_CID_AUTOGAIN:
		ret = uvc_host_get_control_value(uvc_dev, VIDEO_CID_GAIN,
						 &ctrls->gain.val);
		if (ret != 0) {
			LOG_ERR("Failed to get gain: %d", ret);
			return ret;
		}
		break;

	case VIDEO_CID_EXPOSURE_AUTO:
		ret = uvc_host_get_control_value(uvc_dev, VIDEO_CID_EXPOSURE_AUTO,
						 &ctrls->auto_exposure.val);
		if (ret != 0) {
			LOG_DBG("Failed to get exposure auto: %d", ret);
			return ret;
		}
		break;

	case VIDEO_CID_AUTO_WHITE_BALANCE:
		ret = uvc_host_get_control_value(uvc_dev, VIDEO_CID_AUTO_WHITE_BALANCE,
						 &ctrls->auto_white_balance_temperature.val);
		if (ret != 0) {
			LOG_DBG("Failed to get white balance auto: %d", ret);
			return ret;
		}
		break;

	case VIDEO_CID_EXPOSURE_ABSOLUTE:
		ret = uvc_host_get_control_value(uvc_dev, VIDEO_CID_EXPOSURE_ABSOLUTE,
						 &ctrls->exposure_absolute.val);
		if (ret != 0) {
			LOG_DBG("Failed to get exposure absolute: %d", ret);
			return ret;
		}
		break;

	case VIDEO_CID_WHITE_BALANCE_TEMPERATURE:
		ret = uvc_host_get_control_value(uvc_dev, VIDEO_CID_WHITE_BALANCE_TEMPERATURE,
						 &ctrls->white_balance_temperature.val);
		if (ret != 0) {
			LOG_DBG("Failed to get white balance temperature: %d", ret);
			return ret;
		}
		break;

	default:
		LOG_WRN("Volatile control 0x%08x not supported", id);
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Start/stop streaming
 */
static int usbh_uvc_set_stream(const struct device *dev, bool enable,
					  enum video_buf_type type)
{
	struct uvc_device *uvc_dev;
	struct video_buffer *vbuf;
	uint8_t alt;
	uint8_t interface_num;
	int ret;

	if (!dev) {
		return -EINVAL;
	}

	uvc_dev = dev->data;
	if (!uvc_dev || !uvc_dev->connected) {
		return -ENODEV;
	}

	if (enable) {
		if (!uvc_dev->current_stream_iface_info.current_stream_iface) {
			LOG_ERR("No streaming interface configured");
			return -EINVAL;
		}
		alt = uvc_dev->current_stream_iface_info.current_stream_iface->bAlternateSetting;
		interface_num = uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber;
	} else {
		if (!uvc_dev->current_stream_iface_info.current_stream_iface) {
			LOG_WRN("No interface configured, cannot disable streaming");
			return -EINVAL;
		}

		alt = 0;
		interface_num = uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber;
		uvc_dev->streaming = false;

		/* Cancel all active transfers */
		if (uvc_dev->video_transfer_count > 0) {
			LOG_DBG("Stopping streaming, cancelling %u transfers", 
				uvc_dev->video_transfer_count);

			for (int i = 0; i < uvc_dev->video_transfer_count; i++) {
				if (!uvc_dev->video_transfer[i]) {
					continue;
				}

				ret = usbh_xfer_dequeue(uvc_dev->udev, uvc_dev->video_transfer[i]);
				if (ret != 0) {
					LOG_ERR("Failed to dequeue transfer[%d]: %d", i, ret);
				}
				uvc_dev->video_transfer[i] = NULL;
			}
			uvc_dev->video_transfer_count = 0;
		}
	}

	ret = usbh_device_interface_set(uvc_dev->udev, interface_num, alt, false);
	if (ret != 0) {
		LOG_ERR("Failed to set interface %d alt setting %d: %d", interface_num, alt, ret);
		return ret;
	}

	uvc_dev->streaming = enable;

	if (enable) {
		uvc_dev->streaming = true;
		vbuf = k_fifo_peek_head(&uvc_dev->fifo_in);
		if (vbuf != NULL) {
			vbuf->bytesused = 0;
			memset(vbuf->buffer, 0, vbuf->size);
			uvc_dev->current_vbuf = vbuf;
			uvc_dev->multi_prime_cnt = CONFIG_USBH_VIDEO_MULTIPLE_PRIME_COUNT;
			while (uvc_dev->multi_prime_cnt > 0) {
				ret  = uvc_host_initiate_transfer(uvc_dev, vbuf);
				if (ret != 0) {
					LOG_ERR("Failed to initiate transfer: %d", ret);
					goto err_stream;
				}
				uvc_dev->multi_prime_cnt--;
			}
		}
	}

	LOG_DBG("UVC streaming %s successfully", enable ? "enabled" : "disabled");
	return 0;

err_stream:
	uvc_dev->streaming = false;
	for (int i = 0; i < uvc_dev->video_transfer_count; i++) {
		if (uvc_dev->video_transfer[i]) {
			usbh_xfer_dequeue(uvc_dev->udev, uvc_dev->video_transfer[i]);
			uvc_dev->video_transfer[i] = NULL;
		}
	}
	uvc_dev->video_transfer_count = 0;
	usbh_device_interface_set(uvc_dev->udev, interface_num, 0, false);
	return ret;
}

/**
 * @brief Enqueue video buffer
 */
static int usbh_uvc_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct uvc_device *uvc_dev;

	if (!dev) {
		return -EINVAL;
	}

	uvc_dev = dev->data;
	if (!uvc_dev || !uvc_dev->connected) {
		return -ENODEV;
	}

	vbuf->bytesused = 0;
	vbuf->timestamp = 0;
	vbuf->line_offset = 0;

	k_fifo_put(&uvc_dev->fifo_in, vbuf);

	return 0;
}

/**
 * @brief Dequeue completed video buffer
 */
static int usbh_uvc_dequeue(const struct device *dev, struct video_buffer **vbuf,
				      k_timeout_t timeout)
{
	struct uvc_device *uvc_dev;

	if (!dev) {
		return -EINVAL;
	}

	uvc_dev = dev->data;
	if (!uvc_dev) {
		return -ENODEV;
	}

	*vbuf = k_fifo_get(&uvc_dev->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static struct usbh_class_api uvc_host_class_api = {
	.init = uvc_host_init,
	.probe = usbh_uvc_probe,
	.removed = uvc_host_removed,
};

static struct usbh_class_filter usbh_uvc_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_VIDEO,
		.sub = UVC_SC_VIDEO_INTERFACE_COLLECTION,
		.proto = 0,
	},
	{0},
};

static DEVICE_API(video, uvc_host_video_api) = {
	.set_format = usbh_uvc_set_fmt,
	.get_format = usbh_uvc_get_fmt,
	.get_caps = usbh_uvc_get_caps,
	.set_frmival = usbh_uvc_set_frmival,
	.get_frmival = usbh_uvc_get_frmival,
	.enum_frmival = usbh_uvc_enum_frmival,
#ifdef CONFIG_POLL
	.set_signal = usbh_uvc_set_signal,
#endif
	.get_volatile_ctrl = usbh_uvc_get_volatile_ctrl,
	.set_ctrl = usbh_uvc_set_ctrl,
	.set_stream = usbh_uvc_set_stream,
	.enqueue = usbh_uvc_enqueue,
	.dequeue = usbh_uvc_dequeue,
};

#define USBH_VIDEO_DT_DEVICE_DEFINE(n)						\
	static struct uvc_device uvc_device_##n;				\
	DEVICE_DT_INST_DEFINE(n, uvc_host_preinit, NULL,			\
			      &uvc_device_##n, NULL,				\
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,		\
			      &uvc_host_video_api);				\
	USBH_DEFINE_CLASS(uvc_host_c_data_##n, &uvc_host_class_api,		\
			  (void *)DEVICE_DT_INST_GET(n), usbh_uvc_filters);	\
	VIDEO_DEVICE_DEFINE(usb_camera_##n, (void *)DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(USBH_VIDEO_DT_DEVICE_DEFINE)
