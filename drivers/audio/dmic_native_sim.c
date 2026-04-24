/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_native_sim_dmic

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <nsi_host_io.h>
#include <nsi_host_trampolines.h>
#include <nsi_hw_scheduler.h>
#include <nsi_tracing.h>

#ifdef CONFIG_ARCH_POSIX
#include "cmdline.h"
#include "soc.h"
#endif

#include <zephyr/audio/audio_caps.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_ARCH_POSIX
static const char *ns_dmic_file_path_override;
#endif

struct ns_dmic_block {
	void *mem_block;
	size_t size;
};

struct ns_dmic_stream {
	struct k_mem_slab *mem_slab;
	uint32_t pcm_rate;
	uint16_t block_size;
	uint8_t pcm_width;
	uint8_t num_channels;
};

struct ns_dmic_config {
	const char *path;
	uint8_t max_channels;
};

struct ns_dmic_data {
	struct k_mutex lock;
	struct k_sem worker_sem;
	struct k_thread worker_thread;

	K_KERNEL_STACK_MEMBER(worker_stack, CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);
	struct k_msgq rx_queue;
	char rx_queue_buffer[CONFIG_AUDIO_DMIC_NATIVE_SIM_QUEUE_SIZE *
			     sizeof(struct ns_dmic_block)];
	struct ns_dmic_stream stream;
	enum dmic_state state;
	int fd;
	int last_error;
	uint64_t next_deadline_us;
	uint32_t session_id;
	bool configured;
};

static const char *ns_dmic_path_get(const struct ns_dmic_config *cfg)
{
#ifdef CONFIG_ARCH_POSIX
	if (ns_dmic_file_path_override != NULL) {
		return ns_dmic_file_path_override;
	}
#endif

	return cfg->path;
}

static uint8_t ns_dmic_bytes_per_sample(uint8_t pcm_width)
{
	return DIV_ROUND_UP(pcm_width, 8U);
}

static uint32_t ns_dmic_frame_bytes(const struct ns_dmic_stream *stream)
{
	return stream->num_channels * ns_dmic_bytes_per_sample(stream->pcm_width);
}

static uint64_t ns_dmic_bytes_per_sec(const struct ns_dmic_stream *stream)
{
	return (uint64_t)stream->pcm_rate * ns_dmic_frame_bytes(stream);
}

static void ns_dmic_close_file(struct ns_dmic_data *data)
{
	if (data->fd >= 0) {
		(void)nsi_host_close(data->fd);
		data->fd = -1;
	}
}

static void ns_dmic_free_block(struct ns_dmic_stream *stream, const struct ns_dmic_block *block)
{
	if ((stream->mem_slab != NULL) && (block->mem_block != NULL)) {
		k_mem_slab_free(stream->mem_slab, block->mem_block);
	}
}

static void ns_dmic_flush_queue(struct ns_dmic_data *data)
{
	struct ns_dmic_block block;

	while (k_msgq_get(&data->rx_queue, &block, K_NO_WAIT) == 0) {
		ns_dmic_free_block(&data->stream, &block);
	}
}

static void ns_dmic_reset_stream(struct ns_dmic_data *data)
{
	ns_dmic_close_file(data);
	ns_dmic_flush_queue(data);
	memset(&data->stream, 0, sizeof(data->stream));
	data->next_deadline_us = 0U;
	data->last_error = 0;
	data->configured = false;
	data->state = DMIC_STATE_INITIALIZED;
	data->session_id++;
}

static void ns_dmic_set_error(struct ns_dmic_data *data, int error)
{
	data->state = DMIC_STATE_ERROR;
	data->last_error = error;
	data->next_deadline_us = 0U;
	data->session_id++;
	ns_dmic_close_file(data);
}

static void ns_dmic_pace(struct ns_dmic_data *data, size_t size)
{
	uint64_t deadline;
	uint64_t duration_us;
	uint64_t bytes_per_sec;
	uint64_t now;

	bytes_per_sec = ns_dmic_bytes_per_sec(&data->stream);
	deadline = data->next_deadline_us;
	now = nsi_hws_get_time();

	if ((deadline == 0U) || (deadline < now)) {
		deadline = now;
	}

	if (bytes_per_sec == 0U) {
		duration_us = 0U;
	} else {
		duration_us = DIV_ROUND_UP((uint64_t)size * USEC_PER_SEC, bytes_per_sec);
	}

	data->next_deadline_us = deadline + duration_us;

	if (deadline > now) {
		uint32_t sleep_us = (uint32_t)min(deadline - now, (uint64_t)UINT32_MAX);

		if (sleep_us > 0U) {
			k_usleep(sleep_us);
		}
	}
}

static int ns_dmic_open_file(const struct ns_dmic_config *cfg, struct ns_dmic_data *data)
{
	const char *path = ns_dmic_path_get(cfg);
	int fd;

	if ((path == NULL) || (path[0] == '\0')) {
		return -EINVAL;
	}

	ns_dmic_close_file(data);

	fd = nsi_host_open_read(path);
	if (fd < 0) {
		nsi_print_warning("%s could not be opened (%s)\n", path, strerror(errno));
		return -errno;
	}

	data->fd = fd;
	data->next_deadline_us = nsi_hws_get_time();
	return 0;
}

static int ns_dmic_validate_channel_map(const struct pdm_chan_cfg *channel, uint8_t max_channels)
{
	uint8_t pdm0;
	uint8_t pdm1;
	enum pdm_lr lr0;
	enum pdm_lr lr1;

	if ((channel->req_num_streams != 1U) || (channel->req_num_chan == 0U) ||
	    (channel->req_num_chan > max_channels)) {
		return -EINVAL;
	}

	for (uint8_t chan = 0U; chan < channel->req_num_chan; chan += 2U) {
		dmic_parse_channel_map(channel->req_chan_map_lo, channel->req_chan_map_hi, chan,
				       &pdm0, &lr0);

		if ((chan + 1U) >= channel->req_num_chan) {
			continue;
		}

		dmic_parse_channel_map(channel->req_chan_map_lo, channel->req_chan_map_hi,
				       chan + 1U, &pdm1, &lr1);
		if ((pdm0 != pdm1) || (lr0 == lr1)) {
			return -EINVAL;
		}
	}

	return 0;
}

static int ns_dmic_validate_cfg(const struct ns_dmic_config *cfg, struct dmic_cfg *config)
{
	struct pcm_stream_cfg *stream;
	uint32_t frame_bytes;
	int ret;

	if ((config == NULL) || (config->streams == NULL)) {
		return -EINVAL;
	}

	stream = &config->streams[0];

	ret = ns_dmic_validate_channel_map(&config->channel, cfg->max_channels);
	if (ret < 0) {
		return ret;
	}

	if ((config->io.min_pdm_clk_freq != 0U) && (config->io.max_pdm_clk_freq != 0U) &&
	    (config->io.min_pdm_clk_freq > config->io.max_pdm_clk_freq)) {
		return -EINVAL;
	}

	if ((stream->pcm_rate == 0U) || (stream->pcm_width == 0U)) {
		return 0;
	}

	if ((stream->mem_slab == NULL) || (stream->block_size == 0U)) {
		return -EINVAL;
	}

	if ((stream->pcm_width != 8U) && (stream->pcm_width != 16U) && (stream->pcm_width != 24U) &&
	    (stream->pcm_width != 32U)) {
		return -ENOTSUP;
	}

	frame_bytes = config->channel.req_num_chan * ns_dmic_bytes_per_sample(stream->pcm_width);
	if ((frame_bytes == 0U) || ((stream->block_size % frame_bytes) != 0U)) {
		return -EINVAL;
	}

	if (!ns_dmic_path_get(cfg) || (ns_dmic_path_get(cfg)[0] == '\0')) {
		return -EINVAL;
	}

	return 0;
}

static void ns_dmic_worker(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	struct ns_dmic_data *data = dev->data;
	struct ns_dmic_block block = {0};
	uint32_t session;
	ssize_t bytes_read;
	int ret;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		(void)k_sem_take(&data->worker_sem, K_FOREVER);

		while (true) {
			k_mutex_lock(&data->lock, K_FOREVER);
			if (data->state != DMIC_STATE_ACTIVE) {
				k_mutex_unlock(&data->lock);
				break;
			}

			session = data->session_id;
			ret = k_mem_slab_alloc(data->stream.mem_slab, &block.mem_block, K_NO_WAIT);
			if (ret < 0) {
				ns_dmic_set_error(data, -ENOMEM);
				k_mutex_unlock(&data->lock);
				break;
			}

			ns_dmic_pace(data, data->stream.block_size);
			bytes_read =
				nsi_host_read(data->fd, block.mem_block, data->stream.block_size);
			if (bytes_read <= 0) {
				ns_dmic_free_block(&data->stream, &block);
				block.mem_block = NULL;
				block.size = 0U;
				ns_dmic_set_error(data, -EIO);
				k_mutex_unlock(&data->lock);
				break;
			}

			block.size = (size_t)bytes_read;

			if ((data->state != DMIC_STATE_ACTIVE) || (data->session_id != session)) {
				ns_dmic_free_block(&data->stream, &block);
				block.mem_block = NULL;
				block.size = 0U;
				k_mutex_unlock(&data->lock);
				break;
			}

			ret = k_msgq_put(&data->rx_queue, &block, K_NO_WAIT);
			if (ret < 0) {
				ns_dmic_free_block(&data->stream, &block);
				block.mem_block = NULL;
				block.size = 0U;
				ns_dmic_set_error(data, -EIO);
				k_mutex_unlock(&data->lock);
				break;
			}

			if (block.size < data->stream.block_size) {
				ns_dmic_set_error(data, -EIO);
				k_mutex_unlock(&data->lock);
				block.mem_block = NULL;
				block.size = 0U;
				break;
			}

			k_mutex_unlock(&data->lock);
			block.mem_block = NULL;
			block.size = 0U;
		}
	}
}

static int ns_dmic_configure(const struct device *dev, struct dmic_cfg *config)
{
	const struct ns_dmic_config *cfg = dev->config;
	struct ns_dmic_data *data = dev->data;
	struct pcm_stream_cfg *stream;
	int ret;

	if (config == NULL) {
		return -EINVAL;
	}

	stream = &config->streams[0];

	k_mutex_lock(&data->lock, K_FOREVER);
	if ((data->state == DMIC_STATE_ACTIVE) || (data->state == DMIC_STATE_PAUSED)) {
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	ret = ns_dmic_validate_cfg(cfg, config);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	if ((stream->pcm_rate == 0U) || (stream->pcm_width == 0U)) {
		ns_dmic_reset_stream(data);
		config->channel.act_num_streams = 0U;
		config->channel.act_num_chan = 0U;
		config->channel.act_chan_map_lo = 0U;
		config->channel.act_chan_map_hi = 0U;
		k_mutex_unlock(&data->lock);
		return 0;
	}

	ret = ns_dmic_open_file(cfg, data);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ns_dmic_flush_queue(data);
	data->stream.mem_slab = stream->mem_slab;
	data->stream.pcm_rate = stream->pcm_rate;
	data->stream.block_size = stream->block_size;
	data->stream.pcm_width = stream->pcm_width;
	data->stream.num_channels = config->channel.req_num_chan;
	data->next_deadline_us = nsi_hws_get_time();
	data->last_error = 0;
	data->configured = true;
	data->state = DMIC_STATE_CONFIGURED;
	data->session_id++;

	config->channel.act_num_streams = config->channel.req_num_streams;
	config->channel.act_num_chan = config->channel.req_num_chan;
	config->channel.act_chan_map_lo = config->channel.req_chan_map_lo;
	config->channel.act_chan_map_hi = config->channel.req_chan_map_hi;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int ns_dmic_trigger(const struct device *dev, enum dmic_trigger cmd)
{
	struct ns_dmic_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (cmd) {
	case DMIC_TRIGGER_START:
		if (!data->configured || ((data->state != DMIC_STATE_CONFIGURED) &&
					  (data->state != DMIC_STATE_ACTIVE))) {
			ret = -EIO;
			break;
		}

		if (data->state != DMIC_STATE_ACTIVE) {
			data->state = DMIC_STATE_ACTIVE;
			data->last_error = 0;
			data->next_deadline_us = nsi_hws_get_time();
			data->session_id++;
			k_sem_give(&data->worker_sem);
		}
		break;

	case DMIC_TRIGGER_PAUSE:
		if (data->state != DMIC_STATE_ACTIVE) {
			ret = -EIO;
			break;
		}

		data->state = DMIC_STATE_PAUSED;
		data->next_deadline_us = 0U;
		data->session_id++;
		break;

	case DMIC_TRIGGER_RELEASE:
		if (data->state != DMIC_STATE_PAUSED) {
			ret = -EIO;
			break;
		}

		data->state = DMIC_STATE_ACTIVE;
		data->next_deadline_us = nsi_hws_get_time();
		data->session_id++;
		k_sem_give(&data->worker_sem);
		break;

	case DMIC_TRIGGER_STOP:
		if ((data->state != DMIC_STATE_ACTIVE) && (data->state != DMIC_STATE_PAUSED)) {
			ret = -EIO;
			break;
		}

		data->state = DMIC_STATE_CONFIGURED;
		data->next_deadline_us = 0U;
		data->last_error = 0;
		data->session_id++;
		ns_dmic_flush_queue(data);
		break;

	case DMIC_TRIGGER_RESET:
		if ((data->state == DMIC_STATE_INITIALIZED) && !data->configured) {
			ret = -ENOSYS;
			break;
		}

		ns_dmic_reset_stream(data);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int ns_dmic_read(const struct device *dev, uint8_t stream, void **buffer, size_t *size,
			int32_t timeout)
{
	struct ns_dmic_data *data = dev->data;
	struct ns_dmic_block block;
	enum dmic_state state;
	int last_error;
	k_timeout_t read_timeout;
	int ret;

	ARG_UNUSED(stream);

	if ((buffer == NULL) || (size == NULL)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	state = data->state;
	last_error = data->last_error;
	k_mutex_unlock(&data->lock);

	if ((state == DMIC_STATE_UNINIT) || (state == DMIC_STATE_INITIALIZED)) {
		return -EIO;
	}

	read_timeout = (state == DMIC_STATE_ERROR) ? K_NO_WAIT : SYS_TIMEOUT_MS(timeout);
	ret = k_msgq_get(&data->rx_queue, &block, read_timeout);
	if (ret < 0) {
		if (state == DMIC_STATE_ERROR) {
			return (last_error != 0) ? last_error : -EIO;
		}

		return ret;
	}

	*buffer = block.mem_block;
	*size = block.size;
	return 0;
}

static int ns_dmic_get_caps(const struct device *dev, struct audio_caps *caps)
{
	const struct ns_dmic_config *cfg = dev->config;

	if (caps == NULL) {
		return -EINVAL;
	}

	memset(caps, 0, sizeof(*caps));
	caps->min_total_channels = 1U;
	caps->max_total_channels = cfg->max_channels;
	caps->supported_sample_rates =
		AUDIO_SAMPLE_RATE_7350 | AUDIO_SAMPLE_RATE_8000 | AUDIO_SAMPLE_RATE_11025 |
		AUDIO_SAMPLE_RATE_12000 | AUDIO_SAMPLE_RATE_14700 | AUDIO_SAMPLE_RATE_16000 |
		AUDIO_SAMPLE_RATE_20000 | AUDIO_SAMPLE_RATE_22050 | AUDIO_SAMPLE_RATE_24000 |
		AUDIO_SAMPLE_RATE_29400 | AUDIO_SAMPLE_RATE_32000 | AUDIO_SAMPLE_RATE_44100 |
		AUDIO_SAMPLE_RATE_48000 | AUDIO_SAMPLE_RATE_50000 | AUDIO_SAMPLE_RATE_50400 |
		AUDIO_SAMPLE_RATE_64000 | AUDIO_SAMPLE_RATE_88200 | AUDIO_SAMPLE_RATE_96000 |
		AUDIO_SAMPLE_RATE_100800 | AUDIO_SAMPLE_RATE_128000 | AUDIO_SAMPLE_RATE_176400 |
		AUDIO_SAMPLE_RATE_192000 | AUDIO_SAMPLE_RATE_352800 | AUDIO_SAMPLE_RATE_384000;
	caps->supported_bit_widths =
		AUDIO_BIT_WIDTH_8 | AUDIO_BIT_WIDTH_16 | AUDIO_BIT_WIDTH_24 | AUDIO_BIT_WIDTH_32;
	caps->min_num_buffers = 1U;
	caps->min_frame_interval = 1U;
	caps->max_frame_interval = UINT32_MAX;
	caps->interleaved = true;

	return 0;
}

static const struct _dmic_ops ns_dmic_ops = {
	.configure = ns_dmic_configure,
	.trigger = ns_dmic_trigger,
	.read = ns_dmic_read,
	.get_caps = ns_dmic_get_caps,
};

static int ns_dmic_init(const struct device *dev)
{
	struct ns_dmic_data *data = dev->data;

	k_mutex_init(&data->lock);
	k_sem_init(&data->worker_sem, 0, K_SEM_MAX_LIMIT);
	k_msgq_init(&data->rx_queue, data->rx_queue_buffer, sizeof(struct ns_dmic_block),
		    CONFIG_AUDIO_DMIC_NATIVE_SIM_QUEUE_SIZE);

	data->fd = -1;
	data->state = DMIC_STATE_INITIALIZED;
	data->session_id = 1U;

	(void)k_thread_create(&data->worker_thread, data->worker_stack,
			      K_KERNEL_STACK_SIZEOF(data->worker_stack), ns_dmic_worker,
			      (void *)dev, NULL, NULL, CONFIG_AUDIO_DMIC_NATIVE_SIM_THREAD_PRIORITY,
			      0, K_NO_WAIT);

	return 0;
}

#ifdef CONFIG_ARCH_POSIX
static void ns_dmic_options(void)
{
	static struct args_struct_t ns_dmic_options[] = {
		{
			.option = "dmic",
			.name = "path",
			.type = 's',
			.dest = (void *)&ns_dmic_file_path_override,
			.descript = "Path to binary file to be used as native_sim DMIC input. "
				    "Overrides CONFIG_AUDIO_DMIC_NATIVE_SIM_FILE_PATH if set",
		},
		ARG_TABLE_ENDMARKER,
	};

	native_add_command_line_opts(ns_dmic_options);
}

NATIVE_TASK(ns_dmic_options, PRE_BOOT_1, 1);
#endif

/* clang-format off */
#define NS_DMIC_INIT(inst) \
	static const struct ns_dmic_config ns_dmic_config_##inst = { \
		.path = CONFIG_AUDIO_DMIC_NATIVE_SIM_FILE_PATH, \
		.max_channels = DT_INST_PROP(inst, max_channels), \
	}; \
	static struct ns_dmic_data ns_dmic_data_##inst; \
	DEVICE_DT_INST_DEFINE(inst, ns_dmic_init, NULL, \
			      &ns_dmic_data_##inst, \
			      &ns_dmic_config_##inst, POST_KERNEL, \
			      CONFIG_AUDIO_DMIC_INIT_PRIORITY, \
			      &ns_dmic_ops);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(NS_DMIC_INIT)
