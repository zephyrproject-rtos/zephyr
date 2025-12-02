/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * Copyright 2025 - 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>

#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_ch9.h>

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_device.h"

#include "uvc.h"
#include "../../../drivers/video/video_ctrls.h"
#include "../../../drivers/video/video_device.h"

LOG_MODULE_REGISTER(usbh_uvc, CONFIG_USBH_UVC_LOG_LEVEL);

#define UVC_FRAME_ID_INVALID                  0xFF
#define UVC_PAYLOAD_HEADER_MIN_SIZE           11
#define UVC_DEFAULT_FRAME_INTERVAL            333333
#define UVC_FRAME_DESC_MIN_SIZE_WITH_INTERVAL 26
#define UVC_FRAME_DESC_MIN_SIZE_STEPWISE      38
#define UVC_BANDWIDTH_MARGIN_PERCENT          110
#define UVC_MJPEG_COMPRESSION_RATIO           6

/* USB UVC control parameters structure */
struct uvc_ctrls {
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

/* UVC device descriptors collection */
struct uvc_device_descriptors {
	const struct uvc_control_header_descriptor *vc_header;
	const struct uvc_input_terminal_descriptor *vc_itd;
	const struct uvc_output_terminal_descriptor *vc_otd;
	const struct uvc_camera_terminal_descriptor *vc_ctd;
	const struct uvc_selector_unit_descriptor *vc_sud;
	const struct uvc_processing_unit_descriptor *vc_pud;
	const struct uvc_encoding_unit_descriptor *vc_encoding_unit;
	const struct uvc_extension_unit_descriptor *vc_extension_unit;
	const struct uvc_stream_header_descriptor *vs_input_header;
};

struct uvc_stream_iface_info {
	const struct usb_if_descriptor *current_stream_iface;
	const struct usb_ep_descriptor *current_stream_ep;
	uint32_t cur_ep_mps_mult;
};

struct uvc_format_info {
	struct video_format video_fmt;
	uint32_t frmival_100ns;
	uint8_t format_index;
	uint8_t frame_index;
	const struct uvc_format_descriptor *format_ptr;
	const struct uvc_frame_descriptor *frame_ptr;
};

struct uvc_vs_format_mjpeg_info {
	const struct uvc_format_mjpeg_descriptor *vs_mjpeg_format[CONFIG_USBH_VIDEO_MAX_FORMATS];
	uint8_t num_mjpeg_formats;
};

struct uvc_vs_format_uncompressed_info {
	const struct uvc_format_uncomp_descriptor
		*uncompressed_format[CONFIG_USBH_VIDEO_MAX_FORMATS];
	uint8_t num_uncompressed_formats;
};

struct uvc_format_info_all {
	struct uvc_vs_format_mjpeg_info format_mjpeg;
	struct uvc_vs_format_uncompressed_info format_uncompressed;
};

struct uvc_host_data {
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
	const struct usb_if_descriptor *stream_ifaces[CONFIG_USBH_VIDEO_MAX_STREAM_INTERFACE];
	struct uvc_stream_iface_info current_stream_iface_info;
	struct uvc_device_descriptors uvc_descriptors;

	struct uvc_format_info_all formats;
	struct uvc_format_info current_format;
	struct video_format_cap format_caps[CONFIG_USBH_VIDEO_MAX_CAPS];
	uint8_t format_caps_count;
	struct uvc_probe probe_data;
	struct uvc_ctrls ctrls;
};

static int uvc_host_stream_iso_req_cb(struct usb_device *const dev,
				      struct uhc_transfer *const xfer);

/* Select default video format for UVC device */
static int uvc_host_select_default_format(struct uvc_host_data *const host_data)
{
	struct uvc_vs_format_uncompressed_info *uncompressed_info =
		&host_data->formats.format_uncompressed;
	struct uvc_vs_format_mjpeg_info *mjpeg_info = &host_data->formats.format_mjpeg;
	const struct uvc_format_uncomp_descriptor *uncomp_format = NULL;
	const struct uvc_format_mjpeg_descriptor *mjpeg_format = NULL;
	const void *format_ptr;
	uint32_t pixelformat;
	uint8_t format_index;
	uint8_t frame_subtype;
	bool is_mjpeg;
	int ret;

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
	const struct usb_desc_header *head = format_ptr;

	head = usbh_desc_get_next(head);
	while (head != NULL) {
		const struct uvc_frame_descriptor *frame =
			(const struct uvc_frame_descriptor *)head;
		if (head->bDescriptorType == USB_DESC_CS_INTERFACE &&
		    frame->bDescriptorSubtype == frame_subtype &&
		    head->bLength >= sizeof(struct uvc_frame_descriptor)) {
			struct video_format *fmt = &host_data->current_format.video_fmt;
			uint32_t frmival = 0;
			uint16_t width = sys_le16_to_cpu(frame->wWidth);
			uint16_t height = sys_le16_to_cpu(frame->wHeight);

			if (frame_subtype == UVC_VS_FRAME_FRAME_BASED) {
				const struct uvc_frame_based_continuous_descriptor *fb_desc =
					(const void *)frame;
				frmival = sys_le32_to_cpu(fb_desc->dwDefaultFrameInterval);
			}

			if (frame_subtype == UVC_VS_FRAME_UNCOMPRESSED ||
			    frame_subtype == UVC_VS_FRAME_MJPEG) {
				const struct uvc_frame_continuous_descriptor *std_desc =
					(const void *)frame;
				frmival = sys_le32_to_cpu(std_desc->dwDefaultFrameInterval);
			}

			fmt->pixelformat = pixelformat;
			fmt->width = width;
			fmt->height = height;

			ret = video_estimate_fmt_size(fmt);
			if (ret != 0) {
				LOG_ERR("Failed to estimate format size: %d", ret);
				return ret;
			}

			host_data->current_format.frmival_100ns = frmival;
			host_data->current_format.format_index = format_index;
			host_data->current_format.frame_index = frame->bFrameIndex;
			host_data->current_format.format_ptr = format_ptr;
			host_data->current_format.frame_ptr = frame;

			LOG_INF("Set default format: %s %ux%u@%ufps (format_idx=%u, frame_idx=%u)",
				is_mjpeg ? "MJPEG" : VIDEO_FOURCC_TO_STR(pixelformat), width,
				height, (NSEC_PER_SEC / 100) / frmival, format_index,
				frame->bFrameIndex);
			return 0;
		}

		head = usbh_desc_get_next(head);
	}

	LOG_ERR("No valid format/frame descriptors found");
	return -ENOTSUP;
}

/* Configure UVC device interfaces */
static int uvc_host_configure_device(struct uvc_host_data *const host_data)
{
	struct usb_device *udev = host_data->udev;
	const struct usb_if_descriptor *ctrl_iface;
	const struct usb_if_descriptor *stream_iface;
	int ret;

	ctrl_iface = host_data->current_control_interface;
	stream_iface = host_data->current_stream_iface_info.current_stream_iface;

	if (!ctrl_iface) {
		LOG_ERR("No control interface found");
		return -ENODEV;
	}

	if (!stream_iface) {
		LOG_ERR("No streaming interface found");
		return -ENODEV;
	}

	/* Set control interface to default alternate setting (0) */
	ret = usbh_device_interface_set(udev, ctrl_iface->bInterfaceNumber, 0, false);
	if (ret != 0) {
		LOG_ERR("Failed to set control interface alternate setting: %d", ret);
		return ret;
	}

	/* Set streaming interface to idle state (alternate 0) */
	ret = usbh_device_interface_set(udev, stream_iface->bInterfaceNumber, 0, false);

	if (ret != 0) {
		LOG_ERR("Failed to set streaming interface alternate setting: %d", ret);
		return ret;
	}

	LOG_INF("UVC device configured successfully (control: interface %u, streaming: interface "
		"%u",
		ctrl_iface->bInterfaceNumber, stream_iface->bInterfaceNumber);

	return 0;
}

static bool usbh_uvc_desc_is_valid_vs_header(const void *const desc)
{
	const struct uvc_stream_header_descriptor *const header_desc = desc;

	if (!usbh_desc_is_valid(desc, sizeof(struct uvc_stream_header_descriptor),
				USB_DESC_CS_INTERFACE)) {
		return false;
	}

	return header_desc->bDescriptorSubtype == UVC_VS_INPUT_HEADER;
}

static bool usbh_uvc_desc_is_valid_vc_header(const void *const desc)
{
	const struct uvc_control_header_descriptor *const header_desc = desc;

	if (!usbh_desc_is_valid(desc, sizeof(struct uvc_control_header_descriptor),
				USB_DESC_CS_INTERFACE)) {
		return false;
	}

	return header_desc->bDescriptorSubtype == UVC_VC_HEADER;
}

static const void *usbh_uvc_desc_get_vs_end(const struct usb_if_descriptor *if_desc)
{
	const struct uvc_stream_header_descriptor *header_desc;
	const void *vs_end;

	if (!usbh_desc_is_valid_interface(if_desc)) {
		LOG_ERR("Invalid interface descriptor");
		return NULL;
	}

	header_desc = (const void *)usbh_desc_get_next(if_desc);

	if (!usbh_uvc_desc_is_valid_vs_header(header_desc)) {
		LOG_ERR("Invalid VS header descriptor");
		return NULL;
	}

	vs_end = (uint8_t *)header_desc + sys_le16_to_cpu(header_desc->wTotalLength);

	return vs_end;
}

static const void *usbh_uvc_desc_get_vc_end(const struct usb_if_descriptor *if_desc)
{
	const struct uvc_control_header_descriptor *header_desc;
	const void *vc_end;

	if (!usbh_desc_is_valid_interface(if_desc)) {
		LOG_ERR("Invalid interface descriptor");
		return NULL;
	}

	header_desc = (const void *)usbh_desc_get_next(if_desc);

	if (!usbh_uvc_desc_is_valid_vc_header(header_desc)) {
		LOG_ERR("Invalid VC header descriptor");
		return NULL;
	}

	vc_end = (uint8_t *)header_desc + sys_le16_to_cpu(header_desc->wTotalLength);

	return vc_end;
}

static int usbh_uvc_parse_vc_desc(struct uvc_host_data *const host_data, const void *const desc_beg,
				  const void *const desc_end)
{
	const struct usb_desc_header *desc = usbh_desc_get_next(desc_beg);

	for (; desc != NULL && (uint8_t *)desc < (uint8_t *)desc_end;
	     desc = usbh_desc_get_next(desc)) {
		if (desc->bDescriptorType == USB_DESC_INTERFACE ||
		    desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
			break;
		}

		if (desc->bDescriptorType != USB_DESC_CS_INTERFACE) {
			LOG_WRN("VideoControl descriptor: unknown type %u, skipping",
				desc->bDescriptorType);
			continue;
		}

		const struct usb_cs_desc_header *const cs_desc = (const void *)desc;

		switch (cs_desc->bDescriptorSubtype) {
		case UVC_VC_HEADER: {
			const struct uvc_control_header_descriptor *header_desc =
				(const void *)desc;

			if (desc->bLength < sizeof(struct uvc_control_header_descriptor)) {
				LOG_ERR("Invalid VC header descriptor length: %u", desc->bLength);
				return -EINVAL;
			}

			host_data->uvc_descriptors.vc_header = header_desc;
			LOG_DBG("Found VideoControl Header: UVC v%u.%u",
				(sys_le16_to_cpu(header_desc->bcdUVC) >> 8) & 0xFF,
				sys_le16_to_cpu(header_desc->bcdUVC) & 0xFF);
			break;
		}

		case UVC_VC_INPUT_TERMINAL: {
			const struct uvc_input_terminal_descriptor *it_desc = (const void *)desc;

			if (desc->bLength < sizeof(struct uvc_input_terminal_descriptor)) {
				LOG_ERR("Invalid input terminal descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			if (sys_le16_to_cpu(it_desc->wTerminalType) == UVC_ITT_CAMERA) {
				const struct uvc_camera_terminal_descriptor *ct_desc =
					(const void *)desc;
				host_data->uvc_descriptors.vc_ctd = ct_desc;
				LOG_DBG("Found Camera Terminal: ID=%u", ct_desc->bTerminalID);
			} else {
				host_data->uvc_descriptors.vc_itd = it_desc;
				LOG_DBG("Found Input Terminal: ID=%u", it_desc->bTerminalID);
			}
			break;
		}

		case UVC_VC_OUTPUT_TERMINAL: {
			const struct uvc_output_terminal_descriptor *ot_desc = (const void *)desc;

			if (desc->bLength < sizeof(struct uvc_output_terminal_descriptor)) {
				LOG_ERR("Invalid output terminal descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			host_data->uvc_descriptors.vc_otd = ot_desc;
			LOG_DBG("Found Output Terminal: ID=%u", ot_desc->bTerminalID);
			break;
		}

		case UVC_VC_SELECTOR_UNIT: {
			const struct uvc_selector_unit_descriptor *su_desc = (const void *)desc;

			if (desc->bLength < 5) {
				LOG_ERR("Invalid selector unit descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			host_data->uvc_descriptors.vc_sud = su_desc;
			LOG_DBG("Found Selector Unit: ID=%u", su_desc->bUnitID);
			break;
		}

		case UVC_VC_PROCESSING_UNIT: {
			const struct uvc_processing_unit_descriptor *pu_desc = (const void *)desc;

			if (desc->bLength < 8) {
				LOG_ERR("Invalid processing unit descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			host_data->uvc_descriptors.vc_pud = pu_desc;
			LOG_DBG("Found Processing Unit: ID=%u", pu_desc->bUnitID);
			break;
		}

		case UVC_VC_ENCODING_UNIT: {
			const struct uvc_encoding_unit_descriptor *enc_desc = (const void *)desc;

			if (desc->bLength < 8) {
				LOG_ERR("Invalid encoding unit descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			host_data->uvc_descriptors.vc_encoding_unit = enc_desc;
			LOG_DBG("Found Encoding Unit: ID=%u", enc_desc->bUnitID);
			break;
		}

		case UVC_VC_EXTENSION_UNIT: {
			const struct uvc_extension_unit_descriptor *eu_desc = (const void *)desc;

			if (desc->bLength < 24) {
				LOG_ERR("Invalid extension unit descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			host_data->uvc_descriptors.vc_extension_unit = eu_desc;
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

static int usbh_uvc_parse_vs_desc(struct uvc_host_data *const host_data, const void *const desc_beg,
				  const void *const desc_end)
{
	const struct usb_desc_header *desc = usbh_desc_get_next(desc_beg);

	for (; desc != NULL && (uint8_t *)desc < (uint8_t *)desc_end;
	     desc = usbh_desc_get_next(desc)) {
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

		const struct usb_cs_desc_header *const cs_desc = (const void *)desc;

		switch (cs_desc->bDescriptorSubtype) {
		case UVC_VS_INPUT_HEADER: {
			const struct uvc_stream_header_descriptor *header_desc = (const void *)desc;

			if (desc->bLength < sizeof(struct uvc_stream_header_descriptor)) {
				LOG_ERR("Invalid VS input header descriptor length: %u",
					desc->bLength);
				return -EINVAL;
			}

			host_data->uvc_descriptors.vs_input_header = header_desc;
			LOG_DBG("Found VS input header: formats=%u", header_desc->bNumFormats);
			break;
		}

		case UVC_VS_FORMAT_UNCOMPRESSED: {
			const struct uvc_format_uncomp_descriptor *format_desc = (const void *)desc;
			struct uvc_vs_format_uncompressed_info *info =
				&host_data->formats.format_uncompressed;

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
			const struct uvc_format_mjpeg_descriptor *format_desc = (const void *)desc;
			struct uvc_vs_format_mjpeg_info *info = &host_data->formats.format_mjpeg;

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
			LOG_DBG("Added MJPEG format[%u]: index=%u", info->num_mjpeg_formats - 1,
				format_desc->bFormatIndex);
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

static void usbh_uvc_parse_vs_interface_alt_desc(struct uvc_host_data *const host_data,
						 uint8_t iface, const void *const desc_beg)
{
	const struct usb_desc_header *desc = (const void *)desc_beg;
	const struct usb_if_descriptor *if_desc;
	int i;

	if (desc_beg == NULL) {
		if_desc = (const void *)usbh_desc_get_iface(host_data->udev, iface);
		if (if_desc == NULL) {
			return;
		}
		desc = (const void *)usbh_desc_get_next_alt_setting(if_desc);
	}

	for (; desc != NULL; desc = usbh_desc_get_next(desc)) {

		if (desc->bDescriptorType != USB_DESC_INTERFACE) {
			continue;
		}

		if_desc = (const void *)desc;

		if (if_desc->bInterfaceNumber != iface ||
		    if_desc->bInterfaceClass != USB_BCC_VIDEO ||
		    if_desc->bInterfaceSubClass != UVC_SC_VIDEOSTREAMING) {
			return;
		}

		for (i = 0; i < CONFIG_USBH_VIDEO_MAX_STREAM_INTERFACE; i++) {
			if (host_data->stream_ifaces[i] != NULL) {
				continue;
			}

			host_data->stream_ifaces[i] = (const void *)if_desc;
			LOG_DBG("Added VideoStreaming interface %u alt %u to stream_ifaces[%d]",
				if_desc->bInterfaceNumber, if_desc->bAlternateSetting, i);
			break;
		}
	}
}

/* Parse all UVC descriptors from device */
static int uvc_host_parse_descriptors(struct usbh_class_data *const c_data,
				      struct uvc_host_data *const host_data, uint8_t iface)
{
	const struct usb_association_descriptor *iad_desc;
	const struct usb_if_descriptor *if_desc;
	bool has_vc_if = false;
	bool has_vs_if = false;
	int ret;

	iad_desc = (const void *)usbh_desc_get_iad(host_data->udev, iface);

	if (iad_desc == NULL) {
		LOG_ERR("Failed to find interface association for interface %u", iface);
		return -ENOSYS;
	}

	if (iad_desc->bDescriptorType != USB_DESC_INTERFACE_ASSOC) {
		LOG_WRN("Interface %u is not a valid interface association, skipping", iface);
		return -ENOTSUP;
	}

	for (uint8_t i = 0; i < iad_desc->bInterfaceCount; i++) {
		if_desc = (const void *)usbh_desc_get_iface(host_data->udev, iface + i);

		if (if_desc == NULL) {
			LOG_ERR("Not as many interfaces (%u) as announced (%u)", i,
				iad_desc->bInterfaceCount);
			return -EBADMSG;
		}

		if (if_desc->bInterfaceClass == USB_BCC_VIDEO &&
		    if_desc->bInterfaceSubClass == UVC_SC_VIDEOCONTROL) {
			const void *vc_end;

			if (has_vc_if) {
				LOG_WRN("Skipping extra VideoControl interface");
				continue;
			}

			host_data->current_control_interface = if_desc;

			vc_end = usbh_uvc_desc_get_vc_end(if_desc);
			if (vc_end == NULL) {
				LOG_ERR("Invalid VideoControl interface descriptor");
				return -EBADMSG;
			}

			ret = usbh_uvc_parse_vc_desc(host_data, if_desc, vc_end);
			if (ret != 0) {
				LOG_ERR("Failed to parse VC descriptor");
				return ret;
			}

			has_vc_if = true;
		}

		/* Process VideoStreaming interface */
		if (if_desc->bInterfaceClass == USB_BCC_VIDEO &&
		    if_desc->bInterfaceSubClass == UVC_SC_VIDEOSTREAMING) {
			const void *vs_end;

			if (has_vs_if) {
				LOG_WRN("Skipping extra VideoStreaming interface");
				continue;
			}

			host_data->current_stream_iface_info.current_stream_iface = if_desc;

			/* Get the end of VideoStreaming descriptors */
			vs_end = usbh_uvc_desc_get_vs_end(if_desc);
			if (vs_end == NULL) {
				LOG_ERR("Invalid VideoStreaming interface descriptor");
				return -EBADMSG;
			}

			ret = usbh_uvc_parse_vs_desc(host_data, if_desc, vs_end);
			if (ret != 0) {
				LOG_ERR("Failed to parse VS descriptor");
				return ret;
			}

			/* Parse alternate stream interface descriptors */
			usbh_uvc_parse_vs_interface_alt_desc(host_data, if_desc->bInterfaceNumber,
							     vs_end);

			has_vs_if = true;
		}
	}

	if (!has_vs_if) {
		LOG_ERR("No VideoStreaming interface found");
		return -EINVAL;
	}

	if (!has_vc_if) {
		LOG_ERR("No VideoControl interface found");
		return -EINVAL;
	}

	LOG_INF("Interface %u associated with UVC class", iface);

	return 0;
}

/* Parse default frame interval from descriptor */
static uint32_t uvc_host_get_frame_default_intervals(const struct uvc_frame_descriptor *frame_desc)
{
	uint32_t default_interval = 0;
	uint32_t max_interval = UVC_DEFAULT_FRAME_INTERVAL;
	uint8_t interval_type = 0;

	if (frame_desc->bDescriptorSubtype == UVC_VS_FRAME_FRAME_BASED) {
		const struct uvc_frame_based_continuous_descriptor *fb_desc =
			(const void *)frame_desc;

		default_interval = sys_le32_to_cpu(fb_desc->dwDefaultFrameInterval);
		interval_type = fb_desc->bFrameIntervalType;
	}

	if (frame_desc->bDescriptorSubtype == UVC_VS_FRAME_UNCOMPRESSED ||
	    frame_desc->bDescriptorSubtype == UVC_VS_FRAME_MJPEG) {
		const struct uvc_frame_continuous_descriptor *std_desc = (const void *)frame_desc;

		default_interval = sys_le32_to_cpu(std_desc->dwDefaultFrameInterval);
		interval_type = std_desc->bFrameIntervalType;
	}

	if (default_interval != 0) {
		return default_interval;
	}

	if (interval_type == 0) {
		if (frame_desc->bDescriptorSubtype == UVC_VS_FRAME_FRAME_BASED) {
			const struct uvc_frame_based_continuous_descriptor *fb_desc =
				(const void *)frame_desc;

			max_interval = sys_le32_to_cpu(fb_desc->dwMaxFrameInterval);
		}

		if (frame_desc->bDescriptorSubtype == UVC_VS_FRAME_UNCOMPRESSED ||
		    frame_desc->bDescriptorSubtype == UVC_VS_FRAME_MJPEG) {
			const struct uvc_frame_continuous_descriptor *continuous_desc =
				(const void *)frame_desc;

			max_interval = sys_le32_to_cpu(continuous_desc->dwMaxFrameInterval);
		}
	} else {
		if (frame_desc->bDescriptorSubtype == UVC_VS_FRAME_FRAME_BASED) {
			const struct uvc_frame_based_discrete_descriptor *fb_desc =
				(const void *)frame_desc;
			max_interval = sys_le32_to_cpu(fb_desc->dwFrameInterval[interval_type - 1]);
		}

		if (frame_desc->bDescriptorSubtype == UVC_VS_FRAME_UNCOMPRESSED ||
		    frame_desc->bDescriptorSubtype == UVC_VS_FRAME_MJPEG) {
			const struct uvc_frame_discrete_descriptor *discrete_desc =
				(const void *)frame_desc;

			max_interval =
				sys_le32_to_cpu(discrete_desc->dwFrameInterval[interval_type - 1]);
		}
	}

	return max_interval;
}

/* Find matching frame in specific format type */
static int uvc_host_find_frame_in_format(const struct uvc_format_descriptor *format_header,
					 uint16_t target_width, uint16_t target_height,
					 uint8_t expected_frame_subtype,
					 const struct uvc_frame_descriptor **found_frame,
					 uint32_t *found_interval)
{
	const struct usb_desc_header *desc_buf = usbh_desc_get_next(format_header);
	int frames_found = 0;

	while (frames_found < format_header->bNumFrameDescriptors && desc_buf != NULL) {
		const struct uvc_frame_descriptor *frame_header = (const void *)desc_buf;

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
					uvc_host_get_frame_default_intervals(frame_header);
				return 0;
			}
			frames_found++;
		}

		if (frame_header->bDescriptorType == USB_DESC_CS_INTERFACE &&
		    (frame_header->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED ||
		     frame_header->bDescriptorSubtype == UVC_VS_FORMAT_MJPEG ||
		     frame_header->bDescriptorSubtype == UVC_VS_FORMAT_FRAME_BASED)) {
			break;
		}

		desc_buf = usbh_desc_get_next(desc_buf);
	}

	return -ENOTSUP;
}

/* Find format and frame matching specifications */
static int uvc_host_find_format(struct uvc_host_data *const host_data,
				struct video_format *const fmt,
				const struct uvc_format_descriptor **format,
				const struct uvc_frame_descriptor **frame, uint32_t *frmival)
{
	struct uvc_vs_format_uncompressed_info *uncompressed_info =
		&host_data->formats.format_uncompressed;

	LOG_DBG("Looking for format: %s %ux%u", VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width,
		fmt->height);

	for (uint8_t i = 0; i < uncompressed_info->num_uncompressed_formats; i++) {
		const struct uvc_format_uncomp_descriptor *format_desc =
			uncompressed_info->uncompressed_format[i];

		if (!format_desc) {
			continue;
		}

		uint32_t desc_pixelformat = uvc_guid_to_fourcc(format_desc->guidFormat);

		if (desc_pixelformat == fmt->pixelformat) {
			LOG_DBG("Found matching uncompressed format: index=%u",
				format_desc->bFormatIndex);

			if (uvc_host_find_frame_in_format((const void *)format_desc, fmt->width,
							  fmt->height, UVC_VS_FRAME_UNCOMPRESSED,
							  frame, frmival) == 0) {
				*format = (const void *)format_desc;
				LOG_DBG("Found frame: format=%p, frame=%p, interval=%u", *format,
					*frame, *frmival);
				return 0;
			}
		}
	}

	/* Search MJPEG formats */
	if (fmt->pixelformat == VIDEO_PIX_FMT_JPEG) {
		struct uvc_vs_format_mjpeg_info *mjpeg_info = &host_data->formats.format_mjpeg;

		for (uint8_t i = 0; i < mjpeg_info->num_mjpeg_formats; i++) {
			const struct uvc_format_mjpeg_descriptor *format_desc =
				mjpeg_info->vs_mjpeg_format[i];

			if (!format_desc) {
				continue;
			}

			LOG_DBG("Checking MJPEG format: index=%u", format_desc->bFormatIndex);

			if (uvc_host_find_frame_in_format((const void *)format_desc, fmt->width,
							  fmt->height, UVC_VS_FRAME_MJPEG, frame,
							  frmival) == 0) {
				*format = (const void *)format_desc;
				LOG_DBG("Found MJPEG frame: format=%p, frame=%p, interval=%u",
					*format, *frame, *frmival);
				return 0;
			}
		}
	}

	LOG_ERR("Format %s %ux%u not supported by device", VIDEO_FOURCC_TO_STR(fmt->pixelformat),
		fmt->width, fmt->height);
	return -ENOTSUP;
}

/* Check if an endpoint has enough bandwidth for the given transfer parameters */
static bool uvc_host_ep_has_enough_bandwidth(const struct usb_ep_descriptor *ep_desc,
					     const struct usb_if_descriptor *if_desc,
					     enum usb_device_speed device_speed,
					     uint32_t required_bandwidth,
					     uint32_t max_payload_transfer_size)
{
	uint32_t ep_payload_size;
	uint32_t ep_bandwidth;
	uint8_t interval;
	uint16_t max_packet_size;

	/* Validate endpoint descriptor */
	if (!usbh_desc_is_valid_endpoint(ep_desc)) {
		LOG_WRN("Invalid endpoint descriptor, skipping");
		return false;
	}

	/* Check if this is an isochronous IN endpoint */
	if ((ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) != USB_EP_TYPE_ISO ||
	    (ep_desc->bEndpointAddress & USB_EP_DIR_MASK) != USB_EP_DIR_IN) {
		return false;
	}

	/* Calculate bandwidth */
	interval = 1 << (ep_desc->bInterval - 1);
	max_packet_size = USB_MPS_EP_SIZE(ep_desc->wMaxPacketSize);

	if (device_speed == USB_SPEED_SPEED_HS) {
		ep_payload_size = USB_MPS_TO_TPL(ep_desc->wMaxPacketSize);
		/*
		 * ep_bandwidth in bytes/sec. High-speed: interval in microframes (125μs), 8000
		 * microframes per second.
		 */
		ep_bandwidth = (ep_payload_size * 8000) / interval;
	} else {
		ep_payload_size = max_packet_size;
		/*
		 * ep_bandwidth in bytes/sec. Full-speed: interval in frames (1ms), 1000 frames
		 * per second
		 */
		ep_bandwidth = (max_packet_size * 1000) / interval;
	}

	LOG_DBG("Iface %u Alt %u EP 0x%02x: mps=%u, payload=%u, interval=%u (%s), bw=%u B/s",
		if_desc->bInterfaceNumber, if_desc->bAlternateSetting, ep_desc->bEndpointAddress,
		max_packet_size, ep_payload_size, interval,
		(device_speed == USB_SPEED_SPEED_HS) ? "uframes" : "frames", ep_bandwidth);

	/* Check if this endpoint meets requirements */
	if (ep_bandwidth >= required_bandwidth && ep_payload_size >= max_payload_transfer_size) {
		return true;
	}

	return false;
}

/* Calculate endpoint bandwidth */
static uint32_t uvc_host_get_endpoint_bandwidth(const struct usb_ep_descriptor *ep_desc,
						enum usb_device_speed device_speed)
{
	uint32_t ep_payload_size;
	uint8_t interval;
	uint16_t max_packet_size;

	interval = 1 << (ep_desc->bInterval - 1);
	max_packet_size = USB_MPS_EP_SIZE(ep_desc->wMaxPacketSize);

	if (device_speed == USB_SPEED_SPEED_HS) {
		ep_payload_size = USB_MPS_TO_TPL(ep_desc->wMaxPacketSize);
		return (ep_payload_size * 8000) / interval;
	} else {
		return (max_packet_size * 1000) / interval;
	}
}

/* Get endpoint payload size */
static uint32_t uvc_host_get_endpoint_payload_size(const struct usb_ep_descriptor *ep_desc,
						   enum usb_device_speed device_speed)
{
	uint16_t max_packet_size = USB_MPS_EP_SIZE(ep_desc->wMaxPacketSize);

	if (device_speed == USB_SPEED_SPEED_HS) {
		return USB_MPS_TO_TPL(ep_desc->wMaxPacketSize);
	} else {
		return max_packet_size;
	}
}

/* Scan endpoints in an interface for suitable bandwidth */
static const struct usb_ep_descriptor *
uvc_host_scan_interface_endpoints(const struct usb_if_descriptor *if_desc,
				  enum usb_device_speed device_speed, uint32_t required_bandwidth,
				  uint32_t max_payload_transfer_size, uint32_t *found_bandwidth)
{
	const struct usb_desc_header *desc;
	const struct usb_ep_descriptor *best_ep = NULL;
	uint32_t best_bandwidth = UINT32_MAX;
	int ep_count = 0;

	LOG_DBG("Checking interface %u alt %u (%u endpoints)", if_desc->bInterfaceNumber,
		if_desc->bAlternateSetting, if_desc->bNumEndpoints);

	/* Iterate through all descriptors following the interface descriptor */
	desc = (const struct usb_desc_header *)if_desc;
	while ((desc = usbh_desc_get_next(desc)) != NULL && ep_count < if_desc->bNumEndpoints) {
		/* Stop if we hit another interface descriptor */
		if (desc->bDescriptorType == USB_DESC_INTERFACE) {
			break;
		}

		/* Process endpoint descriptors */
		if (desc->bDescriptorType == USB_DESC_ENDPOINT) {
			const struct usb_ep_descriptor *ep_desc = (const void *)desc;

			if (uvc_host_ep_has_enough_bandwidth(ep_desc, if_desc, device_speed,
							     required_bandwidth,
							     max_payload_transfer_size)) {
				uint32_t ep_bandwidth =
					uvc_host_get_endpoint_bandwidth(ep_desc, device_speed);

				/* Select endpoint with smallest sufficient bandwidth */
				if (ep_bandwidth < best_bandwidth) {
					best_bandwidth = ep_bandwidth;
					best_ep = ep_desc;
				}
			}

			ep_count++;
		}
	}

	if (best_ep && found_bandwidth) {
		*found_bandwidth = best_bandwidth;
	}

	return best_ep;
}

/* Select streaming alternate setting based on bandwidth */
static int uvc_host_select_streaming_alternate(struct uvc_host_data *const host_data,
					       uint32_t required_bandwidth)
{
	const struct usb_if_descriptor *selected_interface = NULL;
	const struct usb_ep_descriptor *selected_endpoint = NULL;
	struct uvc_stream_iface_info *stream_info;
	uint32_t optimal_bandwidth = UINT32_MAX;
	uint32_t selected_payload_size = 0;
	enum usb_device_speed device_speed = host_data->udev->speed;
	uint32_t max_payload_transfer_size =
		sys_le32_to_cpu(host_data->probe_data.dwMaxPayloadTransferSize);

	stream_info = &host_data->current_stream_iface_info;

	LOG_DBG("Required bandwidth: %u bytes/sec, Max payload: %u bytes (device speed: %s)",
		required_bandwidth, max_payload_transfer_size,
		(device_speed == USB_SPEED_SPEED_HS) ? "High Speed" : "Full Speed");

	/* Scan all streaming interfaces */
	for (uint8_t i = 0;
	     i < CONFIG_USBH_VIDEO_MAX_STREAM_INTERFACE && host_data->stream_ifaces[i]; i++) {
		const struct usb_if_descriptor *if_desc = host_data->stream_ifaces[i];
		const struct usb_ep_descriptor *ep_desc;
		uint32_t ep_bandwidth;

		/* Skip Alt 0 (idle state) */
		if (if_desc->bAlternateSetting == 0) {
			continue;
		}

		ep_desc =
			uvc_host_scan_interface_endpoints(if_desc, device_speed, required_bandwidth,
							  max_payload_transfer_size, &ep_bandwidth);

		if (ep_desc && ep_bandwidth < optimal_bandwidth) {
			optimal_bandwidth = ep_bandwidth;
			selected_interface = if_desc;
			selected_endpoint = ep_desc;
			selected_payload_size =
				uvc_host_get_endpoint_payload_size(ep_desc, device_speed);

			LOG_DBG("Selected optimal EP: iface %u alt %u EP 0x%02x, bw=%u, payload=%u",
				if_desc->bInterfaceNumber, if_desc->bAlternateSetting,
				ep_desc->bEndpointAddress, ep_bandwidth, selected_payload_size);
		}
	}

	if (!selected_endpoint) {
		LOG_ERR("No EP satisfies bandwidth %u and payload size %u", required_bandwidth,
			max_payload_transfer_size);
		return -ENOTSUP;
	}

	stream_info->current_stream_iface = selected_interface;
	stream_info->current_stream_ep = selected_endpoint;
	stream_info->cur_ep_mps_mult = selected_payload_size;

	LOG_DBG("Selected iface %u alt %u EP 0x%02x (bw=%u, payload=%u)",
		selected_interface->bInterfaceNumber, selected_interface->bAlternateSetting,
		selected_endpoint->bEndpointAddress, optimal_bandwidth, selected_payload_size);

	return 0;
}

/* Calculate required bandwidth for current video format */
static uint32_t uvc_host_calculate_required_bandwidth(const struct uvc_format_info *current_format)
{
	uint32_t byte_per_sec;

	if (current_format->frmival_100ns == 0) {
		LOG_ERR("Invalid frmival: %u", current_format->frmival_100ns);
		return 0;
	}

	byte_per_sec = current_format->video_fmt.size *
		       ((NSEC_PER_SEC / 100ULL) / current_format->frmival_100ns);

	/*
	 * For JPEG/H264: apply typical compression ratio
	 * Note: current_format->size already uses worst-case (width * height * 2)
	 * from video_estimate_fmt_size(), but in practice MJPEG compresses 6:1 to 20:1
	 */
	if (current_format->video_fmt.pixelformat == VIDEO_PIX_FMT_JPEG ||
	    current_format->video_fmt.pixelformat == VIDEO_PIX_FMT_H264) {
		byte_per_sec /= UVC_MJPEG_COMPRESSION_RATIO;
	}

	/* Add safety margin for USB bandwidth allocation */
	byte_per_sec = (byte_per_sec * UVC_BANDWIDTH_MARGIN_PERCENT + 99) / 100;

	LOG_DBG("Calculated bandwidth: %u bytes/sec for %s %ux%u@%ufps (frame size: %u bytes)",
		byte_per_sec, VIDEO_FOURCC_TO_STR(current_format->video_fmt.pixelformat),
		current_format->video_fmt.width, current_format->video_fmt.height,
		(NSEC_PER_SEC / 100) / current_format->frmival_100ns,
		current_format->video_fmt.size);

	return byte_per_sec;
}

/* Send VideoStreaming GET request  */
static int uvc_host_vs_get(struct uvc_host_data *const host_data, uint8_t request,
			   uint8_t control_selector, void *data, uint8_t data_len)
{
	const struct usb_if_descriptor *stream_iface;
	uint8_t bmRequestType;
	uint16_t wValue, wIndex;
	struct net_buf *buf = NULL;
	int ret;

	if (data_len == 0) {
		LOG_ERR("Invalid data length: %u", data_len);
		return -EINVAL;
	}

	if (data == NULL) {
		LOG_ERR("Data buffer is NULL");
		return -EINVAL;
	}

	stream_iface = host_data->current_stream_iface_info.current_stream_iface;
	if (stream_iface == NULL) {
		LOG_ERR("Stream interface is NULL");
		return -EINVAL;
	}

	buf = usbh_xfer_buf_alloc(host_data->udev, data_len);
	if (!buf) {
		LOG_ERR("Failed to allocate transfer buffer of size %u", data_len);
		return -ENOMEM;
	}

	bmRequestType = (USB_REQTYPE_DIR_TO_HOST << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |
			(USB_REQTYPE_RECIPIENT_INTERFACE << 0);

	wValue = control_selector << 8;
	wIndex = stream_iface->bInterfaceNumber;

	LOG_DBG("VS GET request: req=0x%02x, cs=0x%02x, len=%u", request, control_selector,
		data_len);

	ret = usbh_req_setup(host_data->udev, bmRequestType, request, wValue, wIndex, data_len,
			     buf);
	if (ret != 0) {
		LOG_ERR("Failed to send VS GET request 0x%02x: %d", request, ret);
		goto cleanup;
	}

	/* Copy received data */
	if (buf->len > 0) {
		size_t copy_len = MIN(buf->len, data_len);

		memcpy(data, buf->data, copy_len);

		if (buf->len != data_len) {
			LOG_WRN("VS GET: expected %u bytes, got %zu bytes", data_len, buf->len);
		}

		LOG_DBG("VS GET received %zu bytes", buf->len);
	} else {
		LOG_WRN("VS GET returned no data");
		ret = -ENODATA;
		goto cleanup;
	}

	ret = 0;

cleanup:
	if (buf) {
		usbh_xfer_buf_free(host_data->udev, buf);
	}

	return ret;
}

/* Send VideoStreaming SET request  */
static int uvc_host_vs_set(struct uvc_host_data *const host_data, uint8_t request,
			   uint8_t control_selector, const void *data, uint8_t data_len)
{
	const struct usb_if_descriptor *stream_iface;
	uint8_t bmRequestType;
	uint16_t wValue, wIndex;
	struct net_buf *buf = NULL;
	int ret;

	if (data_len == 0) {
		LOG_ERR("Invalid data length: %u", data_len);
		return -EINVAL;
	}

	stream_iface = host_data->current_stream_iface_info.current_stream_iface;
	if (stream_iface == NULL) {
		LOG_ERR("Stream interface is NULL");
		return -EINVAL;
	}

	buf = usbh_xfer_buf_alloc(host_data->udev, data_len);
	if (!buf) {
		LOG_ERR("Failed to allocate transfer buffer of size %u", data_len);
		return -ENOMEM;
	}

	bmRequestType = (USB_REQTYPE_DIR_TO_DEVICE << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |
			(USB_REQTYPE_RECIPIENT_INTERFACE << 0);

	if (data) {
		net_buf_add_mem(buf, data, data_len);
	}

	wValue = control_selector << 8;
	wIndex = stream_iface->bInterfaceNumber;

	LOG_DBG("VS SET request: req=0x%02x, cs=0x%02x, len=%u", request, control_selector,
		data_len);

	ret = usbh_req_setup(host_data->udev, bmRequestType, request, wValue, wIndex, data_len,
			     buf);
	if (ret != 0) {
		LOG_ERR("Failed to send VS SET request 0x%02x: %d", request, ret);
		goto cleanup;
	}

	LOG_DBG("Successfully completed VS SET request 0x%02x", request);
	ret = 0;

cleanup:
	if (buf) {
		usbh_xfer_buf_free(host_data->udev, buf);
	}

	return ret;
}

/* Send VideoStreaming request */
static int uvc_host_vs_request(struct uvc_host_data *const host_data, uint8_t request,
			       uint8_t control_selector, void *data, uint8_t data_len)
{
	switch (request) {
	/* SET requests */
	case UVC_SET_CUR:
		return uvc_host_vs_set(host_data, request, control_selector, data, data_len);

	/* GET requests */
	case UVC_GET_CUR:
	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_RES:
	case UVC_GET_LEN:
	case UVC_GET_INFO:
	case UVC_GET_DEF:
		return uvc_host_vs_get(host_data, request, control_selector, data, data_len);

	default:
		LOG_ERR("Unsupported UVC request: 0x%02x", request);
		return -ENOTSUP;
	}
}

/* Set UVC video format and configure streaming */
static int uvc_host_set_format(struct uvc_host_data *const host_data,
			       struct video_format *const fmt)
{
	struct uvc_stream_iface_info *stream_info;
	const struct usb_if_descriptor *stream_iface;
	const struct uvc_format_descriptor *format;
	const struct uvc_frame_descriptor *frame;
	uint32_t frmival;
	uint32_t required_bandwidth;
	int ret;

	stream_info = &host_data->current_stream_iface_info;

	/* Find matching format and frame descriptors */
	ret = uvc_host_find_format(host_data, fmt, &format, &frame, &frmival);
	if (ret != 0) {
		LOG_ERR("Unsupported format: %s %ux%u", VIDEO_FOURCC_TO_STR(fmt->pixelformat),
			fmt->width, fmt->height);
		return ret;
	}

	/* Prepare probe/commit structure */
	memset(&host_data->probe_data, 0, sizeof(host_data->probe_data));
	host_data->probe_data.bmHint = sys_cpu_to_le16(0x0001);
	host_data->probe_data.bFormatIndex = format->bFormatIndex;
	host_data->probe_data.bFrameIndex = frame->bFrameIndex;
	host_data->probe_data.dwFrameInterval = sys_cpu_to_le32(frmival);

	LOG_INF("Setting format: %s %ux%u (format_index=%u, frame_index=%u, interval=%u)",
		VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height,
		format->bFormatIndex, frame->bFrameIndex, frmival);

	/* PROBE SET */
	ret = uvc_host_vs_request(host_data, UVC_SET_CUR, UVC_VS_PROBE_CONTROL,
				  &host_data->probe_data, sizeof(host_data->probe_data));
	if (ret == -EPIPE) {
		LOG_WRN("Request 0x%02x not supported by device, control selector 0x%02x",
			UVC_SET_CUR, UVC_VS_PROBE_CONTROL);
		return ret;
	} else if (ret != 0) {
		LOG_ERR("PROBE SET request failed: %d", ret);
		return ret;
	}

	/* PROBE GET */
	memset(&host_data->probe_data, 0, sizeof(host_data->probe_data));
	ret = uvc_host_vs_request(host_data, UVC_GET_CUR, UVC_VS_PROBE_CONTROL,
				  &host_data->probe_data, sizeof(host_data->probe_data));
	if (ret == -EPIPE) {
		LOG_WRN("Request 0x%02x not supported by device, control selector 0x%02x",
			UVC_GET_CUR, UVC_VS_PROBE_CONTROL);
		return ret;
	} else if (ret != 0) {
		LOG_ERR("PROBE GET request failed: %d", ret);
		return ret;
	}

	/* COMMIT */
	ret = uvc_host_vs_request(host_data, UVC_SET_CUR, UVC_VS_COMMIT_CONTROL,
				  &host_data->probe_data, sizeof(host_data->probe_data));
	if (ret == -EPIPE) {
		LOG_WRN("Request 0x%02x not supported by device, control selector 0x%02x",
			UVC_SET_CUR, UVC_VS_COMMIT_CONTROL);
		return ret;
	} else if (ret != 0) {
		LOG_ERR("COMMIT request failed: %d", ret);
		return ret;
	}

	ret = video_estimate_fmt_size(fmt);
	if (ret != 0) {
		LOG_ERR("Failed to estimate format size: %d", ret);
		return ret;
	}

	/* Update device current format */
	k_mutex_lock(&host_data->lock, K_FOREVER);
	host_data->current_format.video_fmt.pixelformat = fmt->pixelformat;
	host_data->current_format.video_fmt.width = fmt->width;
	host_data->current_format.video_fmt.height = fmt->height;
	host_data->current_format.video_fmt.pitch = fmt->pitch;
	host_data->current_format.video_fmt.size = fmt->size;
	host_data->current_format.frmival_100ns = frmival;
	host_data->current_format.format_index = format->bFormatIndex;
	host_data->current_format.frame_index = frame->bFrameIndex;
	host_data->current_format.format_ptr = format;
	host_data->current_format.frame_ptr = frame;
	k_mutex_unlock(&host_data->lock);

	/* Calculate required bandwidth */
	required_bandwidth = uvc_host_calculate_required_bandwidth(&host_data->current_format);
	if (required_bandwidth == 0) {
		LOG_WRN("Cannot calculate required bandwidth");
		return -EINVAL;
	}

	/* Select streaming interface alternate setting */
	ret = uvc_host_select_streaming_alternate(host_data, required_bandwidth);
	if (ret != 0) {
		LOG_ERR("Select stream alternate failed: %d", ret);
		return ret;
	}

	stream_iface = stream_info->current_stream_iface;

	/* Configure streaming interface */
	ret = usbh_device_interface_set(host_data->udev, stream_iface->bInterfaceNumber,
					stream_iface->bAlternateSetting, false);
	if (ret != 0) {
		LOG_ERR("Failed to set streaming interface %u alternate %u: %d",
			stream_iface->bInterfaceNumber, stream_iface->bAlternateSetting, ret);
		return ret;
	}

	LOG_INF("Set streaming interface %u alternate %u successfully",
		stream_iface->bInterfaceNumber, stream_iface->bAlternateSetting);

	LOG_INF("UVC format set successfully: %s %ux%u@%ufps",
		VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height,
		(NSEC_PER_SEC / 100) / host_data->current_format.frmival_100ns);

	return 0;
}

/* Set UVC device frame rate */
static int uvc_host_set_frame_rate(const struct device *dev, uint32_t fps)
{
	struct uvc_host_data *const host_data = dev->data;
	uint32_t best_frmival = 0;
	uint32_t required_bandwidth = 0;
	uint32_t target_frmival = 0;
	const struct usb_if_descriptor *stream_iface;
	struct video_frmival_enum fie = {0};
	int ret;

	if (fps == 0) {
		return -EINVAL;
	}

	/* target_frmival in 100ns */
	target_frmival = (NSEC_PER_SEC / 100) / fps;

	k_mutex_lock(&host_data->lock, K_FOREVER);

	if (host_data->current_format.frmival_100ns == target_frmival) {
		LOG_DBG("Frame rate already set to %u fps", fps);
		k_mutex_unlock(&host_data->lock);
		return 0;
	}

	fie.discrete.numerator = target_frmival;
	fie.discrete.denominator = NSEC_PER_SEC / 100;

	k_mutex_unlock(&host_data->lock);

	video_closest_frmival(dev, &fie);

	best_frmival =
		(fie.discrete.numerator * (NSEC_PER_SEC / 100ULL)) / fie.discrete.denominator;

	LOG_DBG("Selected frame interval index: %u, interval: %u (100ns units)", fie.index,
		best_frmival);
	LOG_INF("Setting frame rate: %u fps -> %u fps", fps, (NSEC_PER_SEC / 100) / best_frmival);

	k_mutex_lock(&host_data->lock, K_FOREVER);

	memset(&host_data->probe_data, 0, sizeof(host_data->probe_data));
	host_data->probe_data.bmHint = sys_cpu_to_le16(0x0001);
	host_data->probe_data.bFormatIndex = host_data->current_format.format_index;
	host_data->probe_data.bFrameIndex = host_data->current_format.frame_index;
	host_data->probe_data.dwFrameInterval = sys_cpu_to_le32(best_frmival);

	k_mutex_unlock(&host_data->lock);

	ret = uvc_host_vs_request(host_data, UVC_SET_CUR, UVC_VS_PROBE_CONTROL,
				  &host_data->probe_data, sizeof(host_data->probe_data));
	if (ret != 0) {
		LOG_ERR("PROBE SET request failed: %d", ret);
		return ret;
	}

	memset(&host_data->probe_data, 0, sizeof(host_data->probe_data));
	ret = uvc_host_vs_request(host_data, UVC_GET_CUR, UVC_VS_PROBE_CONTROL,
				  &host_data->probe_data, sizeof(host_data->probe_data));
	if (ret != 0) {
		LOG_ERR("PROBE GET request failed: %d", ret);
		return ret;
	}

	ret = uvc_host_vs_request(host_data, UVC_SET_CUR, UVC_VS_COMMIT_CONTROL,
				  &host_data->probe_data, sizeof(host_data->probe_data));
	if (ret != 0) {
		LOG_ERR("COMMIT request failed: %d", ret);
		return ret;
	}

	k_mutex_lock(&host_data->lock, K_FOREVER);
	host_data->current_format.frmival_100ns = best_frmival;
	k_mutex_unlock(&host_data->lock);

	LOG_INF("Frame rate successfully set to %u fps",
		(NSEC_PER_SEC / 100) / host_data->current_format.frmival_100ns);

	required_bandwidth = uvc_host_calculate_required_bandwidth(&host_data->current_format);
	if (required_bandwidth == 0) {
		LOG_ERR("Cannot calculate required bandwidth");
		return -EINVAL;
	}

	ret = uvc_host_select_streaming_alternate(host_data, required_bandwidth);
	if (ret != 0) {
		LOG_ERR("Failed to select streaming alternate: %d", ret);
		return ret;
	}

	stream_iface = host_data->current_stream_iface_info.current_stream_iface;

	ret = usbh_device_interface_set(host_data->udev, stream_iface->bInterfaceNumber,
					stream_iface->bAlternateSetting, false);
	if (ret != 0) {
		LOG_ERR("Failed to set streaming interface %u alternate %u: %d",
			stream_iface->bInterfaceNumber, stream_iface->bAlternateSetting, ret);
		return ret;
	}

	LOG_INF("Set streaming interface %u alternate %u successfully",
		stream_iface->bInterfaceNumber, stream_iface->bAlternateSetting);

	return 0;
}

/* Parse frame descriptors for a specific format */
static int uvc_host_parse_format_frames(const void *format_ptr, uint8_t num_frames,
					uint32_t pixelformat, uint8_t frame_subtype,
					struct video_format_cap *caps_array, int start_index)
{
	const struct usb_desc_header *head = format_ptr;
	int frames_found = 0;
	int cap_index = start_index;

	/* Skip the format descriptor */
	head = usbh_desc_get_next(head);

	while (head != NULL && frames_found < num_frames) {
		const struct uvc_frame_descriptor *frame_header =
			(const struct uvc_frame_descriptor *)head;

		if (head->bDescriptorType == USB_DESC_CS_INTERFACE &&
		    frame_header->bDescriptorSubtype == frame_subtype) {

			if (head->bLength >= sizeof(struct uvc_frame_descriptor)) {
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
				if (cap_index >= CONFIG_USBH_VIDEO_MAX_CAPS - 1) {
					break;
				}

				frames_found++;
			}
		} else if (head->bDescriptorType == USB_DESC_CS_INTERFACE &&
			   (frame_header->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED ||
			    frame_header->bDescriptorSubtype == UVC_VS_FORMAT_MJPEG)) {
			break;
		}

		head = usbh_desc_get_next(head);
	}

	return cap_index;
}

/* Create video format capabilities from UVC descriptors */
static int uvc_host_create_format_caps(struct uvc_host_data *const host_data)
{
	struct uvc_vs_format_uncompressed_info *uncompressed_info =
		&host_data->formats.format_uncompressed;
	struct uvc_vs_format_mjpeg_info *mjpeg_info = &host_data->formats.format_mjpeg;
	int total_caps = 0;
	int cap_index = 0;

	memset(host_data->format_caps, 0, sizeof(host_data->format_caps));
	host_data->format_caps_count = 0;

	/* Count frames in uncompressed formats */
	for (uint8_t i = 0; i < uncompressed_info->num_uncompressed_formats; i++) {
		const struct uvc_format_uncomp_descriptor *format =
			uncompressed_info->uncompressed_format[i];

		if (format) {
			total_caps += format->bNumFrameDescriptors;
		}
	}

	/* Count frames in MJPEG formats */
	for (uint8_t i = 0; i < mjpeg_info->num_mjpeg_formats; i++) {
		const struct uvc_format_mjpeg_descriptor *format = mjpeg_info->vs_mjpeg_format[i];

		if (format) {
			total_caps += format->bNumFrameDescriptors;
		}
	}

	if (total_caps == 0) {
		LOG_WRN("No supported formats found");
		return -ENOENT;
	}

	if (total_caps > CONFIG_USBH_VIDEO_MAX_CAPS) {
		LOG_WRN("Too many format caps (%d), max supported is %d", total_caps,
			CONFIG_USBH_VIDEO_MAX_CAPS);
		LOG_WRN("Should increase CONFIG_USBH_VIDEO_MAX_CAPS");
		total_caps = CONFIG_USBH_VIDEO_MAX_CAPS;
	}

	/* Process uncompressed formats */
	for (uint8_t i = 0; i < uncompressed_info->num_uncompressed_formats; i++) {
		const struct uvc_format_uncomp_descriptor *format =
			uncompressed_info->uncompressed_format[i];

		if (!format) {
			continue;
		}

		uint32_t pixelformat = uvc_guid_to_fourcc(format->guidFormat);

		if (pixelformat == 0) {
			LOG_WRN("Unsupported GUID format in format index %u", format->bFormatIndex);
			continue;
		}

		cap_index = uvc_host_parse_format_frames(format, format->bNumFrameDescriptors,
							 pixelformat, UVC_VS_FRAME_UNCOMPRESSED,
							 host_data->format_caps, cap_index);

		if (cap_index >= CONFIG_USBH_VIDEO_MAX_CAPS - 1) {
			LOG_WRN("Format caps array full at %d entries, stopping", cap_index);
			goto done;
		}
	}

	/* Process MJPEG formats */
	for (uint8_t i = 0; i < mjpeg_info->num_mjpeg_formats; i++) {
		const struct uvc_format_mjpeg_descriptor *format = mjpeg_info->vs_mjpeg_format[i];

		if (!format) {
			continue;
		}

		cap_index = uvc_host_parse_format_frames(format, format->bNumFrameDescriptors,
							 VIDEO_PIX_FMT_JPEG, UVC_VS_FRAME_MJPEG,
							 host_data->format_caps, cap_index);

		/* Check array bounds */
		if (cap_index >= CONFIG_USBH_VIDEO_MAX_CAPS - 1) {
			LOG_WRN("Format caps array full at %d entries, stopping", cap_index);
			goto done;
		}
	}

done:
	host_data->format_caps_count = cap_index;

	LOG_INF("Created %d format capabilities based on UVC descriptors", cap_index);
	return 0;
}

/* Get UVC device capabilities */
static int uvc_host_get_device_caps(struct uvc_host_data *const host_data,
				    struct video_caps *const caps)
{
	int ret = 0;

	k_mutex_lock(&host_data->lock, K_FOREVER);

	caps->min_vbuf_count = 1;

	/* Check if format caps already created */
	if (host_data->format_caps_count > 0) {
		/* Already created, use existing */
		caps->format_caps = host_data->format_caps;
		LOG_DBG("Using existing %d format capabilities", host_data->format_caps_count);
	} else {
		/* First call, create format caps */
		ret = uvc_host_create_format_caps(host_data);
		if (ret != 0) {
			LOG_ERR("Failed to create format capabilities: %d", ret);
			ret = -ENOMEM;
			goto unlock;
		}
		caps->format_caps = host_data->format_caps;
	}

unlock:
	k_mutex_unlock(&host_data->lock);
	return ret;
}

/* UVC host pre-initialization */
static int uvc_host_preinit(const struct device *dev)
{
	struct uvc_host_data *const host_data = dev->data;

	k_fifo_init(&host_data->fifo_in);
	k_fifo_init(&host_data->fifo_out);
	k_mutex_init(&host_data->lock);

	return 0;
}

/* Initiate new video transfer */
static int uvc_host_initiate_transfer(struct uvc_host_data *const host_data,
				      struct video_buffer *const vbuf)
{
	struct uvc_stream_iface_info *stream_info = &host_data->current_stream_iface_info;
	const struct usb_ep_descriptor *stream_ep = stream_info->current_stream_ep;
	struct net_buf *buf;
	struct uhc_transfer *xfer;
	int ret;

	LOG_DBG("Initiating transfer: ep=0x%02x, vbuf=%p", stream_ep->bEndpointAddress, vbuf);

	xfer = usbh_xfer_alloc(host_data->udev, stream_ep->bEndpointAddress,
			       uvc_host_stream_iso_req_cb, host_data);
	if (!xfer) {
		LOG_ERR("Failed to allocate transfer");
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(host_data->udev, stream_info->cur_ep_mps_mult);
	if (!buf) {
		LOG_ERR("Failed to allocate buffer");
		usbh_xfer_free(host_data->udev, xfer);
		return -ENOMEM;
	}

	buf->len = 0;
	host_data->vbuf_offset = 0;
	xfer->buf = buf;

	host_data->video_transfer[host_data->video_transfer_count++] = xfer;

	ret = usbh_xfer_enqueue(host_data->udev, xfer);
	if (ret != 0) {
		LOG_ERR("Enqueue failed: ret=%d", ret);
		net_buf_unref(buf);
		usbh_xfer_free(host_data->udev, xfer);
		return ret;
	}

	return 0;
}

/* Continue existing video transfer */
static int uvc_host_continue_transfer(struct uvc_host_data *const host_data,
				      struct uhc_transfer *const xfer, struct video_buffer *vbuf)
{
	struct uvc_stream_iface_info *stream_info = &host_data->current_stream_iface_info;
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(host_data->udev, stream_info->cur_ep_mps_mult);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOMEM;
	}

	buf->len = 0;
	xfer->buf = buf;

	ret = usbh_xfer_enqueue(host_data->udev, xfer);
	if (ret != 0) {
		LOG_ERR("Enqueue failed: ret=%d", ret);
		net_buf_unref(buf);
		return ret;
	}

	return 0;
}

/* ISO transfer completion callback */
static int uvc_host_stream_iso_req_cb(struct usb_device *const dev, struct uhc_transfer *const xfer)
{
	struct uvc_host_data *const host_data = (void *)xfer->priv;
	struct net_buf *buf = xfer->buf;
	struct video_buffer *vbuf = host_data->current_vbuf;
	struct uvc_payload_header *payload_header = NULL;
	uint32_t header_length = 0;
	uint32_t data_size = 0;
	uint32_t presentation_time = 0;
	uint8_t end_frame = 0;
	uint8_t frame_id = 0;

	if (!vbuf) {
		LOG_DBG("No current buffer available, ignoring callback");
		goto cleanup;
	}

	if (!host_data->streaming) {
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

	payload_header = (void *)buf->data;
	end_frame = payload_header->bmHeaderInfo & UVC_BMHEADERINFO_END_OF_FRAME;
	frame_id = payload_header->bmHeaderInfo & UVC_BMHEADERINFO_FRAMEID;
	presentation_time = sys_get_le32((uint8_t *)&payload_header->dwPresentationTime);
	header_length = payload_header->bHeaderLength;

	if (buf->len <= UVC_PAYLOAD_HEADER_MIN_SIZE) {
		goto cleanup;
	}

	data_size = buf->len - header_length;

	if (data_size > 0) {
		if (vbuf->bytesused == 0U) {
			if (host_data->current_frame_timestamp != presentation_time) {
				host_data->current_frame_timestamp = presentation_time;
			}
		} else if (presentation_time != host_data->current_frame_timestamp) {
			host_data->save_picture = 0;
		}

		if (host_data->save_picture && vbuf) {
			if (data_size > (vbuf->size - vbuf->bytesused)) {
				LOG_WRN("Buffer overflow: used=%u, payload=%u, capacity=%u",
					vbuf->bytesused, data_size, vbuf->size);
				host_data->save_picture = 0;
				vbuf->bytesused = 0U;
				host_data->vbuf_offset = 0;
			} else if (frame_id == host_data->expect_frame_id) {
				memcpy((void *)(((uint8_t *)vbuf->buffer) + vbuf->bytesused),
				       (void *)(((uint8_t *)buf->data) + header_length), data_size);
				vbuf->bytesused += data_size;
				host_data->vbuf_offset = vbuf->bytesused;

				LOG_DBG("Processed %u payload bytes (FID:%u), total: %u, EOF: %u",
					data_size, frame_id, vbuf->bytesused, end_frame);
			} else {
				host_data->save_picture = 0;
				LOG_DBG("Frame ID mismatch: expected %u, got %u - discarding",
					host_data->expect_frame_id, frame_id);
			}
		}
	}

	/* Handle end of frame */
	if (end_frame) {
		if (host_data->save_picture) {
			if (vbuf && vbuf->bytesused != 0) {
				LOG_DBG("Frame completed: %u bytes (FID: %u)", vbuf->bytesused,
					frame_id);

				k_mutex_lock(&host_data->lock, K_FOREVER);

				if (!host_data->streaming) {
					k_mutex_unlock(&host_data->lock);
					goto cleanup;
				}

				if (host_data->current_vbuf != vbuf) {
					LOG_DBG("Buffer %p already processed by another callback",
						vbuf);
					k_mutex_unlock(&host_data->lock);
					goto cleanup;
				}

				k_fifo_get(&host_data->fifo_in, K_NO_WAIT);
				k_fifo_put(&host_data->fifo_out, vbuf);

				host_data->expect_frame_id = host_data->expect_frame_id ^ 1;
				host_data->save_picture = 1;

				host_data->vbuf_offset = 0;
				host_data->transfer_count = 0;

				LOG_DBG("Raising VIDEO_BUF_DONE signal");
				k_poll_signal_raise(host_data->sig, VIDEO_BUF_DONE);

				vbuf = k_fifo_peek_head(&host_data->fifo_in);
				if (vbuf != NULL) {
					vbuf->bytesused = 0;
					memset(vbuf->buffer, 0, vbuf->size);
					host_data->current_vbuf = vbuf;
				}
				k_mutex_unlock(&host_data->lock);
			}
		} else {
			/* discard the first frame */
			if (host_data->discard_first_frame) {
				host_data->discard_first_frame = 0;
			}

			if (vbuf) {
				if (vbuf->bytesused != 0) {
					host_data->discard_frame_cnt++;
				}
				vbuf->bytesused = 0U;
				host_data->vbuf_offset = 0;
			}

			host_data->expect_frame_id = frame_id ^ 1;
			host_data->save_picture = 1;
		}
	}

cleanup:
	net_buf_unref(buf);
	if (host_data->streaming && vbuf) {
		uvc_host_continue_transfer(host_data, xfer, vbuf);
	}
	return 0;
}

/* Enumerate frame intervals for a given frame */
static int uvc_host_enum_frame_intervals(const struct uvc_frame_descriptor *frame_ptr,
					 struct video_frmival_enum *frmival_enum)
{
	uint8_t interval_type = 0;
	const uint8_t *interval_data = NULL;

	if (!frame_ptr || !frmival_enum) {
		return -EINVAL;
	}

	if (frame_ptr->bLength < UVC_FRAME_DESC_MIN_SIZE_WITH_INTERVAL) {
		LOG_ERR("Frame descriptor too short for interval data");
		return -EINVAL;
	}

	if (frame_ptr->bDescriptorSubtype == UVC_VS_FRAME_FRAME_BASED) {
		const struct uvc_frame_based_continuous_descriptor *fb_desc =
			(const void *)frame_ptr;

		interval_type = fb_desc->bFrameIntervalType;
		interval_data =
			((const uint8_t *)frame_ptr) +
			offsetof(struct uvc_frame_based_continuous_descriptor, bFrameIntervalType) +
			1;
	}

	if (frame_ptr->bDescriptorSubtype == UVC_VS_FRAME_UNCOMPRESSED ||
	    frame_ptr->bDescriptorSubtype == UVC_VS_FRAME_MJPEG) {
		const struct uvc_frame_continuous_descriptor *std_desc = (const void *)frame_ptr;

		interval_type = std_desc->bFrameIntervalType;
		interval_data =
			((const uint8_t *)frame_ptr) +
			offsetof(struct uvc_frame_continuous_descriptor, bFrameIntervalType) + 1;
	}

	LOG_DBG("Enumerating frame intervals: frame_index=%u, interval_type=%u, fie_index=%u",
		frame_ptr->bFrameIndex, interval_type, frmival_enum->index);

	if (interval_type == 0) {
		if (frmival_enum->index > 0) {
			return -EINVAL;
		}

		if (frame_ptr->bLength < UVC_FRAME_DESC_MIN_SIZE_STEPWISE) {
			LOG_ERR("Frame descriptor too short for stepwise intervals");
			return -EINVAL;
		}

		frmival_enum->type = VIDEO_FRMIVAL_TYPE_STEPWISE;
		frmival_enum->stepwise.min.numerator = sys_get_le32(interval_data);
		frmival_enum->stepwise.min.denominator = (NSEC_PER_SEC / 100);
		frmival_enum->stepwise.max.numerator = sys_get_le32(interval_data + 4);
		frmival_enum->stepwise.max.denominator = (NSEC_PER_SEC / 100);
		frmival_enum->stepwise.step.numerator = sys_get_le32(interval_data + 8);
		frmival_enum->stepwise.step.denominator = (NSEC_PER_SEC / 100);

		LOG_DBG("Stepwise intervals: min=%u, max=%u, step=%u (100ns)",
			frmival_enum->stepwise.min.numerator, frmival_enum->stepwise.max.numerator,
			frmival_enum->stepwise.step.numerator);

	} else {
		uint8_t num_intervals = interval_type;

		if (frmival_enum->index >= num_intervals) {
			return -EINVAL;
		}

		if (frame_ptr->bLength <
		    (UVC_FRAME_DESC_MIN_SIZE_WITH_INTERVAL + num_intervals * 4)) {
			LOG_ERR("Frame descriptor too short for %u discrete intervals",
				num_intervals);
			return -EINVAL;
		}

		frmival_enum->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
		frmival_enum->discrete.numerator =
			sys_get_le32(interval_data + frmival_enum->index * 4);
		frmival_enum->discrete.denominator = (NSEC_PER_SEC / 100);

		LOG_DBG("Discrete interval[%u]: %u (100ns units)", frmival_enum->index,
			frmival_enum->discrete.numerator);
	}

	return 0;
}

/* Send VideoControl GET request */
static int uvc_host_vc_get(struct uvc_host_data *const host_data, uint8_t request,
			   uint8_t control_selector, uint8_t entity_id, void *data,
			   uint8_t data_len)
{
	const struct usb_if_descriptor *ctrl_iface;
	struct net_buf *buf;
	uint16_t wValue;
	uint16_t wIndex;
	uint8_t bmRequestType;
	int ret;

	if (data_len == 0 || data == NULL) {
		LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	ctrl_iface = host_data->current_control_interface;
	if (ctrl_iface == NULL) {
		LOG_ERR("Control interface is NULL");
		return -EINVAL;
	}

	buf = usbh_xfer_buf_alloc(host_data->udev, data_len);
	if (!buf) {
		LOG_ERR("Failed to allocate transfer buffer of size %u", data_len);
		return -ENOMEM;
	}

	bmRequestType = (USB_REQTYPE_DIR_TO_HOST << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |
			(USB_REQTYPE_RECIPIENT_INTERFACE << 0);

	if (request >= UVC_GET_CUR_ALL) {
		wValue = 0x0000;
	} else {
		wValue = control_selector << 8;
	}

	wIndex = (entity_id << 8) | ctrl_iface->bInterfaceNumber;

	LOG_DBG("VC GET: req=0x%02x, cs=0x%02x, entity=0x%02x, len=%u", request, control_selector,
		entity_id, data_len);

	ret = usbh_req_setup(host_data->udev, bmRequestType, request, wValue, wIndex, data_len,
			     buf);
	if (ret != 0) {
		goto cleanup;
	}

	if (buf->len > 0) {
		size_t copy_len = MIN(buf->len, data_len);

		memcpy(data, buf->data, copy_len);

		if (buf->len != data_len) {
			LOG_WRN("VC GET: expected %u bytes, got %zu bytes", data_len, buf->len);
		}
	}

	ret = 0;

cleanup:
	if (buf) {
		usbh_xfer_buf_free(host_data->udev, buf);
	}
	return ret;
}

/* Send VideoControl SET request */
static int uvc_host_vc_set(struct uvc_host_data *const host_data, uint8_t request,
			   uint8_t control_selector, uint8_t entity_id, const void *data,
			   uint8_t data_len)
{
	const struct usb_if_descriptor *ctrl_iface;
	struct net_buf *buf;
	uint16_t wValue;
	uint16_t wIndex;
	uint8_t bmRequestType;
	int ret;

	if (data_len == 0) {
		LOG_ERR("Invalid data length: %u", data_len);
		return -EINVAL;
	}

	ctrl_iface = host_data->current_control_interface;
	if (ctrl_iface == NULL) {
		LOG_ERR("Control interface is NULL");
		return -EINVAL;
	}

	buf = usbh_xfer_buf_alloc(host_data->udev, data_len);
	if (!buf) {
		LOG_ERR("Failed to allocate transfer buffer of size %u", data_len);
		return -ENOMEM;
	}

	bmRequestType = (USB_REQTYPE_DIR_TO_DEVICE << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |
			(USB_REQTYPE_RECIPIENT_INTERFACE << 0);

	if (request == UVC_SET_CUR_ALL) {
		wValue = 0x0000;
	} else {
		wValue = control_selector << 8;
	}

	wIndex = (entity_id << 8) | ctrl_iface->bInterfaceNumber;

	if (data) {
		net_buf_add_mem(buf, data, data_len);
	}

	LOG_DBG("VC SET: req=0x%02x, cs=0x%02x, entity=0x%02x, len=%u", request, control_selector,
		entity_id, data_len);

	ret = usbh_req_setup(host_data->udev, bmRequestType, request, wValue, wIndex, data_len,
			     buf);
	if (ret != 0) {
		LOG_ERR("VC SET failed: %d", ret);
	}

	if (buf) {
		usbh_xfer_buf_free(host_data->udev, buf);
	}
	return ret;
}

/* Send VideoControl request */
static int uvc_host_vc_request(struct uvc_host_data *const host_data, uint8_t request,
			       uint8_t control_selector, uint8_t entity_id, void *data,
			       uint8_t data_len)
{
	switch (request) {
	/* SET requests */
	case UVC_SET_CUR:
	case UVC_SET_CUR_ALL:
		return uvc_host_vc_set(host_data, request, control_selector, entity_id, data,
				       data_len);

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
		return uvc_host_vc_get(host_data, request, control_selector, entity_id, data,
				       data_len);

	default:
		LOG_ERR("Unsupported UVC request: 0x%02x", request);
		return -ENOTSUP;
	}
}

/* Check if Processing Unit supports specific control */
static bool uvc_host_pu_supports_control(struct uvc_host_data *const host_data,
					 uint32_t bmcontrol_bit)
{
	uint32_t controls = 0;

	if (!host_data->uvc_descriptors.vc_pud) {
		return false;
	}

	const struct uvc_processing_unit_descriptor *pud = host_data->uvc_descriptors.vc_pud;

	if (pud->bControlSize == 0) {
		return false;
	}

	for (uint8_t i = 0; i < pud->bControlSize && i < 3; i++) {
		controls |= ((uint32_t)pud->bmControls[i]) << (i * 8);
	}

	return (controls & bmcontrol_bit) != 0;
}

/* Check if Camera Terminal supports specific control */
static bool uvc_host_ct_supports_control(struct uvc_host_data *const host_data,
					 uint32_t bmcontrol_bit)
{
	uint32_t controls = 0;

	if (!host_data->uvc_descriptors.vc_ctd) {
		return false;
	}

	const struct uvc_camera_terminal_descriptor *vc_ctd = host_data->uvc_descriptors.vc_ctd;

	if (vc_ctd->bControlSize == 0) {
		return false;
	}

	for (uint8_t i = 0; i < vc_ctd->bControlSize && i < 3; i++) {
		controls |= ((uint32_t)vc_ctd->bmControls[i]) << (i * 8);
	}

	return (controls & bmcontrol_bit) != 0;
}

/* Initialize individual control based on CID */
static int uvc_host_init_control_by_cid(const struct device *dev, uint32_t cid,
					const struct video_ctrl_range *range)
{
	struct uvc_host_data *const host_data = dev->data;
	struct uvc_ctrls *ctrls = &host_data->ctrls;
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

/* Extract control bitmap from bmControls array */
static uint32_t uvc_host_extract_control_bitmap(const uint8_t *bmControls, uint8_t control_size)
{
	uint32_t controls = 0;

	for (uint8_t i = 0; i < control_size; i++) {
		controls |= ((uint32_t)bmControls[i]) << (i * 8);
	}

	return controls;
}

/* Extract control value from data buffer based on size and type */
static int32_t uvc_host_extract_control_value(const uint8_t *data, uint8_t size, uint8_t type)
{
	switch (size) {
	case 1:
		if (type == UVC_CONTROL_SIGNED) {
			return (int32_t)(int8_t)data[0];
		} else {
			return (int32_t)(uint8_t)data[0];
		}

	case 2:
		if (type == UVC_CONTROL_SIGNED) {
			return (int32_t)(int16_t)sys_le16_to_cpu(*(uint16_t *)data);
		} else {
			return (int32_t)(uint16_t)sys_le16_to_cpu(*(uint16_t *)data);
		}

	case 4:
		return (int32_t)sys_le32_to_cpu(*(uint32_t *)data);

	default:
		LOG_ERR("Unsupported control data size: %u", size);
		return 0;
	}
}

/* Query actual control range from UVC device */
static int uvc_host_query_control_range(struct uvc_host_data *const host_data, uint8_t entity_id,
					const struct uvc_control_map *map,
					struct video_ctrl_range *range)
{
	uint8_t data[UVC_MAX_CTRL_SIZE];
	int ret;

	/* Query minimum value */
	ret = uvc_host_vc_request(host_data, UVC_GET_MIN, map->selector, entity_id, data,
				  map->size);
	if (ret != 0) {
		LOG_DBG("Failed to get min value for selector 0x%02x: %d", map->selector, ret);
		return ret;
	}
	range->min = uvc_host_extract_control_value(data, map->size, map->type);

	/* Query maximum value */
	ret = uvc_host_vc_request(host_data, UVC_GET_MAX, map->selector, entity_id, data,
				  map->size);
	if (ret == -EPIPE) {
		LOG_WRN("Request 0x%02x not supported by device for entity %d, control selector "
			"0x%02x",
			UVC_GET_MAX, entity_id, map->selector);
		return ret;
	} else if (ret != 0) {
		LOG_DBG("Failed to get max value for selector 0x%02x: %d", map->selector, ret);
		return ret;
	}
	range->max = uvc_host_extract_control_value(data, map->size, map->type);

	/* Query step/resolution value */
	ret = uvc_host_vc_request(host_data, UVC_GET_RES, map->selector, entity_id, data,
				  map->size);
	if (ret == -EPIPE) {
		LOG_WRN("Request 0x%02x not supported by device for entity %d, control selector "
			"0x%02x",
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
	ret = uvc_host_vc_request(host_data, UVC_GET_DEF, map->selector, entity_id, data,
				  map->size);
	if (ret == -EPIPE) {
		LOG_WRN("Request 0x%02x not supported by device for entity %d, control selector "
			"0x%02x",
			UVC_GET_DEF, entity_id, map->selector);
		return ret;
	} else if (ret != 0) {
		range->def = (range->min + range->max) / 2;
		LOG_DBG("Using middle value as default for selector 0x%02x", map->selector);
	} else {
		range->def = uvc_host_extract_control_value(data, map->size, map->type);
	}

	LOG_DBG("Control 0x%02x range: [%d, %d, %d, %d]", map->selector, range->min, range->max,
		range->step, range->def);

	return 0;
}

/* Initialize controls for a specific UVC unit/terminal */
static int uvc_host_init_unit_controls(struct uvc_host_data *const host_data,
				       const struct device *dev, uint8_t unit_subtype,
				       uint8_t entity_id, uint32_t supported_controls,
				       int *initialized_count)
{
	const struct uvc_control_map *map;
	size_t map_length;
	int ret;

	ret = uvc_get_control_map(unit_subtype, &map, &map_length);
	if (ret != 0) {
		LOG_ERR("Failed to get control map for unit subtype %u: %d", unit_subtype, ret);
		return ret;
	}

	LOG_DBG("Processing %zu controls for unit subtype %u, entity ID %u", map_length,
		unit_subtype, entity_id);

	for (size_t i = 0; i < map_length; i++) {
		const struct uvc_control_map *ctrl_map = &map[i];
		struct video_ctrl_range range;

		if (!(supported_controls & BIT(ctrl_map->bit))) {
			LOG_DBG("Control bit %u not supported by device", ctrl_map->bit);
			continue;
		}

		ret = uvc_host_query_control_range(host_data, entity_id, ctrl_map, &range);
		if (ret != 0) {
			continue;
		}

		ret = uvc_host_init_control_by_cid(dev, ctrl_map->cid, &range);
		if (ret == -ENOTSUP) {
			LOG_WRN("Control cid %u not supported by device", ctrl_map->cid);
		} else if (ret != 0) {
			LOG_WRN("Failed to initialize control CID 0x%08x: %d", ctrl_map->cid, ret);
			continue;
		}

		(*initialized_count)++;
		LOG_DBG("Successfully initialized control CID 0x%08x", ctrl_map->cid);
	}

	return 0;
}

/* Initialize USB UVC controls based on device capabilities */
static int usb_host_camera_init_controls(const struct device *dev)
{
	struct uvc_host_data *host_data = dev->data;
	struct uvc_device_descriptors *desc;
	uint32_t control_bitmap;
	int initialized_count = 0;
	int ret;

	desc = &host_data->uvc_descriptors;

	if (!desc->vc_pud) {
		LOG_WRN("No processing unit found, skipping control initialization");
		return 0;
	}

	/* Initialize Processing Unit controls */
	if (desc->vc_pud) {
		LOG_DBG("Found Processing Unit (ID: %u)", desc->vc_pud->bUnitID);

		control_bitmap = uvc_host_extract_control_bitmap(desc->vc_pud->bmControls,
								 desc->vc_pud->bControlSize);

		ret = uvc_host_init_unit_controls(host_data, dev, UVC_VC_PROCESSING_UNIT,
						  desc->vc_pud->bUnitID, control_bitmap,
						  &initialized_count);
		if (ret != 0) {
			LOG_ERR("Failed to initialize Processing Unit controls: %d", ret);
		}
	} else {
		LOG_WRN("No Processing Unit found, skipping PU controls");
	}

	/* Initialize Camera Terminal controls */
	if (desc->vc_ctd) {
		LOG_DBG("Found Camera Terminal (ID: %u)", desc->vc_ctd->bTerminalID);

		control_bitmap = uvc_host_extract_control_bitmap(desc->vc_ctd->bmControls,
								 desc->vc_ctd->bControlSize);

		ret = uvc_host_init_unit_controls(host_data, dev, UVC_VC_INPUT_TERMINAL,
						  desc->vc_ctd->bTerminalID, control_bitmap,
						  &initialized_count);
		if (ret != 0) {
			LOG_ERR("Failed to initialize Camera Terminal controls: %d", ret);
		}
	} else {
		LOG_DBG("No Camera Terminal found, skipping CT controls");
	}

	/* Initialize Selector Unit controls */
	if (desc->vc_sud) {
		LOG_DBG("Found Selector Unit (ID: %u)", desc->vc_sud->bUnitID);

		control_bitmap = BIT(0);

		ret = uvc_host_init_unit_controls(host_data, dev, UVC_VC_SELECTOR_UNIT,
						  desc->vc_sud->bUnitID, control_bitmap,
						  &initialized_count);
		if (ret != 0) {
			LOG_ERR("Failed to initialize Selector Unit controls: %d", ret);
		}
	}

	/* Initialize Extension Unit controls */
	if (desc->vc_extension_unit) {
		LOG_DBG("Found Extension Unit (ID: %u)", desc->vc_extension_unit->bUnitID);

		control_bitmap = uvc_host_extract_control_bitmap(
			desc->vc_extension_unit->bmControls, desc->vc_extension_unit->bControlSize);

		ret = uvc_host_init_unit_controls(host_data, dev, UVC_VC_EXTENSION_UNIT,
						  desc->vc_extension_unit->bUnitID, control_bitmap,
						  &initialized_count);
		if (ret != 0) {
			LOG_ERR("Failed to initialize Extension Unit controls: %d", ret);
		}
	}

	LOG_INF("Successfully initialized %d UVC controls", initialized_count);
	return 0;
}

/* Initialize UVC host class */
static int uvc_host_init(struct usbh_class_data *const c_data, struct usbh_context *const uhs_ctx)
{
	const struct device *dev = c_data->priv;
	struct uvc_host_data *host_data = (void *)dev->data;

	c_data->uhs_ctx = uhs_ctx;

	LOG_INF("Initializing UVC host data");

	host_data->udev = NULL;
	host_data->connected = false;
	host_data->streaming = false;
	host_data->vbuf_offset = 0;
	host_data->transfer_count = 0;
	host_data->current_vbuf = NULL;
	host_data->save_picture = 0;
	host_data->current_frame_timestamp = 0;

	k_fifo_init(&host_data->fifo_in);
	k_fifo_init(&host_data->fifo_out);

	k_mutex_init(&host_data->lock);

	memset(&host_data->ctrls, 0, sizeof(struct uvc_ctrls));

	for (uint8_t i = 0; i < CONFIG_USBH_VIDEO_MAX_STREAM_INTERFACE; i++) {
		host_data->stream_ifaces[i] = NULL;
	}

	host_data->current_control_interface = NULL;
	memset(&host_data->current_stream_iface_info, 0, sizeof(struct uvc_stream_iface_info));

	host_data->uvc_descriptors.vc_header = NULL;
	host_data->uvc_descriptors.vc_itd = NULL;
	host_data->uvc_descriptors.vc_otd = NULL;
	host_data->uvc_descriptors.vc_ctd = NULL;
	host_data->uvc_descriptors.vc_sud = NULL;
	host_data->uvc_descriptors.vc_pud = NULL;
	host_data->uvc_descriptors.vc_encoding_unit = NULL;
	host_data->uvc_descriptors.vc_extension_unit = NULL;

	host_data->uvc_descriptors.vs_input_header = NULL;

	memset(&host_data->formats, 0, sizeof(struct uvc_format_info_all));
	memset(host_data->format_caps, 0, sizeof(host_data->format_caps));
	host_data->format_caps_count = 0;
	memset(&host_data->current_format, 0, sizeof(struct uvc_format_info));

	host_data->expect_frame_id = UVC_FRAME_ID_INVALID;
	host_data->discard_first_frame = 1;
	host_data->discard_frame_cnt = 0;
	host_data->multi_prime_cnt = CONFIG_USBH_VIDEO_MULTIPLE_PRIME_COUNT;

	LOG_INF("UVC host data initialized successfully");
	return 0;
}

/* Handle UVC device connection */
static int usbh_uvc_probe(struct usbh_class_data *const c_data, struct usb_device *const udev,
			  uint8_t iface)
{
	const struct device *dev = c_data->priv;
	struct uvc_host_data *host_data = (void *)dev->data;
	int ret;

	LOG_INF("UVC device connected");

	if (!udev || udev->state != USB_STATE_CONFIGURED) {
		LOG_ERR("USB device not properly configured");
		return -ENODEV;
	}

	if (!host_data) {
		LOG_ERR("No UVC device instance available");
		return -ENODEV;
	}

	k_mutex_lock(&host_data->lock, K_FOREVER);

	host_data->udev = udev;

	ret = uvc_host_parse_descriptors(c_data, host_data, iface);
	if (ret != 0) {
		LOG_ERR("Failed to parse UVC descriptors: %d", ret);
		goto error_cleanup;
	}

	ret = uvc_host_configure_device(host_data);
	if (ret != 0) {
		LOG_ERR("Failed to configure UVC device: %d", ret);
		goto error_cleanup;
	}

	ret = uvc_host_select_default_format(host_data);
	if (ret != 0) {
		LOG_ERR("Failed to set UVC default format : %d", ret);
		goto error_cleanup;
	}

	ret = usb_host_camera_init_controls(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize USB UVC controls: %d", ret);
		goto error_cleanup;
	}

	host_data->connected = true;

#ifdef CONFIG_POLL
	if (host_data->sig) {
		k_poll_signal_raise(host_data->sig, VIDEO_LINK_UP);
		LOG_DBG("UVC device connected signal raised");
	}
#endif
	k_mutex_unlock(&host_data->lock);

	LOG_INF("UVC device (addr=%d) initialization completed", host_data->udev->addr);
	return 0;

error_cleanup:
	host_data->udev = NULL;
	k_mutex_unlock(&host_data->lock);
	return ret;
}

/* Handle UVC device disconnection */
static int uvc_host_removed(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct uvc_host_data *host_data = (void *)dev->data;
	struct video_buffer *vbuf;
	int ret = 0;

	k_mutex_lock(&host_data->lock, K_FOREVER);

	host_data->streaming = false;
	host_data->connected = false;

	/* Dequeue all active video transfers */
	if (host_data->video_transfer_count > 0) {
		LOG_DBG("Cancelling %u active video transfers", host_data->video_transfer_count);

		for (uint8_t i = 0; i < host_data->video_transfer_count; i++) {
			if (host_data->video_transfer[i]) {
				ret = usbh_xfer_dequeue(host_data->udev,
							host_data->video_transfer[i]);
				if (ret != 0) {
					LOG_ERR("Failed to dequeue video transfer[%d]: %d", i, ret);
				}
				host_data->video_transfer[i] = NULL;
				LOG_DBG("Video transfer[%d] cancelled", i);
			}
		}
		host_data->video_transfer_count = 0;
	}

	while ((vbuf = k_fifo_get(&host_data->fifo_in, K_NO_WAIT)) != NULL) {
		vbuf->bytesused = 0;
		k_fifo_put(&host_data->fifo_out, vbuf);
	}

	host_data->current_vbuf = NULL;
	host_data->vbuf_offset = 0;
	host_data->transfer_count = 0;

	LOG_DBG("Cleaning up UVC controls");
	memset(&host_data->ctrls, 0, sizeof(host_data->ctrls));

	memset(host_data->stream_ifaces, 0, sizeof(host_data->stream_ifaces));
	host_data->current_control_interface = NULL;
	memset(&host_data->current_stream_iface_info, 0,
	       sizeof(host_data->current_stream_iface_info));

	LOG_DBG("Clearing Video Control descriptors");
	host_data->uvc_descriptors.vc_header = NULL;
	host_data->uvc_descriptors.vc_itd = NULL;
	host_data->uvc_descriptors.vc_otd = NULL;
	host_data->uvc_descriptors.vc_ctd = NULL;
	host_data->uvc_descriptors.vc_sud = NULL;
	host_data->uvc_descriptors.vc_pud = NULL;
	host_data->uvc_descriptors.vc_encoding_unit = NULL;
	host_data->uvc_descriptors.vc_extension_unit = NULL;

	LOG_DBG("Clearing Video Streaming descriptors");
	host_data->uvc_descriptors.vs_input_header = NULL;

	memset(&host_data->formats, 0, sizeof(host_data->formats));
	memset(&host_data->current_format, 0, sizeof(host_data->current_format));
	LOG_DBG("Clearing format capabilities");
	memset(host_data->format_caps, 0, sizeof(host_data->format_caps));
	host_data->format_caps_count = 0;
	memset(&host_data->probe_data, 0, sizeof(host_data->probe_data));

	host_data->udev = NULL;

#ifdef CONFIG_POLL
	if (host_data->sig) {
		k_poll_signal_raise(host_data->sig, VIDEO_LINK_DOWN);
		LOG_DBG("UVC device disconnected signal raised");
	}
#endif

	k_mutex_unlock(&host_data->lock);

	LOG_INF("UVC device removal completed");

	return 0;
}

/* Set video format */
static int usbh_uvc_set_fmt(const struct device *dev, struct video_format *const fmt)
{
	struct uvc_host_data *host_data = (void *)dev->data;
	int ret;

	k_mutex_lock(&host_data->lock, K_FOREVER);

	ret = uvc_host_set_format(host_data, fmt);
	if (ret != 0) {
		LOG_ERR("Failed to set UVC format: %d", ret);
		goto unlock;
	}

	ret = video_estimate_fmt_size(fmt);
	if (ret < 0) {
		LOG_ERR("Failed to estimate format size: %d", ret);
		goto unlock;
	}

unlock:
	k_mutex_unlock(&host_data->lock);
	return ret;
}

/* Get current video format */
static int usbh_uvc_get_fmt(const struct device *dev, struct video_format *const fmt)
{
	struct uvc_host_data *host_data = dev->data;

	k_mutex_lock(&host_data->lock, K_FOREVER);

	*fmt = host_data->current_format.video_fmt;

	k_mutex_unlock(&host_data->lock);

	return 0;
}

/* Get device capabilities */
static int usbh_uvc_get_caps(const struct device *dev, struct video_caps *const caps)
{
	struct uvc_host_data *host_data = dev->data;

	return uvc_host_get_device_caps(host_data, caps);
}

/* Set frame interval (frame rate) */
static int usbh_uvc_set_frmival(const struct device *dev, struct video_frmival *const frmival)
{
	struct uvc_host_data *host_data = dev->data;
	uint32_t fps;
	int ret;

	k_mutex_lock(&host_data->lock, K_FOREVER);

	if (!host_data->connected) {
		ret = -ENODEV;
		goto exit;
	}

	if (frmival->numerator == 0 || frmival->denominator == 0) {
		ret = -EINVAL;
		goto exit;
	}

	fps = frmival->denominator / frmival->numerator;

	ret = uvc_host_set_frame_rate(dev, fps);
	if (ret != 0) {
		LOG_ERR("Failed to set UVC frame rate: %d", ret);
	}

exit:
	k_mutex_unlock(&host_data->lock);
	return ret;
}

/* Get current frame interval */
static int usbh_uvc_get_frmival(const struct device *dev, struct video_frmival *const frmival)
{
	struct uvc_host_data *host_data = dev->data;
	uint32_t fps;
	int ret = 0;

	k_mutex_lock(&host_data->lock, K_FOREVER);

	if (!host_data->connected) {
		ret = -ENODEV;
		goto exit;
	}

	if (host_data->current_format.frmival_100ns == 0) {
		LOG_ERR("Invalid current format frmival: %u",
			host_data->current_format.frmival_100ns);
		ret = -EINVAL;
		goto exit;
	}

	fps = (NSEC_PER_SEC / 100) / host_data->current_format.frmival_100ns;
	frmival->numerator = 1;
	frmival->denominator = fps;

	LOG_DBG("Current frame interval: %u/%u (fps=%u)", frmival->numerator, frmival->denominator,
		fps);

exit:
	k_mutex_unlock(&host_data->lock);
	return ret;
}

/* Enumerate supported frame intervals */
static int usbh_uvc_enum_frmival(const struct device *dev, struct video_frmival_enum *frmival_enum)
{
	struct uvc_host_data *host_data = dev->data;
	int ret;

	k_mutex_lock(&host_data->lock, K_FOREVER);

	if (!host_data->connected) {
		ret = -ENODEV;
		goto exit;
	}

	ret = uvc_host_enum_frame_intervals(host_data->current_format.frame_ptr, frmival_enum);
	if (ret != 0) {
		LOG_DBG("Failed to enumerate frame intervals: %d", ret);
	}

exit:
	k_mutex_unlock(&host_data->lock);
	return ret;
}

#ifdef CONFIG_POLL
/* Set poll signal for UVC device events */
static int usbh_uvc_set_signal(const struct device *dev, struct k_poll_signal *sig)
{
	struct uvc_host_data *host_data = dev->data;

	k_mutex_lock(&host_data->lock, K_FOREVER);
	host_data->sig = sig;
	k_mutex_unlock(&host_data->lock);

	LOG_DBG("Signal set for UVC device %p", host_data);

	return 0;
}
#endif

/* Search for control in a specific UVC entity type */
static int uvc_host_search_control_in_entity(uint8_t subtype, uint32_t cid, uint8_t *found_type,
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

/* Find control mapping by CID based on available entities */
static int uvc_host_find_control_mapping(struct uvc_host_data *const host_data, uint32_t cid,
					 uint8_t *entity_subtype,
					 const struct uvc_control_map **map)
{
	int ret;

	/* Search in Processing Unit if it exists */
	if (host_data->uvc_descriptors.vc_pud) {
		ret = uvc_host_search_control_in_entity(UVC_VC_PROCESSING_UNIT, cid, entity_subtype,
							map);
		if (ret == 0) {
			goto exit_find;
		}
		LOG_DBG("Not found control CID 0x%08x in Processing Unit", cid);
	}

	/* Search in Camera Terminal if it exists */
	if (host_data->uvc_descriptors.vc_ctd) {
		ret = uvc_host_search_control_in_entity(UVC_VC_INPUT_TERMINAL, cid, entity_subtype,
							map);
		if (ret == 0) {
			goto exit_find;
		}
		LOG_DBG("Not found control CID 0x%08x in Camera Terminal", cid);
	}

	/* Search in Selector Unit if it exists */
	if (host_data->uvc_descriptors.vc_sud) {
		ret = uvc_host_search_control_in_entity(UVC_VC_SELECTOR_UNIT, cid, entity_subtype,
							map);
		if (ret == 0) {
			goto exit_find;
		}
		LOG_DBG("Not found control CID 0x%08x in Selector Unit", cid);
	}

	/* Search in Extension Unit if it exists */
	if (host_data->uvc_descriptors.vc_extension_unit) {
		ret = uvc_host_search_control_in_entity(UVC_VC_EXTENSION_UNIT, cid, entity_subtype,
							map);
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

/* Get entity ID for a specific unit subtype */
static uint8_t uvc_host_get_entity_id(struct uvc_host_data *const host_data, uint8_t unit_subtype)
{
	struct uvc_device_descriptors *desc = &host_data->uvc_descriptors;

	switch (unit_subtype) {
	case UVC_VC_PROCESSING_UNIT:
		return desc->vc_pud ? desc->vc_pud->bUnitID : 0;

	case UVC_VC_INPUT_TERMINAL:
		return desc->vc_ctd ? desc->vc_ctd->bTerminalID : 0;

	case UVC_VC_SELECTOR_UNIT:
		return desc->vc_sud ? desc->vc_sud->bUnitID : 0;

	case UVC_VC_EXTENSION_UNIT:
		return desc->vc_extension_unit ? desc->vc_extension_unit->bUnitID : 0;

	default:
		LOG_ERR("Unknown unit subtype: %u", unit_subtype);
		return 0;
	}
}

/* Encode control value into data buffer */
static int uvc_host_encode_control_value(struct video_control *ctrl,
					 const struct uvc_control_map *map, uint8_t *data)
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

/* Check if control is supported */
static bool uvc_host_is_control_supported(struct uvc_host_data *const host_data,
					  uint8_t unit_subtype, uint8_t entity_id,
					  uint32_t control_bit)
{
	switch (unit_subtype) {
	case UVC_VC_PROCESSING_UNIT:
		return uvc_host_pu_supports_control(host_data, control_bit);

	case UVC_VC_INPUT_TERMINAL:
		return uvc_host_ct_supports_control(host_data, control_bit);

	case UVC_VC_SELECTOR_UNIT:
		return true;

	case UVC_VC_EXTENSION_UNIT:
		return true;

	default:
		return false;
	}
}

/* Set control value */
static int usbh_uvc_set_ctrl(const struct device *dev, uint32_t id)
{
	struct uvc_host_data *const host_data = dev->data;
	const struct uvc_control_map *map;
	struct video_control control = {.id = id};
	uint8_t unit_subtype;
	uint8_t entity_id;
	uint8_t data[UVC_MAX_CTRL_SIZE] = {0};
	int ret;

	k_mutex_lock(&host_data->lock, K_FOREVER);

	if (!host_data->connected) {
		k_mutex_unlock(&host_data->lock);
		return -ENODEV;
	}

	k_mutex_unlock(&host_data->lock);

	ret = video_get_ctrl(dev, &control);
	if (ret != 0) {
		LOG_ERR("Failed to get control 0x%08x: %d", id, ret);
		return ret;
	}

	ret = uvc_host_find_control_mapping(host_data, id, &unit_subtype, &map);
	if (ret != 0) {
		LOG_ERR("Control 0x%08x not found in mapping", id);
		return -EINVAL;
	}

	entity_id = uvc_host_get_entity_id(host_data, unit_subtype);
	if (entity_id == 0) {
		LOG_ERR("Entity not found for subtype %u", unit_subtype);
		return -ENODEV;
	}

	if (!uvc_host_is_control_supported(host_data, unit_subtype, entity_id, BIT(map->bit))) {
		LOG_WRN("Control 0x%08x not supported", id);
		return -ENOTSUP;
	}

	ret = uvc_host_encode_control_value(&control, map, data);
	if (ret != 0) {
		LOG_ERR("Failed to encode control value: %d", ret);
		return ret;
	}

	ret = uvc_host_vc_request(host_data, UVC_SET_CUR, map->selector, entity_id, data,
				  map->size);
	if (ret != 0) {
		LOG_ERR("Failed to set control 0x%08x: %d", id, ret);
		return ret;
	}

	return 0;
}

/* Generic getter for UVC control values */
static int uvc_host_get_control_value(struct uvc_host_data *const host_data, uint32_t cid,
				      int32_t *value)
{
	const struct uvc_control_map *map;
	uint8_t unit_subtype;
	uint8_t entity_id;
	uint8_t data[UVC_MAX_CTRL_SIZE] = {0};
	int ret;

	ret = uvc_host_find_control_mapping(host_data, cid, &unit_subtype, &map);
	if (ret != 0) {
		LOG_DBG("Control 0x%08x mapping not found", cid);
		return ret;
	}

	entity_id = uvc_host_get_entity_id(host_data, unit_subtype);
	if (entity_id == 0) {
		LOG_ERR("Entity not found for control 0x%08x", cid);
		return -ENODEV;
	}

	if (!uvc_host_is_control_supported(host_data, unit_subtype, entity_id, BIT(map->bit))) {
		LOG_DBG("Control 0x%08x not supported", cid);
		return -ENOTSUP;
	}

	ret = uvc_host_vc_request(host_data, UVC_GET_CUR, map->selector, entity_id, data,
				  map->size);
	if (ret != 0) {
		LOG_ERR("Failed to get control 0x%08x: %d", cid, ret);
		return ret;
	}

	switch (map->size) {
	case 1:
		if (map->type == UVC_CONTROL_SIGNED) {
			*value = (int32_t)(int8_t)data[0];
		} else {
			*value = (int32_t)(uint8_t)data[0];
		}
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

/* Get volatile control values */
static int usbh_uvc_get_volatile_ctrl(const struct device *dev, uint32_t id)
{
	struct uvc_host_data *const host_data = dev->data;
	struct uvc_ctrls *ctrls = &host_data->ctrls;
	int ret;

	switch (id) {
	case VIDEO_CID_AUTOGAIN:
		ret = uvc_host_get_control_value(host_data, VIDEO_CID_GAIN, &ctrls->gain.val);
		if (ret != 0) {
			LOG_ERR("Failed to get gain: %d", ret);
			return ret;
		}
		break;

	case VIDEO_CID_EXPOSURE_AUTO:
		ret = uvc_host_get_control_value(host_data, VIDEO_CID_EXPOSURE_AUTO,
						 &ctrls->auto_exposure.val);
		if (ret != 0) {
			LOG_DBG("Failed to get exposure auto: %d", ret);
			return ret;
		}
		break;

	case VIDEO_CID_AUTO_WHITE_BALANCE:
		ret = uvc_host_get_control_value(host_data, VIDEO_CID_AUTO_WHITE_BALANCE,
						 &ctrls->auto_white_balance_temperature.val);
		if (ret != 0) {
			LOG_DBG("Failed to get white balance auto: %d", ret);
			return ret;
		}
		break;

	case VIDEO_CID_EXPOSURE_ABSOLUTE:
		ret = uvc_host_get_control_value(host_data, VIDEO_CID_EXPOSURE_ABSOLUTE,
						 &ctrls->exposure_absolute.val);
		if (ret != 0) {
			LOG_DBG("Failed to get exposure absolute: %d", ret);
			return ret;
		}
		break;

	case VIDEO_CID_WHITE_BALANCE_TEMPERATURE:
		ret = uvc_host_get_control_value(host_data, VIDEO_CID_WHITE_BALANCE_TEMPERATURE,
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

/* Start/stop streaming */
static int usbh_uvc_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct uvc_host_data *const host_data = dev->data;
	struct uvc_stream_iface_info *stream_info = &host_data->current_stream_iface_info;
	const struct usb_if_descriptor *stream_iface = stream_info->current_stream_iface;
	struct video_buffer *vbuf;
	uint8_t alt;
	uint8_t interface_num;
	int ret;

	k_mutex_lock(&host_data->lock, K_FOREVER);

	if (!host_data->connected) {
		k_mutex_unlock(&host_data->lock);
		return -ENODEV;
	}

	k_mutex_unlock(&host_data->lock);

	if (enable) {
		if (!stream_iface) {
			LOG_ERR("No streaming interface configured");
			return -EINVAL;
		}
		alt = stream_iface->bAlternateSetting;
		interface_num = stream_iface->bInterfaceNumber;
	} else {
		if (!stream_iface) {
			LOG_WRN("No interface configured, cannot disable streaming");
			return -EINVAL;
		}

		alt = 0;
		interface_num = stream_iface->bInterfaceNumber;
		host_data->streaming = false;

		/* Cancel all active transfers */
		if (host_data->video_transfer_count > 0) {
			LOG_DBG("Stopping streaming, cancelling %u transfers",
				host_data->video_transfer_count);

			for (uint8_t i = 0; i < host_data->video_transfer_count; i++) {
				if (!host_data->video_transfer[i]) {
					continue;
				}

				ret = usbh_xfer_dequeue(host_data->udev,
							host_data->video_transfer[i]);
				if (ret != 0) {
					LOG_ERR("Failed to dequeue transfer[%d]: %d", i, ret);
				}
				host_data->video_transfer[i] = NULL;
			}
			host_data->video_transfer_count = 0;
		}
	}

	ret = usbh_device_interface_set(host_data->udev, interface_num, alt, false);
	if (ret != 0) {
		LOG_ERR("Failed to set interface %d alt setting %d: %d", interface_num, alt, ret);
		return ret;
	}

	host_data->streaming = enable;

	if (enable) {
		host_data->streaming = true;
		vbuf = k_fifo_peek_head(&host_data->fifo_in);
		if (vbuf != NULL) {
			vbuf->bytesused = 0;
			memset(vbuf->buffer, 0, vbuf->size);
			host_data->current_vbuf = vbuf;
			host_data->multi_prime_cnt = CONFIG_USBH_VIDEO_MULTIPLE_PRIME_COUNT;
			while (host_data->multi_prime_cnt > 0) {
				ret = uvc_host_initiate_transfer(host_data, vbuf);
				if (ret != 0) {
					LOG_ERR("Failed to initiate transfer: %d", ret);
					goto err_stream;
				}
				host_data->multi_prime_cnt--;
			}
		}
	}

	LOG_DBG("UVC streaming %s successfully", enable ? "enabled" : "disabled");
	return 0;

err_stream:
	host_data->streaming = false;
	for (uint8_t i = 0; i < host_data->video_transfer_count; i++) {
		if (host_data->video_transfer[i]) {
			usbh_xfer_dequeue(host_data->udev, host_data->video_transfer[i]);
			host_data->video_transfer[i] = NULL;
		}
	}
	host_data->video_transfer_count = 0;
	usbh_device_interface_set(host_data->udev, interface_num, 0, false);
	return ret;
}

/* Enqueue video buffer */
static int usbh_uvc_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct uvc_host_data *const host_data = dev->data;

	k_mutex_lock(&host_data->lock, K_FOREVER);

	if (!host_data->connected) {
		k_mutex_unlock(&host_data->lock);
		return -ENODEV;
	}

	k_mutex_unlock(&host_data->lock);

	vbuf->bytesused = 0;
	vbuf->timestamp = 0;
	vbuf->line_offset = 0;

	k_fifo_put(&host_data->fifo_in, vbuf);

	return 0;
}

/* Dequeue completed video buffer */
static int usbh_uvc_dequeue(const struct device *dev, struct video_buffer **vbuf,
			    k_timeout_t timeout)
{
	struct uvc_host_data *const host_data = dev->data;

	*vbuf = k_fifo_get(&host_data->fifo_out, timeout);
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

#define USBH_VIDEO_DEVICE_DEFINE(n, _)                                                             \
	static struct uvc_host_data uvc_host_data##n;                                              \
	DEVICE_DEFINE(usbh_uvc_##n, "usbh_uvc_" #n, uvc_host_preinit, NULL, &uvc_host_data##n,     \
		      NULL, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &uvc_host_video_api);         \
	USBH_DEFINE_CLASS(uvc_host_c_data_##n, &uvc_host_class_api,                                \
			  (void *)DEVICE_GET(usbh_uvc_##n), usbh_uvc_filters);                     \
	VIDEO_DEVICE_DEFINE(usb_video_##n, DEVICE_GET(usbh_uvc_##n), NULL);

LISTIFY(CONFIG_USBH_UVC_INSTANCES_COUNT, USBH_VIDEO_DEVICE_DEFINE, (;), _)
