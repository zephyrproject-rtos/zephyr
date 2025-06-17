/*
 * Copyright 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/drivers/i2s.h>
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dmic_i2s_sample, LOG_LEVEL_INF);

#define I2S_TX DT_ALIAS(i2s_tx)
#define I2S_RX DT_ALIAS(i2s_rx)

#define SAMPLE_BIT_WIDTH (16U) /* 16U, 24U or 32U */

#if CONFIG_I2S_DMIC_INPUT
#define NUMBER_OF_CHANNELS CONFIG_DMIC_CHANNELS /* 1 or 2 channels supported (PDM to I2S mode) */
#else
#define NUMBER_OF_CHANNELS CONFIG_I2S_ONLY_CHANNELS /* 1 or 2 channels supported (I2S only mode)   \
						     */
#if CONFIG_I2S_LOOPBACK
#define SEQ_NUM_DATA_IN 1
#else
#define SINE_WAVE_DATA_IN 0
#define SEQ_NUM_DATA_IN   1
#endif
#endif

#if (SAMPLE_BIT_WIDTH == 16)
#define BYTES_PER_SAMPLE sizeof(int16_t)
#else
#define BYTES_PER_SAMPLE sizeof(int32_t)
#endif

#define SAMPLE_BLOCKS_MS  10
#define SAMPLES_PER_BLOCK ((CONFIG_SAMPLE_FREQ * SAMPLE_BLOCKS_MS / 1000) * NUMBER_OF_CHANNELS)
#define TIMEOUT           (1000U)

#define BLOCK_SIZE  (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)
#define BLOCK_COUNT 4

K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);

#if CONFIG_I2S_LOOPBACK
uint8_t rx_temp_buf[BLOCK_SIZE * 100];

static bool check_i2s_data(uint32_t rxtx_sample_num, void *rx_databuf)
{
	int i, index_0 = 0;

	/*
	 * Find the first element of Tx buffer in Rx buffer, and return the index.
	 * Rx will delay N samples.
	 */
#if (SAMPLE_BIT_WIDTH == 16)
	uint16_t *rx_databuf_16 = (uint16_t *)rx_databuf;

	for (i = 0; i < rxtx_sample_num; i++) {
		if (rx_databuf_16[i] == 0xCD00) {
			index_0 = i;
			break;
		}
	}

	for (i = 0; i < (rxtx_sample_num - index_0); i++) {
		if (rx_databuf_16[i + index_0] != (0xCD00 | ((i % SAMPLES_PER_BLOCK) & 0xFF))) {
			printk("idx %d 0x%x buf[%d] = 0x%x 0x%x", index_0,
			       rx_databuf_16[i + index_0 - 1], i, rx_databuf_16[i + index_0],
			       rx_databuf_16[i + index_0 + 1]);
			return false;
		}
	}
#else
	uint32_t *rx_databuf_32 = (uint32_t *)rx_databuf;

	for (i = 0; i < rxtx_sample_num; i++) {
		if (rx_databuf_32[i] == 0xCD0000) {
			index_0 = i;
			break;
		}
	}

	for (i = 0; i < (rxtx_sample_num - index_0); i++) {
		if (rx_databuf_32[i + index_0] != (0xCD0000 | ((i % SAMPLES_PER_BLOCK) & 0xFF))) {
			printk("idx %d 0x%x buf[%d] = 0x%x 0x%x", index_0,
			       rx_databuf_32[i + index_0 - 1], i, rx_databuf_32[i + index_0],
			       rx_databuf_32[i + index_0 + 1]);
			return false;
		}
	}
#endif

	return true;
}
#endif

int main(void)
{
	const struct device *const i2s_dev = DEVICE_DT_GET(I2S_TX);
#if CONFIG_I2S_LOOPBACK
	const struct device *const i2s_rx_dev = DEVICE_DT_GET(I2S_RX);
	uint32_t rx_buf_index = 0;
#endif
#if CONFIG_I2S_DMIC_INPUT
	const struct device *const dmic_dev = DEVICE_DT_GET(DT_NODELABEL(dmic_dev));
#endif
#if SINE_WAVE_DATA_IN
	uint32_t ti = 0;
#endif

	struct i2s_config i2s_config_param;
	int ret;
	int tx_count = 0;

	if (!device_is_ready(i2s_dev)) {
		LOG_ERR("%s is not ready", i2s_dev->name);
		return 0;
	}

	LOG_INF("DMIC I2S sample");

#if CONFIG_I2S_DMIC_INPUT
	LOG_INF("PDM to I2S mode");

	if (!device_is_ready(dmic_dev)) {
		LOG_ERR("%s is not ready", dmic_dev->name);
		return 0;
	}
#elif CONFIG_I2S_LOOPBACK
	LOG_INF("I2S loopback mode");

	if (!device_is_ready(i2s_rx_dev)) {
		LOG_ERR("%s is not ready", i2s_rx_dev->name);
		return 0;
	}
#else
	LOG_INF("I2S TX only mode");
#endif

#if CONFIG_I2S_DMIC_INPUT
	struct pcm_stream_cfg stream = {
		.pcm_width = SAMPLE_BIT_WIDTH,
		.mem_slab = &mem_slab,
	};
	struct dmic_cfg dmic_config_param = {
		.io = {
				/* These fields can be used to limit the PDM clock
				 * configurations that the driver is allowed to use
				 * to those supported by the microphone.
				 */
				.min_pdm_clk_freq = 1000000,
				.max_pdm_clk_freq = 3500000,
				.min_pdm_clk_dc = 40,
				.max_pdm_clk_dc = 60,
			},
		.streams = &stream,
		.channel = {
				.req_num_streams = 1,
			},
	};

	dmic_config_param.channel.req_num_chan = NUMBER_OF_CHANNELS;

#if (NUMBER_OF_CHANNELS == 1)
	dmic_config_param.channel.req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT);
#elif (NUMBER_OF_CHANNELS == 2)
	dmic_config_param.channel.req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT) |
						    dmic_build_channel_map(1, 0, PDM_CHAN_RIGHT);
#endif

	dmic_config_param.streams[0].pcm_rate = CONFIG_SAMPLE_FREQ;
	dmic_config_param.streams[0].block_size = BLOCK_SIZE;

	LOG_INF("PCM output rate: %u, channels: %u", dmic_config_param.streams[0].pcm_rate,
		dmic_config_param.channel.req_num_chan);

	ret = dmic_configure(dmic_dev, &dmic_config_param);
	if (ret < 0) {
		LOG_ERR("Failed to configure PDM: %d", ret);
		return ret;
	}
#endif

	i2s_config_param.word_size = SAMPLE_BIT_WIDTH;
	i2s_config_param.channels = NUMBER_OF_CHANNELS;
	i2s_config_param.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_config_param.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
	i2s_config_param.frame_clk_freq = CONFIG_SAMPLE_FREQ;
	i2s_config_param.mem_slab = &mem_slab;
	i2s_config_param.block_size = BLOCK_SIZE;
	i2s_config_param.timeout = TIMEOUT;

	LOG_INF("I2S sample rate: %u, ch num: %d", i2s_config_param.frame_clk_freq,
		i2s_config_param.channels);
	LOG_INF("Block_size: %u, block window %d ms", i2s_config_param.block_size,
		SAMPLE_BLOCKS_MS);

	ret = i2s_configure(i2s_dev, I2S_DIR_TX, &i2s_config_param);

	if (ret < 0) {
		LOG_ERR("Failed to configure i2s TX: %d", ret);
		return 0;
	}

#if CONFIG_I2S_LOOPBACK
	i2s_config_param.options = I2S_OPT_BIT_CLK_SLAVE | I2S_OPT_FRAME_CLK_SLAVE;
	i2s_config_param.mem_slab = &mem_slab;

	ret = i2s_configure(i2s_rx_dev, I2S_DIR_RX, &i2s_config_param);
	if (ret < 0) {
		LOG_ERR("Failed to configure i2s RX: %d", ret);
		return 0;
	}
#endif

	bool started = false;

	LOG_INF("Start streams");

#if CONFIG_I2S_DMIC_INPUT
	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_START);
	if (ret < 0) {
		LOG_ERR("START trigger failed: %d", ret);
		return ret;
	}
#endif

	while (1) {

#if CONFIG_I2S_DMIC_INPUT
		void *buffer;
		uint32_t size;

		ret = dmic_read(dmic_dev, 0, &buffer, &size, TIMEOUT);
		if (ret < 0) {
			LOG_ERR("Read failed: %d", ret);
			return ret;
		}

		if (size != BLOCK_SIZE) {
			LOG_ERR("Read data size %d and configured size %d are mismatched.\n", size,
				BLOCK_SIZE);
			return 0;
		}

		ret = i2s_write(i2s_dev, buffer, BLOCK_SIZE);
#else
		void *mem_block;

		ret = k_mem_slab_alloc(&mem_slab, &mem_block, Z_TIMEOUT_TICKS(TIMEOUT));
		if (ret < 0) {
			LOG_ERR("Failed to allocate TX block");
			return 0;
		}
#if SINE_WAVE_DATA_IN
		const float M_PI = 3.14159265358979323846f;
		const uint32_t f = 2000;
		const uint32_t fs = i2s_config_param.frame_clk_freq;

		if (i2s_config_param.word_size == 16) {
			const uint32_t amp = 0x4000;
			int16_t *i2s_data_buf_16bit = (int16_t *)mem_block;

			for (size_t i = 0; i < SAMPLES_PER_BLOCK; i++) {
				i2s_data_buf_16bit[i] =
					(int16_t)(amp * sinf(2 * M_PI * f * ((ti++) % fs) / fs));
			}

		} else if ((i2s_config_param.word_size == 24) ||
			   (i2s_config_param.word_size == 32)) {
			const uint32_t amp = 0x400000;
			int32_t *i2s_data_buf_32bit = (int32_t *)mem_block;

			for (size_t i = 0; i < SAMPLES_PER_BLOCK; i++) {
				i2s_data_buf_32bit[i] =
					((int32_t)(amp * sinf(2 * M_PI * f * ((ti++) % fs) / fs))) &
					0xFFFFFF;
			}
		}
#elif SEQ_NUM_DATA_IN
		if (i2s_config_param.word_size == 16) {
			int16_t *i2s_data_buf_16bit = (int16_t *)mem_block;

			for (int i = 0; i < SAMPLES_PER_BLOCK; i++) {
				i2s_data_buf_16bit[i] = (i & 0xFF) | 0xCD00;
			}
		} else {
			int32_t *i2s_data_buf_32bit = (int32_t *)mem_block;

			for (int i = 0; i < SAMPLES_PER_BLOCK; i++) {
				i2s_data_buf_32bit[i] = (i & 0xFF) | 0xCD0000;
			}
		}
#endif
		ret = i2s_write(i2s_dev, mem_block, BLOCK_SIZE);
#endif

		if (ret < 0) {
			LOG_ERR("Failed to write data: %d", ret);
			break;
		}

		if (!started) {
#if CONFIG_I2S_LOOPBACK
			i2s_trigger(i2s_rx_dev, I2S_DIR_RX, I2S_TRIGGER_START);
#endif
			i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
			started = true;
		}

#if CONFIG_I2S_LOOPBACK
		void *rx_buffer;
		uint32_t rx_size = 0;

		ret = i2s_read(i2s_rx_dev, &rx_buffer, &rx_size);

		if (ret < 0) {
			LOG_ERR("Failed to read data: %d", ret);
			break;
		}

#if SEQ_NUM_DATA_IN
		memcpy(&rx_temp_buf[rx_buf_index], rx_buffer, rx_size);
		rx_buf_index += rx_size;
		if (rx_buf_index >= (BLOCK_SIZE * 100)) {
			if (check_i2s_data(SAMPLES_PER_BLOCK * 100, (void *)rx_temp_buf)) {
				LOG_INF("%d bytes passed", BLOCK_SIZE * 100);
			} else {
				LOG_INF("Failed");
			}
			rx_buf_index = 0;
		}
#endif
		k_mem_slab_free(&mem_slab, rx_buffer);
#endif
		if (tx_count++ > 100) {
			break;
		}
	}

#if CONFIG_I2S_DMIC_INPUT
	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_STOP);
	if (ret < 0) {
		LOG_ERR("STOP trigger failed: %d", ret);
		return ret;
	}
#endif

	if (i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_STOP) < 0) {
		LOG_ERR("Send I2S trigger STOP failed: %d", ret);
		return 0;
	}

	LOG_INF("Streams stopped");
	return 0;
}
