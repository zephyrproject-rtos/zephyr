/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/logging/log.h>

#include <src/core/mp_buffer.h>

#include "mp_zaud.h"
#include "mp_zaud_i2s_codec_sink.h"
#include "mp_zaud_property.h"

LOG_MODULE_REGISTER(mp_zaud_i2s_codec_sink, CONFIG_LIBMP_LOG_LEVEL);

#define DEFAULT_PROP_I2S_DEVICE   DEVICE_DT_GET(DT_ALIAS(i2s_codec_tx))
#define DEFAULT_PROP_CODEC_DEVICE DEVICE_DT_GET(DT_NODELABEL(audio_codec));

static int mp_zaud_i2s_codec_sink_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_zaud_i2s_codec_sink *zaud_i2s_codec_sink = MP_ZAUD_I2S_CODEC_SINK(obj);

	switch (key) {
	case PROP_ZAUD_SINK_SLAB_PTR:
		zaud_i2s_codec_sink->mem_slab = (struct k_mem_slab *)val;
		break;
	default:
		return mp_sink_set_property(obj, key, val);
	}

	return 0;
}

static int mp_zaud_i2s_codec_sink_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_zaud_i2s_codec_sink *zaud_i2s_codec_sink = MP_ZAUD_I2S_CODEC_SINK(obj);

	if (val == NULL) {
		return -1;
	}

	switch (key) {
	case PROP_ZAUD_SRC_SLAB_PTR:
		if (zaud_i2s_codec_sink->mem_slab != NULL) {
			*(void **)val = (void *)zaud_i2s_codec_sink->mem_slab;
		} else {
			*(void **)val = NULL;
		}
		break;
	default:
		return mp_sink_get_property(obj, key, val);
	}
	return 0;
}

static struct mp_caps *mp_zaud_i2s_codec_sink_get_caps(struct mp_sink *sink)
{
	int ret = 0;
	uint8_t i = 0;
	struct audio_caps i2s_caps;
	struct audio_caps codec_caps;
	uint32_t sr = 0;
	uint32_t bw = 0;

	ret = i2s_get_caps(MP_ZAUD_I2S_CODEC_SINK(sink)->i2s_dev, &i2s_caps);

	if (ret != 0) {
		LOG_ERR("Failed to get I2S capabilities");
		return NULL;
	}

	ret = audio_codec_get_caps(MP_ZAUD_I2S_CODEC_SINK(sink)->codec_dev, &codec_caps);

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
	struct mp_structure *structure =
		mp_structure_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_LIST,
				 supported_sample_rate, MP_CAPS_BITWIDTH, MP_TYPE_LIST,
				 supported_bit_width, MP_CAPS_NUM_OF_CHANNEL, MP_TYPE_INT_RANGE,
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
				 1, MP_CAPS_BUFFER_COUNT, MP_TYPE_INT_RANGE,
				 (i2s_caps.min_num_buffers > codec_caps.min_num_buffers)
					 ? i2s_caps.min_num_buffers
					 : codec_caps.min_num_buffers,
				 UINT8_MAX, 1, MP_CAPS_INTERLEAVED, MP_TYPE_BOOLEAN,
				 codec_caps.interleaved, MP_CAPS_END);

	mp_caps_append(caps, structure);

	return caps;
}

static bool mp_zaud_i2s_codec_sink_set_caps(struct mp_sink *sink, struct mp_caps *caps)
{
	struct mp_zaud_i2s_codec_sink *zaud_i2s_codec_sink = MP_ZAUD_I2S_CODEC_SINK(sink);
	struct i2s_config config;
	struct audio_codec_cfg audio_cfg;

	struct mp_structure *first_structure = mp_caps_get_structure(caps, 0);

	int sample_rate =
		mp_value_get_int(mp_structure_get_value(first_structure, MP_CAPS_SAMPLE_RATE));
	int bit_width = mp_value_get_int(mp_structure_get_value(first_structure, MP_CAPS_BITWIDTH));
	int num_of_channel =
		mp_value_get_int(mp_structure_get_value(first_structure, MP_CAPS_NUM_OF_CHANNEL));
	uint32_t frame_interval =
		mp_value_get_int(mp_structure_get_value(first_structure, MP_CAPS_FRAME_INTERVAL));

	if (zaud_i2s_codec_sink->mem_slab == NULL) {
		LOG_ERR("Memory slab not configured");
		return false;
	}

	audio_cfg.dai_route = AUDIO_ROUTE_PLAYBACK;
	audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	audio_cfg.dai_cfg.i2s.word_size = bit_width;
	audio_cfg.dai_cfg.i2s.channels = num_of_channel;
	audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
#ifdef CONFIG_ZAUD_I2S_CODEC_SINK_CODEC_MASTER
	audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
#else
	audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_SLAVE | I2S_OPT_BIT_CLK_SLAVE;
#endif
	audio_cfg.dai_cfg.i2s.frame_clk_freq = sample_rate;
	audio_cfg.dai_cfg.i2s.mem_slab = zaud_i2s_codec_sink->mem_slab;
	audio_cfg.dai_cfg.i2s.block_size =
		(bit_width >> 3) * ((sample_rate * frame_interval / 1000000) * num_of_channel);
	audio_codec_configure(zaud_i2s_codec_sink->codec_dev, &audio_cfg);
	k_msleep(1000);

	config.word_size = bit_width;
	config.channels = num_of_channel;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
#ifdef CONFIG_ZAUD_I2S_CODEC_SINK_I2S_MASTER
	config.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
#else
	config.options = I2S_OPT_BIT_CLK_SLAVE | I2S_OPT_FRAME_CLK_SLAVE;
#endif
	config.frame_clk_freq = sample_rate;
	config.mem_slab = zaud_i2s_codec_sink->mem_slab;
	config.block_size =
		(bit_width >> 3) * ((sample_rate * frame_interval / 1000000) * num_of_channel);
	config.timeout = frame_interval * 10;

	if (i2s_configure(zaud_i2s_codec_sink->i2s_dev, I2S_DIR_TX, &config) < 0) {
		LOG_DBG("Failed to configure codec stream\n");
		return false;
	}

	mp_caps_replace(&(sink->sinkpad.caps), caps);

	return true;
}

bool mp_zaud_i2s_codec_sink_chainfn(struct mp_pad *pad, struct mp_buffer *buffer)
{
	struct mp_zaud_i2s_codec_sink *zaud_i2s_codec_sink =
		MP_ZAUD_I2S_CODEC_SINK(pad->object.container);
	int ret = -1;

	ret = i2s_write(zaud_i2s_codec_sink->i2s_dev, buffer->data, buffer->pool->config.size);
	if (ret < 0) {
		LOG_DBG("Failed to write data: %d\n", ret);
		return false;
	}

	if (!zaud_i2s_codec_sink->started) {
		zaud_i2s_codec_sink->count++;
		if (zaud_i2s_codec_sink->count == 2) {
			i2s_trigger(zaud_i2s_codec_sink->i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
			zaud_i2s_codec_sink->started = true;
		}
	}
	return true;
}

void mp_zaud_i2s_codec_sink_init(struct mp_element *self)
{
	struct mp_sink *sink = MP_SINK(self);
	struct mp_zaud_i2s_codec_sink *zaud_i2s_codec_sink = MP_ZAUD_I2S_CODEC_SINK(self);

	/* Init base class */
	mp_sink_init(self);

#if DT_NODE_EXISTS(DT_ALIAS(i2s_codec_tx))
	zaud_i2s_codec_sink->i2s_dev = DEVICE_DT_GET(DT_ALIAS(i2s_codec_tx));
#else
#error "i2s_codec_tx node alias not found in device tree."
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(audio_codec))
	zaud_i2s_codec_sink->codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));
#else
#error "audio_codec node label not found in device tree."
#endif

	if (!device_is_ready(zaud_i2s_codec_sink->i2s_dev)) {
		LOG_ERR("%s is not ready\n", zaud_i2s_codec_sink->i2s_dev->name);
		return;
	}

	if (!device_is_ready(zaud_i2s_codec_sink->codec_dev)) {
		LOG_ERR("%s is not ready", zaud_i2s_codec_sink->codec_dev->name);
		return;
	}

	self->object.get_property = mp_zaud_i2s_codec_sink_get_property;
	self->object.set_property = mp_zaud_i2s_codec_sink_set_property;

	sink->sinkpad.chainfn = mp_zaud_i2s_codec_sink_chainfn;
	sink->sinkpad.caps = mp_zaud_i2s_codec_sink_get_caps(sink);
	sink->set_caps = mp_zaud_i2s_codec_sink_set_caps;
	sink->get_caps = mp_zaud_i2s_codec_sink_get_caps;

	zaud_i2s_codec_sink->started = false;
	zaud_i2s_codec_sink->count = 0;
	zaud_i2s_codec_sink->mem_slab = NULL;
}
