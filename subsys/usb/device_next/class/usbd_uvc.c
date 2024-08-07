/*
 * Copyright (c) 2024 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_uvc

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_uvc.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(usbd_uvc, CONFIG_USBD_UVC_LOG_LEVEL);

#define UVC_CLASS_ENABLED 0

struct uvc_desc {
	/* UVC Interface association */
	struct usb_association_descriptor iad;
	/* UVC Interface 0: Video Control */
	struct usb_if_descriptor if0;
	struct uvc_interface_header_descriptor if0_header;
	struct uvc_output_terminal_descriptor if0_output;
	struct uvc_input_terminal_descriptor if0_input;
	/* UVC Interface 1: Video Streaming */
	struct usb_if_descriptor if1;
	struct uvc_stream_input_header_descriptor if1_header;
	/* UVC Endpoints */
	struct usb_ep_descriptor ep_fs;
	struct usb_ep_descriptor ep_hs;
	/* USBD Terminator field */
	struct usb_desc_header nil_desc;
};

struct uvc_data {
	/* USBD class structure */
	const struct usbd_class_data *const c_data;
	/* USBD class state */
	atomic_t state;
	/* UVC worker to process the queue */
	struct k_work work;
	/* Video format with same index position as USB formats */
	const struct video_format_cap *fmts;
	/* UVC interface and format currenly selected */
	struct uvc_format_descriptor **format;
	struct uvc_frame_descriptor **frame;
	/* UVC Descriptors */
	struct uvc_desc *const desc;
	struct usb_desc_header *const *const fs_desc;
	struct usb_desc_header *const *const hs_desc;
	struct usb_desc_header *const *const ss_desc;
	/* UVC probe-commit control default values */
	struct uvc_vs_probe_control default_probe;
	/* UVC payload header, passed just before the image data */
	struct uvc_payload_header payload_header;
	/* Video FIFOs for submission (in) and completion (out) queue */
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
};

struct uvc_buf_info {
	struct udc_buf_info udc;
	struct video_buffer *vbuf;
} __packed;

NET_BUF_POOL_VAR_DEFINE(uvc_pool_header, DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * 10,
			CONFIG_USBD_VIDEO_HEADER_SIZE * 10, sizeof(struct uvc_buf_info), NULL);

NET_BUF_POOL_FIXED_DEFINE(uvc_pool_payload, DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * 10, 0,
			  sizeof(struct uvc_buf_info), NULL);

static bool uvc_desc_is_format(struct usb_desc_header *desc)
{
	struct uvc_format_descriptor *format = (void *)desc;

	return desc->bDescriptorType == UVC_CS_INTERFACE &&
	       (format->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED ||
		format->bDescriptorSubtype == UVC_VS_FORMAT_MJPEG);
}

static bool uvc_desc_is_frame(struct usb_desc_header *desc)
{
	struct uvc_frame_descriptor *frame = (void *)desc;

	return desc->bDescriptorType == UVC_CS_INTERFACE &&
	       (frame->bDescriptorSubtype == UVC_VS_FRAME_UNCOMPRESSED ||
		frame->bDescriptorSubtype == UVC_VS_FRAME_MJPEG);
}

static uint8_t uvc_get_bulk_in(struct uvc_data *data)
{
	return data->desc->if1_header.bEndpointAddress;
}

static void uvc_probe_format_index(struct uvc_data *const data, uint8_t bRequest,
				   struct uvc_vs_probe_control *probe)
{
	switch (bRequest) {
	case UVC_GET_MIN:
		probe->bFormatIndex = 1;
		break;
	case UVC_GET_MAX:
		probe->bFormatIndex = data->desc->if1_header.bNumFormats;
		break;
	case UVC_GET_RES:
		probe->bFormatIndex = 1;
		break;
	case UVC_GET_CUR:
		probe->bFormatIndex = (*data->format)->bFormatIndex;
		break;
	case UVC_SET_CUR:
		/* Search a format desc with a matching bFormatIndex through the entire list */
		for (struct usb_desc_header *const *desc = data->fs_desc;; desc++) {
			struct uvc_format_descriptor *format = (void *)*desc;

			if (format->bLength == 0) {
				break;
			}

			if (uvc_desc_is_format(*desc) &&
			    format->bFormatIndex == probe->bFormatIndex) {
				data->format = (void *)desc;
				data->frame = (void *)(desc + 1);
				return;
			}
		}
		LOG_WRN("probe: format index not found");
		break;
	}
}

static void uvc_probe_frame_index(struct uvc_data *const data, uint8_t bRequest,
				  struct uvc_vs_probe_control *probe)
{
	switch (bRequest) {
	case UVC_GET_MIN:
		probe->bFrameIndex = 1;
		break;
	case UVC_GET_MAX:
		probe->bFrameIndex = (*data->format)->bNumFrameDescriptors;
		break;
	case UVC_GET_RES:
		probe->bFrameIndex = 1;
		break;
	case UVC_GET_CUR:
		probe->bFrameIndex = (*data->frame)->bFrameIndex;
		break;
	case UVC_SET_CUR:
		/* Search a frame desc with a matching bFrameIndex below the current format desc */
		for (struct usb_desc_header *const *desc = (void *)(data->format + 1);; desc++) {
			struct uvc_frame_descriptor *frame = (void *)*desc;

			if (!uvc_desc_is_frame(*desc)) {
				break;
			}

			if (frame->bFrameIndex == probe->bFrameIndex) {
				data->frame = (void *)desc;
				return;
			}
		}
		LOG_WRN("probe: frame index not found");
	}
}

static void uvc_probe_frame_interval(struct uvc_data *const data, uint8_t bRequest,
				     struct uvc_vs_probe_control *probe)
{
	switch (bRequest) {
	case UVC_GET_MIN:
		probe->dwFrameInterval = (*data->frame)->dwMinFrameInterval;
	case UVC_GET_MAX:
		probe->dwFrameInterval = (*data->frame)->dwMaxFrameInterval;
	case UVC_GET_CUR:
		/* TODO call the frame interval API on the video source once supported */
		probe->dwFrameInterval = (*data->frame)->dwMaxFrameInterval;
		break;
	case UVC_GET_RES:
		probe->dwFrameInterval = 1;
		break;
	case UVC_SET_CUR:
		/* TODO call the frame interval API on the video source once supported */
		break;
	}
}

static void uvc_probe_key_frame_rate(struct uvc_data *const data, uint8_t bRequest,
				     struct uvc_vs_probe_control *probe)
{
	probe->wKeyFrameRate = 0;
}

static void uvc_probe_p_frame_rate(struct uvc_data *const data, uint8_t bRequest,
				   struct uvc_vs_probe_control *probe)
{
	probe->wPFrameRate = 0;
}

static void uvc_probe_comp_quality(struct uvc_data *const data, uint8_t bRequest,
				   struct uvc_vs_probe_control *probe)
{
	probe->wCompQuality = 0;
}

static void uvc_probe_comp_window_size(struct uvc_data *const data, uint8_t bRequest,
				       struct uvc_vs_probe_control *probe)
{
	probe->wCompWindowSize = 0;
}

static void uvc_probe_delay(struct uvc_data *const data, uint8_t bRequest,
			    struct uvc_vs_probe_control *probe)
{
	/* TODO devicetree */
	probe->wDelay = 1;
}

static void uvc_probe_max_video_frame_size(struct uvc_data *const data, uint8_t bRequest,
					   struct uvc_vs_probe_control *probe)
{
	switch (bRequest) {
	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_CUR:
		probe->dwMaxVideoFrameSize = (*data->frame)->dwMaxVideoFrameBufferSize;
		break;
	case UVC_GET_RES:
		probe->dwMaxVideoFrameSize = 1;
		break;
	case UVC_SET_CUR:
		if (probe->dwMaxVideoFrameSize > 0 &&
		    probe->dwMaxVideoFrameSize != (*data->frame)->dwMaxVideoFrameBufferSize) {
			LOG_WRN("probe: dwMaxVideoFrameSize is read-only");
		}
		break;
	}
}

static void uvc_probe_max_payload_size(struct uvc_data *const data, uint8_t bRequest,
				       struct uvc_vs_probe_control *probe)
{
	switch (bRequest) {
	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_CUR:
		probe->dwMaxPayloadTransferSize = (*data->frame)->dwMaxVideoFrameBufferSize;
		break;
	case UVC_GET_RES:
		probe->dwMaxPayloadTransferSize = 1;
		break;
	case UVC_SET_CUR:
		if (probe->dwMaxPayloadTransferSize > 0 &&
		    probe->dwMaxPayloadTransferSize != (*data->frame)->dwMaxVideoFrameBufferSize) {
			LOG_WRN("probe: dwPayloadTransferSize is read-only");
		}
		break;
	}
}

static void uvc_probe_clock_frequency(struct uvc_data *const data, uint8_t bRequest,
				      struct uvc_vs_probe_control *probe)
{
	switch (bRequest) {
	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_CUR:
	case UVC_GET_RES:
		probe->dwClockFrequency = 1;
		break;
	case UVC_SET_CUR:
		if (probe->dwClockFrequency > 1) {
			LOG_WRN("probe: dwClockFrequency is read-only");
		}
		break;
	}
}

static void uvc_probe_framing_info(struct uvc_data *const data, uint8_t bRequest,
				   struct uvc_vs_probe_control *probe)
{
	/* Include Frame ID and EOF fields in the payload header */
	probe->bmFramingInfo = BIT(0) | BIT(1);
}

static void uvc_probe_prefered_version(struct uvc_data *const data, uint8_t bRequest,
				       struct uvc_vs_probe_control *probe)
{
	probe->bPreferedVersion = 1;
}

static void uvc_probe_min_version(struct uvc_data *const data, uint8_t bRequest,
				  struct uvc_vs_probe_control *probe)
{
	probe->bMaxVersion = 1;
}

static void uvc_probe_max_version(struct uvc_data *const data, uint8_t bRequest,
				  struct uvc_vs_probe_control *probe)
{
	probe->bMaxVersion = 1;
}

static void uvc_probe_usage(struct uvc_data *const data, uint8_t bRequest,
			    struct uvc_vs_probe_control *probe)
{
	probe->bUsage = 0;
}

static void uvc_probe_bit_depth_luma(struct uvc_data *const data, uint8_t bRequest,
				     struct uvc_vs_probe_control *probe)
{
	/* Bit depth of color/chroma/luma channel, ignored as absent from the Video API. */
	probe->bBitDepthLuma = 0;
}

static void uvc_probe_settings(struct uvc_data *const data, uint8_t bRequest,
			       struct uvc_vs_probe_control *probe)
{
	probe->bmSettings = 0;
}

static void uvc_probe_max_ref_frames(struct uvc_data *const data, uint8_t bRequest,
				     struct uvc_vs_probe_control *probe)
{
	probe->bMaxNumberOfRefFramesPlus1 = 1;
}

static void uvc_probe_rate_control_modes(struct uvc_data *const data, uint8_t bRequest,
					 struct uvc_vs_probe_control *probe)
{
	probe->bmRateControlModes = 0;
}

static void uvc_probe_layout_per_stream(struct uvc_data *const data, uint8_t bRequest,
					struct uvc_vs_probe_control *probe)
{
	probe->bmLayoutPerStream = 0;
}

static void uvc_probe_dump(char const *name, const struct uvc_vs_probe_control *probe)
{
	LOG_DBG("probe: UVC_%s", name);
	LOG_DBG("probe: - bmHint = 0x%04x", sys_le16_to_cpu(probe->bmHint));
	LOG_DBG("probe: - bFormatIndex = %u", probe->bFormatIndex);
	LOG_DBG("probe: - bFrameIndex = %u", probe->bFrameIndex);
	LOG_DBG("probe: - dwFrameInterval = %u us", sys_le32_to_cpu(probe->dwFrameInterval) / 10);
	LOG_DBG("probe: - wKeyFrameRate = %u", sys_le16_to_cpu(probe->wKeyFrameRate));
	LOG_DBG("probe: - wPFrameRate = %u", probe->wPFrameRate);
	LOG_DBG("probe: - wCompQuality = %u", probe->wCompQuality);
	LOG_DBG("probe: - wCompWindowSize = %u", probe->wCompWindowSize);
	LOG_DBG("probe: - wDelay = %u ms", sys_le16_to_cpu(probe->wDelay));
	LOG_DBG("probe: - dwMaxVideoFrameSize = %u", sys_le32_to_cpu(probe->dwMaxVideoFrameSize));
	LOG_DBG("probe: - dwMaxPayloadTransferSize = %u",
		sys_le32_to_cpu(probe->dwMaxPayloadTransferSize));
	LOG_DBG("probe: - dwClockFrequency = %u Hz", sys_le32_to_cpu(probe->dwClockFrequency));
	LOG_DBG("probe: - bmFramingInfo = 0x%02x", probe->bmFramingInfo);
	LOG_DBG("probe: - bPreferedVersion = %u", probe->bPreferedVersion);
	LOG_DBG("probe: - bMinVersion = %u", probe->bMinVersion);
	LOG_DBG("probe: - bMaxVersion = %u", probe->bMaxVersion);
	LOG_DBG("probe: - bUsage = %u", probe->bUsage);
	LOG_DBG("probe: - bBitDepthLuma = %u", probe->bBitDepthLuma + 8);
	LOG_DBG("probe: - bmSettings = %u", probe->bmSettings);
	LOG_DBG("probe: - bMaxNumberOfRefFramesPlus1 = %u", probe->bMaxNumberOfRefFramesPlus1);
	LOG_DBG("probe: - bmRateControlModes = %u", probe->bmRateControlModes);
	LOG_DBG("probe: - bmLayoutPerStream = 0x%08llx", probe->bmLayoutPerStream);
}

static int uvc_probe(struct uvc_data *const data, uint16_t bRequest,
		     struct uvc_vs_probe_control *probe)
{
	LOG_DBG("UVC control: probe");

	switch (bRequest) {
	case UVC_GET_DEF:
		memcpy(probe, &data->default_probe, sizeof(*probe));
		break;
	case UVC_SET_CUR:
		uvc_probe_dump("SET_CUR", probe);
		goto action;
	case UVC_GET_CUR:
		uvc_probe_dump("GET_CUR", probe);
		goto action;
	case UVC_GET_MIN:
		uvc_probe_dump("GET_MIN", probe);
		goto action;
	case UVC_GET_MAX:
		uvc_probe_dump("GET_MAX", probe);
		goto action;
	case UVC_GET_RES:
		uvc_probe_dump("SET_RES", probe);
		goto action;
	action:
		/* TODO use bmHint to choose in which order configure the fields */
		uvc_probe_format_index(data, bRequest, probe);
		uvc_probe_frame_index(data, bRequest, probe);
		uvc_probe_frame_interval(data, bRequest, probe);
		uvc_probe_key_frame_rate(data, bRequest, probe);
		uvc_probe_p_frame_rate(data, bRequest, probe);
		uvc_probe_comp_quality(data, bRequest, probe);
		uvc_probe_comp_window_size(data, bRequest, probe);
		uvc_probe_delay(data, bRequest, probe);
		uvc_probe_max_video_frame_size(data, bRequest, probe);
		uvc_probe_max_payload_size(data, bRequest, probe);
		uvc_probe_clock_frequency(data, bRequest, probe);
		uvc_probe_framing_info(data, bRequest, probe);
		uvc_probe_prefered_version(data, bRequest, probe);
		uvc_probe_min_version(data, bRequest, probe);
		uvc_probe_max_version(data, bRequest, probe);
		uvc_probe_usage(data, bRequest, probe);
		uvc_probe_bit_depth_luma(data, bRequest, probe);
		uvc_probe_settings(data, bRequest, probe);
		uvc_probe_max_ref_frames(data, bRequest, probe);
		uvc_probe_rate_control_modes(data, bRequest, probe);
		uvc_probe_layout_per_stream(data, bRequest, probe);
		break;
	default:
		__ASSERT_NO_MSG(false);
	}
	return 0;
}

static int uvc_commit(struct uvc_data *const data, uint16_t bRequest,
		      struct uvc_vs_probe_control *probe)
{
	switch (bRequest) {
	case UVC_GET_CUR:
		uvc_probe(data, bRequest, probe);
		break;
	case UVC_SET_CUR:
		uvc_probe(data, bRequest, probe);
		LOG_DBG("commit: ready to transfer frames");
		/* TODO: signal the application that the current format might have changed */
		break;
	default:
		LOG_ERR("commit: invalid bRequest (%u)", bRequest);
		errno = -EINVAL;
		return 0;
	}
	return 0;
}

static int uvc_probe_or_commit(struct uvc_data *const data,
			       const struct usb_setup_packet *const setup,
			       struct net_buf *const buf)
{
	struct uvc_vs_probe_control probe = {0};

	switch (setup->bRequest) {
	case UVC_GET_LEN:
		net_buf_add_le16(buf, sizeof(probe));
		return 0;
	case UVC_GET_INFO:
		/* As defined by UVC 1.5 Table 4-76 */
		net_buf_add_u8(buf, BIT(0) | BIT(1));
		return 0;
	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_DEF:
	case UVC_GET_RES:
	case UVC_GET_CUR:
		if (net_buf_add(buf, sizeof(probe)) == NULL) {
			return -ENOMEM;
		}
		break;
	case UVC_SET_CUR:
		if (buf->len != sizeof(probe)) {
			LOG_ERR("probe: invalid wLength=%u for Probe or Commit", setup->wLength);
			errno = -EINVAL;
			return 0;
		}
		break;
	default:
		LOG_ERR("probe: invalid bRequest (%u) for Probe or Commit", setup->bRequest);
		errno = -EINVAL;
		return 0;
	}

	/* All remaining request work on a struct uvc_vs_probe_control */
	if (setup->wLength != sizeof(probe)) {
		LOG_ERR("probe: invalid wLength=%u for Probe or Commit", setup->wLength);
		errno = -EINVAL;
		return 0;
	}

	switch (setup->wValue >> 8) {
	case UVC_VS_PROBE_CONTROL:
		return uvc_probe(data, setup->bRequest, (void *)buf->data);
	case UVC_VS_COMMIT_CONTROL:
		return uvc_commit(data, setup->bRequest, (void *)buf->data);
	default:
		errno = -EINVAL;
		return 0;
	}
}

static int uvc_control(struct usbd_class_data *c_data, const struct usb_setup_packet *const setup,
		       struct net_buf *const buf)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;

	switch (setup->wValue >> 8) {
	case UVC_VS_PROBE_CONTROL:
	case UVC_VS_COMMIT_CONTROL:
		return uvc_probe_or_commit(data, setup, buf);
	default:
		LOG_WRN("api: control: unsupported wValue (%u)", setup->wValue);
		errno = -ENOTSUP;
		return 0;
	}
}

static int uvc_control_to_host(struct usbd_class_data *const c_data,
			       const struct usb_setup_packet *const setup,
			       struct net_buf *const buf)
{
	return uvc_control(c_data, setup, buf);
}

static int uvc_control_to_dev(struct usbd_class_data *const c_data,
			      const struct usb_setup_packet *const setup,
			      const struct net_buf *const buf)
{
	return uvc_control(c_data, setup, (struct net_buf *const)buf);
}

static int uvc_request(struct usbd_class_data *const c_data, struct net_buf *buf, int err)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;
	struct uvc_buf_info bi = *(struct uvc_buf_info *)udc_get_buf_info(buf);

	net_buf_unref(buf);

	if (bi.udc.ep == uvc_get_bulk_in(data)) {
		/* Only for the last buffer of the transfer and last transfer of the frame */
		if (bi.vbuf != NULL) {
			/* Upon completion, move the buffer from submission to completion queue */
			LOG_DBG("request: vbuf=%p transferred", bi.vbuf);
			k_fifo_put(&data->fifo_out, bi.vbuf);
		}
	}

	LOG_DBG("request: transfer done, ep=0x%02x buf=%p", bi.udc.ep, buf);

	return err;
}

static void uvc_update(struct usbd_class_data *const c_data, uint8_t iface, uint8_t alternate)
{
	LOG_DBG("update");
}

static int uvc_init(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;

	/* The default probe is the current probe at startup */
	uvc_probe(data, UVC_GET_CUR, &data->default_probe);

	return 0;
}

static void *uvc_get_desc(struct usbd_class_data *const c_data, const enum usbd_speed speed)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;
	struct uvc_desc *desc = data->desc;

	/* The descriptors need some adjustments */

	desc->iad.bFirstInterface = desc->if0.bInterfaceNumber;
	desc->if0_header.baInterfaceNr = desc->if1.bInterfaceNumber;

	switch (speed) {
	case USBD_SPEED_HS:
		desc->if1_header.bEndpointAddress = desc->ep_hs.bEndpointAddress;
		return (void *)data->hs_desc;
	default:
		desc->if1_header.bEndpointAddress = desc->ep_fs.bEndpointAddress;
		return (void *)data->fs_desc;
	}
}

static int uvc_enqueue_usb(struct uvc_data *data, struct net_buf *buf, bool last_of_transfer)
{
	struct udc_buf_info *bi;
	int ret;

	LOG_DBG("queue: usb buf=%p data=%p size=%u len=%u zlp=%u", buf, buf->data, buf->size,
		buf->len, last_of_transfer);

	bi = udc_get_buf_info(buf);
	__ASSERT_NO_MSG(bi != NULL);

	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = uvc_get_bulk_in(data);
	bi->zlp = last_of_transfer;

	ret = usbd_ep_enqueue(data->c_data, buf);
	if (ret != 0) {
		LOG_ERR("enqueue: error from usbd for buf=%p", buf);
		return ret;
	}

	return 0;
}

static void uvc_worker(struct k_work *work)
{
	struct uvc_data *data = CONTAINER_OF(work, struct uvc_data, work);
	struct uvc_buf_info *bi;
	struct video_buffer *vbuf;
	struct net_buf *buf0;
	struct net_buf *buf1;
	int ret;

	if (!atomic_test_bit(&data->state, UVC_CLASS_ENABLED)) {
		LOG_DBG("queue: USB configuration is not enabled yet");
		return;
	}

	vbuf = k_fifo_peek_head(&data->fifo_in);
	if (vbuf == NULL) {
		return;
	}

	LOG_DBG("queue: video buffer of %u bytes", vbuf->bytesused);

	buf0 = net_buf_alloc_len(&uvc_pool_header, CONFIG_USBD_VIDEO_HEADER_SIZE, K_NO_WAIT);
	if (buf0 == NULL) {
		LOG_DBG("queue: failed to allocate the header");
		return;
	}

	if (vbuf->flags | VIDEO_BUF_EOF) {
		/* new frame: toggle the FRAMEID ans flag as EOF */
		data->payload_header.bmHeaderInfo ^= UVC_BMHEADERINFO_FRAMEID;
		data->payload_header.bmHeaderInfo |= UVC_BMHEADERINFO_END_OF_FRAME;
	} else {
		/* Not setting the EOF flag until we reach the last transfer */
		data->payload_header.bmHeaderInfo &= ~UVC_BMHEADERINFO_END_OF_FRAME;
	}

	/* Only the 2 first (uint8_t) fields are supported for now */
	net_buf_add_mem(buf0, &data->payload_header, 2);

	/* Pad the header up to CONFIG_USBD_VIDEO_HEADER_SIZE */
	while (buf0->len < buf0->size) {
		net_buf_add_u8(buf0, 0);
	}

	bi = (void *)udc_get_buf_info(buf0);
	bi->vbuf = NULL;

	ret = uvc_enqueue_usb(data, buf0, false);
	if (ret != 0) {
		LOG_ERR("queue: failed to submit the header to USB");
		net_buf_unref(buf0);
		return;
	}

	buf1 = net_buf_alloc_with_data(&uvc_pool_payload, vbuf->buffer, vbuf->bytesused, K_NO_WAIT);
	if (buf1 == NULL) {
		LOG_ERR("queue: failed to allocate the header");
		net_buf_unref(buf1);
		return;
	}

	LOG_DBG("queue: submitting vbuf=%p", vbuf);

	/* Attach the video buffer to the USB buffer so that we get it back from USB */
	bi = (void *)udc_get_buf_info(buf1);
	bi->vbuf = vbuf;

	ret = uvc_enqueue_usb(data, buf1, true);
	if (ret != 0) {
		LOG_ERR("queue: failed to submit the payload to USB");
		net_buf_unref(buf0);
		net_buf_unref(buf1);
		return;
	}

	k_fifo_get(&data->fifo_in, K_NO_WAIT);
	k_work_submit(&data->work);
}

static void uvc_enable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;

	atomic_set_bit(&data->state, UVC_CLASS_ENABLED);

	/* Catch-up with buffers that might have been delayed */
	k_work_submit(&data->work);
}

static void uvc_disable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;

	atomic_clear_bit(&data->state, UVC_CLASS_ENABLED);
}

struct usbd_class_api uvc_class_api = {
	.enable = uvc_enable,
	.disable = uvc_disable,
	.request = uvc_request,
	.update = uvc_update,
	.control_to_host = uvc_control_to_host,
	.control_to_dev = uvc_control_to_dev,
	.init = uvc_init,
	.get_desc = uvc_get_desc,
};

static int uvc_enqueue(const struct device *dev, enum video_endpoint_id ep,
		       struct video_buffer *vbuf)
{
	struct uvc_data *data = dev->data;

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_ANY) {
		return -EINVAL;
	}

	k_fifo_put(&data->fifo_in, vbuf);

	/* Process this buffer as well as others pending */
	k_work_submit(&data->work);

	return 0;
}

static int uvc_dequeue(const struct device *dev, enum video_endpoint_id ep,
		       struct video_buffer **vbuf, k_timeout_t timeout)
{
	struct uvc_data *data = dev->data;

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_ANY) {
		return -EINVAL;
	}

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int uvc_get_format(const struct device *dev, enum video_endpoint_id ep,
			  struct video_format *fmt)
{
	struct uvc_data *data = dev->data;
	const struct video_format_cap *cap = data->fmts;

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_ANY) {
		return -EINVAL;
	}

	memset(fmt, 0, sizeof(*fmt));

	for (struct usb_desc_header *const *desc = data->fs_desc;; desc++) {
		if ((*desc)->bLength == 0 || cap->pixelformat == 0) {
			/* This could happen if called before uvc_init() */
			LOG_ERR("Overflowing formats or descriptors list");
			return -EINVAL;
		}
		if (desc == (struct usb_desc_header **)data->frame) {
			break;
		}
		if (uvc_desc_is_frame(*desc)) {
			LOG_DBG("frame=yes curr=%u this=%u", ((struct uvc_frame_descriptor *)(*desc))->bFrameIndex, (*data->frame)->bFrameIndex);
			cap++;
		} else {
			LOG_DBG("frame=no");
		}
	}

	fmt->pixelformat = cap->pixelformat;
	fmt->width = (*data->frame)->wWidth;
	fmt->height = (*data->frame)->wHeight;

	if ((*data->format)->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED) {
		struct uvc_uncompressed_format_descriptor *uncompressed = (void *)*data->format;

		fmt->pitch = (*data->frame)->wWidth * uncompressed->bBitsPerPixel / 8;
	}

	return 0;
}

static int uvc_stream_start(const struct device *dev)
{
	/* TODO: resume the stream after it was interrupted if needed */
	return 0;
}

static int uvc_stream_stop(const struct device *dev)
{
	/* TODO: cancel the ongoing USB request and stop the stream */
	return 0;
}

static int uvc_get_caps(const struct device *dev, enum video_endpoint_id ep,
			struct video_caps *caps)
{
	struct uvc_data *data = dev->data;

	caps->format_caps = data->fmts;
	return 0;
}

struct video_driver_api uvc_video_api = {
	.get_format = uvc_get_format,
	.stream_start = uvc_stream_start,
	.stream_stop = uvc_stream_stop,
	.get_caps = uvc_get_caps,
	.enqueue = uvc_enqueue,
	.dequeue = uvc_dequeue,
};

static int uvc_preinit(const struct device *dev)
{
	struct uvc_data *data = dev->data;
	int frame_index = 1;
	int format_index = 1;

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);
	k_work_init(&data->work, &uvc_worker);

	/* Set the frame and format indexes */
	for (struct usb_desc_header *const *desc = data->fs_desc; (*desc)->bLength > 0; desc++) {
		struct uvc_format_descriptor *format_desc = (void *)*desc;
		struct uvc_frame_descriptor *frame_desc = (void *)*desc;

		if ((*desc)->bDescriptorType != UVC_CS_INTERFACE) {
			continue;
		}

		switch (format_desc->bDescriptorSubtype) {
		case UVC_VS_FORMAT_MJPEG:
		case UVC_VS_FORMAT_UNCOMPRESSED:
			if (data->format == NULL) {
				data->format = (void *)desc;
			}
			format_desc->bFormatIndex = format_index++;
			frame_index = 1;
			data->desc->if1_header.bNumFormats++;
			break;
		case UVC_VS_FRAME_MJPEG:
		case UVC_VS_FRAME_UNCOMPRESSED:
			frame_desc->bFrameIndex = frame_index++;
			break;
		}
	}
	data->frame = (void *)(data->format + 1);

	return 0;
}

#define UVC_MJPEG_FRAME_DEFINE(n)                                                                  \
	struct uvc_frame_descriptor n = {                                                          \
		.bLength = sizeof(struct uvc_frame_descriptor),                                    \
		.bDescriptorType = UVC_CS_INTERFACE,                                               \
		.bDescriptorSubtype = UVC_VS_FRAME_MJPEG,                                          \
		.bFrameIndex = DT_CHILD_NUM(n) + 1,                                                \
		.bmCapabilities = 0x00,                                                            \
		.wWidth = sys_cpu_to_le16(DT_PROP_BY_IDX(n, size, 0)),                             \
		.wHeight = sys_cpu_to_le16(DT_PROP_BY_IDX(n, size, 1)),                            \
		.dwMinBitRate = sys_cpu_to_le32(15360000),                                         \
		.dwMaxBitRate = sys_cpu_to_le32(15360000),                                         \
		.dwMaxVideoFrameBufferSize =                                                       \
			sys_cpu_to_le32(DT_PROP_BY_IDX(n, size, 0) * DT_PROP_BY_IDX(n, size, 1) +  \
					CONFIG_USBD_VIDEO_HEADER_SIZE),                            \
		.dwDefaultFrameInterval = sys_cpu_to_le32(10000000 / DT_PROP(n, max_fps)),         \
		.bFrameIntervalType = 0,                                                           \
		.dwMinFrameInterval = sys_cpu_to_le32(10000000 / DT_PROP(n, max_fps)),             \
		.dwMaxFrameInterval = sys_cpu_to_le32(INT32_MAX),                                  \
		.dwFrameIntervalStep = sys_cpu_to_le32(10), /* 1 us */                             \
	};

#define UVC_MJPEG_FORMAT_DEFINE(n)                                                                 \
	struct uvc_mjpeg_format_descriptor n = {                                                   \
		.bLength = sizeof(struct uvc_mjpeg_format_descriptor),                             \
		.bDescriptorType = UVC_CS_INTERFACE,                                               \
		.bDescriptorSubtype = UVC_VS_FORMAT_MJPEG,                                         \
		.bFormatIndex = 0,                                                                 \
		.bNumFrameDescriptors = DT_CHILD_NUM(n),                                           \
		.bmFlags = BIT(0),                                                                 \
		.bDefaultFrameIndex = 1,                                                           \
		.bAspectRatioX = 0,                                                                \
		.bAspectRatioY = 0,                                                                \
		.bmInterlaceFlags = 0x00,                                                          \
		.bCopyProtect = 0,                                                                 \
	};                                                                                         \
                                                                                                   \
	DT_FOREACH_CHILD(n, UVC_MJPEG_FRAME_DEFINE)

#define UVC_UNCOMPRESSED_FRAME_DEFINE(n)                                                           \
	struct uvc_frame_descriptor n = {                                                          \
		.bLength = sizeof(struct uvc_frame_descriptor),                                    \
		.bDescriptorType = UVC_CS_INTERFACE,                                               \
		.bDescriptorSubtype = UVC_VS_FRAME_UNCOMPRESSED,                                   \
		.bFrameIndex = DT_CHILD_NUM(n) + 1,                                                \
		.bmCapabilities = 0x00,                                                            \
		.wWidth = sys_cpu_to_le16(DT_PROP_BY_IDX(n, size, 0)),                             \
		.wHeight = sys_cpu_to_le16(DT_PROP_BY_IDX(n, size, 1)),                            \
		.dwMinBitRate = sys_cpu_to_le32(15360000),                                         \
		.dwMaxBitRate = sys_cpu_to_le32(15360000),                                         \
		.dwMaxVideoFrameBufferSize =                                                       \
			sys_cpu_to_le32(DT_PROP_BY_IDX(n, size, 0) * DT_PROP_BY_IDX(n, size, 1) *  \
					DT_PROP(DT_PARENT(n), bits_per_pixel) / 8 +                \
					CONFIG_USBD_VIDEO_HEADER_SIZE),                            \
		.dwDefaultFrameInterval = sys_cpu_to_le32(10000000 / DT_PROP(n, max_fps)),         \
		.bFrameIntervalType = 0,                                                           \
		.dwMinFrameInterval = sys_cpu_to_le32(10000000 / DT_PROP(n, max_fps)),             \
		.dwMaxFrameInterval = sys_cpu_to_le32(INT32_MAX),                                  \
		.dwFrameIntervalStep = sys_cpu_to_le32(10), /* 1 us */                             \
	};

#define UVC_UNCOMPRESSED_FORMAT_DEFINE(n)                                                          \
	struct uvc_uncompressed_format_descriptor n = {                                            \
		.bLength = sizeof(struct uvc_uncompressed_format_descriptor),                      \
		.bDescriptorType = UVC_CS_INTERFACE,                                               \
		.bDescriptorSubtype = UVC_VS_FORMAT_UNCOMPRESSED,                                  \
		.bFormatIndex = 0,                                                                 \
		.bNumFrameDescriptors = DT_CHILD_NUM(n),                                           \
		.guidFormat = DT_PROP(n, guid),                                                    \
		.bBitsPerPixel = DT_PROP(n, bits_per_pixel),                                       \
		.bDefaultFrameIndex = 1,                                                           \
		.bAspectRatioX = 0,                                                                \
		.bAspectRatioY = 0,                                                                \
		.bmInterlaceFlags = 0x00,                                                          \
		.bCopyProtect = 0,                                                                 \
	};                                                                                         \
                                                                                                   \
	DT_FOREACH_CHILD(n, UVC_UNCOMPRESSED_FRAME_DEFINE)

#define UVC_POINTER(n) ((struct usb_desc_header *)&n),

#define UVC_POINTER_LIST(n)                                                                        \
	IF_ENABLED(DT_NODE_HAS_COMPAT(n, zephyr_uvc_format), (                                     \
		UVC_POINTER(n) DT_FOREACH_CHILD_STATUS_OKAY(n, UVC_POINTER)                        \
	))

#define UVC_VIDEO_FRAME_CAP(n, fmt)                                                                \
	{                                                                                          \
		.pixelformat = video_fourcc(DT_PROP(fmt, fourcc)[0], DT_PROP(fmt, fourcc)[1],      \
					    DT_PROP(fmt, fourcc)[2], DT_PROP(fmt, fourcc)[3]),     \
		.width_min = DT_PROP_BY_IDX(n, size, 0),                                           \
		.width_max = DT_PROP_BY_IDX(n, size, 0),                                           \
		.width_step = 0,                                                                   \
		.height_min = DT_PROP_BY_IDX(n, size, 1),                                          \
		.height_max = DT_PROP_BY_IDX(n, size, 1),                                          \
		.height_step = 0                                                                   \
	},

#define UVC_VIDEO_FORMAT_CAP(n)                                                                    \
	IF_ENABLED(DT_NODE_HAS_COMPAT(n, zephyr_uvc_format), (                                     \
		DT_FOREACH_CHILD_VARGS(n, UVC_VIDEO_FRAME_CAP, n)                                  \
	))

#define UVC_DEVICE_DEFINE(n)                                                                       \
                                                                                                   \
	BUILD_ASSERT(DT_INST_ON_BUS(n, usb),                                                       \
		     "node " DT_NODE_PATH(DT_DRV_INST(n)) " is not"                                \
							  " assigned to a USB device controller"); \
                                                                                                   \
	USBD_DEFINE_CLASS(uvc_##n, &uvc_class_api, (void *)DEVICE_DT_GET(DT_DRV_INST(n)), NULL);   \
                                                                                                   \
	struct uvc_desc uvc_desc_##n = {                                                           \
		.iad =                                                                             \
			{                                                                          \
				.bLength = sizeof(struct usb_association_descriptor),              \
				.bDescriptorType = USB_DESC_INTERFACE_ASSOC,                       \
				.bFirstInterface = 0,                                              \
				.bInterfaceCount = 2,                                              \
				.bFunctionClass = USB_BCC_VIDEO,                                   \
				.bFunctionSubClass = UVC_SC_VIDEO_INTERFACE_COLLECITON,            \
				.bFunctionProtocol = UVC_PC_PROTOCOL_UNDEFINED,                    \
				.iFunction = 0,                                                    \
			},                                                                         \
		.if0 =                                                                             \
			{                                                                          \
				.bLength = sizeof(struct usb_if_descriptor),                       \
				.bDescriptorType = USB_DESC_INTERFACE,                             \
				.bInterfaceNumber = 0,                                             \
				.bAlternateSetting = 0,                                            \
				.bNumEndpoints = 0,                                                \
				.bInterfaceClass = USB_BCC_VIDEO,                                  \
				.bInterfaceSubClass = UVC_SC_VIDEOCONTROL,                         \
				.bInterfaceProtocol = 0,                                           \
				.iInterface = 0,                                                   \
			},                                                                         \
		.if0_header =                                                                      \
			{                                                                          \
				.bLength = sizeof(struct uvc_interface_header_descriptor),         \
				.bDescriptorType = UVC_CS_INTERFACE,                               \
				.bDescriptorSubtype = UVC_VC_HEADER,                               \
				.bcdUVC = 0x0150,                                                  \
				.wTotalLength = sys_cpu_to_le16(                                   \
					sizeof(struct uvc_interface_header_descriptor) +           \
					sizeof(struct uvc_input_terminal_descriptor) +             \
					sizeof(struct uvc_output_terminal_descriptor)),            \
				.dwClockFrequency = 30000000,                                      \
				.bInCollection = 1,                                                \
				.baInterfaceNr = 1,                                                \
			},                                                                         \
		.if0_output =                                                                      \
			{                                                                          \
				.bLength = sizeof(struct uvc_output_terminal_descriptor),          \
				.bDescriptorType = UVC_CS_INTERFACE,                               \
				.bDescriptorSubtype = UVC_VC_OUTPUT_TERMINAL,                      \
				.bTerminalID = 2,                                                  \
				.wTerminalType = sys_cpu_to_le16(UVC_TT_STREAMING),                \
				.bAssocTerminal = 0,                                               \
				.bSourceID = 1,                                                    \
				.iTerminal = 0,                                                    \
			},                                                                         \
		.if0_input =                                                                       \
			{                                                                          \
				.bLength = sizeof(struct uvc_input_terminal_descriptor),           \
				.bDescriptorType = UVC_CS_INTERFACE,                               \
				.bDescriptorSubtype = UVC_VC_INPUT_TERMINAL,                       \
				.bTerminalID = 1,                                                  \
				.wTerminalType = sys_cpu_to_le16(UVC_ITT_CAMERA),                  \
				.bAssocTerminal = 0,                                               \
				.iTerminal = 0,                                                    \
				.wObjectiveFocalLengthMin = sys_cpu_to_le16(0),                    \
				.wObjectiveFocalLengthMax = sys_cpu_to_le16(0),                    \
				.wOcularFocalLength = sys_cpu_to_le16(0),                          \
				.bControlSize = 3,                                                 \
				.bmControls = {0x00, 0x00, 0x00},                                  \
			},                                                                         \
		.if1 =                                                                             \
			{                                                                          \
				.bLength = sizeof(struct usb_if_descriptor),                       \
				.bDescriptorType = USB_DESC_INTERFACE,                             \
				.bInterfaceNumber = 1,                                             \
				.bAlternateSetting = 0,                                            \
				.bNumEndpoints = 1,                                                \
				.bInterfaceClass = USB_BCC_VIDEO,                                  \
				.bInterfaceSubClass = UVC_SC_VIDEOSTREAMING,                       \
				.bInterfaceProtocol = 0,                                           \
				.iInterface = 0,                                                   \
			},                                                                         \
		.if1_header =                                                                      \
			{                                                                          \
				.bLength = sizeof(struct uvc_stream_input_header_descriptor),      \
				.bDescriptorType = UVC_CS_INTERFACE,                               \
				.bDescriptorSubtype = UVC_VS_INPUT_HEADER,                         \
				.bNumFormats = 0,                                                  \
				.wTotalLength = sys_cpu_to_le16(                                   \
					sizeof(struct uvc_stream_input_header_descriptor) +        \
					sizeof(struct uvc_uncompressed_format_descriptor) +        \
					sizeof(struct uvc_frame_descriptor)),                      \
				.bEndpointAddress = 0x81,                                          \
				.bmInfo = 0,                                                       \
				.bTerminalLink = 2,                                                \
				.bStillCaptureMethod = 0,                                          \
				.bTriggerSupport = 0,                                              \
				.bTriggerUsage = 0,                                                \
				.bControlSize = 1,                                                 \
				.bmaControls = {0x00},                                             \
			},                                                                         \
		.ep_fs =                                                                           \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x81,                                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(64),                             \
				.bInterval = 0,                                                    \
			},                                                                         \
		.ep_hs =                                                                           \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x81,                                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(512),                            \
				.bInterval = 0,                                                    \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static struct usb_desc_header *const uvc_fs_desc_##n[] = {                                 \
		(struct usb_desc_header *)&uvc_desc_##n.iad,                                       \
		(struct usb_desc_header *)&uvc_desc_##n.if0,                                       \
		(struct usb_desc_header *)&uvc_desc_##n.if0_header,                                \
		(struct usb_desc_header *)&uvc_desc_##n.if0_output,                                \
		(struct usb_desc_header *)&uvc_desc_##n.if0_input,                                 \
		(struct usb_desc_header *)&uvc_desc_##n.if1,                                       \
		(struct usb_desc_header *)&uvc_desc_##n.if1_header,                                \
		DT_INST_FOREACH_CHILD(n, UVC_POINTER_LIST)                                         \
		(struct usb_desc_header *)&uvc_desc_##n.ep_fs,                                     \
		(struct usb_desc_header *)&uvc_desc_##n.nil_desc,                                  \
	};                                                                                         \
                                                                                                   \
	static struct usb_desc_header *const uvc_hs_desc_##n[] = {                                 \
		(struct usb_desc_header *)&uvc_desc_##n.iad,                                       \
		(struct usb_desc_header *)&uvc_desc_##n.if0,                                       \
		(struct usb_desc_header *)&uvc_desc_##n.if0_header,                                \
		(struct usb_desc_header *)&uvc_desc_##n.if0_output,                                \
		(struct usb_desc_header *)&uvc_desc_##n.if0_input,                                 \
		(struct usb_desc_header *)&uvc_desc_##n.if1,                                       \
		(struct usb_desc_header *)&uvc_desc_##n.if1_header,                                \
		DT_INST_FOREACH_CHILD(n, UVC_POINTER_LIST)                                         \
		(struct usb_desc_header *)&uvc_desc_##n.ep_hs,                                     \
		(struct usb_desc_header *)&uvc_desc_##n.nil_desc,                                  \
	};                                                                                         \
                                                                                                   \
	static const struct video_format_cap fmts_##n[] = {                                        \
		DT_INST_FOREACH_CHILD(n, UVC_VIDEO_FORMAT_CAP)                                     \
		{0}                                                                                \
	};                                                                                         \
                                                                                                   \
	static struct uvc_data uvc_data_##n = {                                                    \
		.c_data = &uvc_##n,                                                                \
		.desc = &uvc_desc_##n,                                                             \
		.fmts = fmts_##n,                                                                  \
		.fs_desc = uvc_fs_desc_##n,                                                        \
		.hs_desc = uvc_hs_desc_##n,                                                        \
		.payload_header.bHeaderLength = CONFIG_USBD_VIDEO_HEADER_SIZE,                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uvc_preinit, NULL, &uvc_data_##n, NULL, POST_KERNEL,              \
			      CONFIG_VIDEO_INIT_PRIORITY, &uvc_video_api);

DT_FOREACH_STATUS_OKAY(zephyr_uvc_format_uncompressed, UVC_UNCOMPRESSED_FORMAT_DEFINE)

DT_FOREACH_STATUS_OKAY(zephyr_uvc_format_mjpeg, UVC_MJPEG_FORMAT_DEFINE)

DT_INST_FOREACH_STATUS_OKAY(UVC_DEVICE_DEFINE)
