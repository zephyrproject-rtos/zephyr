/*
 * Copyright (c) 2024 Charles Dias <charlesdias.cd@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_dcmi

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_stm32.h>

#include <stm32_ll_dma.h>

#include "video_device.h"

LOG_MODULE_REGISTER(video_stm32_dcmi, CONFIG_VIDEO_LOG_LEVEL);

#if CONFIG_VIDEO_BUFFER_POOL_NUM_MAX < 2
#error "The minimum required number of buffers for video_stm32 is 2"
#endif

typedef void (*irq_config_func_t)(const struct device *dev);

struct stream {
	DMA_TypeDef *reg;
	const struct device *dma_dev;
	uint32_t channel;
	struct dma_config cfg;
};

struct video_stm32_dcmi_data {
	const struct device *dev;
	DCMI_HandleTypeDef hdcmi;
	struct video_format fmt;
	int capture_rate;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct video_buffer *vbuf;
};

struct video_stm32_dcmi_config {
	struct stm32_pclken pclken;
	irq_config_func_t irq_config;
	const struct pinctrl_dev_config *pctrl;
	const struct device *sensor_dev;
	const struct stream dma;
};

void HAL_DCMI_ErrorCallback(DCMI_HandleTypeDef *hdcmi)
{
	LOG_WRN("%s", __func__);
}

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
	struct video_stm32_dcmi_data *dev_data =
			CONTAINER_OF(hdcmi, struct video_stm32_dcmi_data, hdcmi);
	struct video_buffer *vbuf;

	HAL_DCMI_Suspend(hdcmi);

	vbuf = k_fifo_get(&dev_data->fifo_in, K_NO_WAIT);

	if (vbuf == NULL) {
		LOG_DBG("Failed to get buffer from fifo");
		goto resume;
	}

	vbuf->timestamp = k_uptime_get_32();
	memcpy(vbuf->buffer, dev_data->vbuf->buffer, vbuf->bytesused);

	k_fifo_put(&dev_data->fifo_out, vbuf);

resume:
	HAL_DCMI_Resume(hdcmi);
}

static void stm32_dcmi_isr(const struct device *dev)
{
	struct video_stm32_dcmi_data *data = dev->data;

	HAL_DCMI_IRQHandler(&data->hdcmi);
}

static void dcmi_dma_callback(const struct device *dev, void *arg, uint32_t channel, int status)
{
	DMA_HandleTypeDef *hdma = arg;

	ARG_UNUSED(dev);

	if (status < 0) {
		LOG_ERR("DMA callback error with channel %d.", channel);
	}

	HAL_DMA_IRQHandler(hdma);
}

void HAL_DMA_ErrorCallback(DMA_HandleTypeDef *hdma)
{
	LOG_WRN("%s", __func__);
}

static int stm32_dma_init(const struct device *dev)
{
	struct video_stm32_dcmi_data *data = dev->data;
	const struct video_stm32_dcmi_config *config = dev->config;
	int ret;

	/* Check if the DMA device is ready */
	if (!device_is_ready(config->dma.dma_dev)) {
		LOG_ERR("%s DMA device not ready", config->dma.dma_dev->name);
		return -ENODEV;
	}

	/*
	 * DMA configuration
	 * Due to use of DMA HAL API in current driver,
	 * both HAL and Zephyr DMA drivers should be configured.
	 * The required configuration for Zephyr DMA driver should only provide
	 * the minimum information to inform the DMA slot will be in used and
	 * how to route callbacks.
	 */
	struct dma_config dma_cfg = config->dma.cfg;
	static DMA_HandleTypeDef hdma;

	/* Proceed to the minimum Zephyr DMA driver init */
	dma_cfg.user_data = &hdma;
	/* HACK: This field is used to inform driver that it is overridden */
	dma_cfg.linked_channel = STM32_DMA_HAL_OVERRIDE;
	ret = dma_config(config->dma.dma_dev, config->dma.channel, &dma_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure DMA channel %d", config->dma.channel);
		return ret;
	}

	/*** Configure the DMA ***/
	/* Set the parameters to be configured */
	hdma.Init.Request		= DMA_REQUEST_DCMI;
	hdma.Init.Direction		= DMA_PERIPH_TO_MEMORY;
	hdma.Init.PeriphInc		= DMA_PINC_DISABLE;
	hdma.Init.MemInc		= DMA_MINC_ENABLE;
	hdma.Init.PeriphDataAlignment	= DMA_PDATAALIGN_WORD;
	hdma.Init.MemDataAlignment	= DMA_MDATAALIGN_WORD;
	hdma.Init.Mode			= DMA_CIRCULAR;
	hdma.Init.Priority		= DMA_PRIORITY_HIGH;
	hdma.Instance			= STM32_DMA_GET_INSTANCE(config->dma.reg,
								 config->dma.channel);
#if defined(CONFIG_SOC_SERIES_STM32F7X) || defined(CONFIG_SOC_SERIES_STM32H7X)
	hdma.Init.FIFOMode		= DMA_FIFOMODE_DISABLE;
#endif

	/* Initialize DMA HAL */
	__HAL_LINKDMA(&data->hdcmi, DMA_Handle, hdma);

	if (HAL_DMA_Init(&hdma) != HAL_OK) {
		LOG_ERR("DCMI DMA Init failed");
		return -EIO;
	}

	return 0;
}

static int stm32_dcmi_enable_clock(const struct device *dev)
{
	const struct video_stm32_dcmi_config *config = dev->config;
	const struct device *dcmi_clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(dcmi_clock)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Turn on DCMI peripheral clock */
	return clock_control_on(dcmi_clock, (clock_control_subsys_t *)&config->pclken);
}

static int video_stm32_dcmi_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct video_stm32_dcmi_config *config = dev->config;
	struct video_stm32_dcmi_data *data = dev->data;
	int ret;

	ret = video_set_format(config->sensor_dev, fmt);
	if (ret < 0) {
		return ret;
	}

	fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;

	data->fmt = *fmt;

	return 0;
}

static int video_stm32_dcmi_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct video_stm32_dcmi_data *data = dev->data;
	const struct video_stm32_dcmi_config *config = dev->config;
	int ret;

	/* Align DCMI format with the one provided by the sensor */
	ret = video_get_format(config->sensor_dev, fmt);
	if (ret < 0) {
		return ret;
	}

	fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;

	data->fmt = *fmt;

	return 0;
}

#define STM32_DCMI_GET_CAPTURE_RATE(capture_rate)					\
	((capture_rate) == 1 ? DCMI_CR_ALL_FRAME :					\
	(capture_rate) == 2 ? DCMI_CR_ALTERNATE_2_FRAME :				\
	(capture_rate) == 4 ? DCMI_CR_ALTERNATE_4_FRAME :				\
	DCMI_CR_ALL_FRAME)

static int video_stm32_dcmi_set_stream(const struct device *dev, bool enable,
				       enum video_buf_type type)
{
	struct video_stm32_dcmi_data *data = dev->data;
	const struct video_stm32_dcmi_config *config = dev->config;
	int err;

	if (!enable) {
		err = video_stream_stop(config->sensor_dev, type);
		if (err < 0) {
			return err;
		}

		err = HAL_DCMI_Stop(&data->hdcmi);
		if (err != HAL_OK) {
			LOG_ERR("Failed to stop DCMI");
			return -EIO;
		}

		/* Release the video buffer allocated when start streaming */
		k_fifo_put(&data->fifo_in, data->vbuf);

		return 0;
	}

	data->vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);

	if (data->vbuf == NULL) {
		LOG_ERR("Failed to dequeue a DCMI buffer.");
		return -ENOMEM;
	}

	/* Set the frame control */
	data->hdcmi.Instance->CR &= ~(DCMI_CR_FCRC_0 | DCMI_CR_FCRC_1);
	data->hdcmi.Instance->CR |= STM32_DCMI_GET_CAPTURE_RATE(data->capture_rate);

	err = HAL_DCMI_Start_DMA(&data->hdcmi, DCMI_MODE_CONTINUOUS,
			(uint32_t)data->vbuf->buffer, data->vbuf->bytesused / 4);
	if (err != HAL_OK) {
		LOG_ERR("Failed to start DCMI DMA");
		return -EIO;
	}

	return video_stream_start(config->sensor_dev, type);
}

static int video_stm32_dcmi_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct video_stm32_dcmi_data *data = dev->data;
	const uint32_t buffer_size = data->fmt.pitch * data->fmt.height;

	if (buffer_size > vbuf->size) {
		return -EINVAL;
	}

	vbuf->bytesused = buffer_size;
	vbuf->line_offset = 0;

	k_fifo_put(&data->fifo_in, vbuf);

	return 0;
}

static int video_stm32_dcmi_dequeue(const struct device *dev, struct video_buffer **vbuf,
				    k_timeout_t timeout)
{
	struct video_stm32_dcmi_data *data = dev->data;

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int video_stm32_dcmi_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct video_stm32_dcmi_config *config = dev->config;

	/* DCMI produces full frames */
	caps->min_line_count = caps->max_line_count = LINE_COUNT_HEIGHT;

	/* Forward the message to the sensor device */
	return video_get_caps(config->sensor_dev, caps);
}

static int video_stm32_dcmi_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct video_stm32_dcmi_config *config = dev->config;
	int ret;

	ret = video_enum_frmival(config->sensor_dev, fie);
	if (ret < 0) {
		return ret;
	}

	/* Adapt the interval in order to report the frame drop capabilities */
	if (fie->type == VIDEO_FRMIVAL_TYPE_DISCRETE) {
		struct video_frmival discrete = fie->discrete;

		fie->type = VIDEO_FRMIVAL_TYPE_STEPWISE;
		fie->stepwise.max = discrete;
		fie->stepwise.min.denominator = discrete.denominator;
		fie->stepwise.min.numerator = discrete.numerator * 4;
		fie->stepwise.step.denominator = discrete.denominator;
		fie->stepwise.step.numerator = discrete.numerator * 2;
	} else {
		fie->stepwise.min.numerator *= 4;
		fie->stepwise.step.numerator *= 2;
	}

	return 0;
}

#define STM32_DCMI_MAX_FRAME_DROP	4
static int video_stm32_dcmi_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_stm32_dcmi_config *config = dev->config;
	struct video_stm32_dcmi_data *data = dev->data;
	struct video_frmival_enum fie = {
		.format = &data->fmt,
	};
	struct video_frmival best_sensor_frmival;
	uint64_t best_diff_nsec = INT32_MAX;
	uint64_t diff_nsec = 0, a, b;
	int best_capture_rate = 1;

	/*
	 * Try to figure out a frameinterval setting allow to reach as close as
	 * possible to the request. At first without relying on DCMI frame control,
	 * then enabling it
	 */
	for (int capture_rate = 1; capture_rate <= STM32_DCMI_MAX_FRAME_DROP; capture_rate *= 2) {
		/*
		 * Take into consideration the drop done by the DCMI hence multiply
		 * denominator by the rate introduced by the DCMI
		 */
		fie.discrete.numerator = frmival->numerator;
		fie.discrete.denominator = frmival->denominator * capture_rate;

		a = video_frmival_nsec(&fie.discrete);
		video_closest_frmival(config->sensor_dev, &fie);
		b = video_frmival_nsec(&fie.discrete);
		diff_nsec = a > b ? a - b : b - a;
		if (diff_nsec < best_diff_nsec) {
			best_diff_nsec = diff_nsec;
			best_sensor_frmival = fie.discrete;
			best_capture_rate = capture_rate;
		}
		if (diff_nsec == 0) {
			break;
		}
	}

	/*
	 * Give back the achieved frame interval achieved, ensuring to take into
	 * consideration the DCMI frame control
	 */
	frmival->numerator = best_sensor_frmival.numerator * best_capture_rate;
	frmival->denominator = best_sensor_frmival.denominator;

	data->capture_rate = best_capture_rate;

	return video_set_frmival(config->sensor_dev, &best_sensor_frmival);
}

static int video_stm32_dcmi_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_stm32_dcmi_config *config = dev->config;
	struct video_stm32_dcmi_data *data = dev->data;
	int ret;

	ret = video_get_frmival(config->sensor_dev, frmival);
	if (ret < 0) {
		return ret;
	}

	frmival->numerator *= data->capture_rate;

	return 0;
}

static DEVICE_API(video, video_stm32_dcmi_driver_api) = {
	.set_format = video_stm32_dcmi_set_fmt,
	.get_format = video_stm32_dcmi_get_fmt,
	.set_stream = video_stm32_dcmi_set_stream,
	.enqueue = video_stm32_dcmi_enqueue,
	.dequeue = video_stm32_dcmi_dequeue,
	.get_caps = video_stm32_dcmi_get_caps,
	.enum_frmival = video_stm32_dcmi_enum_frmival,
	.set_frmival = video_stm32_dcmi_set_frmival,
	.get_frmival = video_stm32_dcmi_get_frmival,
};

static void video_stm32_dcmi_irq_config_func(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		stm32_dcmi_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

#define DCMI_DMA_CHANNEL_INIT(index, src_dev, dest_dev)					\
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_IDX(index, 0)),			\
	.channel = DT_INST_DMAS_CELL_BY_IDX(index, 0, channel),				\
	.reg = (DMA_TypeDef *)DT_REG_ADDR(						\
				DT_PHANDLE_BY_IDX(DT_DRV_INST(0), dmas, 0)),		\
	.cfg = {									\
		.dma_slot = STM32_DMA_SLOT_BY_IDX(index, 0, slot),			\
		.channel_direction = STM32_DMA_CONFIG_DIRECTION(			\
			STM32_DMA_CHANNEL_CONFIG_BY_IDX(index, 0)),			\
		.source_data_size = STM32_DMA_CONFIG_##src_dev##_DATA_SIZE(		\
			STM32_DMA_CHANNEL_CONFIG_BY_IDX(index, 0)),			\
		.dest_data_size = STM32_DMA_CONFIG_##dest_dev##_DATA_SIZE(		\
			STM32_DMA_CHANNEL_CONFIG_BY_IDX(index, 0)),			\
		.source_burst_length = 1,       /* SINGLE transfer */			\
		.dest_burst_length = 1,         /* SINGLE transfer */			\
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(				\
			STM32_DMA_CHANNEL_CONFIG_BY_IDX(index, 0)),			\
		.dma_callback = dcmi_dma_callback,					\
	},										\

PINCTRL_DT_INST_DEFINE(0);

#define STM32_DCMI_GET_BUS_WIDTH(bus_width)						\
	((bus_width) == 8 ? DCMI_EXTEND_DATA_8B :					\
	(bus_width) == 10 ? DCMI_EXTEND_DATA_10B :					\
	(bus_width) == 12 ? DCMI_EXTEND_DATA_12B :					\
	(bus_width) == 14 ? DCMI_EXTEND_DATA_14B :					\
	DCMI_EXTEND_DATA_8B)

#define DCMI_DMA_CHANNEL(id, src, dest)							\
	.dma = {									\
		COND_CODE_1(DT_INST_DMAS_HAS_IDX(id, 0),				\
			(DCMI_DMA_CHANNEL_INIT(id, src, dest)),				\
			(NULL))								\
	},

static struct video_stm32_dcmi_data video_stm32_dcmi_data_0 = {
	.hdcmi = {
		.Instance = (DCMI_TypeDef *) DT_INST_REG_ADDR(0),
		.Init = {
				.SynchroMode = DCMI_SYNCHRO_HARDWARE,
				.PCKPolarity = DT_PROP_OR(DT_INST_ENDPOINT_BY_ID(0, 0, 0),
							  pclk_sample, 0) ?
							  DCMI_PCKPOLARITY_RISING :
							  DCMI_PCKPOLARITY_FALLING,
				.HSPolarity = DT_PROP_OR(DT_INST_ENDPOINT_BY_ID(0, 0, 0),
							 hsync_active, 0) ?
							 DCMI_HSPOLARITY_HIGH : DCMI_HSPOLARITY_LOW,
				.VSPolarity = DT_PROP_OR(DT_INST_ENDPOINT_BY_ID(0, 0, 0),
							 vsync_active, 0) ?
							 DCMI_VSPOLARITY_HIGH : DCMI_VSPOLARITY_LOW,
				.ExtendedDataMode = STM32_DCMI_GET_BUS_WIDTH(
							DT_PROP_OR(DT_INST_ENDPOINT_BY_ID(0, 0, 0),
								   bus_width, 8)),
				.JPEGMode = DCMI_JPEG_DISABLE,
				.ByteSelectMode = DCMI_BSM_ALL,
				.ByteSelectStart = DCMI_OEBS_ODD,
				.LineSelectMode = DCMI_LSM_ALL,
				.LineSelectStart = DCMI_OELS_ODD,
		},
	},
};

#define SOURCE_DEV(n) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(n, 0, 0)))

static const struct video_stm32_dcmi_config video_stm32_dcmi_config_0 = {
	.pclken = {
		.enr = DT_INST_CLOCKS_CELL(0, bits),
		.bus = DT_INST_CLOCKS_CELL(0, bus)
	},
	.irq_config = video_stm32_dcmi_irq_config_func,
	.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.sensor_dev = SOURCE_DEV(0),
	DCMI_DMA_CHANNEL(0, PERIPHERAL, MEMORY)
};

static int video_stm32_dcmi_init(const struct device *dev)
{
	const struct video_stm32_dcmi_config *config = dev->config;
	struct video_stm32_dcmi_data *data = dev->data;
	int err;

	/* Configure DT provided pins */
	err = pinctrl_apply_state(config->pctrl, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("pinctrl setup failed. Error %d.", err);
		return err;
	}

	/* Initialize DMA peripheral */
	err = stm32_dma_init(dev);
	if (err < 0) {
		LOG_ERR("DMA initialization failed.");
		return err;
	}

	/* Enable DCMI clock */
	err = stm32_dcmi_enable_clock(dev);
	if (err < 0) {
		LOG_ERR("Clock enabling failed.");
		return err;
	}

	data->dev = dev;
	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);
	data->capture_rate = 1;

	/* Run IRQ init */
	config->irq_config(dev);

	/* Initialize DCMI peripheral */
	err = HAL_DCMI_Init(&data->hdcmi);
	if (err != HAL_OK) {
		LOG_ERR("DCMI initialization failed.");
		return -EIO;
	}

	k_sleep(K_MSEC(100));
	LOG_DBG("%s inited", dev->name);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, &video_stm32_dcmi_init,
		    NULL, &video_stm32_dcmi_data_0,
		    &video_stm32_dcmi_config_0,
		    POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,
		    &video_stm32_dcmi_driver_api);

VIDEO_DEVICE_DEFINE(dcmi, DEVICE_DT_INST_GET(0), SOURCE_DEV(0));
