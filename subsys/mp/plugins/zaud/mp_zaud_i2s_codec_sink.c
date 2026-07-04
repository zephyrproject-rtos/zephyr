/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_structure.h>
#include <zephyr/mp/core/mp_value.h>

#include <zephyr/mp/zaud/mp_zaud.h>
#include <zephyr/mp/zaud/mp_zaud_i2s_codec_sink.h>
#include <zephyr/mp/zaud/mp_zaud_property.h>

LOG_MODULE_REGISTER(mp_zaud_i2s_codec_sink, CONFIG_MP_LOG_LEVEL);

#define DEFAULT_PROP_I2S_DEVICE   DEVICE_DT_GET(DT_ALIAS(i2s_codec_tx))
#define DEFAULT_PROP_CODEC_DEVICE DEVICE_DT_GET(DT_NODELABEL(audio_codec));

/*
 * Number of buffers to queue into the I2S TX FIFO before issuing the START
 * trigger. The source and sink are clocked at the same rate, so the I2S
 * consumer can momentarily get one buffer ahead of the source right after
 * start. Priming more than one buffer keeps the TX queue from draining to
 * empty on a scheduling tie, which would otherwise raise a TX underrun.
 * This needs the negotiated buffer count to be at least 2 (true for any
 * double-buffered codec).
 */
#define ZAUD_I2S_SINK_START_PRIME 3

static struct mp_caps *mp_zaud_i2s_codec_sink_supported_caps(struct mp_sink *sink)
{
	int ret = 0;
	uint8_t i = 0;
	struct audio_caps i2s_caps;
	struct audio_caps codec_caps;
	uint32_t sr = 0;
	uint32_t bw = 0;
	struct mp_zaud_i2s_codec_sink *zaud = (struct mp_zaud_i2s_codec_sink *)sink;

	ret = i2s_get_caps(zaud->i2s_dev, &i2s_caps, I2S_DIR_TX);

	if (ret != 0) {
		LOG_ERR("Failed to get I2S capabilities");
		return NULL;
	}

	ret = audio_codec_get_caps(zaud->codec_dev, &codec_caps);

	if (ret != 0) {
		LOG_ERR("Failed to get codec capabilities");
		return NULL;
	}

	struct mp_value *supported_sample_rate = mp_value_new(MP_TYPE_LIST, NULL);
	struct mp_value *supported_bit_width = mp_value_new(MP_TYPE_LIST, NULL);

	uint32_t sample_rates = i2s_caps.supported_sample_rates & codec_caps.supported_sample_rates;
	uint32_t bit_widths = i2s_caps.supported_bit_widths & codec_caps.supported_bit_widths;

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

	if (i2s_caps.interleaved != codec_caps.interleaved) {
		LOG_ERR("Interleaved capabilities mismatch between I2S and codec");
		return NULL;
	}

	struct mp_caps *caps = mp_caps_new(MP_MEDIA_END);
	uint8_t min_num_buffers = i2s_caps.min_num_buffers;

	if (codec_caps.min_num_buffers > min_num_buffers) {
		min_num_buffers = codec_caps.min_num_buffers;
	}

	/*
	 * The sink primes ZAUD_I2S_SINK_START_PRIME buffers into the I2S TX
	 * queue before issuing the START trigger, holding that many buffers
	 * from the shared pool before any are transmitted and returned. Make
	 * sure the negotiated pool can satisfy this, otherwise the source
	 * starves before the sink ever starts.
	 */
	if (min_num_buffers < ZAUD_I2S_SINK_START_PRIME) {
		min_num_buffers = ZAUD_I2S_SINK_START_PRIME;
	}

	struct mp_structure *structure = mp_structure_new(
		MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_LIST, supported_sample_rate,
		MP_CAPS_BITWIDTH, MP_TYPE_LIST, supported_bit_width, MP_CAPS_NUM_OF_CHANNEL,
		MP_TYPE_UINT_RANGE,
		(i2s_caps.min_total_channels > codec_caps.min_total_channels)
			? i2s_caps.min_total_channels
			: codec_caps.min_total_channels,
		(i2s_caps.max_total_channels < codec_caps.max_total_channels)
			? i2s_caps.max_total_channels
			: codec_caps.max_total_channels,
		1, MP_CAPS_FRAME_INTERVAL, MP_TYPE_UINT_RANGE,
		(i2s_caps.min_frame_interval > codec_caps.min_frame_interval)
			? i2s_caps.min_frame_interval
			: codec_caps.min_frame_interval,
		(i2s_caps.max_frame_interval < codec_caps.max_frame_interval)
			? i2s_caps.max_frame_interval
			: codec_caps.max_frame_interval,
		1, MP_CAPS_BUFFER_COUNT, MP_TYPE_UINT_RANGE, min_num_buffers, UINT8_MAX, 1,
		MP_CAPS_INTERLEAVED, MP_TYPE_BOOLEAN, codec_caps.interleaved, MP_CAPS_END);

	mp_caps_append(caps, structure);

	return caps;
}

static void mp_zaud_i2s_codec_sink_update_caps(struct mp_sink *sink)
{
	struct mp_caps *caps = mp_zaud_i2s_codec_sink_supported_caps(sink);

	mp_sink_update_caps(sink, caps);
	mp_caps_unref(caps);
}

static int mp_zaud_i2s_codec_sink_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_zaud_i2s_codec_sink *zaud_i2s_codec_sink = (struct mp_zaud_i2s_codec_sink *)obj;

	switch (key) {
	case PROP_ZAUD_SINK_SLAB_PTR:
		zaud_i2s_codec_sink->mem_slab = (struct k_mem_slab *)val;
		break;
	case PROP_ZAUD_SINK_CLK_ROLE:
		if ((enum mp_zaud_i2s_codec_clk_role)(uintptr_t)val !=
			    MP_ZAUD_I2S_CONTROLLER_CODEC_TARGET &&
		    (enum mp_zaud_i2s_codec_clk_role)(uintptr_t)val !=
			    MP_ZAUD_I2S_TARGET_CODEC_CONTROLLER) {
			LOG_ERR("Invalid clock role value");
			return -EINVAL;
		}
		zaud_i2s_codec_sink->clk_role = (enum mp_zaud_i2s_codec_clk_role)(uintptr_t)val;
		break;
	case PROP_ZAUD_SINK_I2S_DEVICE:
		zaud_i2s_codec_sink->i2s_dev = (const struct device *)val;

		/* Device set, update supported caps */
		mp_zaud_i2s_codec_sink_update_caps(&zaud_i2s_codec_sink->sink);
		break;
	case PROP_ZAUD_SINK_CODEC_DEVICE:
		zaud_i2s_codec_sink->codec_dev = (const struct device *)val;

		/* Device set, update supported caps */
		mp_zaud_i2s_codec_sink_update_caps(&zaud_i2s_codec_sink->sink);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mp_zaud_i2s_codec_sink_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_zaud_i2s_codec_sink *zaud_i2s_codec_sink = (struct mp_zaud_i2s_codec_sink *)obj;

	if (val == NULL) {
		return -1;
	}

	switch (key) {
	case PROP_ZAUD_SINK_SLAB_PTR:
		if (zaud_i2s_codec_sink->mem_slab != NULL) {
			*(void **)val = (void *)zaud_i2s_codec_sink->mem_slab;
		} else {
			*(void **)val = NULL;
		}
		break;
	case PROP_ZAUD_SINK_CLK_ROLE:
		*(enum mp_zaud_i2s_codec_clk_role *)val = zaud_i2s_codec_sink->clk_role;
		break;
	case PROP_ZAUD_SINK_I2S_DEVICE:
		*(const struct device **)val = zaud_i2s_codec_sink->i2s_dev;
		break;
	case PROP_ZAUD_SINK_CODEC_DEVICE:
		*(const struct device **)val = zaud_i2s_codec_sink->codec_dev;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mp_zaud_i2s_codec_sink_set_caps(struct mp_sink *sink, struct mp_caps *caps)
{
	struct mp_zaud_i2s_codec_sink *zaud_i2s_codec_sink = (struct mp_zaud_i2s_codec_sink *)sink;
	struct i2s_config config;
	struct audio_codec_cfg audio_cfg;
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

	if (zaud_i2s_codec_sink->mem_slab == NULL) {
		LOG_ERR("Memory slab not configured");
		return -EINVAL;
	}

	audio_cfg.dai_route = AUDIO_ROUTE_PLAYBACK;
	audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	audio_cfg.dai_cfg.i2s.word_size = bit_width;
	audio_cfg.dai_cfg.i2s.channels = num_of_channel;
	audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;

	if (zaud_i2s_codec_sink->clk_role == MP_ZAUD_I2S_TARGET_CODEC_CONTROLLER) {
		audio_cfg.dai_cfg.i2s.options =
			I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER;
	} else {
		audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_BIT_CLK_TARGET;
	}

	audio_cfg.dai_cfg.i2s.frame_clk_freq = sample_rate;
	audio_cfg.dai_cfg.i2s.mem_slab = zaud_i2s_codec_sink->mem_slab;
	audio_cfg.dai_cfg.i2s.block_size =
		(bit_width >> 3) * ((sample_rate * frame_interval / 1000000) * num_of_channel);
	ret = audio_codec_configure(zaud_i2s_codec_sink->codec_dev, &audio_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure codec: %d", ret);
		return ret;
	}

#ifdef CONFIG_MP_ZAUD_I2S_CODEC_SINK_SET_OUTPUT_VOLUME
	ret = audio_codec_set_property(
		zaud_i2s_codec_sink->codec_dev, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
		(audio_property_value_t){.vol = CONFIG_MP_ZAUD_I2S_CODEC_SINK_OUTPUT_VOLUME});
	if (ret < 0) {
		LOG_WRN("Failed to set codec output volume: %d", ret);
	} else {
		ret = audio_codec_apply_properties(zaud_i2s_codec_sink->codec_dev);
		if (ret < 0) {
			LOG_WRN("Failed to apply codec properties: %d", ret);
		}
	}
#endif

	k_msleep(1000);

	config.word_size = bit_width;
	config.channels = num_of_channel;
	config.format = I2S_FMT_DATA_FORMAT_I2S;

	if (zaud_i2s_codec_sink->clk_role == MP_ZAUD_I2S_CONTROLLER_CODEC_TARGET) {
		config.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER;
	} else {
		config.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
	}

	config.frame_clk_freq = sample_rate;
	config.mem_slab = zaud_i2s_codec_sink->mem_slab;
	config.block_size =
		(bit_width >> 3) * ((sample_rate * frame_interval / 1000000) * num_of_channel);
	config.timeout = frame_interval * 10;

	ret = i2s_configure(zaud_i2s_codec_sink->i2s_dev, I2S_DIR_TX, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure I2S stream: %d", ret);
		return ret;
	}

	return 0;
}

int mp_zaud_i2s_codec_sink_chainfn(struct mp_pad *pad, struct net_buf *in_buf,
				   struct net_buf **out_buf)
{
	struct mp_zaud_i2s_codec_sink *zaud_i2s_codec_sink =
		CONTAINER_OF(pad->object.container, struct mp_zaud_i2s_codec_sink,
			     sink.element.object);
	uint32_t bytes_used = mp_buffer_get_meta(in_buf)->bytes_used;
	int ret = -1;

	ret = i2s_write(zaud_i2s_codec_sink->i2s_dev, in_buf->data, bytes_used);
	if (ret < 0) {
		LOG_DBG("Failed to write data: %d\n", ret);
		*out_buf = NULL;
		return -EIO;
	}

	if (!zaud_i2s_codec_sink->started) {
		zaud_i2s_codec_sink->count++;
		if (zaud_i2s_codec_sink->count == ZAUD_I2S_SINK_START_PRIME) {
			ret = i2s_trigger(zaud_i2s_codec_sink->i2s_dev, I2S_DIR_TX,
					  I2S_TRIGGER_START);
			if (ret < 0) {
				LOG_ERR("Failed to start I2S stream: %d", ret);
				*out_buf = NULL;
				return -EIO;
			}
			zaud_i2s_codec_sink->started = true;
		}
	}

	/* Done with the buffer */
	net_buf_unref(in_buf);

	/* Sink returns NULL - end of chain */
	*out_buf = NULL;

	return 0;
}

void mp_zaud_i2s_codec_sink_init(struct mp_element *self)
{
	struct mp_zaud_i2s_codec_sink *zaud_i2s_codec_sink = (struct mp_zaud_i2s_codec_sink *)self;
	struct mp_sink *sink = &zaud_i2s_codec_sink->sink;

	/* Init base class */
	mp_sink_init(self);

	zaud_i2s_codec_sink->i2s_dev = DEVICE_DT_GET_OR_NULL(DT_ALIAS(i2s_codec_tx));
	zaud_i2s_codec_sink->codec_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(audio_codec));

	zaud_i2s_codec_sink->clk_role = MP_ZAUD_I2S_CONTROLLER_CODEC_TARGET;

	self->object.get_property = mp_zaud_i2s_codec_sink_get_property;
	self->object.set_property = mp_zaud_i2s_codec_sink_set_property;

	sink->sinkpad.chainfn = mp_zaud_i2s_codec_sink_chainfn;
	sink->set_caps = mp_zaud_i2s_codec_sink_set_caps;

	mp_zaud_i2s_codec_sink_update_caps(sink);

	zaud_i2s_codec_sink->started = false;
	zaud_i2s_codec_sink->count = 0;
	zaud_i2s_codec_sink->mem_slab = NULL;
}
