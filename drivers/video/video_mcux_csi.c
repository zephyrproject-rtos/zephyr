/*
 * Copyright (c) 2019, Linaro Limited
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_csi

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/rtio/rtio.h>

#include <fsl_csi.h>

#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif

#include "video_buffer.h"
#include "video_device.h"

struct video_mcux_csi_config {
	CSI_Type *base;
	const struct device *source_dev;
	const struct pinctrl_dev_config *pincfg;
};

struct video_mcux_csi_data {
	const struct device *dev;
	csi_config_t csi_config;
	csi_handle_t csi_handle;
	struct mpsc io_q;
};

static void __frame_done_cb(CSI_Type *base, csi_handle_t *handle, status_t status, void *user_data)
{
	struct video_mcux_csi_data *data = user_data;
	const struct device *dev = data->dev;
	const struct video_mcux_csi_config *config = dev->config;
	enum video_signal_result result = VIDEO_BUF_DONE;
	uint32_t buffer_addr;

	/* IRQ context */
	if (status != kStatus_CSI_FrameDone) {
		return;
	}

	status = CSI_TransferGetFullBuffer(config->base, &(data->csi_handle), &buffer_addr);
	if (status != kStatus_Success) {
		result = VIDEO_BUF_ERROR;
		return;
	}

	struct rtio_iodev_sqe *iodev_sqe = video_pop_io_q(&data->io_q);

	if (iodev_sqe == NULL) {
		return;
	}


	struct video_buffer *vbuf = iodev_sqe->sqe.userdata;

#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_InvalidateByRange(buffer_addr, vbuf->bytesused);
#endif

	if ((uint32_t)vbuf->buffer != buffer_addr) {
		return;
	}

	vbuf->timestamp = k_uptime_get_32();

	rtio_iodev_sqe_ok(iodev_sqe, 0);

	return;
}

#if defined(CONFIG_VIDEO_MCUX_MIPI_CSI2RX)
K_HEAP_DEFINE(csi_heap, 1000);
static struct video_format_cap *fmts;
/*
 * On i.MX RT11xx SoCs which have MIPI CSI-2 Rx, image data from the camera sensor after passing
 * through the pipeline (MIPI CSI-2 Rx --> Video Mux --> CSI) will be implicitly converted to a
 * 32-bits pixel format. For example, an input in RGB565 or YUYV (2-bytes format) will become a
 * XRGB32 or XYUV32 (4-bytes format) respectively, at the output of the CSI.
 */
static inline void video_pix_fmt_convert(struct video_format *fmt, bool isGetFmt)
{
	switch (fmt->pixelformat) {
	case VIDEO_PIX_FMT_XRGB32:
		fmt->pixelformat = isGetFmt ? VIDEO_PIX_FMT_XRGB32 : VIDEO_PIX_FMT_RGB565;
		break;
	case VIDEO_PIX_FMT_XYUV32:
		fmt->pixelformat = isGetFmt ? VIDEO_PIX_FMT_XYUV32 : VIDEO_PIX_FMT_YUYV;
		break;
	case VIDEO_PIX_FMT_RGB565:
		fmt->pixelformat = isGetFmt ? VIDEO_PIX_FMT_XRGB32 : VIDEO_PIX_FMT_RGB565;
		break;
	case VIDEO_PIX_FMT_YUYV:
		fmt->pixelformat = isGetFmt ? VIDEO_PIX_FMT_XYUV32 : VIDEO_PIX_FMT_YUYV;
		break;
	}

	fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;
}
#endif

static int video_mcux_csi_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct video_mcux_csi_config *config = dev->config;
	struct video_mcux_csi_data *data = dev->data;
	status_t ret;
	struct video_format format = *fmt;

	data->csi_config.bytesPerPixel = video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;
	data->csi_config.linePitch_Bytes = fmt->width * data->csi_config.bytesPerPixel;
#if defined(CONFIG_VIDEO_MCUX_MIPI_CSI2RX)
	if (fmt->pixelformat != VIDEO_PIX_FMT_XRGB32 && fmt->pixelformat != VIDEO_PIX_FMT_XYUV32) {
		return -ENOTSUP;
	}
	video_pix_fmt_convert(&format, false);
	data->csi_config.dataBus = kCSI_DataBus24Bit;
#else
	data->csi_config.dataBus = kCSI_DataBus8Bit;
#endif
	data->csi_config.polarityFlags = kCSI_HsyncActiveHigh | kCSI_DataLatchOnRisingEdge;
	data->csi_config.workMode = kCSI_GatedClockMode; /* use VSYNC, HSYNC, and PIXCLK */
	data->csi_config.useExtVsync = true;
	data->csi_config.height = fmt->height;
	data->csi_config.width = fmt->width;

	ret = CSI_Init(config->base, &data->csi_config);
	if (ret != kStatus_Success) {
		return -EIO;
	}

	ret = CSI_TransferCreateHandle(config->base, &data->csi_handle, __frame_done_cb, data);
	if (ret != kStatus_Success) {
		return -EIO;
	}

	if (config->source_dev && video_set_format(config->source_dev, &format)) {
		return -EIO;
	}

	fmt->pitch = data->csi_config.linePitch_Bytes;

	return 0;
}

static int video_mcux_csi_get_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct video_mcux_csi_config *config = dev->config;

	if (config->source_dev && !video_get_format(config->source_dev, fmt)) {
#if defined(CONFIG_VIDEO_MCUX_MIPI_CSI2RX)
		video_pix_fmt_convert(fmt, true);
#endif

		return 0;
	}

	return -EIO;
}

static int video_mcux_csi_set_stream(const struct device *dev, bool enable,
				     enum video_buf_type type)
{
	const struct video_mcux_csi_config *config = dev->config;
	struct video_mcux_csi_data *data = dev->data;
	status_t ret;

	if (enable) {
		ret = CSI_TransferStart(config->base, &data->csi_handle);
		if (ret != kStatus_Success) {
			return -EIO;
		}

		if (config->source_dev && video_stream_start(config->source_dev, type)) {
			return -EIO;
		}
	} else {
		if (config->source_dev && video_stream_stop(config->source_dev, type)) {
			return -EIO;
		}

		ret = CSI_TransferStop(config->base, &data->csi_handle);
		if (ret != kStatus_Success) {
			return -EIO;
		}
	}

	return 0;
}

static int video_mcux_csi_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct video_mcux_csi_config *config = dev->config;
	int err = -ENODEV;

	/* Just forward to source dev for now */
	if (config->source_dev) {
		err = video_get_caps(config->source_dev, caps);
#if defined(CONFIG_VIDEO_MCUX_MIPI_CSI2RX)
		/*
		 * On i.MX RT11xx SoCs which have MIPI CSI-2 Rx, image data from the camera sensor
		 * after passing through the pipeline (MIPI CSI-2 Rx --> Video Mux --> CSI) will be
		 * implicitly converted to a 32-bits pixel format. For example, an input in RGB565
		 * or YUYV (2-bytes format) will become an XRGB32 or XYUV32 (4-bytes format)
		 * respectively, at the output of the CSI. So, we change the pixel formats of the
		 * source caps to reflect this.
		 */
		int ind = 0;

		while (caps->format_caps[ind].pixelformat) {
			ind++;
		}
		k_heap_free(&csi_heap, fmts);
		fmts = k_heap_alloc(&csi_heap, (ind + 1) * sizeof(struct video_format_cap),
				    K_FOREVER);

		for (int i = 0; i <= ind; i++) {
			memcpy(&fmts[i], &caps->format_caps[i], sizeof(fmts[i]));
			if (fmts[i].pixelformat == VIDEO_PIX_FMT_RGB565) {
				fmts[i].pixelformat = VIDEO_PIX_FMT_XRGB32;
			} else if (fmts[i].pixelformat == VIDEO_PIX_FMT_YUYV) {
				fmts[i].pixelformat = VIDEO_PIX_FMT_XYUV32;
			}
		}
		caps->format_caps = fmts;
#endif
	}

	/* NXP MCUX CSI request at least 2 buffer before starting */
	caps->min_vbuf_count = 2;
	/* CSI only operates on buffers of full frame size */
	caps->min_line_count = caps->max_line_count = LINE_COUNT_HEIGHT;

	/* no source dev */
	return err;
}

extern void CSI_DriverIRQHandler(void);
static void video_mcux_csi_isr(const void *p)
{
	ARG_UNUSED(p);
	CSI_DriverIRQHandler();
}

static int video_mcux_csi_init(const struct device *dev)
{
	const struct video_mcux_csi_config *config = dev->config;
	struct video_mcux_csi_data *data = dev->data;
	int err;

	CSI_GetDefaultConfig(&data->csi_config);

	/* check if there is any source device (video ctrl device)
	 * the device is not yet initialized so we only check if it exists
	 */
	if (config->source_dev == NULL) {
		return -ENODEV;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	mpsc_init(&data->io_q);

	return 0;
}

static int video_mcux_csi_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_mcux_csi_config *config = dev->config;

	return video_set_frmival(config->source_dev, frmival);
}

static int video_mcux_csi_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_mcux_csi_config *config = dev->config;

	return video_get_frmival(config->source_dev, frmival);
}

static int video_mcux_csi_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct video_mcux_csi_config *config = dev->config;
	const struct video_format *fie_fmt = fie->format;
	int ret;

#if defined(CONFIG_VIDEO_MCUX_MIPI_CSI2RX)
	struct video_format converted_fmt = *fie->format;

	video_pix_fmt_convert(&converted_fmt, false);
	fie->format = &converted_fmt;
#endif

	ret = video_enum_frmival(config->source_dev, fie);
	fie->format = fie_fmt;

	return ret;
}

// static int video_mcux_csi_enqueue(const struct device *dev, struct video_buffer *vbuf)
// {
// 	const struct video_mcux_csi_config *config = dev->config;
// 	struct video_mcux_csi_data *data = dev->data;
// 	unsigned int to_read;
// 	status_t ret;

// 	struct rtio_iodev_sqe *iodev_sqe = video_pop_io_q(&data->io_q);

// 	if (iodev_sqe == NULL) {
// 		return -EIO;
// 	}

// 	struct video_buffer *vbuf = iodev_sqe->sqe.userdata;

// 	to_read = data->csi_config.linePitch_Bytes * data->csi_config.height;
// 	vbuf->bytesused = to_read;
// 	vbuf->line_offset = 0;

// 	ret = CSI_TransferSubmitEmptyBuffer(config->base, &data->csi_handle,
// 					    (uint32_t)vbuf->buffer);
// 	if (ret != kStatus_Success) {
// 		return -EIO;
// 	}

// 	return 0;
// }

static void video_mcux_csi_iodev_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{

	const struct video_mcux_csi_config *config = dev->config;
	struct video_mcux_csi_data *data = dev->data;
	struct video_buffer *vbuf = iodev_sqe->sqe.userdata;
	status_t ret;

	// struct rtio_iodev_sqe *iodev_sqe = video_pop_io_q(&data->io_q);

	// if (iodev_sqe == NULL) {
	// 	return -EIO;
	// }

	vbuf->bytesused = data->csi_config.linePitch_Bytes * data->csi_config.height;
	vbuf->line_offset = 0;

	ret = CSI_TransferSubmitEmptyBuffer(config->base, &data->csi_handle,
					    (uint32_t)vbuf->buffer);
	if (ret != kStatus_Success) {
		return;
	}

	return;
}

static DEVICE_API(video, video_mcux_csi_driver_api) = {
	.set_format = video_mcux_csi_set_fmt,
	.get_format = video_mcux_csi_get_fmt,
	.set_stream = video_mcux_csi_set_stream,
	// .enqueue = video_mcux_csi_enqueue,
	.get_caps = video_mcux_csi_get_caps,
	.set_frmival = video_mcux_csi_set_frmival,
	.get_frmival = video_mcux_csi_get_frmival,
	.enum_frmival = video_mcux_csi_enum_frmival,
	.iodev_submit = video_mcux_csi_iodev_submit,
};

#if 1 /* Unique Instance */
PINCTRL_DT_INST_DEFINE(0);

#define SOURCE_DEV(n) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(n, 0, 0)))

static const struct video_mcux_csi_config video_mcux_csi_config_0 = {
	.base = (CSI_Type *)DT_INST_REG_ADDR(0),
	.source_dev = SOURCE_DEV(0),
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static struct video_mcux_csi_data video_mcux_csi_data_0;

static int video_mcux_csi_init_0(const struct device *dev)
{
	struct video_mcux_csi_data *data = dev->data;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), video_mcux_csi_isr, NULL, 0);

	irq_enable(DT_INST_IRQN(0));

	data->dev = dev;

	return video_mcux_csi_init(dev);
}

DEVICE_DT_INST_DEFINE(0, &video_mcux_csi_init_0, NULL, &video_mcux_csi_data_0,
		      &video_mcux_csi_config_0, POST_KERNEL, CONFIG_VIDEO_MCUX_CSI_INIT_PRIORITY,
		      &video_mcux_csi_driver_api);

VIDEO_DEVICE_DEFINE(csi, DEVICE_DT_INST_GET(0), SOURCE_DEV(0));

VIDEO_INTERFACE_DEFINE(csi_intf, DEVICE_DT_INST_GET(0), &(video_mcux_csi_data_0.io_q));

#endif
