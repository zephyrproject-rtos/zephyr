/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>

#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/mpipe/aud/mpipe_aud.h>
#include <zephyr/mpipe/aud/mpipe_aud_buffer_pool.h>
#include <zephyr/mpipe/aud/mpipe_aud_i2s_src.h>

LOG_MODULE_REGISTER(mpipe_aud_i2s_src, CONFIG_MPIPE_LOG_LEVEL);

static int (*src_parent_set_property)(struct mpipe_object *, uint32_t, const void *);
static int (*src_parent_get_property)(struct mpipe_object *, uint32_t, void *);

static int mpipe_aud_i2s_src_get_audio_caps(const struct device *dev, struct audio_caps *caps)
{
	return i2s_get_caps(dev, caps, I2S_DIR_RX);
}

static int mpipe_aud_i2s_src_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_aud_i2s_src *self = (struct mpipe_aud_i2s_src *)obj;

	switch (key) {
	case MPIPE_PROP_AUD_I2S_SRC_CODEC_DEVICE:
		self->codec_dev = (const struct device *)val;
		break;
	case MPIPE_PROP_AUD_I2S_SRC_CLK_ROLE:
		if ((enum mpipe_aud_i2s_codec_clk_role)(uintptr_t)val !=
			    MPIPE_AUD_I2S_CONTROLLER_CODEC_TARGET &&
		    (enum mpipe_aud_i2s_codec_clk_role)(uintptr_t)val !=
			    MPIPE_AUD_I2S_TARGET_CODEC_CONTROLLER) {
			LOG_ERR("Invalid clock role value");
			return -EINVAL;
		}
		self->clk_role = (enum mpipe_aud_i2s_codec_clk_role)(uintptr_t)val;
		break;
	default:
		return src_parent_set_property(obj, key, val);
	}

	return 0;
}

static int mpipe_aud_i2s_src_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe_aud_i2s_src *self = (struct mpipe_aud_i2s_src *)obj;

	if (val == NULL) {
		return -1;
	}

	switch (key) {
	case MPIPE_PROP_AUD_I2S_SRC_CODEC_DEVICE:
		*(const struct device **)val = self->codec_dev;
		break;
	case MPIPE_PROP_AUD_I2S_SRC_CLK_ROLE:
		*(enum mpipe_aud_i2s_codec_clk_role *)val = self->clk_role;
		break;
	default:
		return src_parent_get_property(obj, key, val);
	}

	return 0;
}

static int mpipe_aud_i2s_src_enum_caps(struct mpipe_pad *pad, uint32_t index,
				       const struct mpipe_structure *filter,
				       struct mpipe_structure *out)
{
	struct mpipe_src *src = (struct mpipe_src *)pad->object.container;
	struct mpipe_aud_i2s_src *self = (struct mpipe_aud_i2s_src *)src;
	struct mpipe_aud_buffer_pool *pool =
		CONTAINER_OF(src->pool, struct mpipe_aud_buffer_pool, pool);
	struct audio_caps i2s_caps;
	struct audio_caps codec_caps;
	struct audio_caps caps;

	if (pool->aud_dev == NULL) {
		LOG_ERR("I2S device not configured");
		return -ENODEV;
	}

	if (i2s_get_caps(pool->aud_dev, &i2s_caps, I2S_DIR_RX) != 0) {
		LOG_ERR("Failed to get I2S capabilities");
		return -ENODEV;
	}

	if (self->codec_dev == NULL) {
		return mpipe_aud_enum_caps(&i2s_caps, index, filter, out);
	}

	if (audio_codec_get_caps(self->codec_dev, &codec_caps) != 0) {
		LOG_ERR("Failed to get codec capabilities");
		return -ENODEV;
	}

	if (i2s_caps.interleaved != codec_caps.interleaved) {
		LOG_ERR("Interleaved capabilities mismatch between I2S and codec");
		return -ENOTSUP;
	}

	caps.supported_sample_rates =
		i2s_caps.supported_sample_rates & codec_caps.supported_sample_rates;
	caps.supported_bit_widths = i2s_caps.supported_bit_widths & codec_caps.supported_bit_widths;
	caps.min_total_channels = MAX(i2s_caps.min_total_channels, codec_caps.min_total_channels);
	caps.max_total_channels = MIN(i2s_caps.max_total_channels, codec_caps.max_total_channels);
	caps.min_frame_interval = MAX(i2s_caps.min_frame_interval, codec_caps.min_frame_interval);
	caps.max_frame_interval = MIN(i2s_caps.max_frame_interval, codec_caps.max_frame_interval);
	caps.min_num_buffers = MAX(i2s_caps.min_num_buffers, codec_caps.min_num_buffers);
	caps.interleaved = codec_caps.interleaved;

	return mpipe_aud_enum_caps(&caps, index, filter, out);
}

static void mpipe_aud_i2s_src_update_caps(struct mpipe_src *src)
{
	src->src_pad.enum_caps_fn = mpipe_aud_i2s_src_enum_caps;
}

static int mpipe_aud_i2s_src_set_caps(struct mpipe_src *src, const struct mpipe_structure *caps)
{
	struct mpipe_aud_i2s_src *self = (struct mpipe_aud_i2s_src *)src;
	struct mpipe_aud_buffer_pool *pool =
		CONTAINER_OF(src->pool, struct mpipe_aud_buffer_pool, pool);
	struct i2s_config config;
	int ret;

	uint32_t sample_rate, bit_width, num_of_channel, frame_interval;

	if (mpipe_aud_caps_get_uint(caps, MPIPE_CAPS_SAMPLE_RATE, &sample_rate) != 0 ||
	    mpipe_aud_caps_get_uint(caps, MPIPE_CAPS_BITWIDTH, &bit_width) != 0 ||
	    mpipe_aud_caps_get_uint(caps, MPIPE_CAPS_NUM_OF_CHANNEL, &num_of_channel) != 0 ||
	    mpipe_aud_caps_get_uint(caps, MPIPE_CAPS_FRAME_INTERVAL, &frame_interval) != 0) {
		return -EINVAL;
	}

	if (pool->mem_slab == NULL) {
		LOG_ERR("Memory slab not configured");
		return -EINVAL;
	}

	if (self->codec_dev != NULL) {
		struct audio_codec_cfg audio_cfg = {0};

		audio_cfg.dai_route = AUDIO_ROUTE_CAPTURE;
		audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
		audio_cfg.dai_cfg.i2s.word_size = bit_width;
		audio_cfg.dai_cfg.i2s.channels = num_of_channel;
		audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;

		if (self->clk_role == MPIPE_AUD_I2S_TARGET_CODEC_CONTROLLER) {
			audio_cfg.dai_cfg.i2s.options =
				I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER;
		} else {
			audio_cfg.dai_cfg.i2s.options =
				I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_BIT_CLK_TARGET;
		}

		audio_cfg.dai_cfg.i2s.frame_clk_freq = sample_rate;
		audio_cfg.dai_cfg.i2s.mem_slab = pool->mem_slab;
		audio_cfg.dai_cfg.i2s.block_size =
			(bit_width >> 3) *
			((sample_rate * frame_interval / 1000000) * num_of_channel);

		ret = audio_codec_configure(self->codec_dev, &audio_cfg);
		if (ret < 0) {
			LOG_ERR("Failed to configure capture codec: %d", ret);
			return ret;
		}
	}

	config.word_size = bit_width;
	config.channels = num_of_channel;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
	if (self->clk_role == MPIPE_AUD_I2S_TARGET_CODEC_CONTROLLER) {
		config.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
	} else {
		config.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER;
	}
	config.frame_clk_freq = sample_rate;
	config.mem_slab = pool->mem_slab;
	config.block_size =
		(bit_width >> 3) * ((sample_rate * frame_interval / 1000000) * num_of_channel);
	/* timeout is in ms, frame_interval in us: wait a few frame periods. */
	config.timeout = MAX(100U, (frame_interval / 1000U) * 4U);

	ret = i2s_configure(pool->aud_dev, I2S_DIR_RX, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure I2S stream: %d", ret);
		return ret;
	}

	mpipe_pad_set_caps(&src->src_pad, caps);

	return 0;
}

static int mpipe_aud_i2s_src_acquire_buffer(struct mpipe_buffer_pool *pool, struct net_buf **buffer)
{
	struct mpipe_aud_buffer_pool *aud_pool =
		CONTAINER_OF(pool, struct mpipe_aud_buffer_pool, pool);
	struct mpipe_buffer_meta *meta;
	void *mem_block = NULL;
	size_t bytes_used = pool->config.size;
	int err;

	err = i2s_read(aud_pool->aud_dev, &mem_block, &bytes_used);
	if (err < 0) {
		LOG_ERR("Unable to read an I2S buffer: %d", err);
		return err;
	}

	for (uint8_t i = 0; i < pool->config.min_buffers; i++) {
		if (mem_block == aud_pool->blocks[i]) {
			*buffer = net_buf_alloc_with_data(pool->nb_pool, aud_pool->blocks[i],
							  pool->config.size, K_NO_WAIT);
			if (*buffer == NULL) {
				LOG_ERR("Unable to allocate net_buf wrapper for I2S buffer");
				k_mem_slab_free(aud_pool->mem_slab, mem_block);
				return -ENOBUFS;
			}

			(*buffer)->len = bytes_used;

			meta = mpipe_buffer_get_meta(*buffer);
			meta->pool = pool;
			meta->bytes_used = bytes_used;
			meta->timestamp = 0U;
			meta->driver_buf = NULL;
			meta->priv = NULL;

			return 0;
		}
	}

	LOG_ERR("Unable to match I2S buffer %p with mem_slab backing store", mem_block);
	k_mem_slab_free(aud_pool->mem_slab, mem_block);
	return -ENOENT;
}

static int mpipe_aud_i2s_src_start(struct mpipe_buffer_pool *pool)
{
	struct mpipe_aud_buffer_pool *aud_pool =
		CONTAINER_OF(pool, struct mpipe_aud_buffer_pool, pool);

	if (i2s_trigger(aud_pool->aud_dev, I2S_DIR_RX, I2S_TRIGGER_START) < 0) {
		LOG_ERR("Unable to start I2S capture");
		return -EIO;
	}

	LOG_INF("Capture started now");
	return 0;
}

static int (*pool_parent_stop)(struct mpipe_buffer_pool *pool);

static int mpipe_aud_i2s_src_stop(struct mpipe_buffer_pool *pool)
{
	struct mpipe_aud_buffer_pool *aud_pool =
		CONTAINER_OF(pool, struct mpipe_aud_buffer_pool, pool);

	/* DROP returns the RX to READY so a later set_caps can reconfigure it. */
	if (aud_pool->aud_dev != NULL) {
		(void)i2s_trigger(aud_pool->aud_dev, I2S_DIR_RX, I2S_TRIGGER_DROP);
	}

	if (pool_parent_stop != NULL) {
		return pool_parent_stop(pool);
	}

	return 0;
}

int mpipe_aud_i2s_src_init(struct mpipe_aud_i2s_src *aud_i2s_src, uint8_t id)
{
	__ASSERT_NO_MSG(aud_i2s_src != NULL);

	struct mpipe_element *self = &aud_i2s_src->aud_src.src.element;
	struct mpipe_src *src = &aud_i2s_src->aud_src.src;
	int ret = mpipe_aud_src_init(&aud_i2s_src->aud_src, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "aud_i2s_src");

	src->pool = &(aud_i2s_src->pool.pool);
	mpipe_aud_buffer_pool_init(src->pool);

	aud_i2s_src->pool.aud_dev = DEVICE_DT_GET_OR_NULL(DT_ALIAS(i2s_codec_rx));
	aud_i2s_src->codec_dev = DEVICE_DT_GET_OR_NULL(DT_ALIAS(audio_codec_capture));
	aud_i2s_src->clk_role = MPIPE_AUD_I2S_CONTROLLER_CODEC_TARGET;

	aud_i2s_src->aud_src.get_audio_caps = mpipe_aud_i2s_src_get_audio_caps;

	src_parent_set_property = self->object.set_property;
	src_parent_get_property = self->object.get_property;
	self->object.set_property = mpipe_aud_i2s_src_set_property;
	self->object.get_property = mpipe_aud_i2s_src_get_property;

	src->set_caps = mpipe_aud_i2s_src_set_caps;
	src->pool->acquire_buffer = mpipe_aud_i2s_src_acquire_buffer;
	src->pool->start = mpipe_aud_i2s_src_start;

	pool_parent_stop = src->pool->stop;
	src->pool->stop = mpipe_aud_i2s_src_stop;

	mpipe_aud_i2s_src_update_caps(src);

	return 0;
}
