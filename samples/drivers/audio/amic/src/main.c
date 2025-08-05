/*
 * Copyright (c) 2025 Ambiq Micro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/audio/amic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(amic_sample);

/* 16bit sample support only */
#define BYTES_PER_SAMPLE sizeof(int16_t)
#define SAMPLE_BIT_WIDTH (16)

/* Milliseconds to wait for a block to be read. */
#define READ_TIMEOUT 1000

/* Block size is assigned by board overlay file's define. */
#define BLOCK_SIZE_SAMPLES                                                                         \
	(DT_PROP(DT_NODELABEL(amic_dev), audadc_buf_size_samples) / CONFIG_AMIC_CHANNELS)
#define BLOCK_SIZE_MS (BLOCK_SIZE_SAMPLES * 1000 / CONFIG_SAMPLE_FREQ)

/* Size of a block of audio data. */
#define BLOCK_SIZE(_sample_rate, _number_of_channels)                                              \
	(BYTES_PER_SAMPLE * (_sample_rate * BLOCK_SIZE_MS / 1000) * _number_of_channels)

/*
 * Driver will allocate blocks from this slab to receive audio data into them.
 * Application, after getting a given block from the driver and processing its
 * data, needs to free that block.
 */
#define BLOCK_SIZE_BYTES BLOCK_SIZE(CONFIG_SAMPLE_FREQ, CONFIG_AMIC_CHANNELS)
#define BLOCK_COUNT      4

K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE_BYTES, BLOCK_COUNT, 4);

static int do_audadc_transfer(const struct device *amic_dev, struct amic_cfg *cfg,
			      size_t block_count)
{
	int ret;

	LOG_INF("PCM output rate: %u, channels: %u", cfg->streams[0].pcm_rate,
		cfg->streams[0].channel_num);
	LOG_INF("Frame size: %u, frame window: %u ms", BLOCK_SIZE_BYTES, BLOCK_SIZE_MS);

	ret = amic_configure(amic_dev, cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure the driver: %d", ret);
		return ret;
	}

	ret = amic_trigger(amic_dev, AMIC_TRIGGER_START);
	if (ret < 0) {
		LOG_ERR("START trigger failed: %d", ret);
		return ret;
	}

	for (int i = 0; i < block_count; ++i) {
		void *buffer;
		uint32_t size;

		ret = amic_read(amic_dev, 0, &buffer, &size, READ_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("%d - read failed: %d", i, ret);
			return ret;
		}

		LOG_INF("%d - got buffer %p of %u bytes", i, buffer, size);
		k_mem_slab_free(&mem_slab, buffer);
	}

	ret = amic_trigger(amic_dev, AMIC_TRIGGER_STOP);
	if (ret < 0) {
		LOG_ERR("STOP trigger failed: %d", ret);
		return ret;
	}

	return ret;
}

int main(void)
{
	const struct device *const amic_dev = DEVICE_DT_GET(DT_NODELABEL(amic_dev));
	int ret;

	LOG_INF("AMIC sample");

	if (!device_is_ready(amic_dev)) {
		LOG_ERR("%s is not ready", amic_dev->name);
		return 0;
	}

	struct pcm_stream_cfg stream = {
		.pcm_rate = CONFIG_SAMPLE_FREQ,
		.pcm_width = SAMPLE_BIT_WIDTH,
		.channel_num = CONFIG_AMIC_CHANNELS,
		.block_size = BLOCK_SIZE_BYTES,
		.mem_slab = &mem_slab,
	};
	struct amic_cfg cfg = {
		.streams = &stream,
	};

	ret = do_audadc_transfer(amic_dev, &cfg, 2 * BLOCK_COUNT);
	if (ret < 0) {
		return 0;
	}

	LOG_INF("Exiting");
	return 0;
}
