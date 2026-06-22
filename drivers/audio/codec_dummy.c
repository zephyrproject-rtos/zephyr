/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_dummy_codec

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#define DUMMY_CODEC_MAX_BLOCK_SIZE 512U

struct dummy_codec_data {
	struct audio_codec_cfg cfg;
	audio_codec_tx_done_callback_t tx_done;
	void *tx_done_user_data;
	audio_codec_rx_done_callback_t rx_done;
	void *rx_done_user_data;
	uint8_t rx_block[DUMMY_CODEC_MAX_BLOCK_SIZE];
	audio_dai_dir_t active_dir;
	bool started;
	bool configured;
};

static int dummy_codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct dummy_codec_data *data = dev->data;

	if (cfg == NULL) {
		return -EINVAL;
	}

	data->cfg = *cfg;
	data->configured = true;

	return 0;
}

static int dummy_codec_set_property(const struct device *dev, audio_property_t property,
				    audio_channel_t channel, audio_property_value_t val)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(val);

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
	case AUDIO_PROPERTY_OUTPUT_MUTE:
	case AUDIO_PROPERTY_INPUT_VOLUME:
	case AUDIO_PROPERTY_INPUT_MUTE:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int dummy_codec_register_done_callback(const struct device *dev,
					      audio_codec_tx_done_callback_t tx_cb,
					      void *tx_cb_user_data,
					      audio_codec_rx_done_callback_t rx_cb,
					      void *rx_cb_user_data)
{
	struct dummy_codec_data *data = dev->data;

	data->tx_done = tx_cb;
	data->tx_done_user_data = tx_cb_user_data;
	data->rx_done = rx_cb;
	data->rx_done_user_data = rx_cb_user_data;

	return 0;
}

static int dummy_codec_start(const struct device *dev, audio_dai_dir_t dir)
{
	struct dummy_codec_data *data = dev->data;
	size_t block_size;

	if (!data->configured) {
		return -EIO;
	}

	data->started = true;
	data->active_dir = dir;

	if (data->cfg.dai_type != AUDIO_DAI_TYPE_PCM) {
		return 0;
	}

	block_size = MIN(data->cfg.dai_cfg.pcm.block_size, sizeof(data->rx_block));

	if (((dir & AUDIO_DAI_DIR_RX) != 0U) && (data->rx_done != NULL)) {
		data->rx_done(dev, data->rx_block, block_size, data->rx_done_user_data);
	}

	if (((dir & AUDIO_DAI_DIR_TX) != 0U) && (data->tx_done != NULL)) {
		data->tx_done(dev, data->tx_done_user_data);
	}

	return 0;
}

static int dummy_codec_stop(const struct device *dev, audio_dai_dir_t dir)
{
	struct dummy_codec_data *data = dev->data;

	ARG_UNUSED(dir);

	data->started = false;
	data->active_dir = 0U;

	return 0;
}

static int dummy_codec_write(const struct device *dev, uint8_t *data, size_t data_size)
{
	struct dummy_codec_data *dummy_data = dev->data;

	if ((data == NULL) || (data_size == 0U)) {
		return -EINVAL;
	}

	if (!dummy_data->started) {
		return -EIO;
	}

	return 0;
}

static int dummy_codec_init(const struct device *dev)
{
	struct dummy_codec_data *data = dev->data;

	memset(data, 0, sizeof(*data));
	return 0;
}

static DEVICE_API(audio_codec, dummy_codec_api) = {
	.configure = dummy_codec_configure,
	.set_property = dummy_codec_set_property,
	.start = dummy_codec_start,
	.stop = dummy_codec_stop,
	.write = dummy_codec_write,
	.register_done_callback = dummy_codec_register_done_callback,
};

/* clang-format off */
#define DUMMY_CODEC_DEFINE(inst) \
	static struct dummy_codec_data dummy_codec_data_##inst; \
	DEVICE_DT_INST_DEFINE(inst, dummy_codec_init, NULL, \
			      &dummy_codec_data_##inst, NULL, \
			      POST_KERNEL, \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, \
			      &dummy_codec_api)
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(DUMMY_CODEC_DEFINE)
