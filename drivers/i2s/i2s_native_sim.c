/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_native_sim_i2s

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <nsi_errno.h>
#include <nsi_host_trampolines.h>
#include <nsi_hw_scheduler.h>

#include "cmdline.h"
#include "i2s_native_sim_bottom.h"
#include "soc.h"

#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

enum ns_i2s_mode {
	NS_I2S_MODE_RX,
	NS_I2S_MODE_TX,
	NS_I2S_MODE_BOTH,
};

enum ns_i2s_stop_mode {
	NS_I2S_STOP_NONE,
	NS_I2S_STOP_AT_BLOCK,
	NS_I2S_DRAIN_QUEUE,
};

struct ns_i2s_block {
	void *mem_block;
	size_t size;
};

struct ns_i2s_stream {
	struct i2s_config cfg;
	enum i2s_state state;
	enum ns_i2s_stop_mode stop_mode;
	struct k_msgq queue;
	char queue_buffer[CONFIG_I2S_NATIVE_SIM_QUEUE_SIZE * sizeof(struct ns_i2s_block)];
	struct ns_i2s_block active_block;
	int fd;
	int last_error;
	uint64_t next_deadline_us;
	bool missing_file;
	bool active_block_valid;
	bool configured;
};

struct ns_i2s_config {
	enum ns_i2s_mode mode;
	const char *rx_path;
	const char *tx_path;
};

struct ns_i2s_data {
	struct k_mutex lock;
	struct k_sem worker_sem;
	struct k_thread worker_thread;

	K_KERNEL_STACK_MEMBER(worker_stack, CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);
	struct ns_i2s_stream rx;
	struct ns_i2s_stream tx;
	const char *cmd_rx_path;
	const char *cmd_tx_path;
};

static const char *ns_i2s_active_path(const struct ns_i2s_config *cfg,
				      const struct ns_i2s_data *data, enum i2s_dir dir)
{
	if ((dir == I2S_DIR_RX) && (data->cmd_rx_path != NULL)) {
		return data->cmd_rx_path;
	}

	if ((dir == I2S_DIR_TX) && (data->cmd_tx_path != NULL)) {
		return data->cmd_tx_path;
	}

	return (dir == I2S_DIR_RX) ? cfg->rx_path : cfg->tx_path;
}

static uint32_t ns_i2s_word_size_bytes(uint8_t word_size)
{
	if (word_size <= 8U) {
		return 1U;
	}

	if (word_size <= 16U) {
		return 2U;
	}

	return 4U;
}

static uint32_t ns_i2s_frame_bytes(const struct i2s_config *cfg)
{
	uint8_t channels = cfg->channels;

	if ((cfg->format & I2S_FMT_DATA_FORMAT_MASK) == I2S_FMT_DATA_FORMAT_I2S) {
		channels = 2U;
	}

	if (channels == 0U) {
		return 0U;
	}

	return channels * ns_i2s_word_size_bytes(cfg->word_size);
}

static uint64_t ns_i2s_bytes_per_sec(const struct i2s_config *cfg)
{
	return (uint64_t)cfg->frame_clk_freq * ns_i2s_frame_bytes(cfg);
}

static bool ns_i2s_mode_supports_rx(const struct ns_i2s_config *cfg)
{
	return (cfg->mode != NS_I2S_MODE_TX);
}

static bool ns_i2s_mode_supports_tx(const struct ns_i2s_config *cfg)
{
	return (cfg->mode != NS_I2S_MODE_RX);
}

static bool ns_i2s_supports_dir(const struct ns_i2s_config *cfg, enum i2s_dir dir)
{
	switch (dir) {
	case I2S_DIR_RX:
		return ns_i2s_mode_supports_rx(cfg);
	case I2S_DIR_TX:
		return ns_i2s_mode_supports_tx(cfg);
	case I2S_DIR_BOTH:
		return ns_i2s_mode_supports_rx(cfg) && ns_i2s_mode_supports_tx(cfg);
	default:
		return false;
	}
}

static struct ns_i2s_stream *ns_i2s_stream_get(struct ns_i2s_data *data, enum i2s_dir dir)
{
	if (dir == I2S_DIR_RX) {
		return &data->rx;
	}

	if (dir == I2S_DIR_TX) {
		return &data->tx;
	}

	return NULL;
}

static void ns_i2s_close_file(struct ns_i2s_stream *stream)
{
	if (stream->fd >= 0) {
		(void)nsi_host_close(stream->fd);
		stream->fd = -1;
	}
}

static void ns_i2s_free_block(const struct ns_i2s_stream *stream, const struct ns_i2s_block *block)
{
	if ((stream->cfg.mem_slab != NULL) && (block->mem_block != NULL)) {
		k_mem_slab_free(stream->cfg.mem_slab, block->mem_block);
	}
}

static void ns_i2s_flush_queue(struct ns_i2s_stream *stream)
{
	struct ns_i2s_block block;

	if (stream->active_block_valid) {
		ns_i2s_free_block(stream, &stream->active_block);
		memset(&stream->active_block, 0, sizeof(stream->active_block));
		stream->active_block_valid = false;
	}

	while (k_msgq_get(&stream->queue, &block, K_NO_WAIT) == 0) {
		ns_i2s_free_block(stream, &block);
	}
}

static void ns_i2s_stream_ready(struct ns_i2s_stream *stream)
{
	stream->state = I2S_STATE_READY;
	stream->stop_mode = NS_I2S_STOP_NONE;
	stream->last_error = 0;
	stream->missing_file = false;
	stream->next_deadline_us = 0U;
}

static void ns_i2s_stream_reset(struct ns_i2s_stream *stream, bool configured)
{
	ns_i2s_close_file(stream);
	ns_i2s_flush_queue(stream);
	memset(&stream->cfg, 0, sizeof(stream->cfg));
	stream->stop_mode = NS_I2S_STOP_NONE;
	stream->last_error = 0;
	stream->missing_file = false;
	stream->next_deadline_us = 0U;
	stream->configured = configured;
	stream->state = configured ? I2S_STATE_READY : I2S_STATE_NOT_READY;
}

static void ns_i2s_stream_error(struct ns_i2s_stream *stream, int error)
{
	stream->state = I2S_STATE_ERROR;
	stream->stop_mode = NS_I2S_STOP_NONE;
	stream->last_error = error;
	stream->next_deadline_us = 0U;
	ns_i2s_close_file(stream);
}

static void ns_i2s_stream_recover(struct ns_i2s_stream *stream)
{
	ns_i2s_close_file(stream);
	ns_i2s_flush_queue(stream);
	ns_i2s_stream_ready(stream);
}

static bool ns_i2s_loopback_enabled(struct ns_i2s_data *data)
{
	return data->rx.configured && data->tx.configured &&
	       ((data->tx.cfg.options & I2S_OPT_LOOPBACK) != 0U);
}

static void ns_i2s_pace(struct ns_i2s_stream *stream, size_t size)
{
	uint64_t deadline;
	uint64_t duration_us;
	uint64_t bytes_per_sec;
	uint64_t now;

	bytes_per_sec = ns_i2s_bytes_per_sec(&stream->cfg);
	deadline = stream->next_deadline_us;
	now = nsi_hws_get_time();

	if ((deadline == 0U) || (deadline < now)) {
		deadline = now;
	}

	if (bytes_per_sec == 0U) {
		duration_us = 0U;
	} else {
		duration_us = DIV_ROUND_UP((uint64_t)size * USEC_PER_SEC, bytes_per_sec);
	}

	stream->next_deadline_us = deadline + duration_us;

	if (deadline > now) {
		uint32_t sleep_us = (uint32_t)MIN(deadline - now, (uint64_t)UINT32_MAX);

		if (sleep_us > 0U) {
			k_usleep(sleep_us);
		}
	}
}

static int ns_i2s_prepare_file(const struct ns_i2s_config *cfg, struct ns_i2s_data *data,
			       struct ns_i2s_stream *stream, enum i2s_dir dir)
{
	const char *path = ns_i2s_active_path(cfg, data, dir);
	int fd;

	if ((stream->fd >= 0) || stream->missing_file) {
		return 0;
	}

	fd = ns_i2s_open_file_bottom(path, dir == I2S_DIR_RX);

	if (fd < 0) {
		stream->missing_file = true;
		if ((dir == I2S_DIR_RX) && ((fd == -ENOENT) || (fd == -ENOTDIR))) {
			stream->next_deadline_us = nsi_hws_get_time();
			stream->last_error = 0;
		}
		return 0;
	}

	stream->fd = fd;
	stream->missing_file = false;
	stream->next_deadline_us = nsi_hws_get_time();
	stream->last_error = 0;

	return 0;
}

static int ns_i2s_copy_to_rx(struct ns_i2s_data *data, const struct ns_i2s_block *src)
{
	struct ns_i2s_block rx_block = {0};
	int ret;

	if ((data->rx.state == I2S_STATE_NOT_READY) || (data->rx.state == I2S_STATE_ERROR)) {
		return 0;
	}

	ret = k_mem_slab_alloc(data->rx.cfg.mem_slab, &rx_block.mem_block, K_NO_WAIT);
	if (ret < 0) {
		ns_i2s_stream_error(&data->rx, -EIO);
		return -EIO;
	}

	rx_block.size = MIN(src->size, data->rx.cfg.block_size);
	memcpy(rx_block.mem_block, src->mem_block, rx_block.size);
	ret = k_msgq_put(&data->rx.queue, &rx_block, K_NO_WAIT);
	if (ret < 0) {
		ns_i2s_free_block(&data->rx, &rx_block);
		ns_i2s_stream_error(&data->rx, -EIO);
		return ret;
	}

	return 0;
}

static bool ns_i2s_process_tx(const struct device *dev)
{
	const struct ns_i2s_config *cfg = dev->config;
	struct ns_i2s_data *data = dev->data;
	ssize_t written;
	int ret;
	bool keep_running;

	k_mutex_lock(&data->lock, K_FOREVER);

	if ((data->tx.state != I2S_STATE_RUNNING) && (data->tx.state != I2S_STATE_STOPPING)) {
		k_mutex_unlock(&data->lock);
		return false;
	}

	if (!data->tx.active_block_valid) {
		ret = k_msgq_get(&data->tx.queue, &data->tx.active_block, K_NO_WAIT);
		if (ret < 0) {
			if (data->tx.state == I2S_STATE_STOPPING) {
				ns_i2s_stream_ready(&data->tx);
			} else {
				ns_i2s_stream_error(&data->tx, -EIO);
			}
			k_mutex_unlock(&data->lock);
			return false;
		}

		data->tx.active_block_valid = true;
	}

	ns_i2s_pace(&data->tx, data->tx.active_block.size);

	(void)ns_i2s_prepare_file(cfg, data, &data->tx, I2S_DIR_TX);

	if (!data->tx.missing_file) {
		written = nsi_host_write(data->tx.fd, data->tx.active_block.mem_block,
					 data->tx.active_block.size);
		if ((written < 0) || ((size_t)written != data->tx.active_block.size)) {
			ns_i2s_free_block(&data->tx, &data->tx.active_block);
			memset(&data->tx.active_block, 0, sizeof(data->tx.active_block));
			data->tx.active_block_valid = false;
			ns_i2s_stream_error(&data->tx, -EIO);
			k_mutex_unlock(&data->lock);
			return false;
		}
	}

	if (ns_i2s_loopback_enabled(data)) {
		(void)ns_i2s_copy_to_rx(data, &data->tx.active_block);
	}

	ns_i2s_free_block(&data->tx, &data->tx.active_block);
	memset(&data->tx.active_block, 0, sizeof(data->tx.active_block));
	data->tx.active_block_valid = false;

	if ((data->tx.state == I2S_STATE_STOPPING) &&
	    (data->tx.stop_mode == NS_I2S_STOP_AT_BLOCK)) {
		ns_i2s_stream_ready(&data->tx);
		k_mutex_unlock(&data->lock);
		return false;
	}

	if ((data->tx.state == I2S_STATE_STOPPING) && (data->tx.stop_mode == NS_I2S_DRAIN_QUEUE) &&
	    (k_msgq_num_used_get(&data->tx.queue) == 0U)) {
		ns_i2s_stream_ready(&data->tx);
		k_mutex_unlock(&data->lock);
		return false;
	}

	if ((data->tx.state == I2S_STATE_RUNNING) && (k_msgq_num_used_get(&data->tx.queue) == 0U)) {
		ns_i2s_stream_error(&data->tx, -EIO);
		k_mutex_unlock(&data->lock);
		return false;
	}

	keep_running = data->tx.active_block_valid || (k_msgq_num_used_get(&data->tx.queue) > 0U);
	k_mutex_unlock(&data->lock);

	return keep_running;
}

static bool ns_i2s_process_rx_file(const struct device *dev)
{
	const struct ns_i2s_config *cfg = dev->config;
	struct ns_i2s_data *data = dev->data;
	struct ns_i2s_block block = {0};
	ssize_t bytes_read;
	int ret;
	bool keep_running = true;

	k_mutex_lock(&data->lock, K_FOREVER);

	if ((data->rx.state != I2S_STATE_RUNNING) && (data->rx.state != I2S_STATE_STOPPING)) {
		k_mutex_unlock(&data->lock);
		return false;
	}

	ret = k_mem_slab_alloc(data->rx.cfg.mem_slab, &block.mem_block, K_NO_WAIT);
	if (ret < 0) {
		ns_i2s_stream_error(&data->rx, -EIO);
		k_mutex_unlock(&data->lock);
		return false;
	}

	ns_i2s_pace(&data->rx, data->rx.cfg.block_size);

	(void)ns_i2s_prepare_file(cfg, data, &data->rx, I2S_DIR_RX);

	if (data->rx.missing_file) {
		memset(block.mem_block, 0, data->rx.cfg.block_size);
		block.size = data->rx.cfg.block_size;
		ret = k_msgq_put(&data->rx.queue, &block, K_NO_WAIT);
		if (ret < 0) {
			ns_i2s_free_block(&data->rx, &block);
			ns_i2s_stream_error(&data->rx, -EIO);
			k_mutex_unlock(&data->lock);
			return false;
		}

		if (data->rx.state == I2S_STATE_STOPPING) {
			ns_i2s_stream_ready(&data->rx);
			keep_running = false;
		}

		k_mutex_unlock(&data->lock);
		return keep_running;
	}

	bytes_read = nsi_host_read(data->rx.fd, block.mem_block, data->rx.cfg.block_size);
	if (bytes_read == 0) {
		ns_i2s_free_block(&data->rx, &block);
		ns_i2s_stream_error(&data->rx, -ENOMSG);
		k_mutex_unlock(&data->lock);
		return false;
	}

	if (bytes_read < 0) {
		ns_i2s_free_block(&data->rx, &block);
		ns_i2s_stream_error(&data->rx, -EIO);
		k_mutex_unlock(&data->lock);
		return false;
	}

	block.size = (size_t)bytes_read;
	ret = k_msgq_put(&data->rx.queue, &block, K_NO_WAIT);
	if (ret < 0) {
		ns_i2s_free_block(&data->rx, &block);
		ns_i2s_stream_error(&data->rx, -EIO);
		k_mutex_unlock(&data->lock);
		return false;
	}

	if (data->rx.state == I2S_STATE_STOPPING) {
		ns_i2s_stream_ready(&data->rx);
		keep_running = false;
	}

	k_mutex_unlock(&data->lock);

	return keep_running;
}

static void ns_i2s_worker(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	const struct ns_i2s_config *cfg = dev->config;
	struct ns_i2s_data *data = dev->data;
	bool keep_running;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		(void)k_sem_take(&data->worker_sem, K_FOREVER);

		while (true) {
			keep_running = false;

			if (ns_i2s_mode_supports_tx(cfg) &&
			    ((data->tx.state == I2S_STATE_RUNNING) ||
			     (data->tx.state == I2S_STATE_STOPPING))) {
				keep_running = ns_i2s_process_tx(dev);
			}

			if (!ns_i2s_loopback_enabled(data) && ns_i2s_mode_supports_rx(cfg) &&
			    ((data->rx.state == I2S_STATE_RUNNING) ||
			     (data->rx.state == I2S_STATE_STOPPING))) {
				keep_running = ns_i2s_process_rx_file(dev) || keep_running;
			}

			if (!keep_running) {
				break;
			}
		}
	}
}

static int ns_i2s_validate_cfg(const struct ns_i2s_config *cfg, enum i2s_dir dir,
			       const struct i2s_config *i2s_cfg)
{
	i2s_fmt_t data_format;
	uint32_t frame_bytes;

	if (!ns_i2s_supports_dir(cfg, dir)) {
		return -ENOTSUP;
	}

	if ((i2s_cfg->mem_slab == NULL) || (i2s_cfg->block_size == 0U) ||
	    (i2s_cfg->frame_clk_freq == 0U) || (i2s_cfg->word_size == 0U)) {
		return -EINVAL;
	}

	if ((i2s_cfg->format & I2S_FMT_DATA_ORDER_LSB) != 0U) {
		return -EINVAL;
	}

	data_format = i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK;
	switch (data_format) {
	case I2S_FMT_DATA_FORMAT_I2S:
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		break;
	default:
		return -EINVAL;
	}

	if ((data_format == I2S_FMT_DATA_FORMAT_I2S) && (i2s_cfg->channels != 2U)) {
		return -EINVAL;
	}

	frame_bytes = ns_i2s_frame_bytes(i2s_cfg);
	if ((frame_bytes == 0U) || ((i2s_cfg->block_size % frame_bytes) != 0U)) {
		return -EINVAL;
	}

	return 0;
}

static int ns_i2s_configure(const struct device *dev, enum i2s_dir dir,
			    const struct i2s_config *i2s_cfg)
{
	const struct ns_i2s_config *cfg = dev->config;
	struct ns_i2s_data *data = dev->data;
	struct ns_i2s_stream *stream;
	int ret;

	if ((dir == I2S_DIR_BOTH) || !ns_i2s_supports_dir(cfg, dir)) {
		return (dir == I2S_DIR_BOTH) ? -ENOSYS : -EINVAL;
	}

	if (i2s_cfg == NULL) {
		return -EINVAL;
	}

	stream = ns_i2s_stream_get(data, dir);
	if (stream == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (stream->state == I2S_STATE_RUNNING) {
		k_mutex_unlock(&data->lock);
		return -EIO;
	}

	if (i2s_cfg->frame_clk_freq == 0U) {
		ns_i2s_stream_reset(stream, false);
		k_mutex_unlock(&data->lock);
		return 0;
	}

	ret = ns_i2s_validate_cfg(cfg, dir, i2s_cfg);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ns_i2s_stream_reset(stream, true);
	stream->cfg = *i2s_cfg;
	stream->next_deadline_us = 0U;
	k_mutex_unlock(&data->lock);

	return 0;
}

static const struct i2s_config *ns_i2s_config_get(const struct device *dev, enum i2s_dir dir)
{
	const struct ns_i2s_config *cfg = dev->config;
	struct ns_i2s_data *data = dev->data;
	struct ns_i2s_stream *stream;
	const struct i2s_config *ret = NULL;

	if (!ns_i2s_supports_dir(cfg, dir) || (dir == I2S_DIR_BOTH)) {
		return NULL;
	}

	stream = ns_i2s_stream_get(data, dir);
	if ((stream != NULL) && (stream->state != I2S_STATE_NOT_READY)) {
		ret = &stream->cfg;
	}

	return ret;
}

static int ns_i2s_read(const struct device *dev, void **mem_block, size_t *size)
{
	const struct ns_i2s_config *cfg = dev->config;
	struct ns_i2s_data *data = dev->data;
	struct ns_i2s_block block;
	k_timeout_t timeout;
	struct ns_i2s_stream *stream = &data->rx;

	if (!ns_i2s_mode_supports_rx(cfg) || (mem_block == NULL) || (size == NULL)) {
		return -EINVAL;
	}

	if (stream->state == I2S_STATE_NOT_READY) {
		return -EIO;
	}

	if (stream->state == I2S_STATE_ERROR) {
		timeout = K_NO_WAIT;
	} else {
		timeout = SYS_TIMEOUT_MS(stream->cfg.timeout);
	}
	if (k_msgq_get(&stream->queue, &block, timeout) != 0) {
		if (stream->state == I2S_STATE_ERROR) {
			return (stream->last_error == -ENOMSG) ? -ENOMSG : -EIO;
		}

		return -EAGAIN;
	}

	*mem_block = block.mem_block;
	*size = block.size;

	if ((stream->state == I2S_STATE_STOPPING) && (k_msgq_num_used_get(&stream->queue) == 0U)) {
		ns_i2s_stream_ready(stream);
	}

	return 0;
}

static int ns_i2s_write(const struct device *dev, void *mem_block, size_t size)
{
	const struct ns_i2s_config *cfg = dev->config;
	struct ns_i2s_data *data = dev->data;
	struct ns_i2s_stream *stream = &data->tx;
	struct ns_i2s_block block = {
		.mem_block = mem_block,
		.size = size,
	};
	int ret;

	if (!ns_i2s_mode_supports_tx(cfg)) {
		return -EINVAL;
	}

	if (size > stream->cfg.block_size) {
		return -EINVAL;
	}

	if ((stream->state != I2S_STATE_READY) && (stream->state != I2S_STATE_RUNNING)) {
		return -EIO;
	}

	ret = k_msgq_put(&stream->queue, &block, SYS_TIMEOUT_MS(stream->cfg.timeout));
	if (ret < 0) {
		return ret;
	}

	k_sem_give(&data->worker_sem);

	return 0;
}

static int ns_i2s_trigger_single(const struct device *dev, struct ns_i2s_data *data,
					 struct ns_i2s_stream *stream,
					 enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	k_mutex_lock(&data->lock, K_FOREVER);

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (stream->state != I2S_STATE_READY) {
			k_mutex_unlock(&data->lock);
			return -EIO;
		}

		if ((dir == I2S_DIR_TX) && (k_msgq_num_used_get(&stream->queue) == 0U)) {
			k_mutex_unlock(&data->lock);
			return -EIO;
		}

		stream->state = I2S_STATE_RUNNING;
		stream->stop_mode = NS_I2S_STOP_NONE;
		stream->last_error = 0;
		stream->next_deadline_us = nsi_hws_get_time();
		k_mutex_unlock(&data->lock);
		k_sem_give(&data->worker_sem);
		return 0;

	case I2S_TRIGGER_STOP:
		if (stream->state != I2S_STATE_RUNNING) {
			k_mutex_unlock(&data->lock);
			return -EIO;
		}

		stream->state = I2S_STATE_STOPPING;
		stream->stop_mode = NS_I2S_STOP_AT_BLOCK;
		k_mutex_unlock(&data->lock);
		if (dir == I2S_DIR_TX) {
			(void)ns_i2s_process_tx(dev);
		} else {
			k_sem_give(&data->worker_sem);
		}
		return 0;

	case I2S_TRIGGER_DRAIN:
		if (stream->state != I2S_STATE_RUNNING) {
			k_mutex_unlock(&data->lock);
			return -EIO;
		}

		stream->state = I2S_STATE_STOPPING;
		stream->stop_mode = (dir == I2S_DIR_TX) ? NS_I2S_DRAIN_QUEUE : NS_I2S_STOP_AT_BLOCK;
		k_mutex_unlock(&data->lock);
		k_sem_give(&data->worker_sem);
		return 0;

	case I2S_TRIGGER_DROP:
		if (stream->state == I2S_STATE_NOT_READY) {
			k_mutex_unlock(&data->lock);
			return -EIO;
		}

		ns_i2s_stream_recover(stream);
		k_mutex_unlock(&data->lock);
		k_sem_give(&data->worker_sem);
		return 0;

	case I2S_TRIGGER_PREPARE:
		if (stream->state != I2S_STATE_ERROR) {
			k_mutex_unlock(&data->lock);
			return -EIO;
		}

		ns_i2s_stream_recover(stream);
		k_mutex_unlock(&data->lock);
		return 0;

	default:
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}
}

static int ns_i2s_trigger_both_special(struct ns_i2s_data *data, enum i2s_trigger_cmd cmd)
{
	k_mutex_lock(&data->lock, K_FOREVER);

	if (cmd == I2S_TRIGGER_DROP) {
		if ((data->rx.state == I2S_STATE_NOT_READY) ||
		    (data->tx.state == I2S_STATE_NOT_READY)) {
			k_mutex_unlock(&data->lock);
			return -EIO;
		}

		ns_i2s_stream_recover(&data->rx);
		ns_i2s_stream_recover(&data->tx);
		k_mutex_unlock(&data->lock);
		k_sem_give(&data->worker_sem);
		return 0;
	}

	if ((data->rx.state != I2S_STATE_ERROR) &&
	    (data->tx.state != I2S_STATE_ERROR)) {
		k_mutex_unlock(&data->lock);
		return -EIO;
	}

	ns_i2s_stream_recover(&data->rx);
	ns_i2s_stream_recover(&data->tx);
	k_mutex_unlock(&data->lock);

	return 0;
}

static int ns_i2s_trigger_both(const struct device *dev, struct ns_i2s_data *data,
				      enum i2s_trigger_cmd cmd)
{
	int ret;

	if ((cmd == I2S_TRIGGER_DROP) || (cmd == I2S_TRIGGER_PREPARE)) {
		return ns_i2s_trigger_both_special(data, cmd);
	}

	ret = ns_i2s_trigger_single(dev, data, &data->rx, I2S_DIR_RX, cmd);
	if (ret < 0) {
		return ret;
	}

	return ns_i2s_trigger_single(dev, data, &data->tx, I2S_DIR_TX, cmd);
}

static int ns_i2s_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	const struct ns_i2s_config *cfg = dev->config;
	struct ns_i2s_data *data = dev->data;
	struct ns_i2s_stream *stream;

	if (!ns_i2s_supports_dir(cfg, dir)) {
		return (dir == I2S_DIR_BOTH) ? -ENOSYS : -EINVAL;
	}

	if (dir == I2S_DIR_BOTH) {
		return ns_i2s_trigger_both(dev, data, cmd);
	}

	stream = ns_i2s_stream_get(data, dir);
	if (stream == NULL) {
		return -EINVAL;
	}

	return ns_i2s_trigger_single(dev, data, stream, dir, cmd);
}

static DEVICE_API(i2s, ns_i2s_driver_api) = {
	.configure = ns_i2s_configure,
	.config_get = ns_i2s_config_get,
	.trigger = ns_i2s_trigger,
	.read = ns_i2s_read,
	.write = ns_i2s_write,
};

static int ns_i2s_init(const struct device *dev)
{
	struct ns_i2s_data *data = dev->data;

	k_mutex_init(&data->lock);
	k_sem_init(&data->worker_sem, 0, K_SEM_MAX_LIMIT);
	k_msgq_init(&data->rx.queue, data->rx.queue_buffer, sizeof(struct ns_i2s_block),
		    CONFIG_I2S_NATIVE_SIM_QUEUE_SIZE);
	k_msgq_init(&data->tx.queue, data->tx.queue_buffer, sizeof(struct ns_i2s_block),
		    CONFIG_I2S_NATIVE_SIM_QUEUE_SIZE);

	data->rx.state = I2S_STATE_NOT_READY;
	data->rx.stop_mode = NS_I2S_STOP_NONE;
	data->rx.fd = -1;
	data->rx.active_block_valid = false;
	data->tx.state = I2S_STATE_NOT_READY;
	data->tx.stop_mode = NS_I2S_STOP_NONE;
	data->tx.fd = -1;
	data->tx.active_block_valid = false;

	(void)k_thread_create(&data->worker_thread, data->worker_stack,
			      K_KERNEL_STACK_SIZEOF(data->worker_stack), ns_i2s_worker, (void *)dev,
			      NULL, NULL, CONFIG_I2S_NATIVE_SIM_THREAD_PRIORITY, 0, K_NO_WAIT);

	return 0;
}
/* clang-format off */
#define NS_I2S_INIT(inst) \
	static const struct ns_i2s_config ns_i2s_config_##inst = { \
		.mode = DT_INST_ENUM_IDX(inst, stream_direction), \
		.rx_path = CONFIG_I2S_NATIVE_SIM_RX_FILE_PATH, \
		.tx_path = CONFIG_I2S_NATIVE_SIM_TX_FILE_PATH, \
	}; \
	static struct ns_i2s_data ns_i2s_data_##inst; \
	DEVICE_DT_INST_DEFINE(inst, ns_i2s_init, NULL, \
			      &ns_i2s_data_##inst, \
			      &ns_i2s_config_##inst, POST_KERNEL, \
			      CONFIG_I2S_INIT_PRIORITY, \
			      &ns_i2s_driver_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(NS_I2S_INIT)

#define INST_NAME(inst) DEVICE_DT_NAME(DT_DRV_INST(inst))

#define NS_I2S_COMMAND_LINE_OPTS(inst)                                                             \
	{                                                                                          \
		.option = INST_NAME(inst) "_rx",                                                   \
		.name = "path",                                                                    \
		.type = 's',                                                                       \
		.dest = (void *)&ns_i2s_data_##inst.cmd_rx_path,                                   \
		.descript = "Path to binary file to be used as native_sim "                        \
			    "I2S RX input for " INST_NAME(inst)                                  \
			    ". Overrides CONFIG_I2S_NATIVE_SIM_RX_FILE_PATH if set",              \
	},                                                                                         \
	{                                                                                          \
		.option = INST_NAME(inst) "_tx",                                                   \
		.name = "path",                                                                    \
		.type = 's',                                                                       \
		.dest = (void *)&ns_i2s_data_##inst.cmd_tx_path,                                   \
		.descript = "Path to binary file to be used as native_sim "                        \
			    "I2S TX output for " INST_NAME(inst)                                 \
			    ". Overrides CONFIG_I2S_NATIVE_SIM_TX_FILE_PATH if set",              \
	},

static void ns_i2s_options(void)
{
	static struct args_struct_t ns_i2s_options[] = {
		DT_INST_FOREACH_STATUS_OKAY(NS_I2S_COMMAND_LINE_OPTS)
		ARG_TABLE_ENDMARKER,
	};

	native_add_command_line_opts(ns_i2s_options);
}

NATIVE_TASK(ns_i2s_options, PRE_BOOT_1, 1);
