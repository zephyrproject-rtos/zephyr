/*
 * Copyright 2025 NXP
 *
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

#include "usbh_device.h"
#include "usbh_ch9.h"
#include "usbh_uvc.h"

#include "../../../drivers/video/video_ctrls.h"
#include "../../../drivers/video/video_device.h"

LOG_MODULE_REGISTER(usbh_uvc, CONFIG_USBH_VIDEO_LOG_LEVEL);

NET_BUF_POOL_VAR_DEFINE(uvc_host_pool, CONFIG_USBH_VIDEO_NUM_BUFS, 0, 4, NULL);

/**
 * @brief UVC device code table for matching UVC devices
 *
 * This table defines the device matching criteria for USB Video Class (UVC) devices.
 * It includes specific device entries and generic interface matching rules.
 */
static const struct usbh_device_code_table uvc_device_code[] = {
	/* Intel D435i depth camera - specific device match */
	{
		.match_type					= USBH_MATCH_DEVICE,
		.vid						= 0x8086,
		.pid						= 0x0b3a,
		.interface_class_code		= UVC_SC_VIDEOCLASS,
		.interface_subclass_code	= UVC_SC_VIDEOCONTROL,
		.interface_protocol_code	= 0,
	},
	/* Generic UVC video control interface match */
	{
		.match_type					= USBH_MATCH_INTFACE,
		.interface_class_code		= UVC_SC_VIDEOCLASS,
		.interface_subclass_code	= UVC_SC_VIDEOCONTROL,
		.interface_protocol_code	= 0,
	}
};

/**
 * @brief USB UVC camera control parameters structure
 *
 * This structure defines all the video control parameters supported by
 * USB UVC (USB Video Class) devices. Each control is represented by
 * a video_ctrl structure that contains the control's current value,
 * range, and capabilities.
 */
struct usb_camera_ctrls {
	/** Automatic gain control enable/disable */
	struct video_ctrl auto_gain;
	/** Manual gain value adjustment */
	struct video_ctrl gain;
	/** Automatic exposure control mode */
	struct video_ctrl auto_exposure;
	/** Manual exposure time in absolute units */
	struct video_ctrl exposure_absolute;
	/** Image brightness level adjustment */
	struct video_ctrl brightness;
	/** Image contrast level adjustment */
	struct video_ctrl contrast;
	/** Color hue adjustment */
	struct video_ctrl hue;
	/** Color saturation level adjustment */
	struct video_ctrl saturation;
	/** Image sharpness adjustment */
	struct video_ctrl sharpness;
	/** Gamma correction value */
	struct video_ctrl gamma;
	/** White balance temperature setting */
	struct video_ctrl white_balance_temperature;
	/** Automatic white balance enable/disable */
	struct video_ctrl auto_white_balance_temperature;
	/** Backlight compensation level */
	struct video_ctrl backlight_compensation;
	/** Automatic focus enable/disable */
	struct video_ctrl auto_focus;
	/** Manual focus position in absolute units */
	struct video_ctrl focus_absolute;
	/** Power line frequency compensation */
	struct video_ctrl light_freq;
	/** Test pattern generation control */
	struct video_ctrl test_pattern;
	/** Pixel clock rate control */
	struct video_ctrl pixel_rate;
};

struct uvc_device {
	/** Associated USB device */
	struct usb_device *udev;
	/** Start address of descriptors belonging to this uvc class */
	void *desc_start;
	/** End address of descriptors belonging to  this uvc class */
	void *desc_end;
	/** Device access synchronization */
	struct k_mutex lock;
	/** Input buffers to which enqueued video buffers land */
	struct k_fifo fifo_in;
	/** Output buffers from which dequeued buffers are picked */
	struct k_fifo fifo_out;
	/** Device connection status */
	bool connected;
	/** Device streaming status */
	bool streaming;
	/** Signal to alert video devices of buffer-related events */
	struct k_poll_signal *sig;
	/** Byte offset within the currently transmitted video buffer */
	size_t vbuf_offset;
	/** Number of completed transfers for current frame */
	size_t transfer_count;
	/** USB camera control parameters */
	struct usb_camera_ctrls ctrls;
	/** Collection of all available alternate streaming interfaces */
	struct usb_if_descriptor *stream_ifaces[UVC_STREAM_INTERFACES_MAX_ALT];
	/** Currently active VideoControl interface */
	struct usb_if_descriptor *current_control_interface;
	/** Information about current streaming interface */
	struct uvc_stream_iface_info current_stream_iface_info;

    /** Video Control Header descriptor from device */
	struct uvc_vc_header_descriptor *vc_header;
	/** Video Control Input Terminal descriptor from device */
	struct uvc_vc_input_terminal_descriptor *vc_itd;
	/** Video Control Output Terminal descriptor from device */
	struct uvc_vc_output_terminal_descriptor *vc_otd;
	/** Video Control Camera Terminal descriptor from device */
	struct uvc_vc_camera_terminal_descriptor *vc_ctd;
	/** Video Control Selector Unit descriptor from device */
	struct uvc_vc_selector_unit_descriptor *vc_sud;
	/** Video Control Processing Unit descriptor from device */
	struct uvc_vc_processing_unit_descriptor *vc_pud;
	/** Video Control Encoding Unit descriptor from device */
	struct uvc_vc_encoding_unit_descriptor *vc_encoding_unit;
	/** Video Control Extension Unit descriptor from device */
	struct uvc_vc_processing_unit_descriptor *vc_extension_unit;

	/** Video Stream Input Header descriptor from device */
	struct uvc_vs_input_header_descriptor *vs_input_header;
	/** Video Stream Output Header descriptor from device */
	struct uvc_vs_output_header_descriptor *vs_output_header;
	/** Available format groups parsed from descriptors */
	struct uvc_vs_format_info formats;
	/** Currently selected video format */
	struct uvc_vs_format current_format;
	/** Device-supported format capabilities for video API */
	struct video_format_cap* video_format_caps;
	/** UVC probe/commit buffer */
    struct uvc_probe_commit video_probe;
};

static int uvc_host_flush_queue(struct uvc_device *uvc_dev, struct video_buffer *vbuf);

/**
 * @brief Select default video format for UVC device
 *
 * Attempts to find and configure a default video format by first trying
 * uncompressed formats, then falling back to MJPEG if needed.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_select_default_format(struct uvc_device *uvc_dev)
{
	if (!uvc_dev) {
		return -EINVAL;
	}

	struct uvc_vs_format_uncompressed_info *uncompressed_info = &uvc_dev->formats.format_uncompressed;
	struct uvc_vs_format_mjpeg_info *mjpeg_info = &uvc_dev->formats.format_mjpeg;

	/* Try uncompressed formats first */
	if (uncompressed_info->num_uncompressed_formats > 0 &&
		uncompressed_info->uncompressed_format[0]) {

		struct uvc_vs_format_uncompressed *format = uncompressed_info->uncompressed_format[0];

		/* Get pixel format from GUID */
		uint32_t pixelformat = uvc_guid_to_fourcc(format->guidFormat);
		if (pixelformat == 0) {
			LOG_WRN("First uncompressed format has unsupported GUID");
			goto try_mjpeg;
		}

		/* Find first frame descriptor */
		uint8_t *desc_buf = (uint8_t *)format + format->bLength;

		while (desc_buf) {
			struct uvc_frame_header *frame_header = (struct uvc_frame_header *)desc_buf;

			if (frame_header->bLength == 0) break;

			if (frame_header->bDescriptorType == UVC_CS_INTERFACE &&
				frame_header->bDescriptorSubType == UVC_VS_FRAME_UNCOMPRESSED) {

				if (frame_header->bLength >= sizeof(struct uvc_frame_header)) {
					uint16_t width = sys_le16_to_cpu(frame_header->wWidth);
					uint16_t height = sys_le16_to_cpu(frame_header->wHeight);

					/* dwFrameInterval is at fixed offset 26 bytes for uncompressed frames */
					uint32_t frame_interval = 0;
					if (frame_header->bLength >= 30) { /* Ensure sufficient space for dwFrameInterval */
						uint8_t *interval_ptr = desc_buf + 26; /* dwFrameInterval offset */
						frame_interval = sys_le32_to_cpu(*(uint32_t*)interval_ptr);
					}

					/* Configure default format parameters */
					uvc_dev->current_format.pixelformat = pixelformat;
					uvc_dev->current_format.width = width;
					uvc_dev->current_format.height = height;
					uvc_dev->current_format.format_index = format->bFormatIndex;
					uvc_dev->current_format.frame_index = frame_header->bFrameIndex;
					uvc_dev->current_format.frame_interval = frame_interval;
					uvc_dev->current_format.format_ptr = (struct uvc_format_header *)format;
					uvc_dev->current_format.frame_ptr = frame_header;

					/* Calculate FPS (frame_interval is in 100ns units) */
					if (frame_interval > 0) {
						uvc_dev->current_format.fps = 10000000 / frame_interval;
					} else {
						uvc_dev->current_format.fps = 30; /* Default 30fps */
					}

					/* Calculate pitch (bytes per line) */
					uvc_dev->current_format.pitch = width * video_bits_per_pixel(pixelformat) / 8;

					LOG_INF("Set default format: %s %ux%u@%ufps (format_idx=%u, frame_idx=%u)",
							VIDEO_FOURCC_TO_STR(pixelformat),
							width, height, uvc_dev->current_format.fps,
							format->bFormatIndex, frame_header->bFrameIndex);
					return 0;
				}
			}

			desc_buf += frame_header->bLength;
		}
	}

try_mjpeg:
	/* Try MJPEG format if uncompressed format is not available */
	if (mjpeg_info->num_mjpeg_formats > 0 &&
		mjpeg_info->vs_mjpeg_format[0]) {

		struct uvc_vs_format_mjpeg *format = mjpeg_info->vs_mjpeg_format[0];

		/* Find first MJPEG frame descriptor */
		uint8_t *desc_buf = (uint8_t *)format + format->bLength;

		while (desc_buf) {
			struct uvc_frame_header *frame_header = (struct uvc_frame_header *)desc_buf;

			if (frame_header->bLength == 0) break;

			if (frame_header->bDescriptorType == UVC_CS_INTERFACE &&
				frame_header->bDescriptorSubType == UVC_VS_FRAME_MJPEG) {

				if (frame_header->bLength >= sizeof(struct uvc_frame_header)) {
					uint16_t width = sys_le16_to_cpu(frame_header->wWidth);
					uint16_t height = sys_le16_to_cpu(frame_header->wHeight);

					/* dwFrameInterval is also at offset 26 bytes for MJPEG frames */
					uint32_t frame_interval = 0;
					if (frame_header->bLength >= 30) { /* Ensure sufficient space for dwFrameInterval */
						uint8_t *interval_ptr = desc_buf + 26; /* dwFrameInterval offset */
						frame_interval = sys_le32_to_cpu(*(uint32_t*)interval_ptr);
					}

					/* Configure default MJPEG format */
					uvc_dev->current_format.pixelformat = VIDEO_PIX_FMT_MJPEG;
					uvc_dev->current_format.width = width;
					uvc_dev->current_format.height = height;
					uvc_dev->current_format.format_index = format->bFormatIndex;
					uvc_dev->current_format.frame_index = frame_header->bFrameIndex;
					uvc_dev->current_format.frame_interval = frame_interval;
					uvc_dev->current_format.format_ptr = (struct uvc_format_header *)format;
					uvc_dev->current_format.frame_ptr = frame_header;
					/* Calculate FPS */
					if (frame_interval > 0) {
						uvc_dev->current_format.fps = 10000000 / frame_interval;
					} else {
						uvc_dev->current_format.fps = 30; /* Default 30fps */
					}

					/* MJPEG pitch calculation (compressed format typically uses width) */
					uvc_dev->current_format.pitch = width;


					LOG_INF("Set default format: MJPEG %ux%u@%ufps (format_idx=%u, frame_idx=%u)",
							width, height, uvc_dev->current_format.fps,
							format->bFormatIndex, frame_header->bFrameIndex);
					return 0;
				}
			}

			desc_buf += frame_header->bLength;
		}
	}

	LOG_ERR("No valid format/frame descriptors found");
	return -ENOTSUP;
}

/**
 * @brief Check if Processing Unit supports specific control
 * @param uvc_dev UVC device
 * @param bmcontrol_bit Control bit mask to check (UVC_PU_BMCONTROL_*)
 * @return true if supported, false otherwise
 */
static bool uvc_host_pu_supports_control(struct uvc_device *uvc_dev, uint32_t bmcontrol_bit)
{
	if (!uvc_dev || !uvc_dev->vc_pud) {
		return false;
	}

	struct uvc_vc_processing_unit_descriptor *pud = uvc_dev->vc_pud;

	if (pud->bControlSize == 0) {
		return false;
	}

	/* Convert the bmControls array to a 32-bit value for easier bit checking */
	uint32_t controls = 0;
	for (int i = 0; i < pud->bControlSize && i < 4; i++) {
		controls |= ((uint32_t)pud->bmControls[i]) << (i * 8);
	}

	return (controls & bmcontrol_bit) != 0;
}

/**
 * @brief Check if Camera Terminal supports specific control
 * @param uvc_dev UVC device
 * @param bmcontrol_bit Control bit mask to check (UVC_CT_BMCONTROL_*)
 * @return true if supported, false otherwise
 */
static bool uvc_host_ct_supports_control(struct uvc_device *uvc_dev, uint32_t bmcontrol_bit)
{
	if (!uvc_dev || !uvc_dev->vc_ctd) {
		return false;
	}

	struct uvc_vc_camera_terminal_descriptor *vc_ctd = uvc_dev->vc_ctd;

	if (vc_ctd->bControlSize == 0) {
		return false;
	}

	/* Convert the bmControls array to a 32-bit value for easier bit checking */
	uint32_t controls = 0;
	for (int i = 0; i < vc_ctd->bControlSize && i < 4; i++) {
		controls |= ((uint32_t)vc_ctd->bmControls[i]) << (i * 8);
	}

	return (controls & bmcontrol_bit) != 0;
}

/**
 * @brief Initialize USB camera controls based on device capabilities
 *
 * Initializes video controls supported by the UVC device based on
 * Processing Unit and Camera Terminal capabilities.
 *
 * @param dev Video device pointer
 * @return 0 on success, negative error code on failure
 */
static int usb_host_camera_init_controls(const struct device *dev)
{
	int ret;
	struct uvc_device *uvc_dev = dev->data;
	struct usb_camera_ctrls *ctrls = &uvc_dev->ctrls;
	int initialized_count = 0;

	if (!uvc_dev->vc_pud) {
		LOG_WRN("No processing unit found, skipping control initialization");
		return 0;
	}

	LOG_INF("Initializing controls based on processing unit capabilities");

	/* Brightness control */
	if (uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_BRIGHTNESS)) {
		ret = video_init_ctrl(&ctrls->brightness, dev, VIDEO_CID_BRIGHTNESS,
					  (struct video_ctrl_range){.min = -128, .max = 127, .step = 1, .def = 0});
		if (!ret) {
			initialized_count++;
			LOG_DBG("Brightness control initialized");
		}
	}

	/* Contrast control */
	if (uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_CONTRAST)) {
		ret = video_init_ctrl(&ctrls->contrast, dev, VIDEO_CID_CONTRAST,
					  (struct video_ctrl_range){.min = 0, .max = 255, .step = 1, .def = 128});
		if (!ret) {
			initialized_count++;
			LOG_DBG("Contrast control initialized");
		}
	}

	/* Hue control */
	if (uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_HUE)) {
		ret = video_init_ctrl(&ctrls->hue, dev, VIDEO_CID_HUE,
					  (struct video_ctrl_range){.min = -180, .max = 180, .step = 1, .def = 0});
		if (!ret) {
			initialized_count++;
			LOG_DBG("Hue control initialized");
		}
	}

	/* Saturation control */
	if (uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_SATURATION)) {
		ret = video_init_ctrl(&ctrls->saturation, dev, VIDEO_CID_SATURATION,
					  (struct video_ctrl_range){.min = 0, .max = 255, .step = 1, .def = 128});
		if (!ret) {
			initialized_count++;
			LOG_DBG("Saturation control initialized");
		}
	}

	/* Sharpness control */
	if (uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_SHARPNESS)) {
		ret = video_init_ctrl(&ctrls->sharpness, dev, VIDEO_CID_SHARPNESS,
					  (struct video_ctrl_range){.min = 0, .max = 255, .step = 1, .def = 128});
		if (!ret) {
			initialized_count++;
			LOG_DBG("Sharpness control initialized");
		}
	}

	/* Gamma control */
	if (uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_GAMMA)) {
		ret = video_init_ctrl(&ctrls->gamma, dev, VIDEO_CID_GAMMA,
					  (struct video_ctrl_range){.min = 100, .max = 300, .step = 1, .def = 100});
		if (!ret) {
			initialized_count++;
			LOG_DBG("Gamma control initialized");
		}
	}

	/* Gain controls */
	if (uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_GAIN)) {
		ret = video_init_ctrl(&ctrls->auto_gain, dev, VIDEO_CID_AUTOGAIN,
					  (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 1});
		if (!ret) {
			initialized_count++;
			LOG_DBG("Auto gain control initialized");
		}

		ret = video_init_ctrl(&ctrls->gain, dev, VIDEO_CID_GAIN,
					  (struct video_ctrl_range){.min = 0, .max = 255, .step = 1, .def = 0});
		if (!ret) {
			initialized_count++;
			/* Create auto gain cluster if both controls exist */
			if (ctrls->auto_gain.id != 0) {
				video_auto_cluster_ctrl(&ctrls->auto_gain, 2, true);
			}
			LOG_DBG("Gain control initialized");
		}
	}

	/* White Balance Temperature control */
	if (uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_WHITE_BALANCE_TEMPERATURE)) {
		ret = video_init_ctrl(&ctrls->white_balance_temperature, dev, VIDEO_CID_WHITE_BALANCE_TEMPERATURE,
					  (struct video_ctrl_range){.min = 2800, .max = 6500, .step = 1, .def = 4000});
		if (!ret) {
			initialized_count++;
			LOG_DBG("White balance temperature control initialized");
		}
	}

	/* Auto White Balance control */
	if (uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_WHITE_BALANCE_TEMPERATURE_AUTO)) {
		ret = video_init_ctrl(&ctrls->auto_white_balance_temperature, dev, VIDEO_CID_AUTO_WHITE_BALANCE,
					  (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 1});
		if (!ret) {
			initialized_count++;
			LOG_DBG("Auto white balance control initialized");
		}
	}

	/* Backlight Compensation control */
	if (uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_BACKLIGHT_COMPENSATION)) {
		ret = video_init_ctrl(&ctrls->backlight_compensation, dev, VIDEO_CID_BACKLIGHT_COMPENSATION,
					  (struct video_ctrl_range){.min = 0, .max = 2, .step = 1, .def = 1});
		if (!ret) {
			initialized_count++;
			LOG_DBG("Backlight compensation control initialized");
		}
	}

	/* Power line frequency control */
	if (uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_POWER_LINE_FREQUENCY)) {
		ret = video_init_menu_ctrl(&ctrls->light_freq, dev, VIDEO_CID_POWER_LINE_FREQUENCY,
						VIDEO_CID_POWER_LINE_FREQUENCY_AUTO, NULL);
		if (!ret) {
			initialized_count++;
			LOG_DBG("Power line frequency control initialized");
		}
	}

	/* Auto exposure control - Camera Terminal control */
	if (uvc_host_ct_supports_control(uvc_dev, UVC_CT_BMCONTROL_AE_MODE)) {
		ret = video_init_ctrl(&ctrls->auto_exposure, dev, VIDEO_CID_EXPOSURE_AUTO,
					  (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 1});
		if (!ret) {
			initialized_count++;
			LOG_DBG("Auto exposure control initialized");
		}
	}

	/* Exposure absolute control - Camera Terminal control */
	if (uvc_host_ct_supports_control(uvc_dev, UVC_CT_BMCONTROL_EXPOSURE_TIME_ABSOLUTE)) {
		ret = video_init_ctrl(&ctrls->exposure_absolute, dev, VIDEO_CID_EXPOSURE_ABSOLUTE,
					  (struct video_ctrl_range){
					.min = 1,		  /* Minimum exposure time 1μs */
					.max = 10000000,	/* Maximum exposure time 10s (10,000,000μs) */
					.step = 1,
					.def = 33333		/* Default 1/30s ≈ 33.33ms */
					  });
		if (!ret) {
			initialized_count++;
			/* Create auto exposure cluster if both controls exist */
			if (ctrls->auto_exposure.id != 0) {
				video_auto_cluster_ctrl(&ctrls->auto_exposure, 2, true);
			}
			LOG_DBG("Exposure absolute control initialized");
		}
	}

	/* Focus controls - Camera Terminal control */
	if (uvc_host_ct_supports_control(uvc_dev, UVC_CT_BMCONTROL_FOCUS_AUTO)) {
		ret = video_init_ctrl(&ctrls->auto_focus, dev, VIDEO_CID_FOCUS_AUTO,
					  (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 1});
		if (!ret) {
			initialized_count++;
			LOG_DBG("Auto focus control initialized");
		}
	}

	if (uvc_host_ct_supports_control(uvc_dev, UVC_CT_BMCONTROL_FOCUS_ABSOLUTE)) {
		ret = video_init_ctrl(&ctrls->focus_absolute, dev, VIDEO_CID_FOCUS_ABSOLUTE,
					  (struct video_ctrl_range){.min = 0, .max = 1023, .step = 1, .def = 0});
		if (!ret) {
			initialized_count++;
			LOG_DBG("Focus absolute control initialized");
		}
	}

	LOG_INF("Initialized %d camera controls", initialized_count);
	return 0;
}

/**
 * @brief Configure UVC device interfaces
 *
 * Sets up control and streaming interfaces with proper alternate settings.
 * Control interface is set to alternate 0, streaming interface to idle state.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_configure_device(struct uvc_device *uvc_dev)
{
	struct usb_device *udev = uvc_dev->udev;
	int ret;

	if (!uvc_dev || !udev) {
		LOG_ERR("Invalid UVC device or USB device");
		return -EINVAL;
	}

	/* Check if required interfaces were found */
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
	if (ret) {
		LOG_ERR("Failed to set control interface alternate setting: %d", ret);
		return ret;
	}

	/* Set streaming interface to idle state (alternate 0) */
	ret = usbh_device_interface_set(uvc_dev->udev,
									uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber,
									0,
									false);
	if (ret) {
		LOG_ERR("Failed to set streaming interface alternate setting: %d", ret);
		return ret;
	}

	LOG_INF("UVC device configured successfully (control: interface %u, streaming: interface %u)",
		uvc_dev->current_control_interface->bInterfaceNumber,
		uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber);

	return 0;
}

/**
 * @brief Parse USB interface descriptor
 *
 * Identifies and processes Video Control and Video Streaming interfaces
 * from USB interface descriptors.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @param if_desc Pointer to USB interface descriptor
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_parse_interface_descriptor(struct uvc_device *uvc_dev,
										struct usb_if_descriptor *if_desc)
{
	/* Only process Video class interfaces */
	if (if_desc->bInterfaceClass != UVC_SC_VIDEOCLASS) {
		return 0; /* Not a video class interface, skip */
	}

	switch (if_desc->bInterfaceSubClass) {
	case UVC_SC_VIDEOCONTROL:
		/* Video Control interface: save only the first one found */
		if (!uvc_dev->current_control_interface) {
			uvc_dev->current_control_interface = if_desc;
			LOG_INF("Found Video Control interface %u", if_desc->bInterfaceNumber);
		}
		break;

	case UVC_SC_VIDEOSTREAMING:
		/* Video Streaming interface: save to stream_ifaces array for all of alternates including 0 */
		for (int i = 0; i < UVC_STREAM_INTERFACES_MAX_ALT; i++) {
			if (uvc_dev->stream_ifaces[i] == NULL) {
				/* Found empty slot, save interface */
				uvc_dev->stream_ifaces[i] = if_desc;
				/* Save current_stream_iface as alternat 0 interface */
                if (!if_desc->bAlternateSetting) {
					uvc_dev->current_stream_iface_info.current_stream_iface = if_desc;
				}
				break;
			}
		}
		break;

	default:
		LOG_DBG("Unknown video interface subclass %u (interface %u)",
				if_desc->bInterfaceSubClass, if_desc->bInterfaceNumber);
		break;
	}

	return 0;
}

/**
 * @brief Parse UVC class-specific control interface descriptor
 *
 * Parses and processes UVC class-specific interface descriptors including
 * format, frame, and unit/terminal descriptors.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @param control_if Pointer to control interface descriptor
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_parse_cs_vc_interface_descriptor(struct uvc_device *uvc_dev, struct usb_if_descriptor *control_if)
{
	struct usb_cs_desc_header *header;

	/* Basic validation */
	if (!control_if || !uvc_dev || control_if->bLength < 3) {
		LOG_ERR("Invalid parameters or descriptor");
		return -EINVAL;
	}

	/* Skip the interface descriptor itself */
	header = (struct usb_cs_desc_header *)((uint8_t *)control_if + control_if->bLength);

	while ((uint8_t *)header < (uint8_t *)uvc_dev->desc_end) {
		/* Check for end of descriptors or next interface */
		if (header->bDescriptorType == USB_DESC_INTERFACE ||
		    header->bDescriptorType == USB_DESC_INTERFACE_ASSOC ||
		    header->bLength == 0) {
			break;
		}

		if (header->bDescriptorType ==  USB_DESC_CS_INTERFACE) {
			switch (header->bDescriptorSubType) {
			case UVC_VC_HEADER: {
				struct uvc_vc_header_descriptor *header_desc =
					(struct uvc_vc_header_descriptor *)header;

				/* Check descriptor length - at least basic fields */
				if (header->bLength < sizeof(struct uvc_vc_header_descriptor)) {
					LOG_ERR("Invalid VC header descriptor length: %u", header->bLength);
					return -EINVAL;
				}

				/* Additional length check for interface collection */
				if (header->bLength < (sizeof(struct uvc_vc_header_descriptor) +
									header_desc->bInCollection)) {
					LOG_ERR("VC header descriptor too short for interface collection: %u < %u",
							header->bLength,
							(unsigned)(sizeof(struct uvc_vc_header_descriptor) + header_desc->bInCollection));
					return -EINVAL;
				}

				/* Save VideoControl Interface Header descriptor pointer */
				uvc_dev->vc_header = header_desc;
				LOG_DBG("Found VideoControl Header: UVC v%u.%u, TotalLength=%u, ClockFreq=%u Hz, Interfaces=%u",
						(sys_le16_to_cpu(header_desc->bcdUVC) >> 8) & 0xFF,
						sys_le16_to_cpu(header_desc->bcdUVC) & 0xFF,
						sys_le16_to_cpu(header_desc->wTotalLength),
						sys_le32_to_cpu(header_desc->dwClockFrequency),
						header_desc->bInCollection);

				/* Print interface collection for debugging */
				if (header_desc->bInCollection > 0) {
					LOG_HEXDUMP_DBG(header_desc->baInterfaceNr, header_desc->bInCollection,
									"VideoStreaming Interface Numbers");
				}

				break;
			}

			case UVC_VC_INPUT_TERMINAL: {
				struct uvc_vc_input_terminal_descriptor *it_desc =
					(struct uvc_vc_input_terminal_descriptor *)header;

				/* Check descriptor length - at least basic fields */
				if (header->bLength < sizeof(struct uvc_vc_input_terminal_descriptor)) {
					LOG_ERR("Invalid input terminal descriptor length: %u", header->bLength);
					return -EINVAL;
				}

				/* Check if this is Camera Terminal (wTerminalType = 0x0201) */
				if (sys_le16_to_cpu(it_desc->wTerminalType) == UVC_ITT_CAMERA) {
					struct uvc_vc_camera_terminal_descriptor *ct_desc =
						(struct uvc_vc_camera_terminal_descriptor *)header;

					/* Check Camera Terminal descriptor length */
					if (header->bLength < (sizeof(struct uvc_vc_input_terminal_descriptor) + 6 + ct_desc->bControlSize)) {
						LOG_ERR("Invalid camera terminal descriptor length: %u", header->bLength);
						return -EINVAL;
					}

					/* Check if Camera Terminal already exists */
					if (uvc_dev->vc_ctd != NULL) {
						LOG_WRN("Multiple camera terminals found, replacing previous one");
					}

					/* Save Camera Terminal descriptor pointer */
					uvc_dev->vc_ctd = ct_desc;

					LOG_DBG("Found Camera Terminal: ID=%u, Type=0x%04x, ControlSize=%u",
							ct_desc->bTerminalID,
							sys_le16_to_cpu(ct_desc->wTerminalType),
							ct_desc->bControlSize);

					/* Print control bitmap for debugging */
					if (ct_desc->bControlSize > 0) {
						LOG_HEXDUMP_DBG(ct_desc->bmControls, ct_desc->bControlSize,
										"Camera Terminal Controls");
					}
				} else {

					/* Check if Input Terminal already exists */
					if (uvc_dev->vc_itd != NULL) {
						LOG_WRN("Multiple input terminals found, replacing previous one");
					}

					/* Save Input Terminal descriptor pointer */
					uvc_dev->vc_itd = it_desc;

					LOG_DBG("Found Input Terminal: ID=%u, Type=0x%04x",
							it_desc->bTerminalID,
							sys_le16_to_cpu(it_desc->wTerminalType));
				}

				break;
			}

			case UVC_VC_OUTPUT_TERMINAL: {
				struct uvc_vc_output_terminal_descriptor *ot_desc =
					(struct uvc_vc_output_terminal_descriptor *)header;

				/* Check descriptor length - at least basic fields */
				if (header->bLength < sizeof(struct uvc_vc_output_terminal_descriptor)) {
					LOG_ERR("Invalid output terminal descriptor length: %u", header->bLength);
					return -EINVAL;
				}

				/* Check if Output Terminal already exists */
				if (uvc_dev->vc_otd != NULL) {
					LOG_WRN("Multiple output terminals found, replacing previous one");
				}

				/* Save Output Terminal descriptor pointer */
				uvc_dev->vc_otd = ot_desc;

				LOG_DBG("Found Output Terminal: ID=%u, Type=0x%04x, SourceID=%u",
						ot_desc->bTerminalID,
						sys_le16_to_cpu(ot_desc->wTerminalType),
						ot_desc->bSourceID);

				break;
			}

			case UVC_VC_SELECTOR_UNIT: {
				struct uvc_vc_selector_unit_descriptor *su_desc =
					(struct uvc_vc_selector_unit_descriptor *)header;

				/* Check descriptor length - at least basic fields */
				if (header->bLength < 5) { /* bLength + bDescriptorType + bDescriptorSubType + bUnitID + bNrInPins */
					LOG_ERR("Invalid selector unit descriptor length: %u", header->bLength);
					return -EINVAL;
				}

				/* Check if there's enough space for source ID array */
				if (header->bLength < (5 + su_desc->bNrInPins + 1)) { /* 5 basic bytes + source IDs + iSelector */
					LOG_ERR("Selector unit descriptor too short for source IDs: %u < %u",
							header->bLength, 5 + su_desc->bNrInPins + 1);
					return -EINVAL;
				}

				/* Check if Selector Unit already exists */
				if (uvc_dev->vc_sud != NULL) {
					LOG_WRN("Multiple selector units found, replacing previous one");
				}

				/* Save Selector Unit descriptor pointer */
				uvc_dev->vc_sud = su_desc;

				LOG_DBG("Found Selector Unit: ID=%u, InputPins=%u",
						su_desc->bUnitID,
						su_desc->bNrInPins);

				/* Print source IDs for debugging */
				if (su_desc->bNrInPins > 0) {
					LOG_HEXDUMP_DBG(su_desc->baSourceID, su_desc->bNrInPins,
									"Selector Unit Source IDs");
				}

				break;
			}

			case UVC_VC_PROCESSING_UNIT: {
				struct uvc_vc_processing_unit_descriptor *pu_desc =
					(struct uvc_vc_processing_unit_descriptor *)header;

				/* Check descriptor length - at least basic fields */
				if (header->bLength < 8) { /* Basic field length */
					LOG_ERR("Invalid processing unit descriptor length: %u", header->bLength);
					return -EINVAL;
				}

				/* Check if there's enough space for control bitmap */
				if (header->bLength < (8 + pu_desc->bControlSize)) {
					LOG_ERR("Processing unit descriptor too short for control bitmap: %u < %u",
							header->bLength, 8 + pu_desc->bControlSize);
					return -EINVAL;
				}

				/* Check if Processing Unit already exists (most devices have only one) */
				if (uvc_dev->vc_pud != NULL) {
					LOG_WRN("Multiple processing units found, replacing previous one");
				}

				/* Save Processing Unit descriptor pointer */
				uvc_dev->vc_pud = pu_desc;

				LOG_DBG("Found Processing Unit: ID=%u, SourceID=%u, MaxMultiplier=%u, ControlSize=%u",
						pu_desc->bUnitID,
						pu_desc->bSourceID,
						sys_le16_to_cpu(pu_desc->wMaxMultiplier),
						pu_desc->bControlSize);

				/* Print control bitmap for debugging */
				if (pu_desc->bControlSize > 0) {
					LOG_HEXDUMP_DBG(pu_desc->bmControls, pu_desc->bControlSize,
									"Processing Unit Controls");
				}

				break;
			}

			case UVC_VC_ENCODING_UNIT: {
				struct uvc_vc_encoding_unit_descriptor *enc_desc =
					(struct uvc_vc_encoding_unit_descriptor *)header;

				/* Check descriptor length - at least basic fields */
				if (header->bLength < 8) { /* Basic field length */
					LOG_ERR("Invalid encoding unit descriptor length: %u", header->bLength);
					return -EINVAL;
				}

				/* Check if there's enough space for control bitmap */
				if (header->bLength < (8 + enc_desc->bControlSize)) {
					LOG_ERR("Encoding unit descriptor too short for control bitmap: %u < %u",
							header->bLength, 8 + enc_desc->bControlSize);
					return -EINVAL;
				}

				/* Check if Encoding Unit already exists */
				if (uvc_dev->vc_encoding_unit != NULL) {
					LOG_WRN("Multiple encoding units found, replacing previous one");
				}

				/* Save Encoding Unit descriptor pointer */
				uvc_dev->vc_encoding_unit = enc_desc;

				LOG_DBG("Found Encoding Unit: ID=%u, SourceID=%u, ControlSize=%u",
						enc_desc->bUnitID,
						enc_desc->bSourceID,
						enc_desc->bControlSize);

				/* Print control bitmap for debugging */
				if (enc_desc->bControlSize > 0) {
					LOG_HEXDUMP_DBG(enc_desc->bmControls, enc_desc->bControlSize,
									"Encoding Unit Controls");
				}

				break;
			}

			case UVC_VC_EXTENSION_UNIT: {
				struct uvc_vc_extension_unit_descriptor *eu_desc =
					(struct uvc_vc_extension_unit_descriptor *)header;

				/* Check descriptor length - at least basic fields */
				if (header->bLength < 24) { /* Minimum: 3 + 1 + 16 + 1 + 1 + 1 + 1 = 24 bytes */
					LOG_ERR("Invalid extension unit descriptor length: %u", header->bLength);
					return -EINVAL;
				}

				/* Check if there's enough space for source ID array */
				uint8_t min_length = 24 + eu_desc->bNrInPins; /* 24 basic bytes + source IDs */
				if (header->bLength < min_length) {
					LOG_ERR("Extension unit descriptor too short: %u < %u",
							header->bLength, min_length);
					return -EINVAL;
				}

				/* Check if Extension Unit already exists */
				if (uvc_dev->vc_extension_unit != NULL) {
					LOG_WRN("Multiple extension units found, replacing previous one");
				}

				/* Save Extension Unit descriptor pointer */
				uvc_dev->vc_extension_unit = eu_desc;

				LOG_DBG("Found Extension Unit: ID=%u, NumControls=%u, InputPins=%u",
						eu_desc->bUnitID,
						eu_desc->bNumControls,
						eu_desc->bNrInPins);

				/* Print Extension Code GUID for debugging */
				LOG_HEXDUMP_DBG(eu_desc->guidExtensionCode, 16, "Extension Unit GUID");

				break;
			}

			default:
				/* Other CS_INTERFACE descriptor types, ignore */
				LOG_DBG("Ignoring CS_INTERFACE subtype: 0x%02x", header->bDescriptorSubType);
				break;

			}
		}
		/* Move to next descriptor */
		header = (struct usb_cs_desc_header *)((uint8_t *)header + header->bLength);
	}

	return 0;
}

/**
 * @brief Parse UVC class-specific stream interface descriptor
 *
 * Parses and processes UVC class-specific interface descriptors including
 * format, frame, and unit/terminal descriptors.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @param stream_if Pointer to stream interface descriptor
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_parse_cs_vs_interface_descriptor(struct uvc_device *uvc_dev, struct usb_if_descriptor *stream_if)
{
	struct usb_cs_desc_header *header;

	/* Basic validation */
	if (!stream_if || !uvc_dev || stream_if->bLength < 3) {
		LOG_ERR("Invalid parameters or descriptor");
		return -EINVAL;
	}

	/* Skip the interface descriptor itself */
	header = (struct usb_cs_desc_header *)((uint8_t *)stream_if + stream_if->bLength);

	while ((uint8_t *)header < (uint8_t *)uvc_dev->desc_end) {
		/* Check for end of descriptors or next interface */
		if (header->bDescriptorType == USB_DESC_INTERFACE ||
		    header->bDescriptorType == USB_DESC_INTERFACE_ASSOC ||
		    header->bLength == 0) {
			break;
		}

		if (header->bDescriptorType ==  USB_DESC_CS_INTERFACE) {
			switch (header->bDescriptorSubType) {
			case UVC_VS_INPUT_HEADER: {
				struct uvc_vs_input_header_descriptor *header_desc = (struct uvc_vs_input_header_descriptor *)header;

				/* Check descriptor length */
				if (header->bLength < sizeof(struct uvc_vs_input_header_descriptor)) {
					LOG_ERR("Invalid VS input header descriptor length: %u", header->bLength);
					return -EINVAL;
				}

				/* Save descriptor pointer */
				uvc_dev->vs_input_header = header_desc;

				LOG_DBG("Added VS input header: formats=%u, total_len=%u, ep=0x%02x, terminal_link=%u",
						header_desc->bNumFormats,
						header_desc->wTotalLength,
						header_desc->bEndpointAddress,
						header_desc->bTerminalLink);

				break;
			}

			case UVC_VS_OUTPUT_HEADER: {
				struct uvc_vs_output_header_descriptor *header_desc = (struct uvc_vs_output_header_descriptor *)header;

				/* Check descriptor length */
				if (header->bLength < sizeof(struct uvc_vs_output_header_descriptor)) {
					LOG_ERR("Invalid VS output header descriptor length: %u", header->bLength);
					return -EINVAL;
				}

				/* Save descriptor pointer */
				uvc_dev->vs_output_header = header_desc;

				LOG_DBG("Added VS output header: formats=%u, total_len=%u, ep=0x%02x, terminal_link=%u",
						header_desc->bNumFormats,
						header_desc->wTotalLength,
						header_desc->bEndpointAddress,
						header_desc->bTerminalLink);

				break;
			}

			case UVC_VS_FORMAT_UNCOMPRESSED: {
				struct uvc_vs_format_uncompressed *format_desc = (struct uvc_vs_format_uncompressed *)header;
				struct uvc_vs_format_uncompressed_info *info = &uvc_dev->formats.format_uncompressed;

				/* Check descriptor length */
				if (header->bLength < sizeof(struct uvc_vs_format_uncompressed)) {
					LOG_ERR("Invalid uncompressed format descriptor length: %u", header->bLength);
					return -EINVAL;
				}

				/* Check array space */
				if (info->num_uncompressed_formats >= UVC_MAX_UNCOMPRESSED_FORMAT) {
					LOG_WRN("Too many uncompressed formats, ignoring format index %u",
							format_desc->bFormatIndex);
					return 0;
				}

				/* Save descriptor pointer */
				info->uncompressed_format[info->num_uncompressed_formats] = format_desc;
				info->num_uncompressed_formats++;

				LOG_DBG("Added uncompressed format[%u]: index=%u, frames=%u, bpp=%u",
						info->num_uncompressed_formats - 1,
						format_desc->bFormatIndex,
						format_desc->bNumFrameDescriptors,
						format_desc->bBitsPerPixel);

				break;
			}

			case UVC_VS_FORMAT_MJPEG: {
				struct uvc_vs_format_mjpeg *format_desc = (struct uvc_vs_format_mjpeg *)header;
				struct uvc_vs_format_mjpeg_info *info = &uvc_dev->formats.format_mjpeg;

				/* Check descriptor length */
				if (header->bLength < sizeof(struct uvc_vs_format_mjpeg)) {
					LOG_ERR("Invalid MJPEG format descriptor length: %u", header->bLength);
					return -EINVAL;
				}

				/* Check array space */
				if (info->num_mjpeg_formats >= UVC_MAX_MJPEG_FORMAT) {
					LOG_WRN("Too many MJPEG formats, ignoring format index %u",
							format_desc->bFormatIndex);
					return 0;
				}

				/* Save descriptor pointer */
				info->vs_mjpeg_format[info->num_mjpeg_formats] = format_desc;
				info->num_mjpeg_formats++;

				LOG_DBG("Added MJPEG format[%u]: index=%u, frames=%u, flags=0x%02x",
						info->num_mjpeg_formats - 1,
						format_desc->bFormatIndex,
						format_desc->bNumFrameDescriptors,
						format_desc->bmFlags);

				break;
			}

			default:
				/* Other CS_INTERFACE descriptor types, ignore */
				LOG_DBG("Ignoring CS_INTERFACE subtype: 0x%02x", header->bDescriptorSubType);
				break;

			}
		}
		/* Move to next descriptor */
		header = (struct usb_cs_desc_header *)((uint8_t *)header + header->bLength);
	}
	return 0;
}

/**
 * @brief Parse all UVC descriptors from device
 *
 * Parses UVC descriptors from the descriptor segment between desc_start
 * and desc_end which contains all descriptors belonging to this UVC device.
 * First pass processes interface descriptors, second pass handles class-specific descriptors.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_parse_descriptors(struct uvc_device *uvc_dev)
{
	struct usb_if_descriptor *if_desc;
	uint8_t *desc_buf, *desc_end;
	int ret = 0;

	/* Validate descriptor buffer pointers */
	if (!uvc_dev->desc_start || !uvc_dev->desc_end) {
		LOG_ERR("Invalid descriptor range for UVC device");
		return -EINVAL;
	}

	/* Ensure start pointer is before end pointer */
	if (uvc_dev->desc_start >= uvc_dev->desc_end) {
		LOG_ERR("Invalid descriptor range: start >= end");
		return -EINVAL;
	}

	/* Initialize parsing pointers from device descriptor range */
	desc_buf = (uint8_t *)uvc_dev->desc_start;
	desc_end = (uint8_t *)uvc_dev->desc_end;

	LOG_DBG("Parsing UVC descriptors from %p to %p (%zu bytes)",
		desc_buf, desc_end, (size_t)(desc_end - desc_buf));

	/* First pass: Parse all interface descriptors to identify UVC interfaces */
	while (desc_buf < desc_end) {
		struct usb_desc_header *header = (struct usb_desc_header *)desc_buf;

		/* Check for malformed descriptor with zero length */
		if (header->bLength == 0) {
			LOG_WRN("Zero-length descriptor encountered, stopping parse");
			break;
		}

		if (desc_buf + header->bLength > desc_end) {
			LOG_ERR("Descriptor extends beyond valid range");
			return -EINVAL;
		}

		/* Process USB interface descriptors to categorize UVC interfaces */
		if (USB_DESC_INTERFACE == header->bDescriptorType) {
			if_desc = (struct usb_if_descriptor *)desc_buf;
			ret = uvc_host_parse_interface_descriptor(uvc_dev, if_desc);
			LOG_DBG("Parsed interface descriptor (bInterfaceNumber=%d, class=0x%02x)",
				if_desc->bInterfaceNumber, if_desc->bInterfaceClass);
		}

		/* Move to next descriptor in the buffer */
		desc_buf += header->bLength;
	}

	/* Parse class-specific descriptors for Video Control interface */
	ret = uvc_host_parse_cs_vc_interface_descriptor(uvc_dev, uvc_dev->current_control_interface);
	if (ret != 0) {
		LOG_ERR("Failed to parse Video Control interface descriptor: %d", ret);
		return ret;
	}

	/* Parse class-specific descriptors for Video Streaming interface */
	ret = uvc_host_parse_cs_vs_interface_descriptor(uvc_dev, uvc_dev->current_stream_iface_info.current_stream_iface);
	if (ret != 0) {
		LOG_ERR("Failed to parse Video Streaming interface descriptor: %d", ret);
		return ret;
	}

	LOG_INF("Successfully parsed UVC descriptors");
	return 0;
}

/**
 * @brief Parse default frame interval from descriptor
 *
 * Extracts the default frame interval from frame descriptor. If default
 * interval is invalid (0), falls back to maximum supported interval.
 *
 * @param desc_buf Pointer to frame descriptor buffer
 * @param frame_subtype Frame descriptor subtype (UVC_VS_FRAME_UNCOMPRESSED or UVC_VS_FORMAT_MJPEG)
 * @return Default or maximum frame interval in 100ns units
 */
static uint32_t uvc_host_parse_frame_default_intervals(uint8_t *desc_buf, uint8_t frame_subtype)
{
    uint32_t default_interval = 0;
    uint8_t interval_type;
    uint8_t *interval_data;

    if (frame_subtype == UVC_VS_FRAME_UNCOMPRESSED) {
        struct uvc_vs_frame_uncompressed *frame_desc = 
            (struct uvc_vs_frame_uncompressed *)desc_buf;
        default_interval = sys_le32_to_cpu(frame_desc->dwDefaultFrameInterval);
        interval_type = frame_desc->bFrameIntervalType;
        /* Interval data follows immediately after bFrameIntervalType field */
        interval_data = &(frame_desc->bFrameIntervalType) + 1;
    } else if (frame_subtype == UVC_VS_FORMAT_MJPEG) {
        struct uvc_vs_frame_mjpeg *frame_desc = 
            (struct uvc_vs_frame_mjpeg *)desc_buf;
        default_interval = sys_le32_to_cpu(frame_desc->dwDefaultFrameInterval);
        interval_type = frame_desc->bFrameIntervalType;
        /* Interval data follows immediately after bFrameIntervalType field */
        interval_data = &(frame_desc->bFrameIntervalType) + 1;
    } else {
        /* Unsupported frame subtype, use hardcoded fallback */
        return 333333; /* 30fps */
    }

    /* If default interval is valid, use it */
    if (default_interval != 0) {
        return default_interval;
    }

    /* Default interval is invalid, find maximum supported interval */
    uint32_t max_interval = 333333; /* Fallback to 30fps */

    if (interval_type == 0) {
        /* Continuous/stepwise intervals: dwMin, dwMax, dwStep */
        if (interval_data + 8 <= desc_buf + desc_buf[0]) { /* Bounds check */
            max_interval = sys_le32_to_cpu(*(uint32_t*)(interval_data + 4));
        }
    } else if (interval_type > 0) {
        /* Discrete intervals: take the last (typically maximum) value */
        uint32_t last_interval_offset = (interval_type - 1) * 4;
        if (interval_data + last_interval_offset + 4 <= desc_buf + desc_buf[0]) { /* Bounds check */
            max_interval = sys_le32_to_cpu(*(uint32_t*)(interval_data + last_interval_offset));
        }
    }

    return max_interval;
}

/**
 * @brief Find matching frame in specific format type
 *
 * Searches for frame descriptor with specified dimensions and type
 * within a format descriptor.
 *
 * @param format_header Pointer to format header
 * @param target_width Target frame width in pixels
 * @param target_height Target frame height in pixels
 * @param expected_frame_subtype Expected frame descriptor subtype
 * @param found_frame Pointer to store found frame descriptor
 * @param found_interval Pointer to store found frame interval
 * @return 0 on success, negative error code if not found
 */
static int uvc_host_find_frame_in_format(struct uvc_format_header *format_header,
									uint16_t target_width,
									uint16_t target_height,
									uint8_t expected_frame_subtype,
									struct uvc_frame_header **found_frame,
									uint32_t *found_interval)
{
	uint8_t *desc_buf = (uint8_t *)format_header + format_header->bLength;
	int frames_found = 0;

	/* Iterate through all frame descriptors for this format */
	while (frames_found < format_header->bNumFrameDescriptors) {
		struct uvc_frame_header *frame_header = (struct uvc_frame_header *)desc_buf;

		if (frame_header->bLength == 0) break;

		/* Check if this is the expected frame descriptor type */
		if (frame_header->bDescriptorType == UVC_CS_INTERFACE &&
			frame_header->bDescriptorSubType == expected_frame_subtype) {

			if (frame_header->bLength >= sizeof(struct uvc_frame_header)) {
				uint16_t frame_width = sys_le16_to_cpu(frame_header->wWidth);
				uint16_t frame_height = sys_le16_to_cpu(frame_header->wHeight);

				/* Check if dimensions match target */
				if (frame_width == target_width && frame_height == target_height) {
					*found_frame = frame_header;
					/* Parse frame interval from descriptor */
					*found_interval = (frame_header->bLength >= 26) ?
									 uvc_host_parse_frame_default_intervals(desc_buf, expected_frame_subtype) : 333333;
					return 0;
				}
				frames_found++;
			}
		} else if (frame_header->bDescriptorType == UVC_CS_INTERFACE &&
				  (desc_buf[2] == UVC_VS_FORMAT_UNCOMPRESSED || desc_buf[2] == UVC_VS_FORMAT_MJPEG)) {
			/* Encountered new format descriptor, stop searching */
			break;
		}

		desc_buf += frame_header->bLength;
	}

	return -ENOTSUP;
}

/**
 * @brief Find format and frame matching specifications
 *
 * Searches UVC device for format/frame combination matching the specified
 * pixel format and dimensions.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @param pixelformat Target pixel format (FOURCC)
 * @param width Target frame width in pixels
 * @param height Target frame height in pixels
 * @param format Pointer to store found format descriptor
 * @param frame Pointer to store found frame descriptor
 * @param frame_interval Pointer to store frame interval
 * @return 0 on success, negative error code if not supported
 */
static int uvc_host_find_format(struct uvc_device *uvc_dev,
						  uint32_t pixelformat,
						  uint16_t width,
						  uint16_t height,
						  struct uvc_format_header **format,
						  struct uvc_frame_header **frame,
						  uint32_t *frame_interval)
{
	if (!uvc_dev || !format || !frame || !frame_interval) {
		return -EINVAL;
	}

	LOG_DBG("Looking for format: %s %ux%u",
			VIDEO_FOURCC_TO_STR(pixelformat), width, height);

	/* Search uncompressed formats */
	struct uvc_vs_format_uncompressed_info *uncompressed_info = &uvc_dev->formats.format_uncompressed;

	for (int i = 0; i < uncompressed_info->num_uncompressed_formats; i++) {
		struct uvc_vs_format_uncompressed *format_desc = uncompressed_info->uncompressed_format[i];

		if (!format_desc) continue;

		/* Convert GUID to pixel format for comparison */
		uint32_t desc_pixelformat = uvc_guid_to_fourcc(format_desc->guidFormat);

		if (desc_pixelformat == pixelformat) {
			LOG_DBG("Found matching uncompressed format: index=%u", format_desc->bFormatIndex);

			/* Search for matching frame in this format */
			if (uvc_host_find_frame_in_format((struct uvc_format_header *)format_desc,
										width, height, UVC_VS_FRAME_UNCOMPRESSED,
										frame, frame_interval) == 0) {
				*format = (struct uvc_format_header *)format_desc;
				LOG_DBG("Found matching frame: format_addr=%p, frame_addr=%p, interval=%u",
						*format, *frame, *frame_interval);
				return 0;
			}
		}
	}

	/*  Search MJPEG formats */
	if (pixelformat == VIDEO_PIX_FMT_MJPEG) {
		struct uvc_vs_format_mjpeg_info *mjpeg_info = &uvc_dev->formats.format_mjpeg;

		for (int i = 0; i < mjpeg_info->num_mjpeg_formats; i++) {
			struct uvc_vs_format_mjpeg *format_desc = mjpeg_info->vs_mjpeg_format[i];

			if (!format_desc) continue;

			LOG_DBG("Checking MJPEG format: index=%u", format_desc->bFormatIndex);

			/* Search for matching frame in MJPEG format */
			if (uvc_host_find_frame_in_format((struct uvc_format_header *)format_desc,
										width, height, UVC_VS_FRAME_MJPEG,
										frame, frame_interval) == 0) {
				*format = (struct uvc_format_header *)format_desc;
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
 *
 * Searches through available streaming interface alternate settings to find
 * one with sufficient bandwidth for the required video format.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @param required_bandwidth Required bandwidth in bytes per second
 * @return 0 on success, negative error code if no suitable interface found
 */
static int uvc_host_select_streaming_alternate(struct uvc_device *uvc_dev, uint32_t required_bandwidth)
{
	struct usb_if_descriptor *selected_interface = NULL;
	struct usb_ep_descriptor *selected_endpoint = NULL;
	uint32_t optimal_bandwidth = UINT32_MAX;
	uint32_t selected_payload_size = 0;
	bool found_suitable = false;
	enum usbh_speed device_speed = uvc_dev->udev->speed;
	uint32_t max_payload_transfer_size = sys_le32_to_cpu(uvc_dev->video_probe.dwMaxPayloadTransferSize);
	
	LOG_DBG("Required bandwidth: %u bytes/sec, Max payload: %u bytes (device speed: %s)", 
			required_bandwidth, max_payload_transfer_size,
			(device_speed == USB_SPEED_SPEED_HS) ? "High Speed" : "Full Speed");

	/* Iterate through all alternate setting interfaces */
	for (int i = 0; i < UVC_STREAM_INTERFACES_MAX_ALT && uvc_dev->stream_ifaces[i]; i++) {
		struct usb_if_descriptor *if_desc = uvc_dev->stream_ifaces[i];

		/* Skip Alt 0 (idle state) */
		if (if_desc->bAlternateSetting == 0) {
			continue;
		}

		LOG_DBG("Checking interface %u alt %u (%u endpoints)",
				if_desc->bInterfaceNumber, if_desc->bAlternateSetting, if_desc->bNumEndpoints);

		/* Examine all endpoints in this alternate setting */
		uint8_t *ep_buf = (uint8_t *)if_desc + if_desc->bLength;

		for (int ep = 0; ep < if_desc->bNumEndpoints; ep++) {
			struct usb_ep_descriptor *ep_desc = (struct usb_ep_descriptor *)ep_buf;

			/* Check if this is ISO IN endpoint */
			if (ep_desc->bDescriptorType == USB_DESC_ENDPOINT &&
				(ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_ISO &&
				(ep_desc->bEndpointAddress & USB_EP_DIR_MASK) == USB_EP_DIR_IN) {

				uint16_t max_packet_size = sys_le16_to_cpu(ep_desc->wMaxPacketSize) & 0x07FF;
				uint32_t ep_bandwidth;
				uint32_t ep_payload_size;

				/* Calculate endpoint bandwidth based on USB speed */
				if (device_speed == USB_SPEED_SPEED_HS) {
					uint8_t mult = ((sys_le16_to_cpu(ep_desc->wMaxPacketSize) >> 11) & 0x03) + 1;
					uint32_t interval_uframes = 1 << (ep_desc->bInterval - 1);
					ep_payload_size = max_packet_size * mult;
					ep_bandwidth = (ep_payload_size * 8000) / interval_uframes;
				} else {
					ep_payload_size = max_packet_size;
					ep_bandwidth = (max_packet_size * 1000) / ep_desc->bInterval;
				}

				LOG_DBG("  Interface %u Alt %u EP[%d]: addr=0x%02x, maxpkt=%u, payload=%u, bandwidth=%u",
						if_desc->bInterfaceNumber, if_desc->bAlternateSetting, ep,
						ep_desc->bEndpointAddress, max_packet_size, ep_payload_size, ep_bandwidth);

				/* Check if endpoint satisfies requirements and is optimal */
				if (ep_bandwidth >= required_bandwidth && 
					ep_payload_size >= max_payload_transfer_size &&
					ep_bandwidth < optimal_bandwidth) {

					optimal_bandwidth = ep_bandwidth;
					selected_interface = if_desc;
					selected_endpoint = ep_desc;
					selected_payload_size = ep_payload_size;
					found_suitable = true;

					LOG_DBG("Selected optimal endpoint: interface %u alt %u EP 0x%02x, bandwidth=%u, payload=%u",
							if_desc->bInterfaceNumber, if_desc->bAlternateSetting,
							ep_desc->bEndpointAddress, ep_bandwidth, ep_payload_size);
				} else {
					if (ep_bandwidth < required_bandwidth) {
						LOG_DBG("  Endpoint rejected: insufficient bandwidth (%u < %u)", 
								ep_bandwidth, required_bandwidth);
					}
					if (ep_payload_size < max_payload_transfer_size) {
						LOG_DBG("  Endpoint rejected: insufficient payload size (%u < %u)", 
								ep_payload_size, max_payload_transfer_size);
					}
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

	/* Update current streaming interface and endpoint */
	uvc_dev->current_stream_iface_info.current_stream_iface = selected_interface;
	uvc_dev->current_stream_iface_info.current_stream_ep = selected_endpoint;
	uvc_dev->current_stream_iface_info.cur_ep_mps_mult = selected_payload_size;

	LOG_INF("Selected interface %u alternate %u endpoint 0x%02x (bandwidth=%u, payload=%u)",
			selected_interface->bInterfaceNumber, selected_interface->bAlternateSetting,
			selected_endpoint->bEndpointAddress, optimal_bandwidth, selected_payload_size);

	return 0;
}

/**
 * @brief Calculate required bandwidth for current video format
 *
 * Calculates the bandwidth needed for streaming based on current format
 * resolution, frame rate, and pixel format characteristics.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @return Required bandwidth in bytes per second, 0 on error
 */
static uint32_t uvc_host_calculate_required_bandwidth(struct uvc_device *uvc_dev)
{
	uint32_t width = uvc_dev->current_format.width;
	uint32_t height = uvc_dev->current_format.height;
	uint32_t fps = uvc_dev->current_format.fps;
	uint32_t pixelformat = uvc_dev->current_format.pixelformat;
	uint32_t bandwidth;

	if (width == 0 || height == 0 || fps == 0) {
		LOG_ERR("Invalid format parameters: %ux%u@%ufps", width, height, fps);
		return 0;
	}

	/* Calculate bandwidth based on pixel format characteristics */
	switch (pixelformat) {
	case VIDEO_PIX_FMT_MJPEG:
		/* MJPEG compressed format, use empirical compression ratio */
		/* Assume compression ratio 8:1 to 12:1, use conservative 6:1 here */
		bandwidth = (width * height * fps * 3) / 6;  /* RGB24 compressed 6x */
		break;

	case VIDEO_PIX_FMT_YUYV:
		/* YUYV format, 2 bytes per pixel */
		bandwidth = width * height * fps * 2;
		break;

	case VIDEO_PIX_FMT_RGB565:
		/* RGB565 format, 2 bytes per pixel */
		bandwidth = width * height * fps * 2;
		break;

	case VIDEO_PIX_FMT_GREY:
		/* Grayscale format, 1 byte per pixel */
		bandwidth = width * height * fps * 1;
		break;

	default:
		/* Unknown format, assume RGB24 */
		bandwidth = width * height * fps * 3;
		LOG_WRN("Unknown pixel format 0x%08x, assuming RGB24", pixelformat);
		break;
	}

	/* Add 10% margin to ensure stable transmission */
	bandwidth = (bandwidth * 110 + 99) / 100;

	LOG_DBG("Calculated bandwidth: %u bytes/sec for %s %ux%u@%ufps",
			bandwidth, VIDEO_FOURCC_TO_STR(pixelformat), width, height, fps);

	return bandwidth;
}

/**
 * @brief Send UVC streaming interface control request
 *
 * Sends control requests to UVC streaming interface for format negotiation
 * and streaming parameters configuration.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @param request UVC request type (SET_CUR, GET_CUR, etc.)
 * @param control_selector UVC control selector
 * @param data Data buffer for request payload
 * @param data_len Length of data buffer
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_stream_interface_control(struct uvc_device *uvc_dev, uint8_t request,
								uint8_t control_selector, void *data, uint8_t data_len) {

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

	/* Always allocate transfer buffer for both SET and GET requests */
	buf = usbh_xfer_buf_alloc(uvc_dev->udev, data_len);
	if (!buf) {
		LOG_ERR("Failed to allocate transfer buffer of size %u", data_len);
		return -ENOMEM;
	}

	/* Set bmRequestType based on request direction and construct wValue/wIndex */
	switch (request) {
	/* SET requests - Host to Device */
	case UVC_SET_CUR:
		bmRequestType = (USB_REQTYPE_DIR_TO_DEVICE << 7) |
				(USB_REQTYPE_TYPE_CLASS << 5) |
				(USB_REQTYPE_RECIPIENT_INTERFACE << 0);
		/* Copy data to buffer for SET requests */
		if (data) {
			memcpy(buf->data, data, data_len);
		}
		break;

	/* GET requests - Device to Host */
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
		/* No need to copy data for GET requests - buffer will be filled by device */
		break;

	default:
		LOG_ERR("Unsupported UVC request: 0x%02x", request);
		ret = -ENOTSUP;
		goto cleanup;
	}

	/* Construct control selector and interface values */
	wValue = control_selector << 8;
	wIndex = uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber;

	LOG_DBG("UVC control request: req=0x%02x, cs=0x%02x, len=%u",
		request, control_selector, data_len);

	/* Send the control transfer using USB Host API */
	ret = usbh_req_setup(uvc_dev->udev, bmRequestType, request, wValue, wIndex, data_len, buf);
	if (ret < 0) {
		LOG_ERR("Failed to send UVC control request 0x%02x: %d", request, ret);
		goto cleanup;
	}

	/* For GET requests (Device to Host), copy received data from buffer to output */
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
	/* Always free the allocated buffer */
	if (buf) {
		usbh_xfer_buf_free(uvc_dev->udev, buf);
	}

	return ret;
}

/**
 * @brief Get current UVC format
 *
 * Retrieves the currently configured video format from UVC device.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @param fmt Pointer to video format structure to fill
 * @return 0 on success, negative error code on failure
 */
int uvc_host_get_format(struct uvc_device *uvc_dev, struct video_format *fmt)
{
	if (!uvc_dev || !fmt) {
		return -EINVAL;
	}

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);

	/* Convert from current_format to video_format */
	fmt->pixelformat = uvc_dev->current_format.pixelformat;
	fmt->width = uvc_dev->current_format.width;
	fmt->height = uvc_dev->current_format.height;
	fmt->pitch = uvc_dev->current_format.pitch;

	k_mutex_unlock(&uvc_dev->lock);

	return 0;
}

/**
 * @brief Set UVC video format and configure streaming
 *
 * Performs complete UVC format negotiation including probe/commit protocol,
 * bandwidth calculation, and streaming interface configuration.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @param pixelformat Target pixel format (FOURCC)
 * @param width Target frame width in pixels
 * @param height Target frame height in pixels
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_set_format(struct uvc_device *uvc_dev, uint32_t pixelformat,
						  uint32_t width, uint32_t height)
{
	uint32_t frame_interval;
	uint32_t required_bandwidth;
	struct uvc_format_header *format;
	struct uvc_frame_header *frame;
	int ret;

	/* 1. Find matching format and frame descriptors */
	ret = uvc_host_find_format(uvc_dev, pixelformat, width, height,
						 &format, &frame, &frame_interval);
	if (ret) {
		LOG_ERR("Unsupported format: %s %ux%u",
				VIDEO_FOURCC_TO_STR(pixelformat), width, height);
		return ret;
	}

	/* 2. Prepare probe/commit structure with negotiation parameters */
	memset(&uvc_dev->video_probe, 0, sizeof(uvc_dev->video_probe));
	uvc_dev->video_probe.bmHint = sys_cpu_to_le16(0x0001);			/* Frame interval hint */
	uvc_dev->video_probe.bFormatIndex = format->bFormatIndex;		 /* Format index from descriptor */
	uvc_dev->video_probe.bFrameIndex = frame->bFrameIndex;			/* Frame index from descriptor */
	uvc_dev->video_probe.dwFrameInterval = sys_cpu_to_le32(frame_interval); /* Frame interval from descriptor */

	LOG_INF("Setting format: %s %ux%u (format_index=%u, frame_index=%u, interval=%u)",
			VIDEO_FOURCC_TO_STR(pixelformat), width, height,
			format->bFormatIndex, frame->bFrameIndex, frame_interval);

	/* 3. Send PROBE request to set desired parameters */
	ret = uvc_host_stream_interface_control(uvc_dev, UVC_SET_CUR, UVC_VS_PROBE_CONTROL, &uvc_dev->video_probe, sizeof(uvc_dev->video_probe));
	if (ret) {
		LOG_ERR("PROBE SET request failed: %d", ret);
		return ret;
	}

	/* 4. Send PROBE GET request to read negotiated parameters */
	memset(&uvc_dev->video_probe, 0, sizeof(uvc_dev->video_probe));
	ret = uvc_host_stream_interface_control(uvc_dev, UVC_GET_CUR, UVC_VS_PROBE_CONTROL, &uvc_dev->video_probe, sizeof(uvc_dev->video_probe));
	if (ret) {
		LOG_ERR("PROBE GET request failed: %d", ret);
		return ret;
	}

	/* TODO: Validate negotiated parameters against requirements */

	/* 5. Send COMMIT request to finalize negotiated parameters */
	ret = uvc_host_stream_interface_control(uvc_dev, UVC_SET_CUR, UVC_VS_COMMIT_CONTROL, &uvc_dev->video_probe, sizeof(uvc_dev->video_probe));
	if (ret) {
		LOG_ERR("COMMIT request failed: %d", ret);
		return ret;
	}

	/* 6. Update device current format information */
	k_mutex_lock(&uvc_dev->lock, K_FOREVER);
	uvc_dev->current_format.pixelformat = pixelformat;
	uvc_dev->current_format.width = width;
	uvc_dev->current_format.height = height;
	uvc_dev->current_format.format_index = format->bFormatIndex;  /* Use descriptor index */
	uvc_dev->current_format.frame_index = frame->bFrameIndex;	 /* Use descriptor index */
	uvc_dev->current_format.frame_interval = frame_interval;
	uvc_dev->current_format.format_ptr = format;				  /* Save format descriptor pointer */
	uvc_dev->current_format.frame_ptr = frame;					/* Save frame descriptor pointer */

	/* 7. Recalculate FPS and pitch based on negotiated parameters */
	if (frame_interval > 0) {
		uvc_dev->current_format.fps = 10000000 / frame_interval;
	} else {
		uvc_dev->current_format.fps = 30; /* Default 30fps */
	}
	uvc_dev->current_format.pitch = width * video_bits_per_pixel(pixelformat) / 8;

	k_mutex_unlock(&uvc_dev->lock);

	/* 8. Calculate required bandwidth for streaming */
	required_bandwidth = uvc_host_calculate_required_bandwidth(uvc_dev);
	if (required_bandwidth == 0) {
		LOG_WRN("Cannot calculate required bandwidth");
		return -EINVAL;
	}

	/* 9. Select appropriate streaming interface alternate setting */
	ret = uvc_host_select_streaming_alternate(uvc_dev, required_bandwidth);
	if (ret) {
		LOG_ERR("Select stream alternate failed: %d", ret);
		return ret;
	}

	/* 10. Configure streaming interface with selected alternate setting */
	ret = usbh_device_interface_set(uvc_dev->udev,
									uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber,
									uvc_dev->current_stream_iface_info.current_stream_iface->bAlternateSetting,
									false);
	if (ret) {
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
 *
 * Updates the frame rate while keeping current format settings.
 * Only performs UVC probe/commit if the frame interval needs to be changed.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @param fps Frame rate in frames per second
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_set_frame_rate(struct uvc_device *uvc_dev, uint32_t fps)
{
	uint32_t target_frame_interval;
	uint32_t best_frame_interval;
	uint32_t min_diff = UINT32_MAX;
	uint32_t required_bandwidth;
	bool found_exact_match = false;
	int ret;

	if (!uvc_dev || fps == 0) {
		return -EINVAL;
	}

	/* Convert fps to frame interval (units of 100ns) */
	target_frame_interval = 10000000 / fps;

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);

	/* Check if current frame interval is already set to target */
	if (uvc_dev->current_format.frame_interval == target_frame_interval) {
		LOG_DBG("Frame rate already set to %u fps", fps);
		k_mutex_unlock(&uvc_dev->lock);
		return 0;
	}

	/* Get current frame descriptor */
	struct uvc_frame_header *frame_ptr = uvc_dev->current_format.frame_ptr;
	if (!frame_ptr) {
		LOG_ERR("No current frame descriptor available");
		k_mutex_unlock(&uvc_dev->lock);
		return -EINVAL;
	}

	/* Parse frame intervals from current frame descriptor */
	if (frame_ptr->bDescriptorSubType == UVC_VS_FRAME_UNCOMPRESSED) {
		struct uvc_vs_frame_uncompressed *frame_desc =
			(struct uvc_vs_frame_uncompressed *)frame_ptr;

		if (frame_desc->bFrameIntervalType == 0) {
			/* Continuous frame intervals */
			uint32_t *intervals = (uint32_t *)((uint8_t *)frame_desc + sizeof(*frame_desc));
			uint32_t min_interval = sys_le32_to_cpu(intervals[0]);
			uint32_t max_interval = sys_le32_to_cpu(intervals[1]);
			uint32_t step_interval = sys_le32_to_cpu(intervals[2]);

			/* Clamp to supported range */
			if (target_frame_interval < min_interval) {
				best_frame_interval = min_interval;
			} else if (target_frame_interval > max_interval) {
				best_frame_interval = max_interval;
			} else {
				/* Find closest step-aligned interval */
				uint32_t steps = (target_frame_interval - min_interval) / step_interval;
				best_frame_interval = min_interval + (steps * step_interval);
				found_exact_match = (best_frame_interval == target_frame_interval);
			}
		} else {
			/* Discrete frame intervals */
			uint32_t *intervals = (uint32_t *)((uint8_t *)frame_desc + sizeof(*frame_desc));

			for (int i = 0; i < frame_desc->bFrameIntervalType; i++) {
				uint32_t interval = sys_le32_to_cpu(intervals[i]);
				uint32_t diff = (interval > target_frame_interval) ?
					(interval - target_frame_interval) : (target_frame_interval - interval);

				if (diff < min_diff) {
					min_diff = diff;
					best_frame_interval = interval;
					found_exact_match = (diff == 0);
				}
			}
		}
	} else if (frame_ptr->bDescriptorSubType == UVC_VS_FRAME_MJPEG) {
		struct uvc_vs_frame_mjpeg *frame_desc =
			(struct uvc_vs_frame_mjpeg *)frame_ptr;

		/* Similar logic for MJPEG frames */
		if (frame_desc->bFrameIntervalType == 0) {
			uint32_t *intervals = (uint32_t *)((uint8_t *)frame_desc + sizeof(*frame_desc));
			uint32_t min_interval = sys_le32_to_cpu(intervals[0]);
			uint32_t max_interval = sys_le32_to_cpu(intervals[1]);
			uint32_t step_interval = sys_le32_to_cpu(intervals[2]);

			if (target_frame_interval < min_interval) {
				best_frame_interval = min_interval;
			} else if (target_frame_interval > max_interval) {
				best_frame_interval = max_interval;
			} else {
				uint32_t steps = (target_frame_interval - min_interval) / step_interval;
				best_frame_interval = min_interval + (steps * step_interval);
				found_exact_match = (best_frame_interval == target_frame_interval);
			}
		} else {
			uint32_t *intervals = (uint32_t *)((uint8_t *)frame_desc + sizeof(*frame_desc));

			for (int i = 0; i < frame_desc->bFrameIntervalType; i++) {
				uint32_t interval = sys_le32_to_cpu(intervals[i]);
				uint32_t diff = (interval > target_frame_interval) ?
					(interval - target_frame_interval) : (target_frame_interval - interval);

				if (diff < min_diff) {
					min_diff = diff;
					best_frame_interval = interval;
					found_exact_match = (diff == 0);
				}
			}
		}
	} else {
		LOG_ERR("Unsupported frame descriptor type: 0x%02x", frame_ptr->bDescriptorSubType);
		k_mutex_unlock(&uvc_dev->lock);
		return -ENOTSUP;
	}

	/* Initialize probe structure with current format settings */
	memset(&uvc_dev->video_probe, 0, sizeof(uvc_dev->video_probe));
	uvc_dev->video_probe.bmHint = sys_cpu_to_le16(0x0001);
	uvc_dev->video_probe.bFormatIndex = uvc_dev->current_format.format_index;
	uvc_dev->video_probe.bFrameIndex = uvc_dev->current_format.frame_index;
	uvc_dev->video_probe.dwFrameInterval = sys_cpu_to_le32(best_frame_interval);

	k_mutex_unlock(&uvc_dev->lock);

	LOG_INF("Setting frame rate: %u fps -> %u fps (%s match)",
			fps, 10000000 / best_frame_interval,
			found_exact_match ? "exact" : "closest");

	/* Send PROBE request */
	ret = uvc_host_stream_interface_control(uvc_dev, UVC_SET_CUR, UVC_VS_PROBE_CONTROL,
									   &uvc_dev->video_probe, sizeof(uvc_dev->video_probe));
	if (ret) {
		LOG_ERR("PROBE SET request failed: %d", ret);
		return ret;
	}

	/* Send PROBE GET request to read negotiated parameters */
	memset(&uvc_dev->video_probe, 0, sizeof(uvc_dev->video_probe));
	ret = uvc_host_stream_interface_control(uvc_dev, UVC_GET_CUR, UVC_VS_PROBE_CONTROL,
									   &uvc_dev->video_probe, sizeof(uvc_dev->video_probe));
	if (ret) {
		LOG_ERR("PROBE GET request failed: %d", ret);
		return ret;
	}

	/* Send COMMIT request */
	ret = uvc_host_stream_interface_control(uvc_dev, UVC_SET_CUR, UVC_VS_COMMIT_CONTROL,
									   &uvc_dev->video_probe, sizeof(uvc_dev->video_probe));
	if (ret) {
		LOG_ERR("COMMIT request failed: %d", ret);
		return ret;
	}

	/* Update current format with new frame interval */
	k_mutex_lock(&uvc_dev->lock, K_FOREVER);
	uvc_dev->current_format.frame_interval = best_frame_interval;
	uvc_dev->current_format.fps = 10000000 / best_frame_interval;
	k_mutex_unlock(&uvc_dev->lock);

	LOG_INF("Frame rate successfully set to %u fps", uvc_dev->current_format.fps);

	/* Calculate required bandwidth for streaming */
	required_bandwidth = uvc_host_calculate_required_bandwidth(uvc_dev);
	if (required_bandwidth == 0) {
		LOG_ERR("Cannot calculate required bandwidth");
		return -EINVAL;
	}

	/* Select appropriate streaming interface alternate setting */
	ret = uvc_host_select_streaming_alternate(uvc_dev, required_bandwidth);
	if (ret) {
		LOG_ERR("Failed to select streaming alternate: %d", ret);
		return ret;
	}

	/* Configure streaming interface with selected alternate setting */
	ret = usbh_device_interface_set(uvc_dev->udev, 
									uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber, 
									uvc_dev->current_stream_iface_info.current_stream_iface->bAlternateSetting, 
									false);
	if (ret) {
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
 * @brief Create video format capabilities from UVC descriptors
 *
 * Parses UVC format and frame descriptors to create an array of
 * video_format_cap structures for the video subsystem.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @return Pointer to format capabilities array, NULL on failure
 */
static struct video_format_cap* uvc_host_create_format_caps(struct uvc_device *uvc_dev)
{
	struct video_format_cap *caps_array = NULL;
	int total_caps = 0;
	int cap_index = 0;

	if (!uvc_dev) {
		return NULL;
	}

	/* Calculate total number of format capabilities */
	struct uvc_vs_format_uncompressed_info *uncompressed_info = &uvc_dev->formats.format_uncompressed;
	struct uvc_vs_format_mjpeg_info *mjpeg_info = &uvc_dev->formats.format_mjpeg;

	/* Count frames in uncompressed formats */
	for (int i = 0; i < uncompressed_info->num_uncompressed_formats; i++) {
		struct uvc_vs_format_uncompressed *format = uncompressed_info->uncompressed_format[i];
		if (format) {
			total_caps += format->bNumFrameDescriptors;
		}
	}

	/* Count frames in MJPEG formats */
	for (int i = 0; i < mjpeg_info->num_mjpeg_formats; i++) {
		struct uvc_vs_format_mjpeg *format = mjpeg_info->vs_mjpeg_format[i];
		if (format) {
			total_caps += format->bNumFrameDescriptors;
		}
	}

	if (total_caps == 0) {
		LOG_WRN("No supported formats found");
		return NULL;
	}

	/* Allocate format capabilities array (+1 for zero terminator) */
	caps_array = k_malloc(sizeof(struct video_format_cap) * (total_caps + 1));
	if (!caps_array) {
		LOG_ERR("Failed to allocate format caps array");
		return NULL;
	}

	memset(caps_array, 0, sizeof(struct video_format_cap) * (total_caps + 1));

	/* Fill uncompressed formats */
	for (int i = 0; i < uncompressed_info->num_uncompressed_formats; i++) {
		struct uvc_vs_format_uncompressed *format = uncompressed_info->uncompressed_format[i];
		if (!format) continue;

		/* Get pixel format from GUID */
		uint32_t pixelformat = uvc_guid_to_fourcc(format->guidFormat);
		if (pixelformat == 0) {
			LOG_WRN("Unsupported GUID format in format index %u", format->bFormatIndex);
			continue;
		}

		/* Parse all frames for this format */
		uint8_t *desc_buf = (uint8_t *)format + format->bLength;
		int frames_found = 0;

		while (frames_found < format->bNumFrameDescriptors && cap_index < total_caps) {
			struct uvc_frame_header *frame_header = (struct uvc_frame_header *)desc_buf;

			if (frame_header->bLength == 0) break;

			if (frame_header->bDescriptorType == UVC_CS_INTERFACE &&
				frame_header->bDescriptorSubType == UVC_VS_FRAME_UNCOMPRESSED) {

				if (frame_header->bLength >= sizeof(struct uvc_frame_header)) {
					uint16_t width = sys_le16_to_cpu(frame_header->wWidth);
					uint16_t height = sys_le16_to_cpu(frame_header->wHeight);

					/* Create format capability entry */
					caps_array[cap_index].pixelformat = pixelformat;
					caps_array[cap_index].width_min = width;
					caps_array[cap_index].width_max = width;
					caps_array[cap_index].height_min = height;
					caps_array[cap_index].height_max = height;
					caps_array[cap_index].width_step = 0;
					caps_array[cap_index].height_step = 0;

					LOG_DBG("Added format cap[%d]: %s %ux%u",
							cap_index, VIDEO_FOURCC_TO_STR(pixelformat), width, height);

					cap_index++;
					frames_found++;
				}
			} else if (frame_header->bDescriptorType == UVC_CS_INTERFACE &&
					  (desc_buf[2] == UVC_VS_FORMAT_UNCOMPRESSED || desc_buf[2] == UVC_VS_FORMAT_MJPEG)) {
				/* Found new format descriptor, stop parsing current format frames */
				break;
			}

			desc_buf += frame_header->bLength;
		}
	}

	/* Fill MJPEG formats */
	for (int i = 0; i < mjpeg_info->num_mjpeg_formats; i++) {
		struct uvc_vs_format_mjpeg *format = mjpeg_info->vs_mjpeg_format[i];
		if (!format) continue;

		/* MJPEG format uses fixed VIDEO_PIX_FMT_MJPEG */
		uint32_t pixelformat = VIDEO_PIX_FMT_MJPEG;

		/* Parse all frames for this format */
		uint8_t *desc_buf = (uint8_t *)format + format->bLength;
		int frames_found = 0;

		while (frames_found < format->bNumFrameDescriptors && cap_index < total_caps) {
			struct uvc_frame_header *frame_header = (struct uvc_frame_header *)desc_buf;

			if (frame_header->bLength == 0) break;

			if (frame_header->bDescriptorType == UVC_CS_INTERFACE &&
				frame_header->bDescriptorSubType == UVC_VS_FRAME_MJPEG) {
				if (frame_header->bLength >= sizeof(struct uvc_frame_header)) {
					uint16_t width = sys_le16_to_cpu(frame_header->wWidth);
					uint16_t height = sys_le16_to_cpu(frame_header->wHeight);

					/* Create format capability entry */
					caps_array[cap_index].pixelformat = pixelformat;
					caps_array[cap_index].width_min = width;
					caps_array[cap_index].width_max = width;
					caps_array[cap_index].height_min = height;
					caps_array[cap_index].height_max = height;
					caps_array[cap_index].width_step = 0;
					caps_array[cap_index].height_step = 0;

					LOG_DBG("Added format cap[%d]: %s %ux%u",
							cap_index, VIDEO_FOURCC_TO_STR(pixelformat), width, height);

					cap_index++;
					frames_found++;
				}
			} else if (frame_header->bDescriptorType == UVC_CS_INTERFACE &&
					  (desc_buf[2] == UVC_VS_FORMAT_UNCOMPRESSED || desc_buf[2] == UVC_VS_FORMAT_MJPEG)) {
				/* Found new format descriptor, stop parsing current format frames */
				break;
			}

			desc_buf += frame_header->bLength;
		}
	}

	LOG_INF("Created %d format capabilities from UVC descriptors", cap_index);
	return caps_array;
}

/**
 * @brief Get UVC device capabilities
 *
 * Retrieves device capabilities including supported formats and buffer requirements.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @param caps Pointer to video capabilities structure to fill
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_get_device_caps(struct uvc_device *uvc_dev, struct video_caps *caps)
{
	if (!uvc_dev || !caps) {
		return -EINVAL;
	}

	/* Set basic capabilities */
	caps->min_vbuf_count = 1;			/* UVC typically needs 1 buffers */
	caps->min_line_count = LINE_COUNT_HEIGHT;  /* Only support complete frames */
	caps->max_line_count = LINE_COUNT_HEIGHT;  /* Maximum one complete frame */

	/* Create format capabilities array from UVC descriptors */
	if (uvc_dev->video_format_caps) {
		/* Already created, use existing */
		caps->format_caps = uvc_dev->video_format_caps;
	} else {
		/* First call, create format capabilities array */
		uvc_dev->video_format_caps = uvc_host_create_format_caps(uvc_dev);
		if (!uvc_dev->video_format_caps) {
			LOG_ERR("Failed to create format capabilities");
			return -ENOMEM;
		}
		caps->format_caps = uvc_dev->video_format_caps;
	}

	return 0;
}

/**
 * @brief UVC host pre-initialization
 *
 * Initialize basic data structures for UVC device including FIFOs and mutex.
 * Called during device initialization before USB connection.
 *
 * @param dev Pointer to video device structure
 * @return 0 on success
 */
bool uvc_host_preinit(const struct device *dev)
{
	struct uvc_device *uvc_dev = dev->data;

	/* Initialize input and output buffer queues */
	k_fifo_init(&uvc_dev->fifo_in);
	k_fifo_init(&uvc_dev->fifo_out);

	/* Initialize device access mutex */
	k_mutex_init(&uvc_dev->lock);

	return 0;
}

/**
 * @brief Remove UVC payload header and extract video data
 *
 * Processes UVC payload packets by removing the UVC-specific header
 * and extracting the actual video data payload. Validates header
 * integrity and prevents buffer overflow.
 *
 * @param buf Pointer to network buffer containing UVC payload packet
 * @param vbuf Pointer to video buffer to store extracted payload data
 * @return Number of payload bytes extracted on success, negative error code on failure
 * @retval -EINVAL Invalid parameters or malformed header
 * @retval -ENODATA Packet too short to contain valid header
 * @retval -ENOSPC Video buffer would overflow with this payload
 */
static int uvc_host_remove_payload_header(struct net_buf *buf, struct video_buffer *vbuf)
{
	struct uvc_payload_header *header;
	uint8_t header_len;
	size_t payload_len;

	/* Validate input parameters */
	if (!buf || !vbuf || !buf->data) {
		LOG_ERR("Invalid parameters: buf=%p, vbuf=%p", buf, vbuf);
		return -EINVAL;
	}

	/* Check minimum packet size for UVC header */
	if (buf->len < 2) {
		LOG_ERR("Packet too short: %u bytes", buf->len);
		return -ENODATA;
	}

	/* Extract UVC payload header information */
	header = (struct uvc_payload_header *)buf->data;
	header_len = header->bHeaderLength;

	/* Validate header length against packet size */
	if (header_len > buf->len) {
		LOG_ERR("Invalid header length: %u > %u", header_len, buf->len);
		return -EINVAL;
	}

	/* Calculate actual payload data size */
	payload_len = buf->len - header_len;

	/* Prevent video buffer overflow */
	if (vbuf->bytesused + payload_len > vbuf->size) {
		LOG_ERR("Buffer overflow: used=%u, payload=%u, capacity=%u",
				vbuf->bytesused, payload_len, vbuf->size);
		return -ENOSPC;
	}

	LOG_DBG("Header: len=%u, payload=%u, bmHeaderInfo=0x%02x",
			header_len, payload_len, header->bmHeaderInfo);

	/* Copy payload data to video buffer if present */
	if (payload_len > 0) {
		memcpy(vbuf->buffer + vbuf->bytesused, buf->data + header_len, payload_len);
	}

	/* Return number of payload bytes processed */
	return (int)payload_len;
}

/**
 * @brief ISO transfer completion callback
 *
 * Handles completion of isochronous video data transfers. Processes
 * received video data, removes UVC headers, and manages frame completion.
 *
 * @param dev Pointer to USB device
 * @param xfer Pointer to completed transfer
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_stream_iso_req_cb(struct usb_device *const dev, struct uhc_transfer *const xfer)
{
	struct uvc_device *uvc_dev = (struct uvc_device *)xfer->priv;
	struct net_buf *buf = xfer->buf;
	struct video_buffer *vbuf = *(struct video_buffer **)net_buf_user_data(buf);
	struct uvc_payload_header *payload_header;
	uint8_t endOfFrame;
	uint32_t payload_len;

	/* Validate callback parameters */
	if (!buf || !uvc_dev) {
		LOG_ERR("Invalid callback parameters");
		return -EINVAL;
	}

	/* Handle transfer completion status */
	if (xfer->err == -ECONNRESET) {
		LOG_INF("ISO transfer canceled");
	} else if (xfer->err) {
		LOG_WRN("ISO request failed, err %d", xfer->err);
	} else {
		LOG_DBG("ISO request finished, len=%u", buf->len);
	}

	/* Process received video data if present */
	if (buf->len > 0 && vbuf)
	{
		/* Extract frame end marker from payload header */
		payload_header = (struct uvc_payload_header *)buf->data;
		endOfFrame = payload_header->bmHeaderInfo & UVC_BMHEADERINFO_END_OF_FRAME;
		/* Remove UVC header and extract payload data */
		payload_len = uvc_host_remove_payload_header(buf, vbuf);
		if (payload_len < 0) {
			LOG_ERR("Header removal failed: %d", payload_len);
			goto cleanup;
		}

		/* Update video buffer with processed data */
		vbuf->bytesused += payload_len;
		uvc_dev->vbuf_offset = vbuf->bytesused;

		LOG_DBG("Processed %u payload bytes, total: %u, EOF: %u",
				payload_len, vbuf->bytesused, endOfFrame);

		/* Handle frame completion */
		if (endOfFrame) {
			LOG_INF("Frame completed: %u bytes", vbuf->bytesused);
			/* Release network buffer reference */
			net_buf_unref(buf);
			/* Move completed buffer from input to output queue */
			k_fifo_get(&uvc_dev->fifo_in, K_NO_WAIT);
			k_fifo_put(&uvc_dev->fifo_out, vbuf);

			/* Clean up transfer resources */
			uvc_dev->vbuf_offset = 0;
			usbh_xfer_free(dev, xfer);
			uvc_dev->transfer_count = 0;

			/* Signal frame completion to application */
			LOG_DBG("Raising VIDEO_BUF_DONE signal");
			k_poll_signal_raise(uvc_dev->sig, VIDEO_BUF_DONE);
			return 0;
		}
	}

cleanup:
	/* Release network buffer reference */
	net_buf_unref(buf);
	/* Continue processing pending buffers */
	uvc_host_flush_queue(uvc_dev, vbuf);
	return 0;
}

/**
 * @brief Initiate new video transfer
 *
 * Allocates and initializes a new USB transfer for video data streaming.
 * Sets up transfer buffer and associates it with video buffer.
 *
 * @param uvc_dev Pointer to uvc class
 * @param vbuf Pointer to video buffer for data storage
 * @return Pointer to allocated transfer on success, NULL on failure
 */
static struct uhc_transfer *uvc_host_initiate_transfer(struct uvc_device *uvc_dev,
						 struct video_buffer *const vbuf)
{
	struct net_buf *buf;
	struct uhc_transfer *xfer;

	/* Validate input parameters */
	if (!vbuf || !uvc_dev || !uvc_dev->current_stream_iface_info.current_stream_ep) {
		LOG_ERR("Invalid parameters for transfer initiation");
		return NULL;
	}

	LOG_DBG("Initiating transfer: ep=0x%02x, vbuf=%p",
			uvc_dev->current_stream_iface_info.current_stream_ep->bEndpointAddress, vbuf);

	/* Allocate USB transfer with callback */
	xfer = usbh_xfer_alloc(uvc_dev->udev, uvc_dev->current_stream_iface_info.current_stream_ep->bEndpointAddress,
					uvc_host_stream_iso_req_cb, uvc_dev);
	if (!xfer) {
		LOG_ERR("Failed to allocate transfer");
		return NULL;
	}

	/* Allocate transfer buffer with maximum packet size */
	buf = usbh_xfer_buf_alloc(&uvc_dev->udev, uvc_dev->current_stream_iface_info.cur_ep_mps_mult);
	if (!buf) {
		LOG_ERR("Failed to allocate buffer");
		usbh_xfer_free(uvc_dev->udev, xfer);
		return NULL;
	}	
	buf->len = 0;

	/* Reset buffer offset and associate video buffer with transfer */
	uvc_dev->vbuf_offset = 0;
	vbuf->bytesused = 0;
	memset(vbuf->buffer, 0, vbuf->size);
	/* Save video buffer pointer in transfer's user data */
	*(void **)net_buf_user_data(buf) = vbuf;
	xfer->buf = buf;
	vbuf->driver_data = (void *)xfer;

	LOG_DBG("Transfer allocated: buf=%p, size=%u", buf, uvc_dev->current_stream_iface_info.cur_ep_mps_mult);
	return xfer;
}

/**
 * @brief Continue existing video transfer
 *
 * Allocates continuation buffer for ongoing video transfer when
 * multiple packets are needed for a single video buffer.
 *
 * @param uvc_dev Pointer to uvc class
 * @param vbuf Pointer to video buffer being filled
 * @return Pointer to allocated buffer on success, NULL on failure
 */
static struct net_buf *uvc_host_continue_transfer(struct uvc_device *uvc_dev,
						 struct video_buffer *const vbuf)
{
	struct net_buf *buf;

	/* Allocate buffer at offset position for continuation */
	buf = usbh_xfer_buf_alloc(&uvc_dev->udev, uvc_dev->current_stream_iface_info.cur_ep_mps_mult);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return NULL;
	}
	buf->len = 0;

	*(void **)net_buf_user_data(buf) = vbuf;

	return buf;
}

/**
 * @brief Flush video buffer to USB endpoint
 *
 * Initiates or continues USB transfer for a video buffer, handling both
 * initial transfers and continuation transfers for large frames.
 *
 * @param uvc_dev Pointer to uvc class
 * @param vbuf Pointer to video buffer to transfer
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_flush_vbuf(struct uvc_device *uvc_dev, struct video_buffer *const vbuf)
{
	struct net_buf *buf;
	struct uhc_transfer *xfer;
	int ret;

	if (uvc_dev->transfer_count == 0) {
		xfer = uvc_host_initiate_transfer(uvc_dev, vbuf);
	} else {
		buf = uvc_host_continue_transfer(uvc_dev, vbuf);
		xfer = (struct uhc_transfer *)vbuf->driver_data;
		xfer->buf = buf;
	}

	if (xfer == NULL || xfer->buf == NULL) {
		return -ENOMEM;
	}

	ret = usbh_xfer_enqueue(uvc_dev->udev, xfer);
	if (ret != 0) {
		LOG_ERR("Enqueue failed: ret=%d", ret);
		net_buf_unref(buf);
		return ret;
	}

	uvc_dev->transfer_count++;
	return 0;
}

/**
 * @brief Process all pending video buffers in input queue
 *
 * Thread-safe processing of video buffers waiting for USB transfer.
 *
 * @param uvc_dev Pointer to uvc class
 * @param vbuf Pointer to video buffer to flush
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_flush_queue(struct uvc_device *uvc_dev, struct video_buffer *vbuf)
{
	int ret = 0;

	/* Lock to ensure atomic processing of buffer queue */
	LOG_DBG("Locking the UVC stream");
	k_mutex_lock(&uvc_dev->lock, K_FOREVER);

	if (uvc_dev->streaming) {
		ret = uvc_host_flush_vbuf(uvc_dev, vbuf);
		if (ret) {
			LOG_ERR("Failed to flush video buffer: %d", ret);
		}
	}

	LOG_DBG("Unlocking the UVC stream");
	k_mutex_unlock(&uvc_dev->lock);

	return ret;
}

/**
 * @brief Enumerate frame intervals for a given frame
 *
 * Parses UVC frame descriptor to extract supported frame intervals.
 *
 * @param frame_ptr Pointer to UVC frame descriptor
 * @param fie Pointer to frame interval enumeration structure
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_enum_frame_intervals(struct uvc_frame_header *frame_ptr, struct video_frmival_enum *fie)
{
	uint8_t *desc_buf;

	if (!frame_ptr || !fie) {
		return -EINVAL;
	}

	/* Ensure descriptor contains frame interval data */
	if (frame_ptr->bLength < 26) {
		LOG_ERR("Frame descriptor too short for interval data");
		return -EINVAL;
	}

	desc_buf = (uint8_t *)frame_ptr;
	uint8_t interval_type = desc_buf[25]; /* bFrameIntervalType */
	uint8_t *interval_data = desc_buf + 26; /* dwFrameInterval data */

	LOG_DBG("Enumerating frame intervals: frame_index=%u, interval_type=%u, fie_index=%u",
			frame_ptr->bFrameIndex, interval_type, fie->index);

	if (interval_type == 0) {
		/* Continuous/stepwise frame intervals */
		if (fie->index > 0) {
			return -EINVAL; /* Only one entry for continuous type */
		}

		if (frame_ptr->bLength < 38) { /* Need 3 32-bit values */
			LOG_ERR("Frame descriptor too short for stepwise intervals");
			return -EINVAL;
		}

		fie->type = VIDEO_FRMIVAL_TYPE_STEPWISE;
		fie->stepwise.min.numerator = sys_le32_to_cpu(*(uint32_t*)(interval_data + 0));
		fie->stepwise.min.denominator = 10000000; /* 100ns units */
		fie->stepwise.max.numerator = sys_le32_to_cpu(*(uint32_t*)(interval_data + 4));
		fie->stepwise.max.denominator = 10000000;
		fie->stepwise.step.numerator = sys_le32_to_cpu(*(uint32_t*)(interval_data + 8));
		fie->stepwise.step.denominator = 10000000;

		LOG_DBG("Stepwise intervals: min=%u, max=%u, step=%u",
				fie->stepwise.min.numerator, fie->stepwise.max.numerator, fie->stepwise.step.numerator);

	} else if (interval_type > 0) {
		/* Discrete frame intervals */
		uint8_t num_intervals = interval_type;

		if (fie->index >= num_intervals) {
			return -EINVAL; /* Index out of range */
		}

		if (frame_ptr->bLength < (26 + num_intervals * 4)) {
			LOG_ERR("Frame descriptor too short for %u discrete intervals", num_intervals);
			return -EINVAL;
		}

		fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
		fie->discrete.numerator = sys_le32_to_cpu(*(uint32_t*)(interval_data + fie->index * 4));
		fie->discrete.denominator = 10000000; /* 100ns units */

		LOG_DBG("Discrete interval[%u]: %u/10000000 (%u ns)",
				fie->index, fie->discrete.numerator, fie->discrete.numerator * 100);
	} else {
		LOG_ERR("Invalid frame interval type: %u", interval_type);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Get current gain control value from UVC device
 *
 * Retrieves the current gain setting from the UVC Processing Unit.
 *
 * @param uvc_dev Pointer to UVC device structure
 * @param gain_val Pointer to store current gain value
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_get_current_gain(struct uvc_device *uvc_dev, int32_t *gain_val)
{
	uint16_t current_gain;
	struct net_buf *buf;
	int ret;

	if (!uvc_dev || !uvc_dev->udev || !gain_val) {
		LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	/* Allocate buffer for control request */
	buf = usbh_xfer_buf_alloc(uvc_dev->udev, sizeof(current_gain));
	if (!buf) {
		LOG_ERR("Failed to allocate buffer for gain query");
		return -ENOMEM;
	}

	/* Send GET_CUR control request to retrieve current gain */
	ret = usbh_req_setup(uvc_dev->udev,
				(USB_REQTYPE_DIR_TO_HOST << 7) |
				(USB_REQTYPE_TYPE_CLASS << 5) |
				(USB_REQTYPE_RECIPIENT_INTERFACE << 0),
				 UVC_GET_CUR,
				 (UVC_PU_GAIN_CONTROL << 8),
				 uvc_dev->current_control_interface->bInterfaceNumber,
				 sizeof(current_gain),
				 buf);

	if (ret) {
		LOG_ERR("Failed to get current gain value: %d", ret);
		goto cleanup;
	}

	if (buf->len < sizeof(current_gain)) {
		LOG_ERR("Insufficient data received for gain value");
		ret = -EIO;
		goto cleanup;
	}

	/* Extract gain value and convert byte order */
	current_gain = sys_le16_to_cpu(net_buf_pull_le16(buf));
	*gain_val = (int32_t)current_gain;

	LOG_DBG("Current hardware gain value: %d", *gain_val);

cleanup:
	usbh_xfer_buf_free(uvc_dev->udev, buf);
	return ret;
}

/**
 * @brief Send UVC control request to unit or terminal
 * @param uvc_dev UVC device
 * @param request Request type (SET_CUR, GET_CUR, etc.)
 * @param control_selector Control selector (for individual controls, 0 for ALL requests)
 * @param entity_id Unit/Terminal ID
 * @param data Control data buffer (for SET: input data, for GET: output buffer)
 * @param data_len Data size
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_control_unit_and_terminal_request(struct uvc_device *uvc_dev, uint8_t request,
					uint8_t control_selector, uint8_t entity_id,
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

	/* Always allocate transfer buffer for both SET and GET requests */
	buf = usbh_xfer_buf_alloc(uvc_dev->udev, data_len);
	if (!buf) {
		LOG_ERR("Failed to allocate transfer buffer of size %u", data_len);
		return -ENOMEM;
	}

	/* Set bmRequestType based on request direction and construct wValue/wIndex */
	switch (request) {
	/* SET requests - Host to Device */
	case UVC_SET_CUR:
		bmRequestType = (USB_REQTYPE_DIR_TO_DEVICE << 7) |
				(USB_REQTYPE_TYPE_CLASS << 5) |
				(USB_REQTYPE_RECIPIENT_INTERFACE << 0);
		wValue = control_selector << 8;
		wIndex = (entity_id << 8) | uvc_dev->current_control_interface->bInterfaceNumber;

		/* Copy data to buffer for SET requests */
		if (data) {
			memcpy(buf->data, data, data_len);
			net_buf_add(buf, data_len);
		}
		break;

	case UVC_SET_CUR_ALL:
		bmRequestType = (USB_REQTYPE_DIR_TO_DEVICE << 7) |
				(USB_REQTYPE_TYPE_CLASS << 5) |
				(USB_REQTYPE_RECIPIENT_INTERFACE << 0);
		wValue = 0x0000; /* All controls */
		wIndex = (entity_id << 8) | uvc_dev->current_control_interface->bInterfaceNumber;

		/* Copy data to buffer for SET requests */
		if (data) {
			memcpy(buf->data, data, data_len);
			net_buf_add(buf, data_len);
		}
		break;

	/* GET requests - Device to Host */
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
		wValue = control_selector << 8;
		wIndex = (entity_id << 8) | uvc_dev->current_control_interface->bInterfaceNumber;
		/* No need to copy data for GET requests - buffer will be filled by device */
		break;

	case UVC_GET_CUR_ALL:
	case UVC_GET_MIN_ALL:
	case UVC_GET_MAX_ALL:
	case UVC_GET_RES_ALL:
	case UVC_GET_DEF_ALL:
		bmRequestType = (USB_REQTYPE_DIR_TO_HOST << 7) |
				(USB_REQTYPE_TYPE_CLASS << 5) |
				(USB_REQTYPE_RECIPIENT_INTERFACE << 0);
		wValue = 0x0000; /* All controls */
		wIndex = (entity_id << 8) | uvc_dev->current_control_interface->bInterfaceNumber;
		/* No need to copy data for GET requests - buffer will be filled by device */
		break;

	default:
		LOG_ERR("Unsupported UVC request: 0x%02x", request);
		ret = -ENOTSUP;
		goto cleanup;
	}

	LOG_DBG("UVC control request: req=0x%02x, cs=0x%02x, entity=0x%02x, len=%u",
		request, control_selector, entity_id, data_len);

	/* Send the control transfer using USB Host API */
	ret = usbh_req_setup(uvc_dev->udev, bmRequestType, request, wValue, wIndex, data_len, buf);
	if (ret < 0) {
		LOG_ERR("Failed to send UVC control request 0x%02x to entity %d: %d",
			request, entity_id, ret);
		goto cleanup;
	}

	/* For GET requests (Device to Host), copy received data from buffer to output */
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
	/* Always free the allocated buffer */
	if (buf) {
		usbh_xfer_buf_free(uvc_dev->udev, buf);
	}

	return ret;
}

static int uvc_host_init(struct usbh_class_data *cdata)
{
	const struct device *dev = cdata->priv;
	struct uvc_device *uvc_dev = (struct uvc_device *)dev->data;

	if (!cdata || !dev || !uvc_dev) {
		LOG_ERR("Invalid parameters for UVC host init");
		return -EINVAL;
	}

	LOG_INF("Initializing UVC device structure");

	/** Initialize basic device state */
	uvc_dev->udev = NULL;
	uvc_dev->connected = false;

	/** Initialize transfer related parameters */
	uvc_dev->vbuf_offset = 0;
	uvc_dev->transfer_count = 0;

	/** Initialize FIFO queues */
	k_fifo_init(&uvc_dev->fifo_in);
	k_fifo_init(&uvc_dev->fifo_out);

	/** Initialize mutex lock */
	k_mutex_init(&uvc_dev->lock);

	/** Initialize USB camera control structure */
	memset(&uvc_dev->ctrls, 0, sizeof(struct usb_camera_ctrls));

	/** Initialize stream interface array */
	for (int i = 0; i < UVC_STREAM_INTERFACES_MAX_ALT; i++) {
		uvc_dev->stream_ifaces[i] = NULL;
	}

	/** Initialize interface information */
	uvc_dev->current_control_interface = NULL;
	memset(&uvc_dev->current_stream_iface_info, 0, sizeof(struct uvc_stream_iface_info));

	/** Initialize descriptor pointers */
	uvc_dev->vc_header = NULL;
	uvc_dev->vc_itd = NULL;
	uvc_dev->vc_otd = NULL;
	uvc_dev->vc_ctd = NULL;
	uvc_dev->vc_sud = NULL;
	uvc_dev->vc_pud = NULL;
	uvc_dev->vc_encoding_unit = NULL;
	uvc_dev->vc_extension_unit = NULL;
	uvc_dev->vs_input_header = NULL;
	uvc_dev->vs_output_header = NULL;


	/** Initialize format information */
	memset(&uvc_dev->formats, 0, sizeof(struct uvc_vs_format_info));
	if (uvc_dev->video_format_caps) {
		k_free(uvc_dev->video_format_caps);
		uvc_dev->video_format_caps = NULL;
	}

	/** Initialize current format information */
	memset(&uvc_dev->current_format, 0, sizeof(struct uvc_vs_format));

	LOG_INF("UVC device structure initialized successfully");
	return 0;
}

/**
 * @brief Handle UVC device connection
 *
 * Called when a UVC device is connected. Parses descriptors,
 * configures the device, and initializes controls.
 *
 * @param udev USB device
 * @param cdata USB host class data
 * @param desc_start_addr Start of descriptor segment
 * @param desc_end_addr End of descriptor segment  
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_connected(struct usb_device *udev, struct usbh_class_data *cdata, void *desc_start_addr, void *desc_end_addr)
{
	const struct device *dev = cdata->priv;
	struct uvc_device *uvc_dev = (struct uvc_device *)dev->data;
	int ret;

	if (cdata->class_matched)
	{
		return 0; /* Already processed, exit early */
	}
	cdata->class_matched = 1;

	LOG_INF("UVC device connected");

	if (!udev || udev->state != USB_STATE_CONFIGURED) {
		LOG_ERR("USB device not properly configured");
		return -ENODEV;
	}

	if (!uvc_dev) {
		LOG_ERR("No UVC device instance available");
		return -ENODEV;
	}
	/* Associate USB device with UVC device */
	uvc_dev->udev = udev;
	uvc_dev->desc_start = desc_start_addr;
	uvc_dev->desc_end = desc_end_addr;

	/* Check if device is already in use */
	k_mutex_lock(&uvc_dev->lock, K_FOREVER);
	if (uvc_dev->connected) {
		k_mutex_unlock(&uvc_dev->lock);
		LOG_WRN("UVC device instance already in use");
		return -EBUSY;
	}

	/* Parse UVC-specific descriptors */
	ret = uvc_host_parse_descriptors(uvc_dev);
	if (ret) {
		LOG_ERR("Failed to parse UVC descriptors: %d", ret);
		goto error;
	}

	/* Configure UVC device */
	ret = uvc_host_configure_device(uvc_dev);
	if (ret) {
		LOG_ERR("Failed to configure UVC device: %d", ret);
		goto error;
	}

	/* Select default format - does not start actual transmission */
	ret = uvc_host_select_default_format(uvc_dev);
	if (ret) {
		LOG_ERR("Failed to set UVC default format : %d", ret);
		goto error;
	}

	/* Initialize USB camera controls */
	ret = usb_host_camera_init_controls(dev);
	if (ret) {
		LOG_ERR("Failed to initialize USB camera controls: %d", ret);
		goto error;
	}

	/* Mark as connected */
	uvc_dev->connected = true;

	/* Trigger device connection event signal */
#ifdef CONFIG_POLL
	if (uvc_dev->sig) {
		k_poll_signal_raise(uvc_dev->sig, USBH_DEVICE_CONNECTED);
		LOG_DBG("UVC device connected signal raised");
	}
#endif
	k_mutex_unlock(&uvc_dev->lock);

	LOG_INF("UVC device connected successfully");
	return 0;

error:
	uvc_dev->udev = NULL;
	k_mutex_unlock(&uvc_dev->lock);
	return ret;
}

/**
 * @brief Handle UVC device disconnection
 *
 * Called when a UVC device is disconnected. Stops streaming,
 * cleans up resources, and resets device state.
 *
 * @param udev USB device
 * @param cdata USB host class data
 * @return 0 on success, negative error code on failure
 */
static int uvc_host_removed(struct usb_device *udev, struct usbh_class_data *cdata)
{
	const struct device *dev = cdata->priv;
	struct uvc_device *uvc_dev = (struct uvc_device *)dev->data;
	int ret = 0;

	if (!uvc_dev) {
		LOG_ERR("No UVC device instance available");
		return -ENODEV;
	}

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);

	/* Check if device was actually connected */
	if (!uvc_dev->connected || uvc_dev->udev != udev) {
		k_mutex_unlock(&uvc_dev->lock);
		LOG_WRN("UVC device was not connected or different device");
		cdata->class_matched = 0;
		return -ENODEV;
	}

	/* Reset video buffer state */
	uvc_dev->vbuf_offset = 0;
	uvc_dev->transfer_count = 0;

	/* Clean up USB camera controls */
	LOG_DBG("Cleaning up camera controls");
	memset(&uvc_dev->ctrls, 0, sizeof(uvc_dev->ctrls));

	/* Clear streaming interface information */
	uvc_dev->streaming = false;
	memset(uvc_dev->stream_ifaces, 0, sizeof(uvc_dev->stream_ifaces));
	uvc_dev->current_control_interface = NULL;
	memset(&uvc_dev->current_stream_iface_info, 0, sizeof(uvc_dev->current_stream_iface_info));

	/* Clear Video Control descriptors */
	LOG_DBG("Clearing Video Control descriptors");
	uvc_dev->vc_header = NULL;
	uvc_dev->vc_itd = NULL;
	uvc_dev->vc_otd = NULL;
	uvc_dev->vc_ctd = NULL;
	uvc_dev->vc_sud = NULL;
	uvc_dev->vc_pud = NULL;
	uvc_dev->vc_encoding_unit = NULL;
	uvc_dev->vc_extension_unit = NULL;

	/* Clear Video Streaming descriptors */
	LOG_DBG("Clearing Video Streaming descriptors");
	uvc_dev->vs_input_header = NULL;
	uvc_dev->vs_output_header = NULL;

	/* Clear format information */
	memset(&uvc_dev->formats, 0, sizeof(uvc_dev->formats));
	memset(&uvc_dev->current_format, 0, sizeof(uvc_dev->current_format));

	/* Free video format capabilities if allocated */
	if (uvc_dev->video_format_caps) {
		LOG_DBG("Freeing video format capabilities");
		k_free(uvc_dev->video_format_caps);
		uvc_dev->video_format_caps = NULL;
	}

	/* Clear probe/commit buffer */
	memset(&uvc_dev->video_probe, 0, sizeof(uvc_dev->video_probe));

	/* Clear device association */
	uvc_dev->udev = NULL;
	uvc_dev->desc_start = NULL;
	uvc_dev->desc_end = NULL;

	/* Mark as disconnected */
	uvc_dev->connected = false;

	/* Reset class matched flag */
	cdata->class_matched = 0;

	/* Trigger device disconnection event signal */
#ifdef CONFIG_POLL
	if (uvc_dev->sig) {
		k_poll_signal_raise(uvc_dev->sig, USBH_DEVICE_DISCONNECTED);
		LOG_DBG("UVC device disconnected signal raised");
	}
#endif

	k_mutex_unlock(&uvc_dev->lock);
	return 0;
}

/** TODO */
static int uvc_host_suspended(struct usbh_context *const uhs_ctx)
{
	return 0;
}

/** TODO */
static int uvc_host_resumed(struct usbh_context *const uhs_ctx)
{
	return 0;
}

/** TODO */
static int uvc_host_rwup(struct usbh_context *const uhs_ctx)
{
	return 0;
}

/**
 * @brief Video API implementation for setting format
 *
 * Video subsystem callback to set video format. Validates parameters
 * and delegates to UVC-specific format setting function.
 *
 * @param dev Pointer to video device
 * @param fmt Pointer to video format structure
 * @return 0 on success, negative error code on failure
 */
static int video_usb_uvc_host_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct uvc_device *uvc_dev = dev->data;  /* Get UVC device directly */
	int ret;

	if (!uvc_dev || !uvc_dev->udev) {
		LOG_ERR("No UVC device connected");
		return -ENODEV;
	}

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);

	/* Set format using UVC protocol */
	ret = uvc_host_set_format(uvc_dev, fmt->pixelformat, fmt->width, fmt->height);
	if (ret) {
		LOG_ERR("Failed to set UVC format: %d", ret);
		goto unlock;
	}

unlock:
	k_mutex_unlock(&uvc_dev->lock);
	return ret;
}

/**
 * @brief Get current video format
 *
 * Video API implementation to retrieve active video format configuration.
 *
 * @param dev Pointer to video device
 * @param fmt Pointer to store current format
 * @return 0 on success, -ENODEV if device not available
 */
static int video_usb_uvc_host_get_fmt(const struct device *dev, struct video_format *fmt)
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
 * @brief Get UVC device capabilities
 *
 * @param dev Pointer to device structure
 * @param caps Pointer to store device capabilities
 * @return 0 on success, negative error code on failure
 */
static int video_usb_uvc_host_get_caps(const struct device *dev, struct video_caps *caps)
{
	struct uvc_device *uvc_dev = dev->data;

	if (!uvc_dev) {
		return -ENODEV;
	}

	return uvc_host_get_device_caps(uvc_dev, caps);
}

/**
 * @brief Set video frame interval (frame rate)
 *
 * @param dev Pointer to device structure
 * @param frmival Pointer to frame interval structure
 * @return 0 on success, negative error code on failure
 */
static int video_usb_uvc_host_set_frmival(const struct device *dev, struct video_frmival *frmival)
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

	/* Convert frame interval to frame rate */
	fps = frmival->denominator / frmival->numerator;

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);

	ret = uvc_host_set_frame_rate(uvc_dev, fps);
	if (ret) {
		LOG_ERR("Failed to set UVC frame rate: %d", ret);
	}

	k_mutex_unlock(&uvc_dev->lock);
	return ret;
}

/**
 * @brief Get current frame interval of UVC device
 *
 * This function retrieves the current frame interval from the UVC device.
 * The frame interval is expressed as a fraction (numerator/denominator)
 * representing the time between consecutive frames.
 *
 * @param dev Pointer to the video device
 * @param frmival Pointer to structure to store frame interval information
 *
 * @return 0 on success, negative error code on failure
 */
static int video_usb_uvc_host_get_frmival(const struct device *dev, struct video_frmival *frmival)
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

	/* Check if current format is valid */
	if (uvc_dev->current_format.fps == 0) {
		LOG_ERR("Invalid current format fps: %u", uvc_dev->current_format.fps);
		k_mutex_unlock(&uvc_dev->lock);
		return -EINVAL;
	}
	/* Convert fps to frame interval fraction
	 * Frame interval = 1 / fps (in seconds)
	 * So numerator = 1, denominator = fps
	 */
	frmival->numerator = 1;
	frmival->denominator = uvc_dev->current_format.fps;

	k_mutex_unlock(&uvc_dev->lock);

	LOG_DBG("Current frame interval: %u/%u (fps=%u)",
		frmival->numerator, frmival->denominator, uvc_dev->current_format.fps);

	return 0;
}

/**
 * @brief Enumerate supported frame intervals
 *
 * Video API implementation to list supported frame intervals for current format.
 *
 * @param dev Pointer to device structure
 * @param fie Pointer to frame interval enumeration structure
 * @return 0 on success, negative error code on failure
 */
static int video_usb_uvc_host_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	struct uvc_device *uvc_dev = dev->data;
	int ret;

	if (!uvc_dev->connected || !uvc_dev) {
		return -ENODEV;
	}

	k_mutex_lock(&uvc_dev->lock, K_FOREVER);

	ret = uvc_host_enum_frame_intervals(uvc_dev->current_format.frame_ptr , fie);
	if (ret) {
		LOG_DBG("Failed to enumerate frame intervals: %d", ret);
	}

	k_mutex_unlock(&uvc_dev->lock);
	return ret;
}

#ifdef CONFIG_POLL
/**
 * @brief Set poll signal for UVC device events
 *
 * Registers a poll signal that will be raised when video events occur,
 * such as buffer completion or device state changes.
 *
 * @param dev Pointer to video device
 * @param sig Pointer to poll signal structure
 * @return 0 on success, negative error code on failure
 */
static int video_usb_uvc_host_set_signal(const struct device *dev, struct k_poll_signal *sig)
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
 * @brief Get volatile control values from UVC device
 *
 * Retrieves current values for volatile controls (auto-controls) that
 * may change dynamically based on device automatic adjustments.
 *
 * @param dev Pointer to video device
 * @param id Control ID to retrieve
 * @return 0 on success, negative error code on failure
 */
static int video_usb_uvc_host_get_volatile_ctrl(const struct device *dev, uint32_t id)
{
	struct uvc_device *uvc_dev = dev->data;
	struct usb_camera_ctrls *ctrls = &uvc_dev->ctrls;
	int ret = 0;

	switch (id) {
	case VIDEO_CID_EXPOSURE_AUTO:
		/* TODO: Implement exposure value retrieval */
		break;

	case VIDEO_CID_AUTOGAIN:
		/* Auto gain mode: read current actual gain value */
		ret = uvc_host_get_current_gain(uvc_dev, &ctrls->gain.val);
		if (ret) {
			LOG_ERR("Failed to get current gain value: %d", ret);
			return ret;
		}
		LOG_DBG("Updated gain to current value: %d", ctrls->gain.val);
		break;

	case VIDEO_CID_AUTO_WHITE_BALANCE:
		/* TODO: Implement white balance temperature retrieval */
		break;

	default:
		LOG_WRN("Volatile control 0x%08x not supported", id);
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Set UVC control value
 *
 * Video API implementation to set various camera control parameters.
 * Maps video control IDs to UVC-specific control selectors and sends
 * appropriate USB control requests.
 *
 * @param dev Pointer to video device
 * @param ctrl Pointer to video control structure containing ID and value
 * @return 0 on success, negative error code on failure
 */
static int video_usb_uvc_host_set_ctrl(const struct device *dev, struct video_control *ctrl)
{
	struct uvc_device *uvc_dev = dev->data;
	int ret = 0;
	uint8_t entity_id = 0;
	uint8_t control_selector = 0;
	uint8_t data[4] = {0}; /* Buffer for control data */
	uint8_t data_len = 0;

	if (!uvc_dev || !ctrl) {
		return -EINVAL;
	}

	/* Map control ID to UVC entity and control selector */
	switch (ctrl->id) {
	/* Processing Unit Controls */
	case VIDEO_CID_BRIGHTNESS:
		if (!uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_BRIGHTNESS)) {
			return -ENOTSUP;
		}
		entity_id = uvc_dev->vc_pud->bUnitID;
		control_selector = UVC_PU_BRIGHTNESS_CONTROL;
		sys_put_le16(ctrl->val, data);
		data_len = 2;
		break;

	case VIDEO_CID_CONTRAST:
		if (!uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_CONTRAST)) {
			return -ENOTSUP;
		}
		entity_id = uvc_dev->vc_pud->bUnitID;
		control_selector = UVC_PU_CONTRAST_CONTROL;
		sys_put_le16(ctrl->val, data);
		data_len = 2;
		break;

	case VIDEO_CID_HUE:
		if (!uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_HUE)) {
			return -ENOTSUP;
		}
		entity_id = uvc_dev->vc_pud->bUnitID;
		control_selector = UVC_PU_HUE_CONTROL;
		sys_put_le16(ctrl->val, data);
		data_len = 2;
		break;

	case VIDEO_CID_SATURATION:
		if (!uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_SATURATION)) {
			return -ENOTSUP;
		}
		entity_id = uvc_dev->vc_pud->bUnitID;
		control_selector = UVC_PU_SATURATION_CONTROL;
		sys_put_le16(ctrl->val, data);
		data_len = 2;
		break;

	case VIDEO_CID_SHARPNESS:
		if (!uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_SHARPNESS)) {
			return -ENOTSUP;
		}
		entity_id = uvc_dev->vc_pud->bUnitID;
		control_selector = UVC_PU_SHARPNESS_CONTROL;
		sys_put_le16(ctrl->val, data);
		data_len = 2;
		break;

	case VIDEO_CID_GAMMA:
		if (!uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_GAMMA)) {
			return -ENOTSUP;
		}
		entity_id = uvc_dev->vc_pud->bUnitID;
		control_selector = UVC_PU_GAMMA_CONTROL;
		sys_put_le16(ctrl->val, data);
		data_len = 2;
		break;

	case VIDEO_CID_GAIN:
		if (!uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_GAIN)) {
			return -ENOTSUP;
		}
		entity_id = uvc_dev->vc_pud->bUnitID;
		control_selector = UVC_PU_GAIN_CONTROL;
		sys_put_le16(ctrl->val, data);
		data_len = 2;
		break;

	case VIDEO_CID_AUTOGAIN:
		/* Auto gain implemented through gain control's automatic mode */
		if (!uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_GAIN)) {
			return -ENOTSUP;
		}
		entity_id = uvc_dev->vc_pud->bUnitID;
		control_selector = UVC_PU_GAIN_CONTROL;
		data[0] = ctrl->val ? 0xFF : 0x00; /* Auto mode flag */
		data_len = 1;
		break;

	case VIDEO_CID_POWER_LINE_FREQUENCY:
		if (!uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_POWER_LINE_FREQUENCY)) {
			return -ENOTSUP;
		}
		entity_id = uvc_dev->vc_pud->bUnitID;
		control_selector = UVC_PU_POWER_LINE_FREQUENCY_CONTROL;
		data[0] = ctrl->val; /* 0=Disabled, 1=50Hz, 2=60Hz, 3=Auto */
		data_len = 1;
		break;

	case VIDEO_CID_WHITE_BALANCE_TEMPERATURE:
		if (!uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_WHITE_BALANCE_TEMPERATURE)) {
			return -ENOTSUP;
		}
		entity_id = uvc_dev->vc_pud->bUnitID;
		control_selector = UVC_PU_WHITE_BALANCE_TEMP_CONTROL;
		sys_put_le16(ctrl->val, data);
		data_len = 2;
		break;

	case VIDEO_CID_AUTO_WHITE_BALANCE:
		if (!uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_WHITE_BALANCE_TEMPERATURE_AUTO)) {
			return -ENOTSUP;
		}
		entity_id = uvc_dev->vc_pud->bUnitID;
		control_selector = UVC_PU_WHITE_BALANCE_TEMP_AUTO_CONTROL;
		data[0] = ctrl->val ? 1 : 0;
		data_len = 1;
		break;

	case VIDEO_CID_BACKLIGHT_COMPENSATION:
		if (!uvc_host_pu_supports_control(uvc_dev, UVC_PU_BMCONTROL_BACKLIGHT_COMPENSATION)) {
			return -ENOTSUP;
		}
		entity_id = uvc_dev->vc_pud->bUnitID;
		control_selector = UVC_PU_BACKLIGHT_COMPENSATION_CONTROL;
		sys_put_le16(ctrl->val, data);
		data_len = 2;
		break;

	/* Camera Terminal Controls */
	case VIDEO_CID_EXPOSURE_AUTO:
		if (uvc_host_ct_supports_control(uvc_dev, UVC_CT_BMCONTROL_AE_MODE)) {
			entity_id = uvc_dev->vc_ctd->bTerminalID;
			control_selector = UVC_CT_AE_MODE_CONTROL;
			data[0] = ctrl->val;
			data_len = 1;
		} else {
			LOG_WRN("Auto exposure mode control not supported");
			ret = -ENOTSUP;
		}
		break;

	case VIDEO_CID_EXPOSURE_AUTO_PRIORITY:
		if (uvc_host_ct_supports_control(uvc_dev, UVC_CT_BMCONTROL_AE_PRIORITY)) {
			entity_id = uvc_dev->vc_ctd->bTerminalID;
			control_selector = UVC_CT_AE_PRIORITY_CONTROL;
			data[0] = ctrl->val;
			data_len = 1;
		} else {
			LOG_WRN("Auto exposure priority control not supported");
			ret = -ENOTSUP;
		}
		break;

	case VIDEO_CID_EXPOSURE_ABSOLUTE:
		if (uvc_host_ct_supports_control(uvc_dev, UVC_CT_BMCONTROL_EXPOSURE_TIME_ABSOLUTE)) {
			entity_id = uvc_dev->vc_ctd->bTerminalID;
			control_selector = UVC_CT_EXPOSURE_TIME_ABS_CONTROL;
			sys_put_le32(ctrl->val, data);
			data_len = 4;
		} else {
			LOG_WRN("Exposure absolute control not supported");
			ret = -ENOTSUP;
		}
		break;

	case VIDEO_CID_FOCUS_ABSOLUTE:
		if (uvc_host_ct_supports_control(uvc_dev, UVC_CT_BMCONTROL_FOCUS_ABSOLUTE)) {
			entity_id = uvc_dev->vc_ctd->bTerminalID;
			control_selector = UVC_CT_FOCUS_ABS_CONTROL;
			sys_put_le16(ctrl->val, data);
			data_len = 2;
		} else {
			LOG_WRN("Focus absolute control not supported");
			ret = -ENOTSUP;
		}
		break;

	case VIDEO_CID_FOCUS_AUTO:
		if (uvc_host_ct_supports_control(uvc_dev, UVC_CT_BMCONTROL_FOCUS_AUTO)) {
			entity_id = uvc_dev->vc_ctd->bTerminalID;
			control_selector = UVC_CT_FOCUS_AUTO_CONTROL;
			data[0] = ctrl->val;
			data_len = 1;
		} else {
			LOG_WRN("Auto focus control not supported");
			ret = -ENOTSUP;
		}
		break;

	case VIDEO_CID_FOCUS_RELATIVE:
		if (uvc_host_ct_supports_control(uvc_dev, UVC_CT_BMCONTROL_FOCUS_RELATIVE)) {
			entity_id = uvc_dev->vc_ctd->bTerminalID;
			control_selector = UVC_CT_FOCUS_REL_CONTROL;
			sys_put_le16(ctrl->val, data);
			data_len = 2;
		} else {
			LOG_WRN("Focus relative control not supported");
			ret = -ENOTSUP;
		}
		break;

	case VIDEO_CID_ZOOM_ABSOLUTE:
		if (uvc_host_ct_supports_control(uvc_dev, UVC_CT_BMCONTROL_ZOOM_ABSOLUTE)) {
			entity_id = uvc_dev->vc_ctd->bTerminalID;
			control_selector = UVC_CT_ZOOM_ABS_CONTROL;
			sys_put_le16(ctrl->val, data);
			data_len = 2;
		} else {
			LOG_WRN("Zoom absolute control not supported");
			ret = -ENOTSUP;
		}
		break;

	case VIDEO_CID_ZOOM_RELATIVE:
		if (uvc_host_ct_supports_control(uvc_dev, UVC_CT_BMCONTROL_ZOOM_RELATIVE)) {
			entity_id = uvc_dev->vc_ctd->bTerminalID;
			control_selector = UVC_CT_ZOOM_REL_CONTROL;
			data[0] = ctrl->val; /* zoom value */
			data[1] = 0x00; /* digital zoom (not used) */
			data[2] = 0x01; /* speed */
			data_len = 3;
		} else {
			LOG_WRN("Zoom relative control not supported");
			ret = -ENOTSUP;
		}
		break;

	case VIDEO_CID_TILT_RELATIVE:
		if (uvc_host_ct_supports_control(uvc_dev, UVC_CT_BMCONTROL_PAN_TILT_RELATIVE)) {
			entity_id = uvc_dev->vc_ctd->bTerminalID;
			control_selector = UVC_CT_PANTILT_REL_CONTROL;
			data[0] = 0x00; /* pan relative (LSB) */
			data[1] = 0x00; /* pan relative (MSB) */
			sys_put_le16(ctrl->val, &data[2]); /* tilt relative */
			data_len = 4;
		} else {
			LOG_WRN("Tilt relative control not supported");
			ret = -ENOTSUP;
		}
		break;

	case VIDEO_CID_IRIS_ABSOLUTE:
		if (uvc_host_ct_supports_control(uvc_dev, UVC_CT_BMCONTROL_IRIS_ABSOLUTE)) {
			entity_id = uvc_dev->vc_ctd->bTerminalID;
			control_selector = UVC_CT_IRIS_ABS_CONTROL;
			sys_put_le16(ctrl->val, data);
			data_len = 2;
		} else {
			LOG_WRN("Iris absolute control not supported");
			ret = -ENOTSUP;
		}
		break;

	case VIDEO_CID_IRIS_RELATIVE:
		if (uvc_host_ct_supports_control(uvc_dev, UVC_CT_BMCONTROL_IRIS_RELATIVE)) {
			entity_id = uvc_dev->vc_ctd->bTerminalID;
			control_selector = UVC_CT_IRIS_REL_CONTROL;
			data[0] = ctrl->val; /* iris value */
			data_len = 1;
		} else {
			LOG_WRN("Iris relative control not supported");
			ret = -ENOTSUP;
		}
		break;

	default:
		LOG_ERR("Unknown control ID: %u", ctrl->id);
		return -EINVAL;
	}

	/* Send control request if parameters are valid and no error occurred */
	if (ret == 0 && entity_id != 0 && control_selector != 0 && data_len > 0) {
		ret = uvc_host_control_unit_and_terminal_request(uvc_dev, UVC_SET_CUR, control_selector,
							entity_id, data, data_len);
	}

	return ret;
}

/**
 * @brief Video API implementation for starting stream
 *
 * Activates video streaming by setting the streaming interface to
 * the previously negotiated alternate setting.
 *
 * @param dev Pointer to video device
 * @param enable Enable or disable the video stream
 * @param type Buffer type (unused)
 * @return 0 on success, negative error code on failure
 */
static int video_usb_uvc_host_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
    struct uvc_device *uvc_dev;
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
        /* For disable, we need a valid interface to set alt setting to 0 */
        if (!uvc_dev->current_stream_iface_info.current_stream_iface) {
            LOG_WRN("No interface configured, cannot disable streaming");
            return -EINVAL;
        }
        alt = 0;
        interface_num = uvc_dev->current_stream_iface_info.current_stream_iface->bInterfaceNumber;
    }

    /* Activate streaming interface with negotiated alternate setting */
    ret = usbh_device_interface_set(uvc_dev->udev, interface_num, alt, false);
    if (ret) {
        LOG_ERR("Failed to set interface %d alt setting %d: %d", interface_num, alt, ret);
        return ret;
    }

    /* Update streaming state only after successful USB operation */
    uvc_dev->streaming = enable;

    LOG_DBG("UVC streaming %s successfully", enable ? "enabled" : "disabled");
    return 0;
}

/**
 * @brief Enqueue video buffer for capture
 *
 * Video API implementation to queue empty buffer for video capture.
 *
 * @param dev Pointer to video device
 * @param vbuf Pointer to video buffer to enqueue
 * @return 0 on success, negative error code on failure
 */
static int video_usb_uvc_host_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct uvc_device *uvc_dev;
    int ret;

    if (!dev) {
        return -EINVAL;
    }

    uvc_dev = dev->data;
    if (!uvc_dev || !uvc_dev->connected) {
        return -ENODEV;
    }

	/* Initialize buffer state for new capture */
	vbuf->bytesused = 0;
	vbuf->timestamp = 0;
	vbuf->line_offset = 0;

	k_fifo_put(&uvc_dev->fifo_in, vbuf);
	ret = uvc_host_flush_queue(uvc_dev, vbuf);
	if (ret) {
		return ret;
	}
}

/**
 * @brief Dequeue completed video buffer
 *
 * Video API implementation to retrieve filled video buffer.
 *
 * @param dev Pointer to video device
 * @param vbuf Pointer to store dequeued buffer pointer
 * @param timeout Maximum wait time for available buffer
 * @return 0 on success, -EAGAIN if no buffer available
 */
static int video_usb_uvc_host_dequeue(const struct device *dev, struct video_buffer **vbuf,
				k_timeout_t timeout)
{
	struct uvc_device *uvc_dev;
	struct uhc_transfer *xfer;
	int ret;

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

	if (!uvc_dev->connected) {
		xfer = (*vbuf)->driver_data;
		ret = usbh_xfer_dequeue(uvc_dev->udev, xfer);
		if (ret != 0) {
			return ret;
		}
	}
	
	return 0;
}

static const struct usbh_class_api uvc_host_class_api = {
	/** Initialize UVC host class */
	.init = uvc_host_init,
	/** Handle UVC device connection */
	.connected = uvc_host_connected,
	/** Handle UVC device removal */
	.removed = uvc_host_removed,
	/** Handle remote wakeup */
	.rwup = uvc_host_rwup,
	/** Handle device suspend */
	.suspended = uvc_host_suspended,
	/** Handle device resume */
	.resumed = uvc_host_resumed,
};

static DEVICE_API(video, uvc_host_video_api) = {
	.set_format = video_usb_uvc_host_set_fmt,
	.get_format = video_usb_uvc_host_get_fmt,
	.get_caps = video_usb_uvc_host_get_caps,
	.set_frmival = video_usb_uvc_host_set_frmival,
	.get_frmival = video_usb_uvc_host_get_frmival,
	.enum_frmival = video_usb_uvc_host_enum_frmival,
#ifdef CONFIG_POLL
	.set_signal = video_usb_uvc_host_set_signal,
#endif
	.get_volatile_ctrl = video_usb_uvc_host_get_volatile_ctrl,
	.set_ctrl = video_usb_uvc_host_set_ctrl,
	.set_stream = video_usb_uvc_host_set_stream,
	.enqueue = video_usb_uvc_host_enqueue,
	.dequeue = video_usb_uvc_host_dequeue,
};

#define USBH_VIDEO_DT_DEVICE_DEFINE(n)						\
	static struct uvc_device uvc_device_##n;				\
	DEVICE_DT_INST_DEFINE(n, uvc_host_preinit, NULL, 				\
				  &uvc_device_##n, NULL,		\
				  POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,		\
				  &uvc_host_video_api);				\
	USBH_DEFINE_CLASS(uvc_host_c_data_##n, &uvc_host_class_api,		\
							(void *)DEVICE_DT_INST_GET(n),	  \
							uvc_device_code, 2);		\
	VIDEO_DEVICE_DEFINE(usb_camera_##n, (void *)DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(USBH_VIDEO_DT_DEVICE_DEFINE)
