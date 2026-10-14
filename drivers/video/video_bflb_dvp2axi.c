/*
 * SPDX-FileCopyrightText: Copyright Panoramix Labs
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_dvp2axi

#include <zephyr/cache.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/video.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

LOG_MODULE_REGISTER(bflb_dvp2axi, CONFIG_VIDEO_LOG_LEVEL);

#include <bouffalolab/common/cam_reg.h>

#include <bflb_soc.h>
#include <glb_reg.h>

#include "video_common.h"

#define CAM_REG_DVP_DATA_MODE_NONE		0
#define CAM_REG_DVP_DATA_MODE_888_TO_565	2
#define CAM_REG_DVP_DATA_MODE_888_TO_888X	3
#define CAM_REG_DVP_DATA_MODE_YUV		4

#define CAM_INTCLR_POP_FRAME			(1 << 0)
#define CAM_INTCLR_NORMAL			(1 << 4)
#define CAM_INTCLR_MEMORY_OVERWRITE		(1 << 5)
#define CAM_INTCLR_FRAME_OVERWRITE		(1 << 6)
#define CAM_INTCLR_FIFO_OVERWRITE		(1 << 7)
#define CAM_INTCLR_HSYNC_MISMATCH		(1 << 8)
#define CAM_INTCLR_VSYNC_MISMATCH		(1 << 9)

#define CAM_INTMASK_HSYNC_MISMATCH		(1 << 6)
#define CAM_INTMASK_VSYNC_MISMATCH		(1 << 7)
#define CAM_INTMASK_NORMAL			(1 << 8)
#define CAM_INTMASK_MEMORY_OVERWRITE		(1 << 9)
#define CAM_INTMASK_FRAME_OVERWRITE 		(1 << 10)
#define CAM_INTMASK_FIFO_OVERWRITE		(1 << 11)

#define CAM_REG_INT_HCNT_EN			(1 << 6U)
#define CAM_REG_INT_VCNT_EN			(1 << 7U)
#define CAM_REG_INT_NORMAL_EN			(1 << 8U)
#define CAM_REG_INT_MEM_EN			(1 << 9U)
#define CAM_REG_INT_FRAME_EN			(1 << 10U)
#define CAM_REG_INT_FIFO_EN 			(1 << 11U)
#define CAM_STS_NORMAL_INT			(1 << 12U)
#define CAM_STS_MEM_INT				(1 << 13U)
#define CAM_STS_FRAME_INT			(1 << 14U)
#define CAM_STS_FIFO_INT			(1 << 15U)
#define CAM_STS_MEM_INT				(1 << 13U)
#define CAM_STS_FRAME_INT			(1 << 14U)
#define CAM_STS_FIFO_INT			(1 << 15U)
#define CAM_FRAME_VALID_CNT_SHIFT		(16U)
#define CAM_FRAME_VALID_CNT_MASK		(0x1f << CAM_FRAME_VALID_CNT_SHIFT)
#define CAM_STS_HCNT_INT			(1 << 21U)
#define CAM_STS_VCNT_INT			(1 << 22U)
#define CAM_ST_BUS_IDLE				(1 << 24U)
#define CAM_ST_BUS_FUNC				(1 << 25U)
#define CAM_ST_BUS_WAIT				(1 << 26U)
#define CAM_ST_BUS_FLSH				(1 << 27U)
#define CAM_AXI_IDLE				(1 << 28U)
#define CAM_ST_DVP_IDLE				(1 << 29U)

#define CAM_POLARITY_ACTIVE_LOW			0
#define CAM_POLARITY_ACTIVE_HIGH		1

struct bflb_dvp2axi_config {
	uintptr_t base;
	const struct device *source_dev;
	void (*irq_config_func)(const struct device *dev);
	uint8_t axi_burst_length;
	uint8_t axi_data_width;
	uint8_t hsync_active : 1;
	uint8_t vsync_active : 1;
};

struct bflb_dvp2axi_data {
	struct video_format_cap fmts[CONFIG_VIDEO_BFLB_DVP2AXI_MAX_FORMATS];
	size_t num_fmts;
	struct video_format fmt;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct video_buffer *active_vbuf;
	bool is_streaming;
};

void bflb_cam_start(const struct device *dev)
{
	const struct bflb_dvp2axi_config *config = dev->config;
	uint32_t tmp;

	tmp = sys_read32(config->base + CAM_DVP2AXI_CONFIGUE_OFFSET);
	tmp |= CAM_REG_DVP_ENABLE;
	sys_write32(tmp, config->base + CAM_DVP2AXI_CONFIGUE_OFFSET);
}

uint8_t bflb_cam_get_frame_count(const struct device *dev)
{
	const struct bflb_dvp2axi_config *config = dev->config;
	uint32_t tmp;

	tmp = sys_read32(config->base + CAM_DVP_STATUS_AND_ERROR_OFFSET);
	tmp &= CAM_FRAME_VALID_CNT_MASK;
	return (tmp >> CAM_FRAME_VALID_CNT_SHIFT);
}

static int bflb_dvp2axi_get_caps(const struct device *dev, struct video_caps *caps)
{
	struct bflb_dvp2axi_data *data = dev->data;

	caps->format_caps = data->fmts;
	caps->min_vbuf_count = 1;
	caps->buf_align = 32;

	return 0;
}

static int bflb_dvp2axi_set_format(const struct device *dev, struct video_format *fmt)
{
	const struct bflb_dvp2axi_config *config = dev->config;
	struct bflb_dvp2axi_data *data = dev->data;
	size_t fmt_idx;
	int ret;

	ret = video_format_caps_index(data->fmts, fmt, &fmt_idx);
	if (ret < 0) {
		LOG_ERR("Format %s %ux%u unsupported by %s",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height, dev->name);
		return ret;
	}

	ret = video_set_format(config->source_dev, fmt);
	if (ret < 0) {
		return ret;
	}

	ret = video_estimate_fmt_size(fmt);
	if (ret < 0) {
		return ret;
	}

	data->fmt = *fmt;

	return 0;
}

static int bflb_dvp2axi_get_format(const struct device *dev, struct video_format *fmt)
{
	const struct bflb_dvp2axi_config *config = dev->config;

	return video_get_format(config->source_dev, fmt);
}

static int bflb_dvp2axi_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct bflb_dvp2axi_config *config = dev->config;

	return video_set_frmival(config->source_dev, frmival);
}

static int bflb_dvp2axi_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct bflb_dvp2axi_config *config = dev->config;

	return video_get_frmival(config->source_dev, frmival);
}

static int bflb_dvp2axi_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct bflb_dvp2axi_config *config = dev->config;

	return video_enum_frmival(config->source_dev, fie);
}

static void bflb_dvp2axi_apply_config(const struct device *dev)
{
	const struct bflb_dvp2axi_config *config = dev->config;
	struct bflb_dvp2axi_data*data= dev->data;
	uint8_t data_mode = 0;
	uint32_t tmp;

	tmp = (uintptr_t)data->active_vbuf->buffer;
	sys_write32(tmp, config->base + CAM_DVP2AXI_ADDR_START_OFFSET);

	tmp = data->fmt.height << 16 | data->fmt.width;
	sys_write32(tmp, config->base + CAM_DVP2AXI_FRAM_EXM_OFFSET);

	sys_write32(0, config->base + CAM_DVP_DEBUG_OFFSET);

	/* BCNT is byte count */
	sys_write32(data->active_vbuf->size, config->base + CAM_DVP2AXI_FRAME_BCNT_OFFSET);

	/* BCNT is AXI burst count */
	tmp = data->active_vbuf->size
		/ (config->axi_data_width / BITS_PER_BYTE)
		/ 1; //config->axi_burst_length;
	sys_write32(tmp, config->base + CAM_DVP2AXI_MEM_BCNT_OFFSET);

	tmp = sys_read32(config->base + CAM_DVP_STATUS_AND_ERROR_OFFSET);
	tmp |= CAM_REG_INT_HCNT_EN;
	tmp |= CAM_REG_INT_VCNT_EN;
	tmp |= CAM_REG_INT_NORMAL_EN;
	tmp |= CAM_REG_INT_MEM_EN;
	tmp |= CAM_REG_INT_FRAME_EN;
	tmp |= CAM_REG_INT_FIFO_EN;
	sys_write32(tmp, config->base + CAM_DVP_STATUS_AND_ERROR_OFFSET);

	tmp = sys_read32(config->base + CAM_DVP2AXI_CONFIGUE_OFFSET);
	/*  */
	tmp &= ~CAM_REG_DROP_EN;
	/* continuous/trigger mode */
	tmp &= ~CAM_REG_SW_MODE;
	/* Pixel selection for i.e. splitting YUV in Y and UV */
	tmp &= ~CAM_REG_DVP_DATA_BSEL;
	/* format conversion */
	tmp &= ~CAM_REG_DVP_DATA_MODE_MASK;
	tmp |= data_mode << CAM_REG_DVP_DATA_MODE_SHIFT;
	/* burst length */
	tmp &= ~CAM_REG_XLEN_MASK;
	tmp |= config->axi_burst_length == 1 ? (0 << CAM_REG_XLEN_SHIFT) :
		config->axi_burst_length == 4 ? (1 << CAM_REG_XLEN_SHIFT) :
		config->axi_burst_length == 8 ? (2 << CAM_REG_XLEN_SHIFT) :
		config->axi_burst_length == 16 ? (3 << CAM_REG_XLEN_SHIFT) :
		config->axi_burst_length == 32 ? (5 << CAM_REG_XLEN_SHIFT) :
		config->axi_burst_length == 64 ? (6 << CAM_REG_XLEN_SHIFT) : 0;
	/* hsync active high/low */
	if (config->hsync_active) {
		tmp |= CAM_REG_LINE_VLD_POL;
	} else {
		tmp &= ~CAM_REG_LINE_VLD_POL;
	}
	/* vsync active high/low */
	if (config->vsync_active) {
		tmp |= CAM_REG_FRAM_VLD_POL;
	} else {
		tmp &= ~CAM_REG_FRAM_VLD_POL;
	}
	sys_write32(tmp, config->base + CAM_DVP2AXI_CONFIGUE_OFFSET);
}

static void bflb_dvp2axi_trigger(const struct device *dev)
{
	const struct bflb_dvp2axi_config *config = dev->config;
	struct bflb_dvp2axi_data *data = dev->data;
	uint32_t tmp;
	int key;

	key = k_irq_lock();

	if (data->active_vbuf != NULL) {
		LOG_DBG("Already busy with %p, skipping", (void *)data->active_vbuf->buffer);
		goto end;
	}

	data->active_vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (data->active_vbuf == NULL) {
		LOG_DBG("No buffer submitted yet");
		goto end;
	}

	LOG_DBG("Submitting new buffer %p, size %u, format %s %ux%u",
		(void *)data->active_vbuf->buffer, data->active_vbuf->size,
		VIDEO_FOURCC_TO_STR(data->fmt.pixelformat), data->fmt.width, data->fmt.height);

	bflb_dvp2axi_apply_config(dev);

	tmp = sys_read32(config->base + CAM_DVP2AXI_CONFIGUE_OFFSET);
	tmp |= CAM_REG_DVP_ENABLE;
	sys_write32(tmp, config->base + CAM_DVP2AXI_CONFIGUE_OFFSET);
end:
	k_irq_unlock(key);
}

void bflb_dvp2axi_detrigger(const struct device *dev)
{
	const struct bflb_dvp2axi_config *config = dev->config;
	uint32_t tmp;

	tmp = sys_read32(config->base + CAM_DVP2AXI_CONFIGUE_OFFSET);
	tmp &= ~CAM_REG_DVP_ENABLE;
	sys_write32(tmp, config->base + CAM_DVP2AXI_CONFIGUE_OFFSET);
}

static int bflb_dvp2axi_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct bflb_dvp2axi_data *data = dev->data;

	LOG_DBG("Enqueueing frame %p of %u bytes", (void *)vbuf->buffer, vbuf->size);

	/* TODO */
	memset(vbuf->buffer, 0x00, vbuf->size);

	if (data->fmt.pixelformat == 0 || data->fmt.width == 0 || data->fmt.height == 0)  {
		LOG_ERR("Format not conifgured");
		return -EINVAL;
	}

	k_fifo_put(&data->fifo_in, vbuf);

	if (data->is_streaming) {
		bflb_dvp2axi_trigger(dev);
	}

	return 0;
}

static int bflb_dvp2axi_dequeue(const struct device *dev, struct video_buffer **vbuf,
				  k_timeout_t timeout)
{
	struct bflb_dvp2axi_data *data = dev->data;

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		LOG_INF("Failed to get a buffer from %s FIFO", dev->name);
		return -ETIMEDOUT;
	}

	return 0;
}

static int bflb_dvp2axi_flush(const struct device *dev, bool cancel)
{
	struct bflb_dvp2axi_data *data = dev->data;
	struct video_buffer *vbuf = NULL;

	if (cancel) {
		if (data->active_vbuf != NULL) {
			k_fifo_put(&data->fifo_out, data->active_vbuf);
			data->active_vbuf = NULL;
		}
		while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT)) != NULL) {
			k_fifo_put(&data->fifo_out, vbuf);
		}
	} else {
		while (!k_fifo_is_empty(&data->fifo_in)) {
			k_sleep(K_MSEC(1));
		}
	}

	return 0;
}

static int bflb_dvp2axi_set_stream(const struct device *dev, bool stream, enum video_buf_type type)
{
	const struct bflb_dvp2axi_config *config = dev->config;
	struct bflb_dvp2axi_data *data = dev->data;
	int ret;

	if (type != VIDEO_BUF_TYPE_OUTPUT) {
		LOG_ERR("Supporting output buffer type only");
		return -EINVAL;
	}

	if (data->is_streaming == stream) {
		return 0;
	}

	if (stream) {
		LOG_INF("Starting %s", dev->name);

		ret = video_stream_start(config->source_dev, type);
		if (ret < 0) {
			LOG_ERR("Failed to start source device %s", config->source_dev->name);
			return ret;
		}

		bflb_dvp2axi_trigger(dev);
	} else {
		LOG_INF("Stopping %s", dev->name);

		bflb_dvp2axi_detrigger(dev);

		ret = video_stream_stop(config->source_dev, type);
		if (ret < 0) {
			LOG_ERR("Failed to start source device %s", config->source_dev->name);
			return ret;
		}
	}

	data->is_streaming = stream;

	return 0;
}

static DEVICE_API(video, bflb_dvp2axi_api) = {
	.set_format = bflb_dvp2axi_set_format,
	.get_format = bflb_dvp2axi_get_format,
	.set_stream = bflb_dvp2axi_set_stream,
	.get_caps = bflb_dvp2axi_get_caps,
	.enqueue = bflb_dvp2axi_enqueue,
	.dequeue = bflb_dvp2axi_dequeue,
	.flush = bflb_dvp2axi_flush,
	.set_frmival = bflb_dvp2axi_set_frmival,
	.get_frmival = bflb_dvp2axi_get_frmival,
	.enum_frmival = bflb_dvp2axi_enum_frmival,
};

static const void bflb_dvp2axi_isr(const void *p)
{
	const struct device *dev = p;
	const struct bflb_dvp2axi_config *config = dev->config;
	struct bflb_dvp2axi_data *data = dev->data;
	uint32_t tmp;

	tmp = sys_read32(config->base + CAM_DVP_STATUS_AND_ERROR_OFFSET);
	if (tmp & CAM_STS_MEM_INT) {
		sys_write32(CAM_INTCLR_MEMORY_OVERWRITE,
			    config->base + CAM_DVP_FRAME_FIFO_POP_OFFSET);
		LOG_WRN("Memory overwrite occured");
	}
	if (tmp & CAM_STS_FRAME_INT) {
		sys_write32(CAM_INTCLR_FRAME_OVERWRITE,
			    config->base + CAM_DVP_FRAME_FIFO_POP_OFFSET);
		LOG_WRN("Frame overwrite occured");
	}
	if (tmp & CAM_STS_FRAME_INT) {
		sys_write32(CAM_INTCLR_FIFO_OVERWRITE,
			    config->base + CAM_DVP_FRAME_FIFO_POP_OFFSET);
		LOG_WRN("Fifo overwrite occured");
	}
	if (tmp & CAM_STS_HCNT_INT) {
		sys_write32(CAM_INTCLR_HSYNC_MISMATCH,
			    config->base + CAM_DVP_FRAME_FIFO_POP_OFFSET);
		LOG_WRN("HSYNC mismatch occured");
	}
	if (tmp & CAM_STS_VCNT_INT) {
		sys_write32(CAM_INTCLR_VSYNC_MISMATCH,
			    config->base + CAM_DVP_FRAME_FIFO_POP_OFFSET);
		LOG_WRN("VSYNC mismatch occured");
	}
	if (tmp & CAM_STS_NORMAL_INT) {
		sys_write32(CAM_INTCLR_NORMAL,
			    config->base + CAM_DVP_FRAME_FIFO_POP_OFFSET);
	} else {
		return;
	}

	bflb_dvp2axi_detrigger(dev);

	if (data->active_vbuf == NULL) {
		LOG_ERR("%s got ISR without active buffer", dev->name);
		return;
	}

	sys_write32(CAM_INTCLR_POP_FRAME,
		    config->base + CAM_DVP_FRAME_FIFO_POP_OFFSET);

	sys_cache_data_invd_range(data->active_vbuf->buffer, data->active_vbuf->size);
	data->active_vbuf->bytesused = data->active_vbuf->size;
	k_fifo_put(&data->fifo_out, data->active_vbuf);
	data->active_vbuf = NULL;

	bflb_dvp2axi_trigger(dev);
}

static void bflb_dvp2axi_add_format_cap(const struct device *dev,
					const struct video_format_cap *fmt_cap)
{
	struct bflb_dvp2axi_data *data = dev->data;

	if (data->num_fmts + 1 >= CONFIG_VIDEO_BFLB_DVP2AXI_MAX_FORMATS) {
		LOG_WRN("CONFIG_VIDEO_BFLB_DVP2AXI_MAX_FORMATS too small, raise above %u",
			CONFIG_VIDEO_BFLB_DVP2AXI_MAX_FORMATS);
		return;
	}

	data->fmts[data->num_fmts] = *fmt_cap;
	data->num_fmts++;
}

static void bflb_dvp2axi_init_clock(const struct device *dev)
{
	/* Done by camfront to initialize first */
}

static int bflb_dvp2axi_init(const struct device *dev)
{
	const struct bflb_dvp2axi_config *config = dev->config;
	struct bflb_dvp2axi_data *data = dev->data;
	struct video_format fmt = {};
	struct video_caps caps = {.type = VIDEO_BUF_TYPE_OUTPUT};
	int ret;

	LOG_DBG("Initializing %s", dev->name);

	if (!device_is_ready(config->source_dev)) {
		LOG_ERR("%s source %s is not ready", dev->name, config->source_dev->name);
		return -ENODEV;
	}

	bflb_dvp2axi_init_clock(dev);

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);

	/* generate caps */
	ret = video_get_caps(config->source_dev, &caps);
	if (ret != 0) {
		LOG_ERR("Failed to get %s capabilities", config->source_dev->name);
		return ret;
	}
	for (size_t i = 0; caps.format_caps[i].pixelformat != 0; i++) {
		bflb_dvp2axi_add_format_cap(dev, &caps.format_caps[i]);
	}

	/* default format */
	fmt.pixelformat = caps.format_caps[0].pixelformat,
	fmt.width = caps.format_caps[0].width_min,
	fmt.height = caps.format_caps[0].height_min,
	ret = video_set_format(dev, &fmt);
	if (ret != 0) {
		LOG_ERR("Failed to set default format to %s %ux%u",
			VIDEO_FOURCC_TO_STR(fmt.pixelformat), fmt.width, fmt.height);
		return ret;
	}

	config->irq_config_func(dev);

	return 0;
}

#if CONFIG_DEVICE_DEINIT_SUPPORT
static const int bflb_dvp2axi_deinit(const struct device *dev)
{
	const struct bflb_dvp2axi_config *config = dev->config;

	tmp = sys_read32(config->base + CAM_DVP2AXI_CONFIGUE_OFFSET);
	tmp &= ~CAM_REG_DVP_ENABLE;
	sys_write32(tmp, config->base + CAM_DVP2AXI_CONFIGUE_OFFSET);

	return 0;
}
#endif

#define SOURCE_NODE(n) \
	DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(n, 0, 0))

#define SOURCE_LINK_PROP(n, prop) \
	DT_PROP(DT_CHILD(DT_CHILD(SOURCE_NODE(n), port), endpoint_0), prop)

#define BFLB_DVP2AXI_INIT(n)									\
	static void bflb_dvp2axi_irq_config_##n(const struct device *dev)			\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),				\
				bflb_dvp2axi_isr, DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));							\
	}											\
												\
	struct bflb_dvp2axi_data bflb_dvp2axi_data_##n = {					\
	};											\
												\
	const struct bflb_dvp2axi_config bflb_dvp2axi_config_##n = {				\
		.base = DT_INST_REG_ADDR(n),							\
		.irq_config_func = &bflb_dvp2axi_irq_config_##n,				\
		.source_dev = DEVICE_DT_GET(SOURCE_NODE(n)),					\
		.axi_burst_length = DT_INST_PROP(n, axi_burst_length),				\
		.axi_data_width = DT_INST_PROP(n, axi_data_width),				\
		.hsync_active = SOURCE_LINK_PROP(n, hsync_active),				\
		.vsync_active = SOURCE_LINK_PROP(n, vsync_active),				\
	};											\
												\
	DEVICE_DT_INST_DEINIT_DEFINE(n, &bflb_dvp2axi_init, &bflb_dvp2axi_deinit, NULL,		\
				    &bflb_dvp2axi_data_##n, &bflb_dvp2axi_config_##n,		\
				    POST_KERNEL, CONFIG_VIDEO_BFLB_DVP2AXI_INIT_PRIORITY,	\
				    &bflb_dvp2axi_api);						\
												\
	VIDEO_DEVICE_DEFINE(bflb_dvp2axi_##n, DEVICE_DT_INST_GET(n),				\
			    DEVICE_DT_GET(SOURCE_NODE(n)));

DT_INST_FOREACH_STATUS_OKAY(BFLB_DVP2AXI_INIT)
