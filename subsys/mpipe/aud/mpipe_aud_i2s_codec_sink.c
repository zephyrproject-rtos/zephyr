/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/mpipe/aud/mpipe_aud.h>
#include <zephyr/mpipe/aud/mpipe_aud_i2s_codec_sink.h>

LOG_MODULE_REGISTER(mpipe_aud_i2s_codec_sink, CONFIG_MPIPE_LOG_LEVEL);

#define DEFAULT_PROP_I2S_DEVICE   DEVICE_DT_GET(DT_ALIAS(i2s_codec_tx))
#define DEFAULT_PROP_CODEC_DEVICE DEVICE_DT_GET(DT_NODELABEL(audio_codec));

/* Codec from the I2S TX node's codec phandle, else the audio_codec node. */
#if DT_NODE_HAS_PROP(DT_ALIAS(i2s_codec_tx), codec)
#define AUD_SINK_CODEC_NODE DT_PHANDLE(DT_ALIAS(i2s_codec_tx), codec)
#else
#define AUD_SINK_CODEC_NODE DT_NODELABEL(audio_codec)
#endif

/*
 * Number of buffers to queue into the I2S TX FIFO before issuing the START
 * trigger. The source and sink are clocked at the same rate, so the I2S
 * consumer can momentarily get one buffer ahead of the source right after
 * start. Priming more than one buffer keeps the TX queue from draining to
 * empty on a scheduling tie, which would otherwise raise a TX underrun.
 * This needs the negotiated buffer count to be at least 2 (true for any
 * double-buffered codec).
 */
#define AUD_I2S_SINK_START_PRIME 3

/* A capability of this sink is one the I2S link and the codec both support */
static int mpipe_aud_i2s_codec_sink_enum_caps(struct mpipe_pad *pad, uint32_t index,
					      const struct mpipe_structure *filter,
					      struct mpipe_structure *out)
{
	struct mpipe_aud_i2s_codec_sink *aud =
		(struct mpipe_aud_i2s_codec_sink *)pad->object.container;
	struct audio_caps i2s_caps;
	struct audio_caps codec_caps;
	struct audio_caps caps;

	if (i2s_get_caps(aud->i2s_dev, &i2s_caps, I2S_DIR_TX) != 0) {
		LOG_ERR("Failed to get I2S capabilities");
		return -ENODEV;
	}

	if (audio_codec_get_caps(aud->codec_dev, &codec_caps) != 0) {
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
	caps.interleaved = codec_caps.interleaved;

	return mpipe_aud_enum_caps(&caps, index, filter, out);
}

/*
 * Buffer count is settled through the buffer pool query rather than caps. The
 * I2S driver primes its transmit queue before the START trigger, holding that
 * many buffers from the shared pool before any are transmitted and returned,
 * so make sure the upstream pool can satisfy it, otherwise the source starves
 * before the sink ever starts.
 */
static int mpipe_aud_i2s_codec_sink_propose_buffer_pool(struct mpipe_sink *sink,
							struct mpipe_dispatch *query)
{
	struct mpipe_aud_i2s_codec_sink *aud = (struct mpipe_aud_i2s_codec_sink *)sink;
	struct mpipe_buffer_pool_config cfg = {0};
	struct audio_caps i2s_caps;
	struct audio_caps codec_caps;

	if (i2s_get_caps(aud->i2s_dev, &i2s_caps, I2S_DIR_TX) != 0) {
		LOG_ERR("Failed to get I2S capabilities");
		return -ENODEV;
	}

	if (audio_codec_get_caps(aud->codec_dev, &codec_caps) != 0) {
		LOG_ERR("Failed to get codec capabilities");
		return -ENODEV;
	}

	cfg.min_buffers = MAX(MAX(i2s_caps.min_num_buffers, codec_caps.min_num_buffers),
			      AUD_I2S_SINK_START_PRIME);

	query->pool_cfg = cfg;

	return 0;
}

static void mpipe_aud_i2s_codec_sink_update_caps(struct mpipe_sink *sink)
{
	/* The capabilities are enumerated from the devices, so nothing is built here */
	sink->sink_pad.enum_caps_fn = mpipe_aud_i2s_codec_sink_enum_caps;
}

static int mpipe_aud_i2s_codec_sink_set_property(struct mpipe_object *obj, uint32_t key,
						 const void *val)
{
	struct mpipe_aud_i2s_codec_sink *aud_i2s_codec_sink =
		(struct mpipe_aud_i2s_codec_sink *)obj;

	switch (key) {
	case MPIPE_PROP_AUD_SINK_SLAB_PTR:
		aud_i2s_codec_sink->mem_slab = (struct k_mem_slab *)val;
		break;
	case MPIPE_PROP_AUD_SINK_CLK_ROLE:
		if ((enum mpipe_aud_i2s_codec_clk_role)(uintptr_t)val !=
			    MPIPE_AUD_I2S_CONTROLLER_CODEC_TARGET &&
		    (enum mpipe_aud_i2s_codec_clk_role)(uintptr_t)val !=
			    MPIPE_AUD_I2S_TARGET_CODEC_CONTROLLER) {
			LOG_ERR("Invalid clock role value");
			return -EINVAL;
		}
		aud_i2s_codec_sink->clk_role = (enum mpipe_aud_i2s_codec_clk_role)(uintptr_t)val;
		break;
	case MPIPE_PROP_AUD_SINK_I2S_DEVICE:
		aud_i2s_codec_sink->i2s_dev = (const struct device *)val;

		/* Device set, update supported caps */
		mpipe_aud_i2s_codec_sink_update_caps(&aud_i2s_codec_sink->sink);
		break;
	case MPIPE_PROP_AUD_SINK_CODEC_DEVICE:
		aud_i2s_codec_sink->codec_dev = (const struct device *)val;

		/* Device set, update supported caps */
		mpipe_aud_i2s_codec_sink_update_caps(&aud_i2s_codec_sink->sink);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mpipe_aud_i2s_codec_sink_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe_aud_i2s_codec_sink *aud_i2s_codec_sink =
		(struct mpipe_aud_i2s_codec_sink *)obj;

	if (val == NULL) {
		return -1;
	}

	switch (key) {
	case MPIPE_PROP_AUD_SINK_SLAB_PTR:
		if (aud_i2s_codec_sink->mem_slab != NULL) {
			*(void **)val = (void *)aud_i2s_codec_sink->mem_slab;
		} else {
			*(void **)val = NULL;
		}
		break;
	case MPIPE_PROP_AUD_SINK_CLK_ROLE:
		*(enum mpipe_aud_i2s_codec_clk_role *)val = aud_i2s_codec_sink->clk_role;
		break;
	case MPIPE_PROP_AUD_SINK_I2S_DEVICE:
		*(const struct device **)val = aud_i2s_codec_sink->i2s_dev;
		break;
	case MPIPE_PROP_AUD_SINK_CODEC_DEVICE:
		*(const struct device **)val = aud_i2s_codec_sink->codec_dev;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mpipe_aud_i2s_codec_sink_set_caps(struct mpipe_sink *sink,
					     const struct mpipe_structure *caps)
{
	struct mpipe_aud_i2s_codec_sink *aud_i2s_codec_sink =
		(struct mpipe_aud_i2s_codec_sink *)sink;
	struct i2s_config config;
	struct audio_codec_cfg audio_cfg;
	int ret;

	uint32_t sample_rate, bit_width, num_of_channel, frame_interval;

	if (mpipe_aud_caps_get_uint(caps, MPIPE_CAPS_SAMPLE_RATE, &sample_rate) != 0 ||
	    mpipe_aud_caps_get_uint(caps, MPIPE_CAPS_BITWIDTH, &bit_width) != 0 ||
	    mpipe_aud_caps_get_uint(caps, MPIPE_CAPS_NUM_OF_CHANNEL, &num_of_channel) != 0 ||
	    mpipe_aud_caps_get_uint(caps, MPIPE_CAPS_FRAME_INTERVAL, &frame_interval) != 0) {
		return -EINVAL;
	}

	if (aud_i2s_codec_sink->mem_slab == NULL) {
		LOG_ERR("Memory slab not configured");
		return -EINVAL;
	}

	audio_cfg.dai_route = AUDIO_ROUTE_PLAYBACK;
	audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	audio_cfg.dai_cfg.i2s.word_size = bit_width;
	audio_cfg.dai_cfg.i2s.channels = num_of_channel;
	audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;

	if (aud_i2s_codec_sink->clk_role == MPIPE_AUD_I2S_TARGET_CODEC_CONTROLLER) {
		audio_cfg.dai_cfg.i2s.options =
			I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER;
	} else {
		audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_BIT_CLK_TARGET;
	}

	audio_cfg.dai_cfg.i2s.frame_clk_freq = sample_rate;
	audio_cfg.dai_cfg.i2s.mem_slab = aud_i2s_codec_sink->mem_slab;
	audio_cfg.dai_cfg.i2s.block_size =
		(bit_width >> 3) * ((sample_rate * frame_interval / 1000000) * num_of_channel);
	ret = audio_codec_configure(aud_i2s_codec_sink->codec_dev, &audio_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure codec: %d", ret);
		return ret;
	}

#ifdef CONFIG_MPIPE_AUD_I2S_CODEC_SINK_SET_OUTPUT_VOLUME
	ret = audio_codec_set_property(
		aud_i2s_codec_sink->codec_dev, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
		(audio_property_value_t){.vol = CONFIG_MPIPE_AUD_I2S_CODEC_SINK_OUTPUT_VOLUME});
	if (ret < 0) {
		LOG_WRN("Failed to set codec output volume: %d", ret);
	} else {
		ret = audio_codec_apply_properties(aud_i2s_codec_sink->codec_dev);
		if (ret < 0) {
			LOG_WRN("Failed to apply codec properties: %d", ret);
		}
	}
#endif

	k_msleep(1000);

	config.word_size = bit_width;
	config.channels = num_of_channel;
	config.format = I2S_FMT_DATA_FORMAT_I2S;

	if (aud_i2s_codec_sink->clk_role == MPIPE_AUD_I2S_CONTROLLER_CODEC_TARGET) {
		config.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER;
	} else {
		config.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
	}

	config.frame_clk_freq = sample_rate;
	config.mem_slab = aud_i2s_codec_sink->mem_slab;
	config.block_size =
		(bit_width >> 3) * ((sample_rate * frame_interval / 1000000) * num_of_channel);
	config.timeout = frame_interval * 10;

	ret = i2s_configure(aud_i2s_codec_sink->i2s_dev, I2S_DIR_TX, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure I2S stream: %d", ret);
		return ret;
	}

	return 0;
}

int mpipe_aud_i2s_codec_sink_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
				      struct net_buf **out_buf)
{
	__ASSERT_NO_MSG(pad != NULL);
	__ASSERT_NO_MSG(in_buf != NULL);
	__ASSERT_NO_MSG(out_buf != NULL);

	struct mpipe_aud_i2s_codec_sink *aud_i2s_codec_sink = CONTAINER_OF(
		pad->object.container, struct mpipe_aud_i2s_codec_sink, sink.element.object);
	uint32_t bytes_used = mpipe_buffer_get_meta(in_buf)->bytes_used;
	int ret = -1;

	ret = i2s_write(aud_i2s_codec_sink->i2s_dev, in_buf->data, bytes_used);
	if (ret < 0) {
		LOG_DBG("Failed to write data: %d\n", ret);
		net_buf_unref(in_buf);
		*out_buf = NULL;
		return -EIO;
	}

	if (!aud_i2s_codec_sink->started) {
		aud_i2s_codec_sink->count++;
		if (aud_i2s_codec_sink->count == AUD_I2S_SINK_START_PRIME) {
			ret = i2s_trigger(aud_i2s_codec_sink->i2s_dev, I2S_DIR_TX,
					  I2S_TRIGGER_START);
			if (ret < 0) {
				LOG_ERR("Failed to start I2S stream: %d", ret);
				net_buf_unref(in_buf);
				*out_buf = NULL;
				return -EIO;
			}
			aud_i2s_codec_sink->started = true;
		}
	}

	/* Done with the buffer */
	net_buf_unref(in_buf);

	/* Sink returns NULL - end of chain */
	*out_buf = NULL;

	return 0;
}

int mpipe_aud_i2s_codec_sink_init(struct mpipe_aud_i2s_codec_sink *aud_i2s_codec_sink, uint8_t id)
{
	__ASSERT_NO_MSG(aud_i2s_codec_sink != NULL);

	struct mpipe_element *self = &aud_i2s_codec_sink->sink.element;
	struct mpipe_sink *sink = &aud_i2s_codec_sink->sink;
	int ret = mpipe_sink_init(sink, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "aud_i2s_codec_sink");

	aud_i2s_codec_sink->i2s_dev = DEVICE_DT_GET_OR_NULL(DT_ALIAS(i2s_codec_tx));
	aud_i2s_codec_sink->codec_dev = DEVICE_DT_GET_OR_NULL(AUD_SINK_CODEC_NODE);

	aud_i2s_codec_sink->clk_role = MPIPE_AUD_I2S_CONTROLLER_CODEC_TARGET;

	self->object.get_property = mpipe_aud_i2s_codec_sink_get_property;
	self->object.set_property = mpipe_aud_i2s_codec_sink_set_property;

	sink->sink_pad.chain_fn = mpipe_aud_i2s_codec_sink_chain_fn;
	sink->set_caps = mpipe_aud_i2s_codec_sink_set_caps;
	sink->propose_buffer_pool = mpipe_aud_i2s_codec_sink_propose_buffer_pool;

	mpipe_aud_i2s_codec_sink_update_caps(sink);

	aud_i2s_codec_sink->started = false;
	aud_i2s_codec_sink->count = 0;
	aud_i2s_codec_sink->mem_slab = NULL;
	return 0;
}
