/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_video_pio_dma

#include <hardware/pio.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_pio_dma, CONFIG_VIDEO_LOG_LEVEL);

#define VIDEO_PIO_DMA_STACK_SIZE 1024
#define VIDEO_PIO_DMA_PRIORITY   5
#define MAX_FRAME_RATE           10
#define MIN_FRAME_RATE           1
#define VIDEO_PIO_DMA_STACK_SIZE 1024
#define VIDEO_PIO_DMA_PRIORITY   5
#define VIDEO_PIO_DMA_RX_BIT     0
#define VIDEO_PIO_DMA_RELOAD_BIT 1
#define VIDEO_PIO_DMA_ERR_BIT    2

K_THREAD_STACK_DEFINE(video_pio_dma_stack_area, VIDEO_PIO_DMA_STACK_SIZE);

struct video_pio_dma_data {
	const struct device *dev;
	PIO pio;
	size_t pio_sm;
	int pio_program_offset;
	pio_sm_config pio_sm_config;
	pio_program_t pio_program;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct k_work_delayable work;
	struct k_work_q work_q;
	struct k_poll_signal *sig;
	struct video_buffer *vbuf;
	int dma_channel;
	unsigned int frame_rate;
	unsigned int pclk_per_px;
	unsigned int border_px;
	struct video_format fmt;
	atomic_t stream_evt;
	bool stream_on;
};

struct video_pio_dma_config {
	const struct device *pio_dev;
	const struct pinctrl_dev_config *pin_cfg;
	const struct gpio_dt_spec vsync_gpio;
	const struct gpio_dt_spec hsync_gpio;
	const struct gpio_dt_spec pclk_gpio;
	const struct gpio_dt_spec data_gpio;
	const struct device *sensor_dev;
	const struct device *dma_dev;
	const uint32_t data_bits;
};

#ifdef CONFIG_POLL
static int video_pio_dma_set_signal(const struct device *dev, struct k_poll_signal *sig)
{
	struct video_pio_dma_data *data = dev->data;

	if (data->sig && sig != NULL) {
		return -EALREADY;
	}
	data->sig = sig;
	return 0;
}
#endif

static int video_pio_dma_config_pio(const struct device *dev)
{
	const struct video_pio_dma_config *config = dev->config;
	struct video_pio_dma_data *data = dev->data;

	if (data->pio_program_offset >= 0) {
		pio_remove_program(data->pio, &data->pio_program, data->pio_program_offset);
		data->pio_program_offset = -1;
	}
	uint16_t pio_program_instructions[] = {
		/* istruction 0 */
		pio_encode_pull(false, true),
		/* istruction 1 */
		pio_encode_wait_gpio(false, config->vsync_gpio.pin),
		/* istruction 2 */
		pio_encode_wait_gpio(true, config->vsync_gpio.pin),
		/* istruction 3 */
		pio_encode_set(pio_y, data->border_px - 1),
		/* istruction 4 */
		pio_encode_wait_gpio(true, config->hsync_gpio.pin),
		/* istruction 5 */
		pio_encode_wait_gpio(false, config->hsync_gpio.pin),
		/* istruction 6 */
		pio_encode_jmp_y_dec(4),
		/* .wrap_target */
		/* istruction 7 */
		pio_encode_mov(pio_x, pio_osr),
		/* istruction 8 */
		pio_encode_wait_gpio(true, config->hsync_gpio.pin),
		/* istruction 9 */
		pio_encode_set(pio_y, data->border_px * data->pclk_per_px - 1),
		/* istruction 10 */
		pio_encode_wait_gpio(true, config->pclk_gpio.pin),
		/* istruction 11 */
		pio_encode_wait_gpio(false, config->pclk_gpio.pin),
		/* istruction 12 */
		pio_encode_jmp_y_dec(10),
		/* istruction 13 */
		pio_encode_wait_gpio(true, config->pclk_gpio.pin),
		/* istruction 14 */
		pio_encode_in(pio_pins, config->data_bits),
		/* istruction 15 */
		pio_encode_wait_gpio(false, config->pclk_gpio.pin),
		/* istruction 16 */
		pio_encode_jmp_x_dec(13),
		/* istruction 17 */
		pio_encode_wait_gpio(false, config->hsync_gpio.pin),
		/* .wrap */
	};

	data->pio_program.instructions = pio_program_instructions;
	data->pio_program.length = ARRAY_SIZE(pio_program_instructions);
	data->pio_program.origin = -1;

	data->pio_program_offset = pio_add_program(data->pio, &data->pio_program);
	if (data->pio_program_offset >= 0) {
		data->pio_sm_config = pio_get_default_sm_config();
		sm_config_set_in_pins(&data->pio_sm_config, config->data_gpio.pin);
		sm_config_set_in_pin_count(&data->pio_sm_config, config->data_bits);
		sm_config_set_in_shift(&data->pio_sm_config, true, true, 8);
		sm_config_set_wrap(&data->pio_sm_config, data->pio_program_offset + 7,
				   data->pio_program_offset + data->pio_program.length - 1);
		pio_sm_set_consecutive_pindirs(data->pio, data->pio_sm, config->data_gpio.pin,
					       config->data_bits, false);
	}
	return data->pio_program_offset >= 0 ? 0 : data->pio_program_offset;
}

static void video_pio_dma_dma_callback(const struct device *dev, void *user_data, uint32_t channel,
				       int status)
{
	struct video_pio_dma_data *data = user_data;

	atomic_set_bit(&data->stream_evt,
		       status >= 0 ? VIDEO_PIO_DMA_RX_BIT : VIDEO_PIO_DMA_ERR_BIT);
	k_work_reschedule_for_queue(&data->work_q, &data->work, K_NO_WAIT);
}

static int video_pio_dma_stop_capture(const struct device *dev)
{
	struct video_pio_dma_data *data = dev->data;
	const struct video_pio_dma_config *config = dev->config;

	LOG_DBG("video_pio_dma capture stop");
	pio_sm_set_enabled(data->pio, data->pio_sm, false);
	dma_stop(config->dma_dev, data->dma_channel);
	dma_release_channel(config->dma_dev, data->dma_channel);
	return 0;
}

static int video_pio_dma_disable_capture(const struct device *dev)
{
	struct video_pio_dma_data *data = dev->data;

	LOG_DBG("video_pio_dma capture disable");
	pio_sm_set_enabled(data->pio, data->pio_sm, false);
	return 0;
}

static int video_pio_dma_reload_capture(const struct device *dev)
{
	struct video_pio_dma_data *data = dev->data;
	const struct video_pio_dma_config *config = dev->config;
	struct video_buffer *vbuf;
	uint8_t *src_addr;
	int ret;

	LOG_DBG("video_pio_dma capture reload");

	vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (vbuf == NULL) {
		LOG_ERR("No free video buffer");
		return -ENOMEM;
	}

	data->vbuf = vbuf;
	src_addr = (uint8_t *)&data->pio->rxf[data->pio_sm];
	pio_sm_init(data->pio, data->pio_sm, data->pio_program_offset, &data->pio_sm_config);
	dma_reload(config->dma_dev, data->dma_channel, (uint32_t)&src_addr[3],
		   (uint32_t)vbuf->buffer, data->fmt.width * data->fmt.height);
	ret = dma_start(config->dma_dev, data->dma_channel);

	if (ret != 0) {
		LOG_ERR("Failed to start dma (%d)", ret);
		return ret;
	}

	pio_sm_set_enabled(data->pio, data->pio_sm, true);
	pio_sm_put_blocking(data->pio, data->pio_sm, data->fmt.width * data->pclk_per_px - 1);

	return ret;
}

static int video_pio_dma_start_capture(const struct device *dev)
{
	struct video_pio_dma_data *data = dev->data;
	const struct video_pio_dma_config *config = dev->config;
	struct video_buffer *vbuf;
	int ret;

	LOG_DBG("video_pio_dma capture start");

	ret = video_get_format(config->sensor_dev, &data->fmt);
	if (ret != 0) {
		LOG_ERR("Failed to get video format (%d)", ret);
		return ret;
	}

	vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (vbuf == NULL) {
		LOG_ERR("No free video buffer");
		return -ENOMEM;
	}

	data->vbuf = vbuf;
	data->dma_channel = dma_request_channel(config->dma_dev, NULL);
	if (data->dma_channel < 0) {
		LOG_ERR("Failed to request DMA channel (%d)", data->dma_channel);
		return data->dma_channel;
	}

	pio_sm_init(data->pio, data->pio_sm, data->pio_program_offset, &data->pio_sm_config);
	uint8_t *src_addr = (uint8_t *)&data->pio->rxf[data->pio_sm];
	struct dma_block_config block_cfg = {
		.source_address = (uint32_t)&src_addr[3],
		.dest_address = (uint32_t)(vbuf->buffer),
		.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
		.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT,
		.block_size = data->fmt.width * data->fmt.height,
	};

	struct dma_config dma_cfg = {
		.dma_slot = ~pio_get_dreq(data->pio, data->pio_sm, false),
		.channel_direction = PERIPHERAL_TO_MEMORY,
		.block_count = 1,
		.head_block = &block_cfg,
		.source_data_size = sizeof(uint8_t),
		.dest_data_size = sizeof(uint8_t),
		.source_burst_length = block_cfg.block_size,
		.dma_callback = video_pio_dma_dma_callback,
		.user_data = data,
	};
	ret = dma_config(config->dma_dev, data->dma_channel, &dma_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure DMA channel error (%d)", ret);
		dma_release_channel(config->dma_dev, data->dma_channel);
		return ret;
	}

	ret = dma_start(config->dma_dev, data->dma_channel);
	if (ret != 0) {
		LOG_ERR("Failed to start DMA (%d)", ret);
		return ret;
	}

	pio_sm_set_enabled(data->pio, data->pio_sm, true);
	pio_sm_put_blocking(data->pio, data->pio_sm, data->fmt.width * data->pclk_per_px - 1);
	return ret;
}

static int video_pio_dma_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct video_pio_dma_config *config = dev->config;

	return video_get_caps(config->sensor_dev, caps);
}

static int video_pio_dma_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct video_pio_dma_config *config = dev->config;
	struct video_pio_dma_data *data = dev->data;
	int ret;

	ret = video_set_format(config->sensor_dev, fmt);
	if (ret != 0) {
		LOG_ERR("Error set video format (%d)", ret);
		return ret;
	}

	data->fmt = *fmt;
	ret = video_pio_dma_config_pio(dev);
	if (ret != 0) {
		LOG_ERR("Error configuriong PIO (%d)", ret);
		return ret;
	}
	return ret;
}

static int video_pio_dma_get_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct video_pio_dma_config *config = dev->config;

	return video_get_format(config->sensor_dev, fmt);
}

static int video_pio_dma_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct video_pio_dma_config *config = dev->config;
	struct video_pio_dma_data *data = dev->data;
	int ret;

	if (enable && !data->stream_on) {
		ret = video_pio_dma_start_capture(dev);
		if (ret != 0) {
			LOG_ERR("Error start pio dma (%d)", ret);
			return ret;
		}
		ret = video_stream_start(config->sensor_dev, VIDEO_BUF_TYPE_OUTPUT);
		if (ret != 0) {
			LOG_ERR("Error video start stream (%d)", ret);
			return ret;
		}
		data->stream_on = true;
	} else {
		ret = video_pio_dma_stop_capture(dev);
		if (ret != 0) {
			LOG_ERR("Error start pio dma (%d)", ret);
		}
		ret |= video_stream_stop(config->sensor_dev, VIDEO_BUF_TYPE_OUTPUT);
		if (ret != 0) {
			LOG_ERR("Error video stop stream (%d)", ret);
		}
		data->stream_on = false;
	}
	return ret;
}

static int video_pio_dma_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct video_pio_dma_data *data = dev->data;

	k_fifo_put(&data->fifo_in, vbuf);
	return 0;
}

static int video_pio_dma_dequeue(const struct device *dev, struct video_buffer **vbuf,
				 k_timeout_t timeout)
{
	struct video_pio_dma_data *data = dev->data;

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}
	return 0;
}

static int video_pio_dma_flush(const struct device *dev, bool cancel)
{
	struct video_pio_dma_data *data = dev->data;
	struct video_buffer *vbuf;

	if (!cancel) {
		/* wait for all buffer to be processed */
		do {
			k_sleep(K_MSEC(1));
		} while (!k_fifo_is_empty(&data->fifo_in));
	} else {
		while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT))) {
			k_fifo_put(&data->fifo_out, vbuf);
			if (IS_ENABLED(CONFIG_POLL) && data->sig) {
				k_poll_signal_raise(data->sig, VIDEO_BUF_ABORTED);
			}
		}
	}
	return 0;
}
static void video_pio_dma_worker(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct video_pio_dma_data *data = CONTAINER_OF(dwork, struct video_pio_dma_data, work);
	int ret;

	if (data->stream_on) {
		if (atomic_test_and_clear_bit(&data->stream_evt, VIDEO_PIO_DMA_RELOAD_BIT)) {
			ret = video_pio_dma_reload_capture(data->dev);
			if (ret != 0) {
				LOG_WRN("Warning reloading video_pio_dma stream (%d)", ret);
				atomic_set_bit(&data->stream_evt, VIDEO_PIO_DMA_RELOAD_BIT);
				k_work_reschedule_for_queue(&data->work_q, &data->work,
							    K_MSEC(1000 / data->frame_rate));
			}
		}
	}

	if (atomic_test_and_clear_bit(&data->stream_evt, VIDEO_PIO_DMA_RX_BIT)) {
		if (video_pio_dma_disable_capture(data->dev) == 0) {
			data->vbuf->bytesused = data->fmt.height * data->fmt.width;
			data->vbuf->timestamp = k_uptime_get_32();
			k_fifo_put(&data->fifo_out, data->vbuf);
			if (IS_ENABLED(CONFIG_POLL) && data->sig) {
				k_poll_signal_raise(data->sig, VIDEO_BUF_DONE);
			}
			atomic_set_bit(&data->stream_evt, VIDEO_PIO_DMA_RELOAD_BIT);
			k_work_reschedule_for_queue(&data->work_q, &data->work,
						    K_MSEC(1000 / data->frame_rate));
		}
	}

	if (atomic_test_and_clear_bit(&data->stream_evt, VIDEO_PIO_DMA_ERR_BIT)) {
		if (video_pio_dma_disable_capture(data->dev) == 0) {
			k_fifo_put(&data->fifo_in, data->vbuf);
			atomic_set_bit(&data->stream_evt, VIDEO_PIO_DMA_RELOAD_BIT);
			k_work_reschedule_for_queue(&data->work_q, &data->work,
						    K_MSEC(1000 / data->frame_rate));
		}
	}

	k_yield();
}

static int video_pio_dma_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct video_pio_dma_data *data = dev->data;

	data->frame_rate = CLAMP(DIV_ROUND_CLOSEST(frmival->denominator, frmival->numerator),
				 MIN_FRAME_RATE, MAX_FRAME_RATE);
	frmival->numerator = 1;
	frmival->denominator = data->frame_rate;
	return 0;
}

static int video_pio_dma_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct video_pio_dma_data *data = dev->data;

	frmival->numerator = 1;
	frmival->denominator = data->frame_rate;
	return 0;
}

static DEVICE_API(video, video_pio_dma_driver_api) = {
	.set_format = video_pio_dma_set_fmt,
	.get_format = video_pio_dma_get_fmt,
	.set_stream = video_pio_dma_set_stream,
	.get_caps = video_pio_dma_get_caps,
	.enqueue = video_pio_dma_enqueue,
	.dequeue = video_pio_dma_dequeue,
	.set_frmival = video_pio_dma_set_frmival,
	.get_frmival = video_pio_dma_get_frmival,
#ifdef CONFIG_POLL
	.set_signal = video_pio_dma_set_signal,
#endif
	.flush = video_pio_dma_flush,
};

static int video_pio_dma_init(const struct device *dev)
{
	struct video_pio_dma_data *data = dev->data;
	const struct video_pio_dma_config *config = dev->config;
	int ret;

	data->dev = dev;

	if (!device_is_ready(config->pio_dev)) {
		LOG_ERR("PIO not ready");
		return -ENODEV;
	}
	if (!device_is_ready(config->dma_dev)) {
		LOG_ERR("DMA not ready");
		return -ENODEV;
	}

	data->pio = pio_rpi_pico_get_pio(config->pio_dev);
	ret = pio_rpi_pico_allocate_sm(config->pio_dev, &data->pio_sm);
	if (ret != 0) {
		LOG_ERR("Failed to allocate PIO state machine");
		return ret;
	}

	if (config->data_bits == 8) {
		data->pclk_per_px = 1;
	} else if (config->data_bits == 4) {
		data->pclk_per_px = 2;
	} else if (config->data_bits == 1) {
		data->pclk_per_px = 8;
	} else {
		LOG_ERR("Invalid data bits!");
		return -ENODEV;
	}

	/* 2 pixels for borders */
	data->border_px = 2;
	data->pio_program_offset = -1;

	ret = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("Failed to apply pinctrl state");
		return ret;
	}

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);
	data->frame_rate = MAX_FRAME_RATE;
	data->stream_on = false;
	k_work_init_delayable(&data->work, video_pio_dma_worker);
	k_work_queue_init(&data->work_q);
	k_work_queue_start(&data->work_q, video_pio_dma_stack_area,
			   K_THREAD_STACK_SIZEOF(video_pio_dma_stack_area), VIDEO_PIO_DMA_PRIORITY,
			   NULL);
	return ret;
}

#define SOURCE_DEV(inst) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(inst, 0, 0)))

#define VIDEO_PIO_DMA_INIT(inst)                                                                   \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	const struct video_pio_dma_config video_pio_dma_config_##inst = {                          \
		.pio_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                    \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                   \
		.vsync_gpio = GPIO_DT_SPEC_INST_GET(inst, vsync_gpios),                            \
		.hsync_gpio = GPIO_DT_SPEC_INST_GET(inst, hsync_gpios),                            \
		.pclk_gpio = GPIO_DT_SPEC_INST_GET(inst, pclk_gpios),                              \
		.data_gpio = GPIO_DT_SPEC_INST_GET(inst, data_gpios),                              \
		.sensor_dev = SOURCE_DEV(inst),                                                    \
		.dma_dev = DEVICE_DT_GET(DT_PROP(DT_DRV_INST(inst), dma)),                         \
		.data_bits = DT_INST_PROP(inst, data_bits),                                        \
	};                                                                                         \
	struct video_pio_dma_data video_pio_dma_data_##inst;                                       \
	DEVICE_DT_INST_DEFINE(inst, &video_pio_dma_init, NULL, &video_pio_dma_data_##inst,         \
			      &video_pio_dma_config_##inst, POST_KERNEL,                           \
			      CONFIG_VIDEO_INIT_PRIORITY, &video_pio_dma_driver_api);              \
	VIDEO_DEVICE_DEFINE(video_pio_dma_##inst, DEVICE_DT_INST_GET(inst), SOURCE_DEV(inst));

DT_INST_FOREACH_STATUS_OKAY(VIDEO_PIO_DMA_INIT)
