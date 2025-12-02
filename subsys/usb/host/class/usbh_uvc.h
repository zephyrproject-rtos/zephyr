/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBH_CLASS_UVC_H_
#define ZEPHYR_INCLUDE_USBH_CLASS_UVC_H_

#include <zephyr/usb/usb_ch9.h>

#define UVC_FRAME_ID_INVALID                    0xFF
#define UVC_PAYLOAD_HEADER_MIN_SIZE             11
#define UVC_DEFAULT_FRAME_INTERVAL              333333
#define UVC_FRAME_DESC_MIN_SIZE_WITH_INTERVAL   26
#define UVC_FRAME_DESC_MIN_SIZE_STEPWISE        38
#define UVC_BANDWIDTH_MARGIN_PERCENT            110
#define UVC_MJPEG_COMPRESSION_RATIO             6


struct uvc_stream_iface_info {
	struct usb_if_descriptor *current_stream_iface;
	struct usb_ep_descriptor *current_stream_ep;
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
	struct uvc_format_header *format_ptr;
	struct uvc_frame_header *frame_ptr;
};

struct uvc_vs_format_mjpeg_info {
	struct uvc_vs_format_mjpeg *vs_mjpeg_format[CONFIG_USBH_VIDEO_MAX_FORMATS];
	uint8_t num_mjpeg_formats;
};

struct uvc_vs_format_uncompressed_info {
	struct uvc_vs_format_uncompressed *uncompressed_format[CONFIG_USBH_VIDEO_MAX_FORMATS];
	uint8_t num_uncompressed_formats;
};

struct uvc_format_info_all {
	struct uvc_vs_format_mjpeg_info format_mjpeg;
	struct uvc_vs_format_uncompressed_info format_uncompressed;
};

#endif /* ZEPHYR_INCLUDE_USBH_CLASS_UVC_H_ */
