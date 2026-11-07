/*
 * Copyright (c) 2024 Charles Dias <charlesdias.cd@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_dcmi

#include <errno.h>

#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/video/video.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_stm32_dcmi, CONFIG_VIDEO_LOG_LEVEL);

typedef void (*irq_config_func_t)(const struct device *dev);

struct stream {
	const struct device *dma_dev;
	uint32_t channel;
	struct dma_config cfg;
	struct dma_block_config block;
};

enum stm32_dcmi_state {
	STM32_DCMI_STOPPED = 0,
	STM32_DCMI_WAIT_FOR_BUFFER,
	STM32_DCMI_RUNNING,
};

struct video_stm32_dcmi_data {
	const struct device *dev;
	DCMI_HandleTypeDef hdcmi;
	struct video_format fmt;
	int capture_rate;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct k_spinlock lock;
	enum stm32_dcmi_state state;
	struct video_buffer *vbuf;
	bool continuous;
	struct stream dma;
	struct stream dma_m2m;
	uint32_t capture_offset;
	bool copy_pending;
};

struct video_stm32_dcmi_config {
	struct stm32_pclken pclken;
	irq_config_func_t irq_config;
	const struct pinctrl_dev_config *pctrl;
	const struct device *sensor_dev;
};

static void stm32_dcmi_capture_start(struct video_stm32_dcmi_data *data)
{
	data->hdcmi.Instance->CR &= ~(DCMI_CR_CM);
	data->hdcmi.Instance->CR |= data->continuous ? DCMI_MODE_CONTINUOUS : DCMI_MODE_SNAPSHOT;
	data->hdcmi.Instance->CR |= DCMI_CR_CAPTURE;

	__HAL_DCMI_ENABLE_IT(&data->hdcmi, DCMI_IT_ERR | DCMI_IT_OVR);
}

static void stm32_dcmi_abort_capture(struct video_stm32_dcmi_data *data)
{
	data->hdcmi.Instance->CR &= ~DCMI_CR_CAPTURE;
	__HAL_DCMI_DISABLE(&data->hdcmi);
	__HAL_DCMI_DISABLE_IT(&data->hdcmi, DCMI_IT_ERR | DCMI_IT_OVR);
	__HAL_DCMI_CLEAR_FLAG(&data->hdcmi, DCMI_FLAG_ERRRI | DCMI_FLAG_OVRRI);

	if (dma_stop(data->dma.dma_dev, data->dma.channel) < 0) {
		LOG_ERR("Failed to stop DCMI DMA channel");
	}
	if (data->dma.cfg.cyclic &&
	    dma_stop(data->dma_m2m.dma_dev, data->dma_m2m.channel) < 0) {
		LOG_ERR("Failed to stop DCMI M2M DMA channel");
	}
	data->copy_pending = false;
}

static uint32_t stm32_dcmi_dma_period(uint32_t frame_size, uint32_t limit)
{
	uint32_t half_words = frame_size / (2 * sizeof(uint32_t));
	uint32_t max_half_words = limit / (2 * sizeof(uint32_t));
	uint32_t best = 0;

	if ((frame_size % (2 * sizeof(uint32_t))) != 0) {
		return 0;
	}

	for (uint32_t divisor = 1; divisor <= half_words / divisor; divisor++) {
		uint32_t other;

		if ((half_words % divisor) != 0) {
			continue;
		}
		other = half_words / divisor;
		if (divisor <= max_half_words) {
			best = MAX(best, divisor);
		}
		if (other <= max_half_words) {
			best = MAX(best, other);
		}
	}

	return best * 2 * sizeof(uint32_t);
}

static int stm32_dcmi_configure_cyclic(struct video_stm32_dcmi_data *data,
				      struct video_buffer *vbuf, uint32_t frame_size)
{
	struct stream *dma = &data->dma;
	struct stream *m2m = &data->dma_m2m;
	uint32_t period = stm32_dcmi_dma_period(frame_size, frame_size / 2);
	int ret = -ENOTSUP;

	dma->cfg.cyclic = 1;
	dma->cfg.half_complete_callback_en = 1;
	dma->block.source_reload_en = 1;
	dma->block.dest_reload_en = 1;

	/* Let the DMA controller determine the supported period size. */
	while (period != 0) {
		dma->block.dest_address = (uintptr_t)vbuf->buffer + frame_size - period;
		dma->block.block_size = period;
		ret = dma_config(dma->dma_dev, dma->channel, &dma->cfg);
		if (ret == 0) {
			break;
		}
		period = stm32_dcmi_dma_period(frame_size, period / 2);
	}
	if (ret < 0) {
		return ret;
	}

	m2m->block.source_address = dma->block.dest_address;
	m2m->block.dest_address = (uintptr_t)vbuf->buffer;
	m2m->block.block_size = period / 2;
	data->capture_offset = 0;
	data->copy_pending = false;

	return dma_config(m2m->dma_dev, m2m->channel, &m2m->cfg);
}

/* Try to (re)arm the DCMI capture, either in snapshot (if at most one spare
 * buffer is available) or continuous (if at least two spare buffers are
 * available).
 */
static int stm32_dcmi_start_dma(struct video_stm32_dcmi_data *data, struct video_buffer *vbuf);
static int stm32_dcmi_arm_capture(struct video_stm32_dcmi_data *data)
{
	struct video_buffer *vbuf;
	int ret;

	vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (vbuf == NULL) {
		LOG_DBG("arm_capture: no buffer, waiting");
		data->state = STM32_DCMI_WAIT_FOR_BUFFER;
		return -ENOMEM;
	}
	sys_cache_data_invd_range(vbuf->buffer, vbuf->bytesused);

	data->continuous = !k_fifo_is_empty(&data->fifo_in);
	ret = stm32_dcmi_start_dma(data, vbuf);
	if (ret == 0) {
		stm32_dcmi_capture_start(data);
	}

	return ret;
}

/* Complete a frame while holding data->lock. */
static struct video_buffer *stm32_dcmi_finish_frame(struct video_stm32_dcmi_data *data)
{
	struct video_buffer *vbuf_done = data->vbuf;
	struct video_buffer *vbuf_next;

	data->vbuf = NULL;

	if (data->continuous) {
		if (dma_stop(data->dma.dma_dev, data->dma.channel) != 0) {
			LOG_ERR("Failed to stop completed DCMI DMA transfer");
			stm32_dcmi_abort_capture(data);
			data->state = STM32_DCMI_WAIT_FOR_BUFFER;
			data->continuous = false;
			return vbuf_done;
		}

		vbuf_next = k_fifo_get(&data->fifo_in, K_NO_WAIT);
		if (vbuf_next == NULL) {
			stm32_dcmi_abort_capture(data);
			data->state = STM32_DCMI_WAIT_FOR_BUFFER;
			data->continuous = false;
		} else {
			sys_cache_data_invd_range(vbuf_next->buffer, vbuf_next->bytesused);
			if (stm32_dcmi_start_dma(data, vbuf_next) != 0) {
				stm32_dcmi_abort_capture(data);
			}
		}
	} else {
		stm32_dcmi_abort_capture(data);
		stm32_dcmi_arm_capture(data);
	}

	return vbuf_done;
}

static void stm32_dcmi_process_dma_error(struct video_stm32_dcmi_data *data)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->state != STM32_DCMI_RUNNING) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	LOG_DBG("Restart DMA after Error!");
	stm32_dcmi_abort_capture(data);
	data->continuous = false;

	if (data->vbuf != NULL) {
		k_fifo_put(&data->fifo_in, data->vbuf);
		data->vbuf = NULL;
	}

	stm32_dcmi_arm_capture(data);

	k_spin_unlock(&data->lock, key);
}

static void stm32_dcmi_isr(const struct device *dev)
{
	struct video_stm32_dcmi_data *data = dev->data;
	uint32_t misr = data->hdcmi.Instance->MISR;
	bool error = false;

	if ((misr & DCMI_FLAG_ERRRI) != 0) {
		LOG_DBG("Synchronization error detected");
		__HAL_DCMI_CLEAR_FLAG(&data->hdcmi, DCMI_FLAG_ERRRI);
		error = true;
	}
	if ((misr & DCMI_FLAG_OVRRI) != 0) {
		LOG_DBG("Overrun error detected");
		__HAL_DCMI_CLEAR_FLAG(&data->hdcmi, DCMI_FLAG_OVRRI);
		error = true;
	}

	if (error) {
		stm32_dcmi_process_dma_error(data);
	}
}

static void dcmi_dma_callback(const struct device *dev, void *arg, uint32_t channel, int status)
{
	struct video_stm32_dcmi_data *data = arg;
	struct video_buffer *vbuf_done = NULL;
	k_spinlock_key_t key;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel);

	if (status < 0) {
		LOG_ERR("DCMI DMA transfer failed: %d", status);
		stm32_dcmi_process_dma_error(data);
		return;
	}
	if (status != DMA_STATUS_COMPLETE &&
	    !(data->dma.cfg.cyclic &&
	      (status == DMA_STATUS_BLOCK || status == DMA_STATUS_HALF_COMPLETE))) {
		LOG_ERR("Unexpected DCMI DMA status: %d", status);
		stm32_dcmi_process_dma_error(data);
		return;
	}

	key = k_spin_lock(&data->lock);
	if (data->state != STM32_DCMI_RUNNING) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	if (data->dma.cfg.cyclic) {
		struct stream *m2m = &data->dma_m2m;
		uint32_t half_size = data->dma.block.block_size / 2;
		uint32_t tail_offset = data->vbuf->bytesused - data->dma.block.block_size;
		bool second_half = (data->capture_offset % data->dma.block.block_size) != 0;
		int ret;

		if (data->copy_pending ||
		    second_half != (status == DMA_STATUS_COMPLETE)) {
			k_spin_unlock(&data->lock, key);
			LOG_ERR("DCMI DMA staging buffer overrun");
			stm32_dcmi_process_dma_error(data);
			return;
		}

		if (data->capture_offset < tail_offset) {
			ret = dma_reload(m2m->dma_dev, m2m->channel,
					 data->dma.block.dest_address +
					 (second_half ? half_size : 0),
					 (uintptr_t)data->vbuf->buffer + data->capture_offset,
					 half_size);
			if (ret == 0) {
				data->copy_pending = true;
				ret = dma_start(m2m->dma_dev, m2m->channel);
			}
			if (ret < 0) {
				k_spin_unlock(&data->lock, key);
				LOG_ERR("Failed to start DCMI M2M transfer: %d", ret);
				stm32_dcmi_process_dma_error(data);
				return;
			}
		}
		data->capture_offset += half_size;
		if (data->capture_offset < data->vbuf->bytesused) {
			k_spin_unlock(&data->lock, key);
			return;
		}
	}

	vbuf_done = stm32_dcmi_finish_frame(data);
	k_spin_unlock(&data->lock, key);

	if (vbuf_done != NULL) {
		sys_cache_data_invd_range(vbuf_done->buffer, vbuf_done->bytesused);
		vbuf_done->timestamp = k_uptime_get_32();
		k_fifo_put(&data->fifo_out, vbuf_done);
	}
}

static void stm32_dcmi_m2m_callback(const struct device *dev, void *arg, uint32_t channel,
				   int status)
{
	struct video_stm32_dcmi_data *data = arg;
	k_spinlock_key_t key;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel);

	if (status != DMA_STATUS_COMPLETE) {
		LOG_ERR("DCMI M2M DMA transfer failed: %d", status);
		stm32_dcmi_process_dma_error(data);
		return;
	}

	key = k_spin_lock(&data->lock);
	data->copy_pending = false;
	k_spin_unlock(&data->lock, key);
}

static int stm32_dma_init(const struct device *dev)
{
	struct video_stm32_dcmi_data *data = dev->data;
	struct stream *dma = &data->dma;

	/* Check if the DMA device is ready */
	if (!device_is_ready(dma->dma_dev)) {
		LOG_ERR("%s DMA device not ready", dma->dma_dev->name);
		return -ENODEV;
	}

	dma->cfg.head_block = &dma->block;
	dma->cfg.block_count = 1;
	dma->cfg.complete_callback_en = 1;
	dma->cfg.dma_callback = dcmi_dma_callback;

	dma = &data->dma_m2m;
	if (dma->dma_dev != NULL) {
		if (!device_is_ready(dma->dma_dev)) {
			LOG_ERR("%s M2M DMA device not ready", dma->dma_dev->name);
			return -ENODEV;
		}
		if (dma->cfg.channel_direction != MEMORY_TO_MEMORY ||
		    dma->block.source_addr_adj != DMA_ADDR_ADJ_INCREMENT ||
		    dma->block.dest_addr_adj != DMA_ADDR_ADJ_INCREMENT ||
		    (dma->dma_dev == data->dma.dma_dev && dma->channel == data->dma.channel)) {
			return -EINVAL;
		}
		dma->cfg.head_block = &dma->block;
		dma->cfg.block_count = 1;
		dma->cfg.complete_callback_en = 1;
		dma->cfg.dma_callback = stm32_dcmi_m2m_callback;
	}

	return 0;
}

/* Configure and start a DMA transfer for one captured frame. */
static int stm32_dcmi_start_dma(struct video_stm32_dcmi_data *data, struct video_buffer *vbuf)
{
	struct stream *dma = &data->dma;
	int ret;

	dma->block.source_address = (uintptr_t)&data->hdcmi.Instance->DR;
	dma->block.dest_address = (uintptr_t)vbuf->buffer;
	dma->block.block_size = vbuf->bytesused;
	dma->block.next_block = NULL;
	dma->block.source_reload_en = 0;
	dma->block.dest_reload_en = 0;
	dma->cfg.block_count = 1;
	dma->cfg.half_complete_callback_en = 0;
	data->vbuf = vbuf;

	ret = dma_config(dma->dma_dev, dma->channel, &dma->cfg);
	if (ret < 0 && data->dma_m2m.dma_dev != NULL) {
		ret = stm32_dcmi_configure_cyclic(data, vbuf, vbuf->bytesused);
	}
	if (ret < 0) {
		LOG_ERR("Failed to configure DMA channel %u: %d", dma->channel, ret);
		goto error;
	}

	ret = dma_start(dma->dma_dev, dma->channel);
	if (ret < 0) {
		LOG_ERR("Failed to start DCMI DMA transfer: %d", ret);
		goto error;
	}

	data->state = STM32_DCMI_RUNNING;
	__HAL_DCMI_ENABLE(&data->hdcmi);

	return 0;

error:
	stm32_dcmi_abort_capture(data);
	data->vbuf = NULL;
	k_fifo_put(&data->fifo_in, vbuf);
	data->state = STM32_DCMI_WAIT_FOR_BUFFER;
	data->continuous = false;

	return ret;
}

static int stm32_dcmi_enable_clock(const struct device *dev)
{
	const struct video_stm32_dcmi_config *config = dev->config;
	const struct device *dcmi_clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	/* Turn on DCMI peripheral clock */
	return clock_control_on(dcmi_clock, (clock_control_subsys_t)&config->pclken);
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

	ret = video_estimate_fmt_size(fmt);
	if (ret < 0) {
		return ret;
	}

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

	ret = video_estimate_fmt_size(fmt);
	if (ret < 0) {
		return ret;
	}

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
	struct video_buffer *vbuf;
	k_spinlock_key_t key;
	int err;

	if (!enable) {
		key = k_spin_lock(&data->lock);

		vbuf = data->vbuf;
		data->vbuf = NULL;
		data->continuous = false;
		data->state = STM32_DCMI_STOPPED;
		stm32_dcmi_abort_capture(data);

		k_spin_unlock(&data->lock, key);

		err = video_stream_stop(config->sensor_dev, type);
		if (err < 0) {
			return err;
		}

		/* Release the buffer which was being used */
		key = k_spin_lock(&data->lock);
		if (vbuf != NULL) {
			k_fifo_put(&data->fifo_in, vbuf);
		}
		k_spin_unlock(&data->lock, key);

		return 0;
	}

	/* Set the frame control */
	data->hdcmi.Instance->CR &= ~(DCMI_CR_FCRC_0 | DCMI_CR_FCRC_1);
	data->hdcmi.Instance->CR |= STM32_DCMI_GET_CAPTURE_RATE(data->capture_rate);

	key = k_spin_lock(&data->lock);
	err = stm32_dcmi_arm_capture(data);
	if (err < 0) {
		data->state = STM32_DCMI_STOPPED;
		k_spin_unlock(&data->lock, key);
		return err;
	}
	k_spin_unlock(&data->lock, key);

	return video_stream_start(config->sensor_dev, type);
}

static int video_stm32_dcmi_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct video_stm32_dcmi_data *data = dev->data;
	const uint32_t buffer_size = data->fmt.pitch * data->fmt.height;
	k_spinlock_key_t key;

	if (buffer_size > vbuf->size) {
		return -EINVAL;
	}

	vbuf->bytesused = buffer_size;
	vbuf->line_offset = 0;

	key = k_spin_lock(&data->lock);

	k_fifo_put(&data->fifo_in, vbuf);
	if (data->state == STM32_DCMI_WAIT_FOR_BUFFER) {
		stm32_dcmi_arm_capture(data);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int video_stm32_dcmi_dequeue(const struct device *dev, struct video_buffer **vbuf,
				    k_timeout_t timeout)
{
	struct video_stm32_dcmi_data *data = dev->data;

	*vbuf = k_fifo_get(&data->fifo_out, timeout);

	return (*vbuf == NULL) ? -EAGAIN : 0;
}

static int video_stm32_dcmi_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct video_stm32_dcmi_config *config = dev->config;

	caps->min_vbuf_count = 1;

	/* Forward the message to the sensor device */
	return video_get_caps(config->sensor_dev, caps);
}

#define STM32_DCMI_MAX_FRAME_DROP    4
/* Capture rates the DCMI frame control supports: every frame, every 2nd, every 4th */
#define STM32_DCMI_NUM_CAPTURE_RATES 3

static int video_stm32_dcmi_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct video_stm32_dcmi_config *config = dev->config;
	uint32_t capture_rate = BIT(fie->index % STM32_DCMI_NUM_CAPTURE_RATES);
	struct video_frmival_enum sensor_fie = {
		.index = fie->index / STM32_DCMI_NUM_CAPTURE_RATES,
		.format = fie->format,
	};
	int ret;

	/*
	 * Report each sensor interval once per capture rate: dropping every 2nd or 4th
	 * frame multiplies the interval and its step, which one stepwise range with the
	 * sensor step cannot express.
	 */
	ret = video_enum_frmival(config->sensor_dev, &sensor_fie);
	if (ret < 0) {
		return ret;
	}

	fie->type = sensor_fie.type;
	if (sensor_fie.type == VIDEO_FRMIVAL_TYPE_DISCRETE) {
		fie->discrete = sensor_fie.discrete;
		fie->discrete.numerator *= capture_rate;
	} else {
		fie->stepwise = sensor_fie.stepwise;
		fie->stepwise.min.numerator *= capture_rate;
		fie->stepwise.max.numerator *= capture_rate;
		fie->stepwise.step.numerator *= capture_rate;
	}

	return 0;
}

static int video_stm32_dcmi_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_stm32_dcmi_config *config = dev->config;
	struct video_stm32_dcmi_data *data = dev->data;
	struct video_frmival_enum fie = {
		.format = &data->fmt,
	};
	struct video_frmival best_sensor_frmival;
	uint32_t best_diff_us = INT32_MAX;
	uint32_t diff_us = 0, a, b;
	int best_capture_rate = 1;
	int ret;

	a = video_frmival_nsec(frmival) / USEC_PER_MSEC;

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

		ret = video_closest_frmival(config->sensor_dev, &fie);
		if (ret < 0) {
			return ret;
		}
		b = video_frmival_nsec(&fie.discrete) * capture_rate / USEC_PER_MSEC;
		diff_us = a > b ? a - b : b - a;
		if (diff_us < best_diff_us) {
			best_diff_us = diff_us;
			best_sensor_frmival = fie.discrete;
			best_capture_rate = capture_rate;
		}
		if (diff_us == 0) {
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

static int video_stm32_dcmi_set_selection(const struct device *dev, struct video_selection *sel)
{
	const struct video_stm32_dcmi_config *config = dev->config;

	return video_set_selection(config->sensor_dev, sel);
}

static int video_stm32_dcmi_get_selection(const struct device *dev, struct video_selection *sel)
{
	const struct video_stm32_dcmi_config *config = dev->config;

	return video_get_selection(config->sensor_dev, sel);
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
	.set_selection = video_stm32_dcmi_set_selection,
	.get_selection = video_stm32_dcmi_get_selection,
};

static void video_stm32_dcmi_irq_config_func(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		stm32_dcmi_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

#define DCMI_DMA_CHANNEL_INIT(inst, index, src_dev, dest_dev)				\
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_IDX(inst, index)),		\
	.channel = DT_INST_DMAS_CELL_BY_IDX(inst, index, channel),			\
	.cfg = {									\
		.dma_slot = STM32_DT_INST_DMA_SLOT_BY_IDX(inst, index),			\
		.channel_direction = STM32_DMA_CONFIG_DIRECTION(			\
			STM32_DT_INST_DMA_CHANNEL_CONFIG_BY_IDX(inst, index)),		\
		.source_data_size = STM32_DMA_CONFIG_##src_dev##_DATA_SIZE(		\
			STM32_DT_INST_DMA_CHANNEL_CONFIG_BY_IDX(inst, index)),		\
		.dest_data_size = STM32_DMA_CONFIG_##dest_dev##_DATA_SIZE(		\
			STM32_DT_INST_DMA_CHANNEL_CONFIG_BY_IDX(inst, index)),		\
		/* single source transfers, 4 beat bursts to the destination */	\
		.source_burst_length = STM32_DMA_CONFIG_##src_dev##_DATA_SIZE(		\
			STM32_DT_INST_DMA_CHANNEL_CONFIG_BY_IDX(inst, index)),		\
		.dest_burst_length = 4 * STM32_DMA_CONFIG_##dest_dev##_DATA_SIZE(	\
			STM32_DT_INST_DMA_CHANNEL_CONFIG_BY_IDX(inst, index)),		\
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(				\
			STM32_DT_INST_DMA_CHANNEL_CONFIG_BY_IDX(inst, index)),		\
		.user_data = &video_stm32_dcmi_data_0,					\
	},										\
	.block = {									\
		.source_addr_adj = STM32_DMA_CONFIG_PERIPHERAL_ADDR_INC(		\
				STM32_DT_INST_DMA_CHANNEL_CONFIG_BY_IDX(inst, index)) ?	\
				DMA_ADDR_ADJ_INCREMENT : DMA_ADDR_ADJ_NO_CHANGE,	\
		.dest_addr_adj = STM32_DMA_CONFIG_MEMORY_ADDR_INC(			\
				STM32_DT_INST_DMA_CHANNEL_CONFIG_BY_IDX(inst, index)) ?	\
				DMA_ADDR_ADJ_INCREMENT : DMA_ADDR_ADJ_NO_CHANGE,	\
		.fifo_mode_control = STM32_DMA_FEATURES_FIFO_THRESHOLD(			\
				DT_INST_DMAS_CELL_BY_IDX(inst, index, features)),	\
	}

PINCTRL_DT_INST_DEFINE(0);

#define STM32_DCMI_GET_BUS_WIDTH(bus_width)						\
	((bus_width) == 8 ? DCMI_EXTEND_DATA_8B :					\
	(bus_width) == 10 ? DCMI_EXTEND_DATA_10B :					\
	(bus_width) == 12 ? DCMI_EXTEND_DATA_12B :					\
	(bus_width) == 14 ? DCMI_EXTEND_DATA_14B :					\
	DCMI_EXTEND_DATA_8B)

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
	.dma = {
		DCMI_DMA_CHANNEL_INIT(0, 0, PERIPHERAL, MEMORY),
	},
	.dma_m2m = {
		COND_CODE_1(DT_INST_DMAS_HAS_IDX(0, 1),
			    (DCMI_DMA_CHANNEL_INIT(0, 1, PERIPHERAL, MEMORY)), ())
	}
};

#define SOURCE_DEV(n) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(n, 0, 0)))

static const struct video_stm32_dcmi_config video_stm32_dcmi_config_0 = {
	.pclken = STM32_DT_INST_CLOCK_INFO(0),
	.irq_config = video_stm32_dcmi_irq_config_func,
	.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.sensor_dev = SOURCE_DEV(0),
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
	data->state = STM32_DCMI_STOPPED;

	/* Run IRQ init */
	config->irq_config(dev);

	/* Initialize DCMI peripheral */
	if (HAL_DCMI_Init(&data->hdcmi) != HAL_OK) {
		LOG_ERR("DCMI initialization failed.");
		return -EIO;
	}

	/* Avoid unused interrupts reasons */
	__HAL_DCMI_DISABLE_IT(&data->hdcmi, DCMI_IT_LINE | DCMI_IT_VSYNC | DCMI_IT_FRAME);

	k_sleep(K_MSEC(100));

	LOG_DBG("%s inited", dev->name);

	return err;
}

DEVICE_DT_INST_DEFINE(0, &video_stm32_dcmi_init,
		    NULL, &video_stm32_dcmi_data_0,
		    &video_stm32_dcmi_config_0,
		    POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,
		    &video_stm32_dcmi_driver_api);

VIDEO_DEVICE_DEFINE(dcmi, DEVICE_DT_INST_GET(0), SOURCE_DEV(0));
