/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Video Class host driver header
 *
 */

#ifndef ZEPHYR_INCLUDE_USBH_CLASS_UVC_H_
#define ZEPHYR_INCLUDE_USBH_CLASS_UVC_H_

#include <zephyr/usb/usb_ch9.h>

struct uvc_stream_iface_info {
	struct usb_if_descriptor *current_stream_iface;	 /* Stream interface */
	struct usb_ep_descriptor *current_stream_ep;		/* Stream endpoint */
	uint32_t cur_ep_mps_mult;						  /* Endpoint max packet size including multiple transactions */
};

/* UVC format structure */
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

/* Video stream format information structure */
struct uvc_format_info_all {
	struct uvc_vs_format_mjpeg_info format_mjpeg;
	struct uvc_vs_format_uncompressed_info format_uncompressed;
};

#endif /* ZEPHYR_INCLUDE_USBH_CLASS_UVC_H_ */
