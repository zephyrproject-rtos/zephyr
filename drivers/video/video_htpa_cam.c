/*
 * Copyright (c) 2026 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video/htpa_sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(htpa, CONFIG_VIDEO_LOG_LEVEL);

struct htpa_acquisition_conf {
	uint32_t id;
	uint8_t mbit_calib;
	uint8_t bias_calib;
	uint8_t clk_calib;
	uint8_t bpa_calib;
};

struct htpa_frame {
	uint8_t *top_offsets;
	uint8_t *bottom_offsets;
	uint8_t *top_pixels;
	uint8_t *bottom_pixels;
};

struct htpa_config {
	const struct video_format_cap *hm_htpa_caps;
	const struct device *calibration_flash;
	const struct video_htpa_raw_layout *sensor;
};

#define HTPA_CONVERSION_TIMEOUT_MS 100U
#define HTPA_FRAME_INTERVAL_US     DIV_ROUND_UP(USEC_PER_SEC, CONFIG_VIDEO_HTPA_CAM_MAX_FPS)
#define HTPA_FRAME_SIZE(block_count, block_length) (2U * ((block_count) + 1U) * (block_length))

#ifdef CONFIG_DCACHE
#define HTPA_RX_ALIGNMENT CONFIG_DCACHE_LINE_SIZE
#else
#define HTPA_RX_ALIGNMENT 4U
#endif
#define HTPA_MAX_BLOCK_SIZE MAX(VIDEO_HTPA_120X84_BLOCK_LENGTH, VIDEO_HTPA_160X120_BLOCK_LENGTH)
#define HTPA_RX_SIZE ROUND_UP(HTPA_MAX_BLOCK_SIZE, HTPA_RX_ALIGNMENT)

struct htpa_data {
	struct spi_dt_spec spi;

	struct video_format fmt;
	struct k_fifo framebuffer_take_queue;
	struct k_fifo framebuffer_release_queue;
	struct k_thread worker_thread;

	K_KERNEL_STACK_MEMBER(worker_stack, CONFIG_VIDEO_HTPA_CAM_WORKER_STACK_SIZE);
	struct k_sem worker_sem;
	struct k_mutex lock;
	struct k_condvar state_changed;
	struct video_buffer *in_flight;
	bool streaming;
	bool flushing;
	bool canceling;
	atomic_t in_flight_canceled;

	int communication_error;
	uint32_t communication_error_count;

	struct htpa_acquisition_conf acquisition_conf;

	int64_t next_frame_ticks;
	uint32_t acquisition_time;
	uint32_t frames_since_offsets;
	bool offsets_valid;
	uint8_t frame_marker;
	uint8_t top_offsets[HTPA_MAX_BLOCK_SIZE];
	uint8_t bottom_offsets[HTPA_MAX_BLOCK_SIZE];
	uint8_t rx_block[HTPA_RX_SIZE] __aligned(HTPA_RX_ALIGNMENT);
};

/*
 * Configuration register (write only):
 *
 * |  7  |  6  |  5  |  4  |   3   |    2     |   1   |    0   |
 * | RESERVED  |   Block   | Start | VDD_MEAS | BLIND | WAKEUP |
 */
#define CONFIGURATION_REGISTER 0x01

/*
 * Status register (read only):
 *
 * |  7  |  6  |  5  |  4  |   3   |    2     |   1   |   0   |
 * | RESERVED  |   Block   | RESERVED | VDD_MEAS | BLIND | EOC |
 */
#define STATUS_REGISTER 0x02

/*
 * Trim register 1 (write only):
 *
 * |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
 * | RESERVED  |  REF_CAL  |         MBIT          |
 */
#define TRIM_REGISTER1 0x03

/*
 * Trim register 2 (write only):
 *
 * |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
 * |    RESERVED     |       BIAS TRIM TOP         |
 */
#define TRIM_REGISTER2 0x04

/*
 * Trim register 3 (write only):
 *
 * |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
 * |    RESERVED     |       BIAS TRIM BOT         |
 */
#define TRIM_REGISTER3 0x05

/*
 * Trim register 4 (write only):
 *
 * |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
 * |    RESERVED     |             CLK TRIM        |
 */
#define TRIM_REGISTER4 0x06

/* Select the top-half sensor data for reading or writing. */
#define TOP_HALF_SENSOR_DATA_REGISTER 0x0A

/* Select the bottom-half sensor data for reading or writing. */
#define BOTTOM_HALF_SENSOR_DATA_REGISTER 0x0B

static uint8_t htpa_flash_read_bytewise(const struct device *dev, uint32_t address)
{
	const struct htpa_config *cfg = dev->config;
	struct htpa_data *data = dev->data;
	uint8_t value;
	int ret;

	ret = flash_read(cfg->calibration_flash, address, &value, sizeof(value));
	if (ret != 0) {
		data->communication_error = ret;
		data->communication_error_count++;
		return 0U;
	}

	return value;
}

static int htpa_flash_read_acquisition_conf(const struct device *dev,
					    struct htpa_acquisition_conf *acquisition_conf)
{
	enum {
		HTPA_FLASH_MBIT_OFFSET = 0x1a,
		HTPA_FLASH_BIAS_OFFSET = 0x1b,
		HTPA_FLASH_CLK_OFFSET = 0x1c,
		HTPA_FLASH_BPA_OFFSET = 0x1d,
		HTPA_FLASH_ID_OFFSET = 0x74,
	};
	const struct htpa_config *cfg = dev->config;
	struct htpa_data *data = dev->data;

	if (!device_is_ready(cfg->calibration_flash)) {
		LOG_ERR("Calibration flash is not ready");
		return -ENODEV;
	}

	acquisition_conf->id =
		(uint32_t)htpa_flash_read_bytewise(dev, HTPA_FLASH_ID_OFFSET + 3U) << 24 |
		(uint32_t)htpa_flash_read_bytewise(dev, HTPA_FLASH_ID_OFFSET + 2U) << 16 |
		(uint32_t)htpa_flash_read_bytewise(dev, HTPA_FLASH_ID_OFFSET + 1U) << 8 |
		(uint32_t)htpa_flash_read_bytewise(dev, HTPA_FLASH_ID_OFFSET);
	acquisition_conf->mbit_calib = htpa_flash_read_bytewise(dev, HTPA_FLASH_MBIT_OFFSET);
	acquisition_conf->bias_calib = htpa_flash_read_bytewise(dev, HTPA_FLASH_BIAS_OFFSET);
	acquisition_conf->clk_calib = htpa_flash_read_bytewise(dev, HTPA_FLASH_CLK_OFFSET);
	acquisition_conf->bpa_calib = htpa_flash_read_bytewise(dev, HTPA_FLASH_BPA_OFFSET);

	if ((data->acquisition_conf.id == 0U) || (data->acquisition_conf.id == UINT32_MAX)) {
		LOG_ERR("Invalid calibration data id");
		return -EINVAL;
	}

	return 0;
}

static int htpa_cam_write_reg(const struct device *dev, uint8_t reg, uint8_t value)
{
	struct htpa_data *data = dev->data;
	struct spi_buf tx_buf[2];
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};
	int ret;

	tx_buf[0].buf = &reg;
	tx_buf[0].len = 1;
	tx_buf[1].buf = &value;
	tx_buf[1].len = 1;

	ret = spi_transceive_dt(&data->spi, &tx, NULL);
	if (ret != 0) {
		data->communication_error = ret;
		LOG_ERR("%s, write sensor reg: %u, failed: %d", data->spi.bus->name, reg, ret);
		return -EIO;
	}

	return 0;
}

static int htpa_cam_read(const struct device *dev, uint8_t reg, uint8_t *rx_buffer, size_t rx_len)
{
	struct htpa_data *data = dev->data;
	struct spi_buf tx_buf = {
		.buf = &reg,
		.len = 1,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	struct spi_buf rx_buf[2] = {
		{
			.buf = NULL,
			.len = 1,
		},
		{
			.buf = rx_len > 1U ? data->rx_block : rx_buffer,
			.len = rx_len,
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};
	int ret;

	if (rx_len > sizeof(data->rx_block)) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_DCACHE) && rx_len > 1U) {
		ret = sys_cache_data_flush_and_invd_range(data->rx_block, sizeof(data->rx_block));
		if (ret != 0) {
			return ret;
		}
	}

	ret = spi_transceive_dt(&data->spi, &tx, &rx);
	if (ret != 0) {
		data->communication_error = ret;
		LOG_ERR("%s, read sensor reg: %u, failed: %d", data->spi.bus->name, reg, ret);
		return -EIO;
	}

	if (rx_len > 1U) {
		if (IS_ENABLED(CONFIG_DCACHE)) {
			/* Preserve CPU-written data when the SPI driver uses programmed I/O. */
			ret = sys_cache_data_flush_and_invd_range(data->rx_block,
								 sizeof(data->rx_block));
			if (ret != 0) {
				return ret;
			}
		}
		memcpy(rx_buffer, data->rx_block, rx_len);
	}

	return 0;
}

static int htpa_cam_read_reg(const struct device *dev, uint8_t reg, uint8_t *value)
{
	return htpa_cam_read(dev, reg, value, 1);
}

static int htpa_cam_has_errors_active(const struct device *dev)
{
	struct htpa_data *data = dev->data;

	if ((data->communication_error_count > 0) || (data->communication_error != 0)) {
		LOG_ERR("Communication error(s): %d during sensor readout",
			data->communication_error);
		data->communication_error_count = 0;
		data->communication_error = 0;
		return -EIO;
	}

	return 0;
}

static int htpa_cam_wakeup(const struct device *dev)
{
	struct htpa_data *data = dev->data;
	uint8_t status;
	uint32_t retry_counter = 100;

	if (htpa_cam_write_reg(dev, CONFIGURATION_REGISTER, 0x01) != 0) {
		LOG_ERR("Failed to wakeup sensor %s", dev->name);
		return -EINVAL;
	}

	do {
		int ret = htpa_cam_read_reg(dev, STATUS_REGISTER, &status);

		if (ret != 0) {
			return ret;
		}
		k_sleep(K_MSEC(1));
		retry_counter--;
		if (retry_counter == 0) {
			LOG_ERR("Failed to wakeup sensor %s", dev->name);
			return -EINVAL;
		}
	} while ((status & 0x01) == 0);

	if (htpa_cam_write_reg(dev, TRIM_REGISTER1, data->acquisition_conf.mbit_calib) != 0) {
		LOG_ERR("Failed to write TRIM_REGISTER1 %s", dev->name);
		return -EINVAL;
	}

	if (htpa_cam_write_reg(dev, TRIM_REGISTER2, data->acquisition_conf.bias_calib) != 0) {
		LOG_ERR("Failed to write TRIM_REGISTER2 %s", dev->name);
		return -EINVAL;
	}

	if (htpa_cam_write_reg(dev, TRIM_REGISTER3, data->acquisition_conf.bpa_calib) != 0) {
		LOG_ERR("Failed to write TRIM_REGISTER3 %s", dev->name);
		return -EINVAL;
	}

	if (htpa_cam_write_reg(dev, TRIM_REGISTER4, data->acquisition_conf.clk_calib) != 0) {
		LOG_ERR("Failed to write TRIM_REGISTER4 %s", dev->name);
		return -EINVAL;
	}

	return 0;
}

static void htpa_cam_mark_bit(uint8_t *pixels, const uint8_t *offsets, uint32_t width,
			      uint32_t x, int32_t value)
{
	for (uint32_t row = 0U; row < 2U; row++) {
		for (uint32_t col = 0U; col < 2U; col++) {
			uint32_t i = row * width + x + col;
			uint16_t offset = sys_get_be16(offsets + i * 2U);
			int32_t sample = CLAMP((int32_t)offset + value, 0, UINT16_MAX);

			sys_put_be16((uint16_t)sample, pixels + i * 2U);
		}
	}
}

static void htpa_cam_mark_blocks(const struct video_htpa_raw_layout *sensor,
				 const struct htpa_frame *raw, uint32_t width, uint8_t frame_marker)
{
	for (uint32_t half = 0U; half < 2U; half++) {
		const uint8_t *offsets =
			(half == 0U ? raw->top_offsets : raw->bottom_offsets) + sensor->data_offset;
		uint8_t *blocks = half == 0U ? raw->top_pixels : raw->bottom_pixels;

		for (uint32_t block = 0U; block < sensor->block_count; block++) {
			uint8_t *pixels =
				blocks + block * sensor->block_length + sensor->data_offset;

			for (uint32_t end = 0U; end < 2U; end++) {
				uint8_t marker = end == 0U ? (uint8_t)block : frame_marker;

				for (uint32_t bit = 0U; bit < 8U; bit++) {
					int32_t value =
						(marker & BIT(7U - bit)) != 0U ? 30000 : -30000;
					uint32_t x = (end == 0U ? 0U : width - 16U) + bit * 2U;

					htpa_cam_mark_bit(pixels, offsets, width, x, value);
				}
			}
		}
	}
}

static int htpa_grab_image(const struct device *dev, struct video_buffer *frame)
{
	const struct htpa_config *cfg = dev->config;
	const struct video_htpa_raw_layout *sensor = cfg->sensor;
	struct htpa_data *data = dev->data;
	struct htpa_frame raw = {
		.top_offsets = frame->buffer,
		.bottom_offsets = frame->buffer + sensor->block_length,
		.top_pixels = frame->buffer + 2U * sensor->block_length,
		.bottom_pixels = frame->buffer + (2U + sensor->block_count) * sensor->block_length,
	};
	struct {
		uint32_t write;
		uint32_t sleep;
		uint32_t poll;
		uint32_t top;
		uint32_t bottom;
		uint32_t loops;
	} timing[VIDEO_HTPA_160X120_BLOCK_COUNT + 1U];
	uint8_t status;
	uint8_t frame_marker = data->frame_marker;
	bool read_offsets = !data->offsets_valid ||
			    data->frames_since_offsets >= CONFIG_VIDEO_HTPA_CAM_OFFSET_INTERVAL;
	int ret;
	uint32_t frame_start = k_cycle_get_32();

	if (IS_ENABLED(CONFIG_VIDEO_HTPA_CAM_BLOCK_MARKERS)) {
		data->frame_marker++;
	}

	for (uint32_t block = 0; block < sensor->block_count + (read_offsets ? 1U : 0U); block++) {
		uint8_t *top_block;
		uint8_t *bottom_block;
		uint8_t configuration;
		uint32_t waiting_loops = 0;
		uint32_t start;

		if (block == sensor->block_count) {
			configuration = 0x0bU;
			top_block = raw.top_offsets;
			bottom_block = raw.bottom_offsets;
		} else {
			configuration = (uint8_t)(0x09U | (block << 4));
			top_block = raw.top_pixels + block * sensor->block_length;
			bottom_block = raw.bottom_pixels + block * sensor->block_length;
		}

		if (atomic_get(&data->in_flight_canceled) != 0) {
			return -ECANCELED;
		}

		start = k_cycle_get_32();
		ret = htpa_cam_write_reg(dev, CONFIGURATION_REGISTER, configuration);
		if (ret != 0) {
			return ret;
		}

		timing[block].write = k_cycle_get_32() - start;
		start = k_cycle_get_32();
		k_sleep(K_USEC(data->acquisition_time));
		timing[block].sleep = k_cycle_get_32() - start;
		start = k_cycle_get_32();
		status = 0U;
		int64_t deadline = k_uptime_get() + HTPA_CONVERSION_TIMEOUT_MS;

		while ((status & 0x1U) == 0U) {
			ret = htpa_cam_read_reg(dev, STATUS_REGISTER, &status);
			if (ret != 0) {
				return ret;
			}
			if ((status & 0x1U) != 0U) {
				continue;
			}
			if (k_uptime_get() >= deadline) {
				LOG_ERR("Sensor conversion timed out on block %u", block);
				return -ETIMEDOUT;
			}
			waiting_loops++;
			k_sleep(K_MSEC(1));
		}

		timing[block].poll = k_cycle_get_32() - start;
		timing[block].loops = waiting_loops;
		if (waiting_loops > 0) {
			data->acquisition_time =
				MIN(data->acquisition_time + 100U,
				    HTPA_CONVERSION_TIMEOUT_MS * USEC_PER_MSEC);
		}

		start = k_cycle_get_32();
		ret = htpa_cam_read(dev, TOP_HALF_SENSOR_DATA_REGISTER, top_block,
				     sensor->block_length);
		if (ret != 0) {
			return ret;
		}

		timing[block].top = k_cycle_get_32() - start;
		start = k_cycle_get_32();
		ret = htpa_cam_read(dev, BOTTOM_HALF_SENSOR_DATA_REGISTER, bottom_block,
				     sensor->block_length);
		if (ret != 0) {
			return ret;
		}
		timing[block].bottom = k_cycle_get_32() - start;
	}

	if (atomic_get(&data->in_flight_canceled) != 0) {
		return -ECANCELED;
	}

	if (read_offsets) {
		memcpy(data->top_offsets, raw.top_offsets, sensor->block_length);
		memcpy(data->bottom_offsets, raw.bottom_offsets, sensor->block_length);
		data->offsets_valid = true;
		data->frames_since_offsets = 0U;
	} else {
		memcpy(raw.top_offsets, data->top_offsets, sensor->block_length);
		memcpy(raw.bottom_offsets, data->bottom_offsets, sensor->block_length);
	}
	data->frames_since_offsets++;

	if (IS_ENABLED(CONFIG_VIDEO_HTPA_CAM_BLOCK_MARKERS)) {
		htpa_cam_mark_blocks(sensor, &raw, data->fmt.width, frame_marker);
	}

	LOG_INF("Frame acquisition: %u us", k_cyc_to_us_floor32(k_cycle_get_32() - frame_start));
	for (uint32_t block = 0U; block < sensor->block_count + (read_offsets ? 1U : 0U);
	     block++) {
		LOG_DBG("Block %u%s: write %u us, sleep %u us, poll %u us (%u retries), "
			"top %u us, bottom %u us", block,
			block == sensor->block_count ? " (offsets)" : "",
			k_cyc_to_us_floor32(timing[block].write),
			k_cyc_to_us_floor32(timing[block].sleep),
			k_cyc_to_us_floor32(timing[block].poll), timing[block].loops,
			k_cyc_to_us_floor32(timing[block].top),
			k_cyc_to_us_floor32(timing[block].bottom));
	}
	return 0;
}

static void hm_htpa_worker(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct htpa_data *data = dev->data;
	struct video_buffer *vbuf;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&data->worker_sem, K_FOREVER);

		while (true) {
			int ret;

			k_mutex_lock(&data->lock, K_FOREVER);
			if (data->canceling || (!data->streaming && !data->flushing)) {
				k_mutex_unlock(&data->lock);
				break;
			}

			vbuf = k_fifo_get(&data->framebuffer_take_queue, K_NO_WAIT);
			if (vbuf == NULL) {
				k_condvar_broadcast(&data->state_changed);
				k_mutex_unlock(&data->lock);
				break;
			}

			data->in_flight = vbuf;
			atomic_clear(&data->in_flight_canceled);
			k_mutex_unlock(&data->lock);

			if (data->next_frame_ticks > k_uptime_ticks()) {
				k_sleep(K_TIMEOUT_ABS_TICKS(data->next_frame_ticks));
			}
			data->next_frame_ticks = k_uptime_ticks() +
				k_us_to_ticks_ceil64(HTPA_FRAME_INTERVAL_US);
			ret = atomic_get(&data->in_flight_canceled) != 0 ?
				-ECANCELED : htpa_grab_image(dev, vbuf);
			if (htpa_cam_has_errors_active(dev) != 0 && ret == 0) {
				ret = -EIO;
			}

			k_mutex_lock(&data->lock, K_FOREVER);
			if (ret != 0 || atomic_get(&data->in_flight_canceled)) {
				vbuf->bytesused = 0;
				vbuf->timestamp = k_uptime_get_32();
				if (ret != 0 && ret != -ECANCELED) {
					LOG_ERR("Frame acquisition failed: %d", ret);
				}
			} else {
				vbuf->bytesused = data->fmt.size;
				vbuf->timestamp = k_uptime_get_32();
			}

			k_fifo_put(&data->framebuffer_release_queue, vbuf);
			data->in_flight = NULL;
			k_condvar_broadcast(&data->state_changed);
			k_mutex_unlock(&data->lock);
		}
	}
}

static int htpa_api_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct htpa_config *cfg = dev->config;

	caps->type = VIDEO_BUF_TYPE_OUTPUT;
	caps->format_caps = cfg->hm_htpa_caps;
	caps->min_vbuf_count = 1;
	caps->buf_align = sizeof(uint16_t);

	return 0;
}

static int htpa_api_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct htpa_data *data = dev->data;

	if (fmt->type != data->fmt.type) {
		return -ENOTSUP;
	}

	if (fmt->pixelformat != data->fmt.pixelformat) {
		return -ENOTSUP;
	}

	if (fmt->width != data->fmt.width || fmt->height != data->fmt.height) {
		return -ENOTSUP;
	}

	fmt->pitch = data->fmt.pitch;
	fmt->size = data->fmt.size;

	memcpy(&data->fmt, fmt, sizeof(data->fmt));

	return 0;
}

static int htpa_api_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct htpa_data *data = dev->data;

	memcpy(fmt, &data->fmt, sizeof(*fmt));

	return 0;
}

static int htpa_api_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct htpa_data *data = dev->data;

	if (type != VIDEO_BUF_TYPE_OUTPUT) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->streaming = enable;
	LOG_INF("SPI IR camera stream %s", enable ? "enabled" : "disabled");

	if (enable) {
		k_sem_give(&data->worker_sem);
	}
	k_mutex_unlock(&data->lock);

	return 0;
}

static int htpa_api_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct htpa_data *data = dev->data;
	struct video_format *fmt = &data->fmt;

	if (vbuf->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	if (vbuf->size < fmt->size) {
		return -EINVAL;
	}
	if (!IS_ALIGNED((uintptr_t)vbuf->buffer, sizeof(uint16_t))) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	if (data->flushing) {
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}
	vbuf->bytesused = 0;
	vbuf->timestamp = 0;
	k_fifo_put(&data->framebuffer_take_queue, vbuf);

	if (data->streaming) {
		k_sem_give(&data->worker_sem);
	}
	k_mutex_unlock(&data->lock);

	return 0;
}

static int htpa_api_dequeue(const struct device *dev, struct video_buffer **vbuf,
			    k_timeout_t timeout)
{
	struct htpa_data *data = dev->data;

	*vbuf = k_fifo_get(&data->framebuffer_release_queue, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int htpa_api_flush(const struct device *dev, bool cancel)
{
	struct htpa_data *data = dev->data;
	struct video_buffer *vbuf;

	k_mutex_lock(&data->lock, K_FOREVER);
	while (data->flushing) {
		k_condvar_wait(&data->state_changed, &data->lock, K_FOREVER);
	}

	data->flushing = true;
	data->canceling = cancel;
	if (cancel) {
		if (data->in_flight != NULL) {
			atomic_set(&data->in_flight_canceled, 1);
		}

		while ((vbuf = k_fifo_get(&data->framebuffer_take_queue, K_NO_WAIT)) != NULL) {
			vbuf->bytesused = 0;
			vbuf->timestamp = k_uptime_get_32();
			k_fifo_put(&data->framebuffer_release_queue, vbuf);
		}
	}

	k_sem_give(&data->worker_sem);
	while (data->in_flight != NULL ||
	       (!cancel && !k_fifo_is_empty(&data->framebuffer_take_queue))) {
		k_condvar_wait(&data->state_changed, &data->lock, K_FOREVER);
	}

	data->canceling = false;
	data->flushing = false;
	k_condvar_broadcast(&data->state_changed);
	k_mutex_unlock(&data->lock);

	return 0;
}

static int htpa_api_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	ARG_UNUSED(dev);
	*frmival = (struct video_frmival){.numerator = 1U,
					  .denominator = CONFIG_VIDEO_HTPA_CAM_MAX_FPS};
	return 0;
}

static int htpa_api_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	uint64_t ns;

	if (frmival->denominator == 0U) {
		return -EINVAL;
	}
	ns = video_frmival_nsec(frmival);
	/* UVC represents frame intervals in units of 100 ns. */
	if (ns < NSEC_PER_SEC / CONFIG_VIDEO_HTPA_CAM_MAX_FPS - 100U ||
	    ns > NSEC_PER_SEC / CONFIG_VIDEO_HTPA_CAM_MAX_FPS + 100U) {
		return -ENOTSUP;
	}
	return htpa_api_get_frmival(dev, frmival);
}

static int htpa_api_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	struct htpa_data *data = dev->data;

	if (fie->index != 0U || fie->format->pixelformat != data->fmt.pixelformat ||
	    fie->format->width != data->fmt.width || fie->format->height != data->fmt.height) {
		return -EINVAL;
	}
	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	return htpa_api_get_frmival(dev, &fie->discrete);
}

static DEVICE_API(video, htpa_api) = {
	.set_format = htpa_api_set_fmt,
	.get_format = htpa_api_get_fmt,
	.set_frmival = htpa_api_set_frmival,
	.get_frmival = htpa_api_get_frmival,
	.enum_frmival = htpa_api_enum_frmival,
	.set_stream = htpa_api_set_stream,
	.get_caps = htpa_api_get_caps,
	.enqueue = htpa_api_enqueue,
	.dequeue = htpa_api_dequeue,
	.flush = htpa_api_flush,
};

static int htpa_cam_init(const struct device *dev)
{
	const struct htpa_config *cfg = dev->config;
	struct htpa_data *data = dev->data;
	struct video_format default_fmt = {
		.type = VIDEO_BUF_TYPE_OUTPUT,
		.pixelformat = cfg->hm_htpa_caps[0].pixelformat,
		.width = cfg->hm_htpa_caps[0].width_max,
		.height = cfg->hm_htpa_caps[0].height_max,
		.pitch = 0,
		.size = HTPA_FRAME_SIZE(cfg->sensor->block_count, cfg->sensor->block_length),
	};

	data->communication_error = 0;
	data->communication_error_count = 0;

	if (!spi_is_ready_dt(&data->spi)) {
		LOG_ERR("SPI bus for sensor and flash not ready");
		return -ENODEV;
	}

	if (htpa_flash_read_acquisition_conf(dev, &data->acquisition_conf) != 0) {
		LOG_ERR("Error reading calibration data");
		return -EINVAL;
	}

	LOG_INF("Calibration data id: %d", data->acquisition_conf.id);

	/*
	 * The CS pin (PA24) is shared:
	 *   LOW  = camera deselected, flash selected
	 *   HIGH = camera selected, flash deselected
	 *
	 * The flash is no longer needed, so leave the line configured
	 * according to the camera CS definition.
	 */
	data->spi.config.cs.gpio.dt_flags &= ~GPIO_ACTIVE_LOW;

	int ret = gpio_pin_configure_dt(&data->spi.config.cs.gpio, GPIO_OUTPUT_ACTIVE);

	if (ret < 0) {
		return ret;
	}

	if (htpa_cam_wakeup(dev) != 0 || htpa_cam_has_errors_active(dev) != 0) {
		LOG_ERR("Failed to initialize sensor %s", dev->name);
		return -EINVAL;
	}

	LOG_INF("HTPA camera device is ready");

	memcpy(&data->fmt, &default_fmt, sizeof(data->fmt));
	k_fifo_init(&data->framebuffer_take_queue);
	k_fifo_init(&data->framebuffer_release_queue);
	k_sem_init(&data->worker_sem, 0, 1);
	k_mutex_init(&data->lock);
	k_condvar_init(&data->state_changed);
	data->in_flight = NULL;
	data->streaming = false;
	data->flushing = false;
	data->canceling = false;
	atomic_clear(&data->in_flight_canceled);
	k_thread_create(&data->worker_thread, data->worker_stack,
			K_THREAD_STACK_SIZEOF(data->worker_stack), hm_htpa_worker, (void *)dev,
			NULL, NULL, CONFIG_VIDEO_HTPA_CAM_WORKER_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&data->worker_thread, "htpa_worker");

	LOG_INF("SPI IR camera initialized on %s", data->spi.bus->name);

	return 0;
}

#define HTPA_DEFAULT_ACQUISITION_TIME_USEC 1000

#define HM_HTPA_DEFINE(inst, model, caps, sensor_cfg)                                              \
	static const struct htpa_config hm_htpa_cfg_##model##_##inst = {                           \
		.hm_htpa_caps = caps,                                                              \
		.calibration_flash = DEVICE_DT_GET(DT_INST_PHANDLE(inst, calibration_flash)),      \
		.sensor = sensor_cfg,                                                              \
	};                                                                                         \
	static struct htpa_data hm_htpa_data_##model##_##inst = {                                  \
		.acquisition_time = HTPA_DEFAULT_ACQUISITION_TIME_USEC,                            \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |           \
							  SPI_LINES_SINGLE),                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, htpa_cam_init, NULL, &hm_htpa_data_##model##_##inst,           \
			      &hm_htpa_cfg_##model##_##inst, POST_KERNEL,                          \
			      CONFIG_VIDEO_INIT_PRIORITY, &htpa_api);

#if defined(CONFIG_DT_HAS_HEIMANN_HTPA_120X84_CAMERA_ENABLED)

#define DT_DRV_COMPAT heimann_htpa_120x84_camera

static const struct video_htpa_raw_layout hm_htpa_sensor_cfg_120x84 =
	VIDEO_HTPA_120X84_RAW_LAYOUT;

static const struct video_format_cap hm_htpa_caps_120x84[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_HTPA_RAW,
		.width_min = VIDEO_HTPA_120X84_WIDTH,
		.width_max = VIDEO_HTPA_120X84_WIDTH,
		.height_min = VIDEO_HTPA_120X84_HEIGHT,
		.height_max = VIDEO_HTPA_120X84_HEIGHT,
		.width_step = 0,
		.height_step = 0,
	},
	{0},
};

#define HM_HTPA_INIT_120X84(inst)                                                                  \
	HM_HTPA_DEFINE(inst, htpa_120x84, hm_htpa_caps_120x84, &hm_htpa_sensor_cfg_120x84)

DT_INST_FOREACH_STATUS_OKAY(HM_HTPA_INIT_120X84)

#undef DT_DRV_COMPAT

#endif /* CONFIG_DT_HAS_HEIMANN_HTPA_120X84_CAMERA_ENABLED */

#if defined(CONFIG_DT_HAS_HEIMANN_HTPA_160X120_CAMERA_ENABLED)

#define DT_DRV_COMPAT heimann_htpa_160x120_camera

static const struct video_htpa_raw_layout hm_htpa_sensor_cfg_160x120 =
	VIDEO_HTPA_160X120_RAW_LAYOUT;

static const struct video_format_cap hm_htpa_caps_160x120[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_HTPA_RAW,
		.width_min = VIDEO_HTPA_160X120_WIDTH,
		.width_max = VIDEO_HTPA_160X120_WIDTH,
		.height_min = VIDEO_HTPA_160X120_HEIGHT,
		.height_max = VIDEO_HTPA_160X120_HEIGHT,
		.width_step = 0,
		.height_step = 0,
	},
	{0},
};

#define HM_HTPA_INIT_160X120(inst)                                                                 \
	HM_HTPA_DEFINE(inst, htpa_160x120, hm_htpa_caps_160x120, &hm_htpa_sensor_cfg_160x120)

DT_INST_FOREACH_STATUS_OKAY(HM_HTPA_INIT_160X120)

#undef DT_DRV_COMPAT

#endif /* CONFIG_DT_HAS_HEIMANN_HTPA_160X120_CAMERA_ENABLED */
