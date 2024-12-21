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

LOG_MODULE_REGISTER(video_stm32_dcmi, CONFIG_VIDEO_LOG_LEVEL);

K_HEAP_DEFINE(video_stm32_buffer_pool, CONFIG_VIDEO_BUFFER_POOL_SZ_MAX);

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
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	uint32_t pixel_format;
	uint32_t height;
	uint32_t width;
	uint32_t pitch;
	uint8_t *buffer;
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
	memcpy(vbuf->buffer, dev_data->buffer, vbuf->bytesused);

	k_fifo_put(&dev_data->fifo_out, vbuf);

resume:
	HAL_DCMI_Resume(hdcmi);
}

static void stm32_dcmi_isr(const struct device *dev)
{
	struct video_stm32_dcmi_data *data = dev->data;

	HAL_DCMI_IRQHandler(&data->hdcmi);
}

static void dmci_dma_callback(const struct device *dev, void *arg,
			 uint32_t channel, int status)
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
	 * Due to use of QSPI HAL API in current driver,
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
	/* Because of the STREAM OFFSET, the DMA channel given here is from 1 - 8 */
	ret = dma_config(config->dma.dma_dev,
			config->dma.channel + STM32_DMA_STREAM_OFFSET, &dma_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure DMA channel %d",
			config->dma.channel + STM32_DMA_STREAM_OFFSET);
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
	hdma.Init.FIFOMode		= DMA_FIFOMODE_DISABLE;

	hdma.Instance = __LL_DMA_GET_STREAM_INSTANCE(config->dma.reg,
						config->dma.channel);

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
	int err;

	if (!device_is_ready(dcmi_clock)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Turn on DCMI peripheral clock */
	err = clock_control_on(dcmi_clock, (clock_control_subsys_t *) &config->pclken);
	if (err < 0) {
		LOG_ERR("Failed to enable DCMI clock. Error %d", err);
		return err;
	}

	return 0;
}

static int video_stm32_dcmi_set_fmt(const struct device *dev,
				  enum video_endpoint_id ep,
				  struct video_format *fmt)
{
	const struct video_stm32_dcmi_config *config = dev->config;
	struct video_stm32_dcmi_data *data = dev->data;
	unsigned int bpp = video_pix_fmt_bpp(fmt->pixelformat);

	if (bpp == 0 || (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL)) {
		return -EINVAL;
	}

	data->pixel_format = fmt->pixelformat;
	data->pitch = fmt->pitch;
	data->height = fmt->height;
	data->width = fmt->width;

	if (video_set_format(config->sensor_dev, ep, fmt)) {
		return -EIO;
	}

	return 0;
}

static int video_stm32_dcmi_get_fmt(const struct device *dev,
				  enum video_endpoint_id ep,
				  struct video_format *fmt)
{
	struct video_stm32_dcmi_data *data = dev->data;
	const struct video_stm32_dcmi_config *config = dev->config;

	if (fmt == NULL || (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL)) {
		return -EINVAL;
	}

	if (!video_get_format(config->sensor_dev, ep, fmt)) {
		/* align DCMI with sensor fmt */
		return video_stm32_dcmi_set_fmt(dev, ep, fmt);
	}

	fmt->pixelformat = data->pixel_format;
	fmt->height = data->height;
	fmt->width = data->width;
	fmt->pitch = data->pitch;

	return 0;
}

static int video_stm32_dcmi_stream_start(const struct device *dev)
{
	struct video_stm32_dcmi_data *data = dev->data;
	const struct video_stm32_dcmi_config *config = dev->config;
	size_t buffer_size = data->pitch * data->height;

	data->buffer = k_heap_alloc(&video_stm32_buffer_pool, buffer_size, K_NO_WAIT);
	if (data->buffer == NULL) {
		LOG_ERR("Failed to allocate DCMI buffer for image. Size %d bytes", buffer_size);
		return -ENOMEM;
	}

	int err = HAL_DCMI_Start_DMA(&data->hdcmi, DCMI_MODE_CONTINUOUS,
			(uint32_t)data->buffer, buffer_size / 4);
	if (err != HAL_OK) {
		LOG_ERR("Failed to start DCMI DMA");
		return -EIO;
	}

	if (video_stream_start(config->sensor_dev)) {
		return -EIO;
	}

	return 0;
}

static int video_stm32_dcmi_stream_stop(const struct device *dev)
{
	struct video_stm32_dcmi_data *data = dev->data;
	const struct video_stm32_dcmi_config *config = dev->config;
	int err;

	if (video_stream_stop(config->sensor_dev)) {
		return -EIO;
	}

	/* Release the buffer allocated in stream_start */
	k_heap_free(&video_stm32_buffer_pool, data->buffer);

	err = HAL_DCMI_Stop(&data->hdcmi);
	if (err != HAL_OK) {
		LOG_ERR("Failed to stop DCMI");
		return -EIO;
	}

	return 0;
}

static int video_stm32_dcmi_enqueue(const struct device *dev,
				  enum video_endpoint_id ep,
				  struct video_buffer *vbuf)
{
	struct video_stm32_dcmi_data *data = dev->data;
	const uint32_t buffer_size = data->pitch * data->height;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	if (buffer_size > vbuf->size) {
		return -EINVAL;
	}

	vbuf->bytesused = buffer_size;
	vbuf->line_offset = 0;

	k_fifo_put(&data->fifo_in, vbuf);

	return 0;
}

static int video_stm32_dcmi_dequeue(const struct device *dev,
				  enum video_endpoint_id ep,
				  struct video_buffer **vbuf,
				  k_timeout_t timeout)
{
	struct video_stm32_dcmi_data *data = dev->data;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int video_stm32_dcmi_get_caps(const struct device *dev,
				   enum video_endpoint_id ep,
				   struct video_caps *caps)
{
	const struct video_stm32_dcmi_config *config = dev->config;
	int ret = -ENODEV;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	/* DCMI produces full frames */
	caps->min_line_count = caps->max_line_count = LINE_COUNT_HEIGHT;

	/* Forward the message to the sensor device */
	ret = video_get_caps(config->sensor_dev, ep, caps);

	return ret;
}

static inline int video_stm32_dcmi_set_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	const struct video_stm32_dcmi_config *config = dev->config;
	int ret;

	/* Forward to source dev if any */
	ret = video_set_ctrl(config->sensor_dev, cid, value);

	return ret;
}

static inline int video_stm32_dcmi_get_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	const struct video_stm32_dcmi_config *config = dev->config;
	int ret;

	/* Forward to source dev if any */
	ret = video_get_ctrl(config->sensor_dev, cid, value);

	return ret;
}

static const struct video_driver_api video_stm32_dcmi_driver_api = {
	.set_format = video_stm32_dcmi_set_fmt,
	.get_format = video_stm32_dcmi_get_fmt,
	.stream_start = video_stm32_dcmi_stream_start,
	.stream_stop = video_stm32_dcmi_stream_stop,
	.enqueue = video_stm32_dcmi_enqueue,
	.dequeue = video_stm32_dcmi_dequeue,
	.get_caps = video_stm32_dcmi_get_caps,
	.set_ctrl = video_stm32_dcmi_set_ctrl,
	.get_ctrl = video_stm32_dcmi_get_ctrl,
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
		.dma_callback = dmci_dma_callback,					\
	},										\

PINCTRL_DT_INST_DEFINE(0);

#define STM32_DCMI_GET_CAPTURE_RATE(capture_rate)					\
	((capture_rate) == 1 ? DCMI_CR_ALL_FRAME :					\
	(capture_rate) == 2 ? DCMI_CR_ALTERNATE_2_FRAME :				\
	(capture_rate) == 4 ? DCMI_CR_ALTERNATE_4_FRAME :				\
	DCMI_CR_ALL_FRAME)

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
				.PCKPolarity = (DT_INST_PROP(0, pixelclk_active) ?
						DCMI_PCKPOLARITY_RISING : DCMI_PCKPOLARITY_FALLING),
				.HSPolarity = (DT_INST_PROP(0, hsync_active) ?
						DCMI_HSPOLARITY_HIGH : DCMI_HSPOLARITY_LOW),
				.VSPolarity = (DT_INST_PROP(0, vsync_active) ?
						DCMI_VSPOLARITY_HIGH : DCMI_VSPOLARITY_LOW),
				.CaptureRate = STM32_DCMI_GET_CAPTURE_RATE(
							DT_INST_PROP(0, capture_rate)),
				.ExtendedDataMode = STM32_DCMI_GET_BUS_WIDTH(
							DT_INST_PROP(0, bus_width)),
				.JPEGMode = DCMI_JPEG_DISABLE,
				.ByteSelectMode = DCMI_BSM_ALL,
				.ByteSelectStart = DCMI_OEBS_ODD,
				.LineSelectMode = DCMI_LSM_ALL,
				.LineSelectStart = DCMI_OELS_ODD,
		},
	},
};

static const struct video_stm32_dcmi_config video_stm32_dcmi_config_0 = {
	.pclken = {
		.enr = DT_INST_CLOCKS_CELL(0, bits),
		.bus = DT_INST_CLOCKS_CELL(0, bus)
	},
	.irq_config = video_stm32_dcmi_irq_config_func,
	.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.sensor_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, sensor)),
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
		return -EIO;
	}

	data->dev = dev;
	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);

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
