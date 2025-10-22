/*
 * Copyright (c) 2025 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>

#include "video_ctrls.h"
#include "video_device.h"

#define DT_DRV_COMPAT st_stm32_jpeg

LOG_MODULE_REGISTER(stm32_jpeg, CONFIG_VIDEO_LOG_LEVEL);

typedef void (*irq_config_func_t)(const struct device *dev);

struct video_common_header {
	struct video_format fmt;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	bool is_streaming;
};

struct video_m2m_common {
	struct video_common_header in;
	struct video_common_header out;
};

#define YCBCR_420_MCU_BLOCK_SIZE	384	/* 4 8x8 Y, 1 8x8 Cb, 1 8x8 Cr */

struct stm32_jpeg_data {
	const struct device *dev;
	struct video_m2m_common m2m;
	JPEG_HandleTypeDef hjpeg;
	struct k_mutex lock;
	bool codec_is_running;
	struct video_buffer *current_in;
	struct video_buffer *current_out;

	struct video_ctrl jpeg_quality;

	uint32_t current_x_mcu;
	uint32_t current_y_mcu;
	uint8_t mcu_ycbcr[YCBCR_420_MCU_BLOCK_SIZE];
};

struct stm32_jpeg_config {
	const struct stm32_pclken jpeg_hclken[1];
	irq_config_func_t irq_config;
	const struct reset_dt_spec reset_jpeg;
};

struct stm32_jpeg_fmt_conf {
	uint32_t pixelformat;
	uint32_t subsampling;
	uint8_t hmcu_div;
	uint8_t vmcu_div;
};

static const struct stm32_jpeg_fmt_conf stm32_jpeg_confs[] = {
	/* JPEG */
	{
		.pixelformat = VIDEO_PIX_FMT_JPEG,
		/* Meaningless but set to 1 to make set_fmt check working */
		.hmcu_div = 1,
		.vmcu_div = 1,
	},
	/* YCrCb 4:2:0 */
	{
		.pixelformat = VIDEO_PIX_FMT_NV12,
		.subsampling = JPEG_420_SUBSAMPLING,
		.hmcu_div = 16,
		.vmcu_div = 16,
	},
	/* TODO: YCrCb 4:2:2 to be added */
	/* TODO: YCrCb 4:4:4 to be added */
};

#define MCU_WIDTH	16
#define MCU_HEIGHT	16
#define MCU_BLOCK_SZ	8

static void stm32_jpeg_nv12_to_ycbcr_mcu(const uint8_t mcu_x, const uint8_t mcu_y,
					 const uint8_t *in_y, const uint8_t *in_uv,
					 uint8_t *out, uint32_t width)
{
	int mcu_idx = 0;

	/* Copy the 4 8x8 Y */
	for (int by = 0; by < 2; ++by) {
		for (int bx = 0; bx < 2; ++bx) {
			for (int y = 0; y < MCU_BLOCK_SZ; ++y) {
				int src_y = mcu_y * MCU_HEIGHT + by * MCU_BLOCK_SZ + y;
				int src_x = mcu_x * MCU_WIDTH + bx * MCU_BLOCK_SZ;
				const uint8_t *src = in_y + src_y * width + src_x;
				uint8_t *dst = out + mcu_idx;

				memcpy(dst, src, MCU_BLOCK_SZ);
				mcu_idx += MCU_BLOCK_SZ;
			}
		}
	}

	/* Copy 1 8x8 Cb block */
	for (int y = 0; y < MCU_BLOCK_SZ; ++y) {
		int src_y = (mcu_y * MCU_HEIGHT) / 2 + y;
		int src_x = (mcu_x * MCU_WIDTH) / 2;
		const uint8_t *src = in_uv + (src_y * width) + (src_x * 2);
		uint8_t *dst = out + mcu_idx;

		for (int x = 0; x < MCU_BLOCK_SZ; ++x) {
			dst[x] = src[x * 2];
		}
		mcu_idx += MCU_BLOCK_SZ;
	}

	/* Copy 1 8x8 Cr block */
	for (int y = 0; y < MCU_BLOCK_SZ; ++y) {
		int src_y = (mcu_y * MCU_HEIGHT) / 2 + y;
		int src_x = (mcu_x * MCU_WIDTH) / 2;
		const uint8_t *src = in_uv + (src_y * width) + (src_x * 2);
		uint8_t *dst = out + mcu_idx;

		for (int x = 0; x < MCU_BLOCK_SZ; ++x) {
			dst[x] = src[x * 2 + 1];
		}
		mcu_idx += MCU_BLOCK_SZ;
	}
}

static const struct stm32_jpeg_fmt_conf *stm32_jpeg_get_conf(uint32_t pixelformat)
{
	for (size_t i = 0; i < ARRAY_SIZE(stm32_jpeg_confs); i++) {
		if (stm32_jpeg_confs[i].pixelformat == pixelformat) {
			return &stm32_jpeg_confs[i];
		}
	}

	return NULL;
}

static void stm32_jpeg_convert_next_mcu(struct stm32_jpeg_data *data)
{
	stm32_jpeg_nv12_to_ycbcr_mcu(data->current_x_mcu++, data->current_y_mcu,
				     data->current_in->buffer, data->current_in->buffer +
				     data->m2m.in.fmt.width * data->m2m.in.fmt.height,
				     data->mcu_ycbcr, data->m2m.in.fmt.width);
	if (data->current_x_mcu >= data->m2m.in.fmt.width / MCU_WIDTH) {
		data->current_x_mcu = 0;
		data->current_y_mcu++;
	}
}

static int stm32_jpeg_start_codec(const struct device *dev)
{
	struct stm32_jpeg_data *data = dev->data;
	struct k_fifo *in_fifo_in = &data->m2m.in.fifo_in;
	struct k_fifo *out_fifo_in = &data->m2m.out.fifo_in;
	int ret;

	if (k_fifo_is_empty(in_fifo_in) || k_fifo_is_empty(out_fifo_in)) {
		/* Nothing to do */
		data->codec_is_running = false;
		return 0;
	}

	data->current_in = k_fifo_get(in_fifo_in, K_NO_WAIT);
	data->current_out = k_fifo_get(out_fifo_in, K_NO_WAIT);

	if (data->m2m.in.fmt.pixelformat != VIDEO_PIX_FMT_JPEG) {
		const struct stm32_jpeg_fmt_conf *conf =
			stm32_jpeg_get_conf(data->m2m.in.fmt.pixelformat);
		JPEG_ConfTypeDef jpeg_conf = {0};
		HAL_StatusTypeDef hret;

		__ASSERT_NO_MSG(conf != NULL);

		/* Reset value of current MCU and output buffer offset */
		data->current_x_mcu = 0;
		data->current_y_mcu = 0;

		/* JPEG Encoding */
		jpeg_conf.ColorSpace = JPEG_YCBCR_COLORSPACE;
		jpeg_conf.ChromaSubsampling = conf->subsampling;
		jpeg_conf.ImageWidth = data->m2m.in.fmt.width;
		jpeg_conf.ImageHeight = data->m2m.in.fmt.height;
		jpeg_conf.ImageQuality = data->jpeg_quality.val;

		hret = HAL_JPEG_ConfigEncoding(&data->hjpeg, &jpeg_conf);
		if (hret != HAL_OK) {
			LOG_ERR("Failed to configure codec for encoding");
			ret = -EIO;
			goto error;
		}

		data->codec_is_running = true;

		/* Convert the first MCU (and store it into mcu_ycbcr) */
		stm32_jpeg_convert_next_mcu(data);

		hret = HAL_JPEG_Encode_IT(&data->hjpeg, data->mcu_ycbcr, YCBCR_420_MCU_BLOCK_SIZE,
					  data->current_out->buffer, data->current_out->size);
		if (hret != HAL_OK) {
			LOG_ERR("Failed to request encoding");
			ret = -EIO;
			goto error;
		}
	} else {
		LOG_ERR("Decoder not yet implemented");
		ret = -EINVAL;
		goto error;
	}

	return 0;

error:
	data->codec_is_running = false;
	return ret;
}

/* Function called when the data have been generated by the JPEG block */
void HAL_JPEG_DataReadyCallback(JPEG_HandleTypeDef *hjpeg, uint8_t *data_out,
				uint32_t data_length)
{
	struct stm32_jpeg_data *data =
			CONTAINER_OF(hjpeg, struct stm32_jpeg_data, hjpeg);

	ARG_UNUSED(data_out);

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Store the output data size and timestamp */
	data->current_out->bytesused = data_length;
	data->current_out->timestamp = k_uptime_get_32();

	k_mutex_unlock(&data->lock);
}

/*
 * Function called when all processing is finished, at that moment we can be
 * sure that buffers won't be used anymore
 */
void HAL_JPEG_EncodeCpltCallback(JPEG_HandleTypeDef *hjpeg)
{
	struct stm32_jpeg_data *data = CONTAINER_OF(hjpeg, struct stm32_jpeg_data, hjpeg);
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Give back the buffers to the application */
	k_fifo_put(&data->m2m.in.fifo_out, data->current_in);
	k_fifo_put(&data->m2m.out.fifo_out, data->current_out);

	/* Try to restart the next processing if needed */
	ret = stm32_jpeg_start_codec(data->dev);
	if (ret) {
		LOG_ERR("Failed to start codec, err: %d", ret);
		goto out;
	}

out:
	k_mutex_unlock(&data->lock);
}

void HAL_JPEG_ErrorCallback(JPEG_HandleTypeDef *hjpeg)
{
	ARG_UNUSED(hjpeg);

	__ASSERT(false, "Got %s", __func__);
}

/*
 * This function is called whenever new input data (MCU) must be given in
 * order to proceed the frame
 */
void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t nb_encoded_data)
{
	struct stm32_jpeg_data *data =
			CONTAINER_OF(hjpeg, struct stm32_jpeg_data, hjpeg);

	ARG_UNUSED(nb_encoded_data);

	/* Convert the next MCU */
	stm32_jpeg_convert_next_mcu(data);

	HAL_JPEG_ConfigInputBuffer(hjpeg, data->mcu_ycbcr, YCBCR_420_MCU_BLOCK_SIZE);
}

static int stm32_jpeg_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct stm32_jpeg_data *data = dev->data;

	*fmt = fmt->type == VIDEO_BUF_TYPE_INPUT ? data->m2m.in.fmt : data->m2m.out.fmt;

	return 0;
}

static int stm32_jpeg_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct stm32_jpeg_data *data = dev->data;
	struct video_common_header *common =
		fmt->type == VIDEO_BUF_TYPE_INPUT ? &data->m2m.in : &data->m2m.out;
	const struct stm32_jpeg_fmt_conf *conf;
	int ret = 0;

	/* Validate the settings */
	conf = stm32_jpeg_get_conf(fmt->pixelformat);
	if (conf == NULL) {
		return -EINVAL;
	}
	if (fmt->width % conf->hmcu_div || fmt->height % conf->vmcu_div) {
		LOG_ERR("Format %s: %d pixels width / %d pixels height multiple required",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), conf->hmcu_div, conf->vmcu_div);
		return -EINVAL;
	}

	/*
	 * For the time being only encode is supported, aka NV12 as input and JPEG as output.
	 * Once decode will also be supported this test can be removed.
	 */
	if ((fmt->type == VIDEO_BUF_TYPE_INPUT && fmt->pixelformat != VIDEO_PIX_FMT_NV12) ||
	    (fmt->type == VIDEO_BUF_TYPE_OUTPUT && fmt->pixelformat != VIDEO_PIX_FMT_JPEG)) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (common->is_streaming) {
		ret = -EBUSY;
		goto out;
	}

	ret = video_estimate_fmt_size(fmt);
	if (ret < 0) {
		goto out;
	}

	common->fmt = *fmt;

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int stm32_jpeg_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct stm32_jpeg_data *data = dev->data;
	struct video_common_header *common =
		type == VIDEO_BUF_TYPE_INPUT ? &data->m2m.in : &data->m2m.out;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Input & Output resolutions are always same so ensure this here */
	if (data->m2m.in.fmt.width != data->m2m.out.fmt.width ||
	    data->m2m.in.fmt.height != data->m2m.out.fmt.height) {
		LOG_ERR("Input & output resolution should match");
		return -EINVAL;
	}

	if (enable == common->is_streaming) {
		ret = -EALREADY;
		goto out;
	}

	common->is_streaming = enable;

	if (enable) {
		ret = stm32_jpeg_start_codec(dev);
	} else {
		data->codec_is_running = false;
	}

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int stm32_jpeg_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct stm32_jpeg_data *data = dev->data;
	struct video_common_header *common =
		vbuf->type == VIDEO_BUF_TYPE_INPUT ? &data->m2m.in : &data->m2m.out;
	int ret = 0;

	/* TODO - need to check for buffer size here */

	k_mutex_lock(&data->lock, K_FOREVER);

	k_fifo_put(&common->fifo_in, vbuf);

	/* Try to start codec if necessary */
	if (!data->codec_is_running) {
		ret = stm32_jpeg_start_codec(dev);
		if (ret) {
			LOG_ERR("Failed to start codec, err: %d", ret);
			goto out;
		}
	}

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int stm32_jpeg_dequeue(const struct device *dev, struct video_buffer **vbuf,
			      k_timeout_t timeout)
{
	struct stm32_jpeg_data *data = dev->data;
	struct video_common_header *common =
		(*vbuf)->type == VIDEO_BUF_TYPE_INPUT ? &data->m2m.in : &data->m2m.out;

	*vbuf = k_fifo_get(&common->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static const struct video_format_cap stm32_jpeg_in_fmts[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_NV12,
		.width_min = 16,
		.width_max = 65520,
		.height_min = 16,
		.height_max = 65520,
		.width_step = 16,
		.height_step = 16,
	},
	{0}
};

static const struct video_format_cap stm32_jpeg_out_fmts[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_JPEG,
		.width_min = 16,
		.width_max = 65520,
		.height_min = 16,
		.height_max = 65520,
		.width_step = 16,
		.height_step = 16,
	},
	{0}
};

static int stm32_jpeg_get_caps(const struct device *dev, struct video_caps *caps)
{
	if (caps->type == VIDEO_BUF_TYPE_OUTPUT) {
		caps->format_caps = stm32_jpeg_out_fmts;
	} else {
		caps->format_caps = stm32_jpeg_in_fmts;
	}

	caps->min_vbuf_count = 1;

	return 0;
}

static DEVICE_API(video, stm32_jpeg_driver_api) = {
	.set_format = stm32_jpeg_set_fmt,
	.get_format = stm32_jpeg_get_fmt,
	.set_stream = stm32_jpeg_set_stream,
	.enqueue = stm32_jpeg_enqueue,
	.dequeue = stm32_jpeg_dequeue,
	.get_caps = stm32_jpeg_get_caps,
};

static int stm32_jpeg_enable_clock(const struct device *dev)
{
	const struct stm32_jpeg_config *config = dev->config;
	const struct device *cc_node = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(cc_node)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Turn on JPEG peripheral clock */
	return clock_control_on(cc_node, (clock_control_subsys_t)&config->jpeg_hclken);
}

static int stm32_jpeg_init(const struct device *dev)
{
	const struct stm32_jpeg_config *cfg = dev->config;
	struct stm32_jpeg_data *data = dev->data;
	HAL_StatusTypeDef hret;
	int ret;

	data->dev = dev;

	ret = stm32_jpeg_enable_clock(dev);
	if (ret < 0) {
		LOG_ERR("Clock enabling failed.");
		return ret;
	}

	if (!device_is_ready(cfg->reset_jpeg.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}
	ret = reset_line_toggle_dt(&cfg->reset_jpeg);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Failed to reset the device.");
		return ret;
	}

	/* Run IRQ init */
	cfg->irq_config(dev);

#if defined(CONFIG_SOC_SERIES_STM32N6X)
	HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_JPEG,
					      RIF_ATTRIBUTE_PRIV | RIF_ATTRIBUTE_SEC);
#endif

	/* Initialise default input / output formats */
	k_mutex_init(&data->lock);
	k_fifo_init(&data->m2m.in.fifo_in);
	k_fifo_init(&data->m2m.in.fifo_out);
	k_fifo_init(&data->m2m.out.fifo_in);
	k_fifo_init(&data->m2m.out.fifo_out);

	/* Initialize default formats */
	data->m2m.in.fmt.type = VIDEO_BUF_TYPE_INPUT;
	data->m2m.in.fmt.width = stm32_jpeg_in_fmts[0].width_min;
	data->m2m.in.fmt.height = stm32_jpeg_in_fmts[0].height_min;
	data->m2m.in.fmt.pixelformat = stm32_jpeg_in_fmts[0].pixelformat;
	data->m2m.out.fmt.type = VIDEO_BUF_TYPE_OUTPUT;
	data->m2m.out.fmt.width = stm32_jpeg_out_fmts[0].width_min;
	data->m2m.out.fmt.height = stm32_jpeg_out_fmts[0].height_min;
	data->m2m.out.fmt.pixelformat = stm32_jpeg_out_fmts[0].pixelformat;

	ret = video_init_ctrl(&data->jpeg_quality, dev, VIDEO_CID_JPEG_COMPRESSION_QUALITY,
			      (struct video_ctrl_range) {.min = 5, .max = 100,
							 .step = 1, .def = 50});
	if (ret < 0) {
		return ret;
	}

	/* Initialize JPEG peripheral */
	hret = HAL_JPEG_Init(&data->hjpeg);
	if (hret != HAL_OK) {
		LOG_ERR("JPEG initialization failed.");
		return -EIO;
	}

	LOG_DBG("%s initialized", dev->name);

	return 0;
}

static void stm32_jpeg_isr(const struct device *dev)
{
	struct stm32_jpeg_data *jpeg = dev->data;

	HAL_JPEG_IRQHandler(&jpeg->hjpeg);
}

#define STM32_JPEG_INIT(n)							\
	static void stm32_jpeg_irq_config_##n(const struct device *dev)		\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			    stm32_jpeg_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static struct stm32_jpeg_data stm32_jpeg_data_##n = {			\
		.hjpeg = {							\
			.Instance = (JPEG_TypeDef *)DT_INST_REG_ADDR(n),	\
		},								\
	};									\
										\
	static const struct stm32_jpeg_config stm32_jpeg_config_##n = {		\
		.jpeg_hclken = STM32_DT_INST_CLOCKS(n),				\
		.irq_config = stm32_jpeg_irq_config_##n,			\
		.reset_jpeg = RESET_DT_SPEC_INST_GET_BY_IDX(n, 0),		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, &stm32_jpeg_init,				\
			      NULL, &stm32_jpeg_data_##n,			\
			      &stm32_jpeg_config_##n,				\
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,		\
			      &stm32_jpeg_driver_api);				\
										\
	VIDEO_DEVICE_DEFINE(jpeg_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_JPEG_INIT)
