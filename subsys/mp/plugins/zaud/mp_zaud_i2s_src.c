/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>

#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/core/mp_structure.h>
#include <zephyr/mp/core/mp_value.h>

#include <zephyr/mp/zaud/mp_zaud.h>
#include <zephyr/mp/zaud/mp_zaud_buffer_pool.h>
#include <zephyr/mp/zaud/mp_zaud_i2s_src.h>
#include <zephyr/mp/zaud/mp_zaud_property.h>

LOG_MODULE_REGISTER(mp_zaud_i2s_src, CONFIG_MP_LOG_LEVEL);

static int (*src_parent_set_property)(struct mp_object *, uint32_t, const void *);
static int (*src_parent_get_property)(struct mp_object *, uint32_t, void *);

static void mp_zaud_i2s_src_update_caps(struct mp_src *src);

/* The base class's callback takes no direction, so bind it to receive. */
static int mp_zaud_i2s_src_get_audio_caps(const struct device *dev, struct audio_caps *caps)
{
	return i2s_get_caps(dev, caps, I2S_DIR_RX);
}

static int mp_zaud_i2s_src_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_zaud_i2s_src *self = (struct mp_zaud_i2s_src *)obj;

	switch (key) {
	case PROP_ZAUD_SRC_CODEC_DEVICE:
		self->codec_dev = (const struct device *)val;

		/* Codec set, update supported caps */
		mp_zaud_i2s_src_update_caps(&self->zaud_src.src);
		break;
	case PROP_ZAUD_SRC_CLK_ROLE:
		if ((enum mp_zaud_i2s_codec_clk_role)(uintptr_t)val !=
			    MP_ZAUD_I2S_CONTROLLER_CODEC_TARGET &&
		    (enum mp_zaud_i2s_codec_clk_role)(uintptr_t)val !=
			    MP_ZAUD_I2S_TARGET_CODEC_CONTROLLER) {
			LOG_ERR("Invalid clock role value");
			return -EINVAL;
		}
		self->clk_role = (enum mp_zaud_i2s_codec_clk_role)(uintptr_t)val;
		break;
	default:
		return src_parent_set_property(obj, key, val);
	}

	return 0;
}

static int mp_zaud_i2s_src_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_zaud_i2s_src *self = (struct mp_zaud_i2s_src *)obj;

	if (val == NULL) {
		return -1;
	}

	switch (key) {
	case PROP_ZAUD_SRC_CODEC_DEVICE:
		*(const struct device **)val = self->codec_dev;
		break;
	case PROP_ZAUD_SRC_CLK_ROLE:
		*(enum mp_zaud_i2s_codec_clk_role *)val = self->clk_role;
		break;
	default:
		return src_parent_get_property(obj, key, val);
	}

	return 0;
}

/* With a capture codec the source can only produce what both it and the I2S
 * receiver support, so intersect the two capability sets.
 */
static struct mp_caps *mp_zaud_i2s_src_supported_caps(struct mp_src *src)
{
	struct mp_zaud_i2s_src *self = (struct mp_zaud_i2s_src *)src;
	struct mp_zaud_buffer_pool *pool =
		CONTAINER_OF(src->pool, struct mp_zaud_buffer_pool, pool);
	struct audio_caps caps_eff;
	struct audio_caps codec_caps;
	int i = 0;
	uint32_t sr = 0;
	uint32_t bw = 0;

	if (self->codec_dev == NULL) {
		return mp_zaud_src_supported_caps(src);
	}

	if (pool->zaud_dev == NULL) {
		LOG_ERR("I2S device not configured");
		return NULL;
	}

	if (i2s_get_caps(pool->zaud_dev, &caps_eff, I2S_DIR_RX) != 0) {
		LOG_ERR("Failed to get I2S capabilities");
		return NULL;
	}

	if (audio_codec_get_caps(self->codec_dev, &codec_caps) != 0) {
		LOG_ERR("Failed to get codec capabilities");
		return NULL;
	}

	if (caps_eff.interleaved != codec_caps.interleaved) {
		LOG_ERR("Interleaved capabilities mismatch between I2S and codec");
		return NULL;
	}

	caps_eff.supported_sample_rates &= codec_caps.supported_sample_rates;
	caps_eff.supported_bit_widths &= codec_caps.supported_bit_widths;
	caps_eff.min_total_channels =
		MAX(caps_eff.min_total_channels, codec_caps.min_total_channels);
	caps_eff.max_total_channels =
		MIN(caps_eff.max_total_channels, codec_caps.max_total_channels);
	caps_eff.min_frame_interval =
		MAX(caps_eff.min_frame_interval, codec_caps.min_frame_interval);
	caps_eff.max_frame_interval =
		MIN(caps_eff.max_frame_interval, codec_caps.max_frame_interval);
	caps_eff.min_num_buffers = MAX(caps_eff.min_num_buffers, codec_caps.min_num_buffers);

	struct mp_value *supported_sample_rate = mp_value_new(MP_TYPE_LIST, NULL);
	struct mp_value *supported_bit_width = mp_value_new(MP_TYPE_LIST, NULL);

	uint32_t sample_rates = caps_eff.supported_sample_rates;
	uint32_t bit_widths = caps_eff.supported_bit_widths;

	i = 0;
	while (sample_rates > 0) {
		if ((sample_rates & 0x1U) != 0U) {
			sr = audio2mp_sample_rate(1 << i);

			if (sr > 0) {
				mp_value_list_append(supported_sample_rate,
						     mp_value_new(MP_TYPE_UINT, sr));
			}
		}
		sample_rates >>= 1;
		i++;
	}

	i = 0;
	while (bit_widths > 0) {
		if ((bit_widths & 0x1U) != 0U) {
			bw = audio2mp_bit_width(1 << i);

			if (bw > 0) {
				mp_value_list_append(supported_bit_width,
						     mp_value_new(MP_TYPE_UINT, bw));
			}
		}
		bit_widths >>= 1;
		i++;
	}

	struct mp_caps *caps = mp_caps_new(MP_MEDIA_END);
	struct mp_structure *structure = mp_structure_new(
		MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_LIST, supported_sample_rate,
		MP_CAPS_BITWIDTH, MP_TYPE_LIST, supported_bit_width, MP_CAPS_NUM_OF_CHANNEL,
		MP_TYPE_UINT_RANGE, caps_eff.min_total_channels, caps_eff.max_total_channels, 1,
		MP_CAPS_FRAME_INTERVAL, MP_TYPE_UINT_RANGE, caps_eff.min_frame_interval,
		caps_eff.max_frame_interval, 1, MP_CAPS_BUFFER_COUNT, MP_TYPE_UINT_RANGE,
		caps_eff.min_num_buffers, UINT8_MAX, 1, MP_CAPS_INTERLEAVED, MP_TYPE_BOOLEAN,
		caps_eff.interleaved, MP_CAPS_END);

	mp_caps_append(caps, structure);

	return caps;
}

static void mp_zaud_i2s_src_update_caps(struct mp_src *src)
{
	struct mp_caps *caps = mp_zaud_i2s_src_supported_caps(src);

	mp_src_update_caps(src, caps);
	mp_caps_unref(caps);
}

static int mp_zaud_i2s_src_set_caps(struct mp_src *src, struct mp_caps *caps)
{
	struct mp_zaud_i2s_src *self = (struct mp_zaud_i2s_src *)src;
	struct mp_zaud_buffer_pool *pool =
		CONTAINER_OF(src->pool, struct mp_zaud_buffer_pool, pool);
	struct i2s_config config;
	int ret;

	struct mp_structure *first_structure = mp_caps_get_structure(caps, 0);

	uint32_t sample_rate =
		mp_value_get_uint(mp_structure_get_value(first_structure, MP_CAPS_SAMPLE_RATE));
	uint32_t bit_width =
		mp_value_get_uint(mp_structure_get_value(first_structure, MP_CAPS_BITWIDTH));
	uint32_t num_of_channel =
		mp_value_get_uint(mp_structure_get_value(first_structure, MP_CAPS_NUM_OF_CHANNEL));
	uint32_t frame_interval =
		mp_value_get_uint(mp_structure_get_value(first_structure, MP_CAPS_FRAME_INTERVAL));

	if (pool->mem_slab == NULL) {
		LOG_ERR("Memory slab not configured");
		return -EINVAL;
	}

	/* The codec sink only configures a codec for playback, so this is what
	 * sets up the capture path.
	 */
	if (self->codec_dev != NULL) {
		struct audio_codec_cfg audio_cfg = {0};

		audio_cfg.dai_route = AUDIO_ROUTE_CAPTURE;
		audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
		audio_cfg.dai_cfg.i2s.word_size = bit_width;
		audio_cfg.dai_cfg.i2s.channels = num_of_channel;
		audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;

		if (self->clk_role == MP_ZAUD_I2S_TARGET_CODEC_CONTROLLER) {
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
	/* When the codec drives the clocks the receiver must consume them. */
	if (self->clk_role == MP_ZAUD_I2S_TARGET_CODEC_CONTROLLER) {
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

	ret = i2s_configure(pool->zaud_dev, I2S_DIR_RX, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure I2S stream: %d", ret);
		return ret;
	}

	mp_caps_replace(&(src->srcpad.caps), caps);

	return 0;
}

static int mp_zaud_i2s_src_acquire_buffer(struct mp_buffer_pool *pool, struct net_buf **buffer)
{
	struct mp_zaud_buffer_pool *zaud_pool =
		CONTAINER_OF(pool, struct mp_zaud_buffer_pool, pool);
	struct mp_buffer_meta *meta;
	void *mem_block = NULL;
	size_t bytes_used = pool->config.size;
	int err;

	err = i2s_read(zaud_pool->zaud_dev, &mem_block, &bytes_used);
	if (err < 0) {
		LOG_ERR("Unable to read an I2S buffer: %d", err);
		return err;
	}

	for (uint8_t i = 0; i < pool->config.min_buffers; i++) {
		if (mem_block == zaud_pool->blocks[i]) {
			*buffer = net_buf_alloc_with_data(pool->nb_pool, zaud_pool->blocks[i],
							  pool->config.size, K_NO_WAIT);
			if (*buffer == NULL) {
				LOG_ERR("Unable to allocate net_buf wrapper for I2S buffer");
				k_mem_slab_free(zaud_pool->mem_slab, mem_block);
				return -ENOBUFS;
			}

			(*buffer)->len = bytes_used;

			meta = mp_buffer_get_meta(*buffer);
			meta->pool = pool;
			meta->bytes_used = bytes_used;
			meta->timestamp = 0U;
			meta->driver_buf = NULL;
			meta->priv = NULL;

			return 0;
		}
	}

	LOG_ERR("Unable to match I2S buffer %p with mem_slab backing store", mem_block);
	k_mem_slab_free(zaud_pool->mem_slab, mem_block);
	return -ENOENT;
}

static int mp_zaud_i2s_src_start(struct mp_buffer_pool *pool)
{
	struct mp_zaud_buffer_pool *zaud_pool =
		CONTAINER_OF(pool, struct mp_zaud_buffer_pool, pool);

	if (i2s_trigger(zaud_pool->zaud_dev, I2S_DIR_RX, I2S_TRIGGER_START) < 0) {
		LOG_ERR("Unable to start I2S capture");
		return -EIO;
	}

	LOG_INF("Capture started now");
	return 0;
}

void mp_zaud_i2s_src_init(struct mp_element *self)
{
	struct mp_src *src = (struct mp_src *)self;
	struct mp_zaud_i2s_src *zaud_i2s_src = (struct mp_zaud_i2s_src *)src;

	/* Init base class */
	mp_zaud_src_init(&zaud_i2s_src->zaud_src.src.element);

	/* Initialize buffer pool */
	src->pool = &(zaud_i2s_src->pool.pool);
	mp_zaud_buffer_pool_init(src->pool);

	zaud_i2s_src->pool.zaud_dev = DEVICE_DT_GET_OR_NULL(DT_ALIAS(i2s_codec_rx));
	zaud_i2s_src->codec_dev = DEVICE_DT_GET_OR_NULL(DT_ALIAS(audio_codec_capture));
	zaud_i2s_src->clk_role = MP_ZAUD_I2S_CONTROLLER_CODEC_TARGET;

	zaud_i2s_src->zaud_src.get_audio_caps = mp_zaud_i2s_src_get_audio_caps;

	src_parent_get_property = self->object.get_property;
	src_parent_set_property = self->object.set_property;
	self->object.get_property = mp_zaud_i2s_src_get_property;
	self->object.set_property = mp_zaud_i2s_src_set_property;

	src->set_caps = mp_zaud_i2s_src_set_caps;
	src->pool->acquire_buffer = mp_zaud_i2s_src_acquire_buffer;
	src->pool->start = mp_zaud_i2s_src_start;

	mp_zaud_i2s_src_update_caps(src);
}
