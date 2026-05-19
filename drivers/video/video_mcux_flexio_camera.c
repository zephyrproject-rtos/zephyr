/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_flexio_camera

#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/misc/nxp_flexio/nxp_flexio.h>
#include <zephyr/logging/log.h>

#include <fsl_flexio.h>
#include <fsl_flexio_camera.h>

#include "video_device.h"

#define LOG_LEVEL CONFIG_VIDEO_LOG_LEVEL
LOG_MODULE_REGISTER(nxp_flexio_camera);

/* Sentinel value meaning "not configured" for optional uint32_t pin fields */
#define FLEXIO_PIN_NOT_USED 0xFFU

struct nxp_flexio_camera_config {
	/* FlexIO parent device (nxp,flexio node) */
	const struct device *flexio_dev;
	/* FlexIO peripheral register block — DT_REG_ADDR(parent) */
	FLEXIO_Type *flexio_base;
	/* Connected sensor device via video-interfaces endpoint */
	const struct device *source_dev;
	/* FlexIO child resource descriptor */
	const struct nxp_flexio_child *child;
	/* Pinctrl configuration */
	const struct pinctrl_dev_config *pincfg;
	/* VSYNC GPIO */
	struct gpio_dt_spec vsync_gpio;
	/* eDMA device and channel */
	const struct device *dma_dev;
	uint32_t dma_channel;
	/* eDMA hardware request/mux source for FlexIO shifter */
	uint32_t dma_slot;
	/* FlexIO pin indices (physical FlexIO pad numbers) */
	uint32_t data_pin_start;
	uint32_t pclk_pin;
	uint32_t href_pin;
	uint32_t xclk_pin;
	uint32_t xclk_frequency;
	/* Number of shifters to use as FIFO.  */
	uint8_t shifter_count;
	/* Index of the last shifter used as the FIFO. */
	uint8_t shifter_end_index;
	/* FlexIO timer indices */
	uint8_t xclk_timer;
	uint8_t pclk_timer;
};

struct nxp_flexio_camera_data {
	/* Back-pointer needed in callbacks */
	const struct device *dev;
	/* SDK camera handle — shifterStartIdx/timerIdx filled after child_attach */
	FLEXIO_CAMERA_Type flexio_cam;
	/* Current pixel format */
	struct video_format fmt;
	/* Buffer queues. Head of fifo_in is the buffer currently being filled
	 * by DMA; it is removed and pushed to fifo_out once the last line completes.
	 */
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	/* Semaphore given when stream runs out of buffers */
	struct k_sem stream_empty;
	bool streaming;
	/* Current line index within the active buffer (0..fmt.height-1) */
	uint32_t current_line;
	/* Pre-built DMA config — dest_address and block_size are updated per line */
	struct dma_config dma_cfg;
	struct dma_block_config dma_block;
	/* VSYNC GPIO callback */
	struct gpio_callback vsync_cb;
};

/*
 * XCLK timer helper
 * Configure a FlexIO timer as Dual-8-bit PWM to generate the camera XCLK.
 * Called from init(), after nxp_flexio_child_attach() has filled timer_index[].
 */
static int nxp_flexio_camera_configure_xclk(const struct device *dev)
{
	const struct nxp_flexio_camera_config *config = dev->config;
	flexio_timer_config_t timer_cfg;
	uint32_t flexio_clk;
	uint32_t sum, half;
	int ret;

	if (config->xclk_pin == FLEXIO_PIN_NOT_USED) {
		return 0;
	}

	ret = nxp_flexio_get_rate(config->flexio_dev, &flexio_clk);
	if (ret < 0) {
		LOG_ERR("Cannot read FlexIO clock rate: %d", ret);
		return ret;
	}

	/* Dual-8-bit PWM: timerCompare = ((highPeriod-1) << 8) | (lowPeriod-1).
	 * Use rounding: sum = round(flexio_clk / xclk_frequency).
	 */
	sum  = (flexio_clk * 2U / config->xclk_frequency + 1U) / 2U;
	half = sum / 2U;

	if (half < 1U) {
		LOG_ERR("XCLK frequency %u Hz too high for FlexIO clock %u Hz",
			config->xclk_frequency, flexio_clk);
		return -EINVAL;
	}

	memset(&timer_cfg, 0, sizeof(timer_cfg));
	timer_cfg.triggerSelect   = 0U; /* Do not care. */
	timer_cfg.triggerSource   = kFLEXIO_TimerTriggerSourceInternal;
	timer_cfg.triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveLow;
	timer_cfg.pinConfig	  = kFLEXIO_PinConfigOutput;
	timer_cfg.pinPolarity     = kFLEXIO_PinActiveHigh;
	timer_cfg.pinSelect	  = config->xclk_pin;
	timer_cfg.timerMode	  = kFLEXIO_TimerModeDual8BitPWM;
	timer_cfg.timerOutput     = kFLEXIO_TimerOutputOneNotAffectedByReset;
	timer_cfg.timerDecrement  = kFLEXIO_TimerDecSrcOnFlexIOClockShiftTimerOutput;
	timer_cfg.timerReset      = kFLEXIO_TimerResetNever;
	timer_cfg.timerDisable    = kFLEXIO_TimerDisableNever;
	timer_cfg.timerEnable     = kFLEXIO_TimerEnabledAlways;
	timer_cfg.timerStop	  = kFLEXIO_TimerStopBitDisabled;
	timer_cfg.timerStart      = kFLEXIO_TimerStartBitDisabled;
	/* upper byte = high-period count - 1, lower byte = low-period count - 1 */
	timer_cfg.timerCompare    = (((half - 1U) & 0xFFU) << 8U) | ((half - 1U) & 0xFFU);

	FLEXIO_SetTimerConfig(config->flexio_base, config->xclk_timer, &timer_cfg);

	return 0;
}

/*
 * Configure and kick off DMA for one line of the active buffer.
 * The FlexIO PCLK-capture timer is gated by HREF, so the shifter only
 * generates DMA requests while HREF is high — i.e. exactly one line of
 * pitch bytes will be transferred before the line goes idle.
 */
static void nxp_flexio_camera_start_line_dma(struct nxp_flexio_camera_data *data)
{
	const struct nxp_flexio_camera_config *config = data->dev->config;
	struct video_buffer *vbuf = k_fifo_peek_head(&data->fifo_in);

	if (vbuf == NULL) {
		return;
	}

	data->dma_block.dest_address = (uint32_t)vbuf->buffer +
					data->current_line * data->fmt.pitch;

	if (dma_config(config->dma_dev, config->dma_channel, &data->dma_cfg) != 0) {
		LOG_ERR("DMA reconfigure failed");
		return;
	}

	FLEXIO_CAMERA_ClearStatusFlags(&data->flexio_cam,
		kFLEXIO_CAMERA_RxDataRegFullFlag | kFLEXIO_CAMERA_RxErrorFlag);

	dma_start(config->dma_dev, config->dma_channel);
	FLEXIO_CAMERA_EnableRxDMA(&data->flexio_cam, true);
}

/*
 * DMA callback — fired once per line. After the final line, the completed buffer is
 * published to fifo_out and we wait for the next VSYNC to start a new frame.
 */
static void nxp_flexio_camera_dma_callback(const struct device *dma_dev,
					   void *user_data,
					   uint32_t channel,
					   int status)
{
	struct nxp_flexio_camera_data *data = user_data;
	struct video_buffer *vbuf;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	FLEXIO_CAMERA_EnableRxDMA(&data->flexio_cam, false);

	if (status < 0) {
		LOG_ERR("DMA transfer error: %d", status);
		return;
	}

	if (!data->streaming) {
		return;
	}

	data->current_line++;

	if (data->current_line < data->fmt.height) {
		nxp_flexio_camera_start_line_dma(data);
		return;
	}

	/* Last line completed — pop the active buffer and publish the frame */
	vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (vbuf == NULL) {
		return;
	}
	vbuf->timestamp   = k_uptime_get_32();
	vbuf->bytesused   = data->fmt.size;
	vbuf->line_offset = 0U;

	k_fifo_put(&data->fifo_out, vbuf);
}

/* VSYNC GPIO interrupt — signals the application that a new frame began. */
static void nxp_flexio_camera_vsync_isr(const struct device *port,
					struct gpio_callback *cb,
					uint32_t pins)
{
	struct nxp_flexio_camera_data *data =
		CONTAINER_OF(cb, struct nxp_flexio_camera_data, vsync_cb);
	struct video_buffer *vbuf;

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	if (!data->streaming) {
		return;
	}

	vbuf = k_fifo_peek_head(&data->fifo_in);
	if (vbuf == NULL) {
		k_sem_give(&data->stream_empty);
		return;
	}

	data->current_line = 0U;
	data->dma_block.block_size = data->fmt.pitch;

	nxp_flexio_camera_start_line_dma(data);
}

static int nxp_flexio_camera_set_format(const struct device *dev,
					struct video_format *fmt)
{
	const struct nxp_flexio_camera_config *config = dev->config;
	struct nxp_flexio_camera_data *data = dev->data;
	flexio_camera_config_t cam_cfg;
	int ret;

	if (!device_is_ready(config->source_dev)) {
		LOG_ERR("Source device not ready");
		return -ENODEV;
	}

	/* Propagate format to sensor */
	ret = video_set_format(config->source_dev, fmt);
	if (ret < 0) {
		return ret;
	}

	/* Build FLEXIO_CAMERA_Type using dynamically allocated indices */
	data->flexio_cam.flexioBase      = config->flexio_base;
	data->flexio_cam.datPinStartIdx  = config->data_pin_start;
	data->flexio_cam.pclkPinIdx      = config->pclk_pin;
	data->flexio_cam.hrefPinIdx      = config->href_pin;
	data->flexio_cam.shifterStartIdx = config->shifter_end_index + 1U - config->shifter_count;
	data->flexio_cam.shifterCount    = config->shifter_count;
	data->flexio_cam.timerIdx        = config->pclk_timer;

	FLEXIO_CAMERA_GetDefaultConfig(&cam_cfg);
	cam_cfg.enablecamera     = false; /* enabled in set_stream */
	cam_cfg.enableInDebug    = false;
	cam_cfg.enableFastAccess = false;

	nxp_flexio_lock(config->flexio_dev);
	FLEXIO_CAMERA_Init(&data->flexio_cam, &cam_cfg);
	nxp_flexio_unlock(config->flexio_dev);

	/* Compute pitch and full-frame size */
	fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;
	fmt->size  = fmt->pitch * fmt->height;
	data->fmt  = *fmt;

	/* Pre-build DMA config. dest_address and block_size are updated per line
	 * in nxp_flexio_camera_start_line_dma().
	 */
	memset(&data->dma_cfg, 0, sizeof(data->dma_cfg));
	memset(&data->dma_block, 0, sizeof(data->dma_block));

	data->dma_cfg.channel_direction   = PERIPHERAL_TO_MEMORY;
	data->dma_cfg.source_data_size    = config->shifter_count * sizeof(uint32_t);
	data->dma_cfg.dest_data_size      = 1U;
	data->dma_cfg.source_burst_length = config->shifter_count * sizeof(uint32_t);
	data->dma_cfg.block_count         = 1U;
	data->dma_cfg.head_block          = &data->dma_block;
	data->dma_cfg.user_data           = data;
	data->dma_cfg.dma_callback        = nxp_flexio_camera_dma_callback;
	data->dma_cfg.dma_slot            = config->dma_slot;
	data->dma_block.source_address    =
			FLEXIO_GetShifterBufferAddress(config->flexio_base, kFLEXIO_ShifterBuffer,
			data->flexio_cam.shifterStartIdx);
	data->dma_block.source_addr_adj   = DMA_ADDR_ADJ_NO_CHANGE;
	data->dma_block.dest_addr_adj     = DMA_ADDR_ADJ_INCREMENT;
	/* dest_address and block_size filled per-frame in VSYNC ISR */

	return 0;
}

static int nxp_flexio_camera_get_format(const struct device *dev,
					struct video_format *fmt)
{
	const struct nxp_flexio_camera_config *config = dev->config;
	struct nxp_flexio_camera_data *data = dev->data;

	if (!device_is_ready(config->source_dev)) {
		LOG_ERR("Source device not ready");
		return -ENODEV;
	}

	int ret = video_get_format(config->source_dev, fmt);

	if (ret < 0) {
		return ret;
	}

	fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;
	fmt->size  = fmt->pitch * fmt->height;
	data->fmt  = *fmt;

	return 0;
}

static int nxp_flexio_camera_get_caps(const struct device *dev,
				      struct video_caps *caps)
{
	const struct nxp_flexio_camera_config *config = dev->config;

	/* Need at least two buffers: one active in DMA, one queued for next frame */
	caps->min_vbuf_count = 2U;

	if (!device_is_ready(config->source_dev)) {
		return -ENODEV;
	}

	return video_get_caps(config->source_dev, caps);
}

static int nxp_flexio_camera_set_stream(const struct device *dev, bool enable,
					enum video_buf_type type)
{
	const struct nxp_flexio_camera_config *config = dev->config;
	struct nxp_flexio_camera_data *data = dev->data;
	int ret;

	if (enable) {
		/* Enable VSYNC GPIO edge interrupt — DMA starts on first VSYNC */
		ret = gpio_pin_interrupt_configure_dt(&config->vsync_gpio,
						      GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			FLEXIO_CAMERA_Enable(&data->flexio_cam, false);
			LOG_ERR("Failed to enable VSYNC interrupt: %d", ret);
			return ret;
		}

		/* Enable FLEXIO camera to start generating XCLK after enabling VSYNC */
		FLEXIO_CAMERA_Enable(&data->flexio_cam, true);

		/* Start the sensor last */
		ret = video_stream_start(config->source_dev, type);
		if (ret < 0) {
			FLEXIO_CAMERA_Enable(&data->flexio_cam, false);
			gpio_pin_interrupt_configure_dt(&config->vsync_gpio,
							GPIO_INT_DISABLE);
			return ret;
		}

		k_sem_reset(&data->stream_empty);
		data->streaming = true;
	} else {
		data->streaming = false;

		/* Stop sensor first so no new frames arrive */
		video_stream_stop(config->source_dev, type);

		gpio_pin_interrupt_configure_dt(&config->vsync_gpio, GPIO_INT_DISABLE);

		dma_stop(config->dma_dev, config->dma_channel);

		FLEXIO_CAMERA_EnableRxDMA(&data->flexio_cam, false);
		FLEXIO_CAMERA_Enable(&data->flexio_cam, false);
	}

	return 0;
}

static int nxp_flexio_camera_enqueue(const struct device *dev,
				     struct video_buffer *vbuf)
{
	struct nxp_flexio_camera_data *data = dev->data;

	if (vbuf->size < data->fmt.size) {
		return -EINVAL;
	}

	/* Clear any stale "stream drained" signal so a subsequent flush(cancel=false)
	 * properly waits for this new buffer to be filled instead of returning
	 * immediately on the leftover give() from a previous starvation.
	 */
	k_sem_reset(&data->stream_empty);

	k_fifo_put(&data->fifo_in, vbuf);

	return 0;
}

static int nxp_flexio_camera_dequeue(const struct device *dev,
				     struct video_buffer **vbuf,
				     k_timeout_t timeout)
{
	struct nxp_flexio_camera_data *data = dev->data;

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int nxp_flexio_camera_flush(const struct device *dev, bool cancel)
{
	const struct nxp_flexio_camera_config *config = dev->config;
	struct nxp_flexio_camera_data *data = dev->data;
	struct video_buffer *vbuf;

	if (!cancel) {
		/* Wait until the DMA engine drains the last queued buffer */
		k_sem_take(&data->stream_empty, K_FOREVER);
	} else {
		dma_stop(config->dma_dev, config->dma_channel);
		/* Forward all pending input buffers to the output queue */
		while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT)) != NULL) {
			k_fifo_put(&data->fifo_out, vbuf);
		}
	}

	return 0;
}

static int nxp_flexio_camera_init(const struct device *dev)
{
	const struct nxp_flexio_camera_config *config = dev->config;
	struct nxp_flexio_camera_data *data = dev->data;
	int ret;

	data->dev = dev;

	/* Validate shifter range: start index = end_index + 1 - count must not underflow.
	 * With enum: [1,2,4,8] on shifter-count and enum: [3,7] on shifter-end-index,
	 * the only invalid combination is count=8 with end_index=3.
	 */
	if (config->shifter_end_index + 1U < config->shifter_count) {
		LOG_ERR("Invalid shifter range: end_index=%u, count=%u",
			config->shifter_end_index, config->shifter_count);
		return -EINVAL;
	}

	if (!device_is_ready(config->flexio_dev)) {
		LOG_ERR("FlexIO parent not ready");
		return -ENODEV;
	}

	if (!device_is_ready(config->dma_dev)) {
		LOG_ERR("DMA device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->vsync_gpio)) {
		LOG_ERR("VSYNC GPIO not ready");
		return -ENODEV;
	}

	/* Allocate shifter and timer resources from the FlexIO parent.
	 * shifter_index[0..N-1] will be filled with contiguous indices
	 * (mcux_flexio assigns them sequentially from the free-list).
	 * timer_index[0] = PCLK capture timer, timer_index[1] = XCLK timer
	 * (only present when xclk-pin is specified in DT).
	 */
	ret = nxp_flexio_child_attach(config->flexio_dev, config->child);
	if (ret < 0) {
		LOG_ERR("FlexIO child attach failed: %d", ret);
		return ret;
	}

	/* Configure XCLK PWM timer now that indices are known */
	ret = nxp_flexio_camera_configure_xclk(dev);
	if (ret < 0) {
		return ret;
	}

	/* Apply pin multiplexing */
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("pinctrl apply failed: %d", ret);
		return ret;
	}

	/* Configure VSYNC as input; interrupt is enabled in set_stream */
	ret = gpio_pin_configure_dt(&config->vsync_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->vsync_cb, nxp_flexio_camera_vsync_isr,
			   BIT(config->vsync_gpio.pin));

	ret = gpio_add_callback(config->vsync_gpio.port, &data->vsync_cb);
	if (ret < 0) {
		return ret;
	}

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);
	k_sem_init(&data->stream_empty, 0, 1);

	return 0;
}

static DEVICE_API(video, nxp_flexio_camera_api) = {
	.set_format = nxp_flexio_camera_set_format,
	.get_format = nxp_flexio_camera_get_format,
	.get_caps   = nxp_flexio_camera_get_caps,
	.set_stream = nxp_flexio_camera_set_stream,
	.enqueue    = nxp_flexio_camera_enqueue,
	.dequeue    = nxp_flexio_camera_dequeue,
	.flush      = nxp_flexio_camera_flush,
};

#define SOURCE_DEV(inst) \
	DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(inst, 0, 0)))

#define MCUX_FLEXIO_CAMERA_INIT(inst)								\
	PINCTRL_DT_INST_DEFINE(inst);								\
	static const struct nxp_flexio_child flexio_cam_child_##inst = {			\
		.isr	= NULL,									\
		.user_data = (void *)DEVICE_DT_INST_GET(inst),					\
	};											\
	static const struct nxp_flexio_camera_config flexio_cam_config_##inst = {		\
		.flexio_dev     = DEVICE_DT_GET(DT_INST_PARENT(inst)),				\
		.flexio_base    = (FLEXIO_Type *)DT_REG_ADDR(DT_INST_PARENT(inst)),		\
		.source_dev     = SOURCE_DEV(inst),						\
		.child	        = &flexio_cam_child_##inst,					\
		.pincfg	        = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
		.vsync_gpio     = GPIO_DT_SPEC_INST_GET(inst, vsync_gpios),			\
		.dma_dev	= DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_IDX(inst, 0)),		\
		.dma_channel    = DT_INST_DMAS_CELL_BY_IDX(inst, 0, mux),			\
		.dma_slot	= DT_INST_DMAS_CELL_BY_IDX(inst, 0, source),			\
		.data_pin_start = DT_INST_PROP(inst, data_pin_start),				\
		.pclk_pin	= DT_INST_PROP(inst, pclk_pin),					\
		.href_pin	= DT_INST_PROP(inst, href_pin),					\
		.xclk_pin	= DT_INST_PROP_OR(inst, xclk_pin, FLEXIO_PIN_NOT_USED),		\
		.xclk_frequency = DT_INST_PROP(inst, xclk_frequency),				\
		.shifter_count  = DT_INST_PROP(inst, shifter_count),				\
		.shifter_end_index = DT_INST_PROP(inst, shifter_end_index),			\
		.xclk_timer	= DT_INST_PROP(inst, xclk_timer),				\
		.pclk_timer	= DT_INST_PROP(inst, pclk_timer),				\
	};											\
	static struct nxp_flexio_camera_data flexio_cam_data_##inst;				\
	DEVICE_DT_INST_DEFINE(inst,								\
			      nxp_flexio_camera_init,						\
			      NULL,								\
			      &flexio_cam_data_##inst,						\
			      &flexio_cam_config_##inst,					\
			      POST_KERNEL,							\
			      CONFIG_VIDEO_MCUX_FLEXIO_CAMERA_INIT_PRIORITY,			\
			      &nxp_flexio_camera_api);						\
	VIDEO_DEVICE_DEFINE(flexio_cam_##inst,							\
			    DEVICE_DT_INST_GET(inst),						\
			    SOURCE_DEV(inst));

DT_INST_FOREACH_STATUS_OKAY(MCUX_FLEXIO_CAMERA_INIT)
