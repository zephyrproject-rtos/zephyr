/*
 * Copyright 2024 NXP
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

#define I2S_TX  DT_ALIAS(i2s_tx)

#define SAMPLE_FREQUENCY    16000

#define SAMPLE_BIT_WIDTH    (16U)
//#define SAMPLE_BIT_WIDTH    (24U)
//#define SAMPLE_BIT_WIDTH    (32U)

#define NUMBER_OF_CHANNELS  2

#define DMIC_INPUT_ENABLE   0

#if (!DMIC_INPUT_ENABLE)
#define SINE_WAVE_DATA_IN       0
#define SEQ_NUM_DATA_IN         1
#endif

#if (SAMPLE_BIT_WIDTH == 16)
#define BYTES_PER_SAMPLE    sizeof(int16_t)
#else
#define BYTES_PER_SAMPLE    sizeof(int32_t)
#endif


/* Such block length provides an echo with the delay of 10 ms. */
#define SAMPLE_BLOCKS_MS    10
#define SAMPLES_PER_BLOCK   ((SAMPLE_FREQUENCY * SAMPLE_BLOCKS_MS / 1000) * NUMBER_OF_CHANNELS)
#define TIMEOUT             (1000U)

#define BLOCK_SIZE  (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)
#define BLOCK_COUNT 4

K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);

int main(void)
{
	const struct device *const i2s_dev = DEVICE_DT_GET(I2S_TX);
#if DMIC_INPUT_ENABLE
	const struct device *const dmic_dev = DEVICE_DT_GET(DT_NODELABEL(dmic_dev));
#endif

	struct i2s_config i2s_config_param;
	int ret;

#if DMIC_INPUT_ENABLE
    LOG_INF("dmic i2s sample\n");

    if (!device_is_ready(dmic_dev)) {
		LOG_ERR("%s is not ready", dmic_dev->name);
		return 0;
	}
#else
    LOG_INF("dmic i2s sample - i2s tx only mode\n");
#endif

	if (!device_is_ready(i2s_dev)) {
		LOG_ERR("%s is not ready\n", i2s_dev->name);
		return 0;
	}

#if DMIC_INPUT_ENABLE
	struct pcm_stream_cfg stream = {
		.pcm_width = SAMPLE_BIT_WIDTH,
		.mem_slab  = &mem_slab,
	};
	struct dmic_cfg dmic_config_param = {
		.io = {
			/* These fields can be used to limit the PDM clock
			 * configurations that the driver is allowed to use
			 * to those supported by the microphone.
			 */
			.min_pdm_clk_freq = 1000000,
			.max_pdm_clk_freq = 3500000,
			.min_pdm_clk_dc   = 40,
			.max_pdm_clk_dc   = 60,
		},
		.streams = &stream,
		.channel = {
			.req_num_streams = 1,
		},
	};

	dmic_config_param.channel.req_num_chan = NUMBER_OF_CHANNELS;

    #if (NUMBER_OF_CHANNELS == 1)
        dmic_config_param.channel.req_chan_map_lo =
            dmic_build_channel_map(0, 0, PDM_CHAN_LEFT);
#elif (NUMBER_OF_CHANNELS == 2)
        dmic_config_param.channel.req_chan_map_lo =
            dmic_build_channel_map(0, 0, PDM_CHAN_LEFT) |
            dmic_build_channel_map(1, 0, PDM_CHAN_RIGHT);
#endif

    dmic_config_param.streams[0].pcm_rate = SAMPLE_FREQUENCY;
    dmic_config_param.streams[0].block_size = BLOCK_SIZE;

	LOG_INF("DMIC PCM output rate: %u, channels: %u",
		dmic_config_param.streams[0].pcm_rate, dmic_config_param.channel.req_num_chan);

	ret = dmic_configure(dmic_dev, &dmic_config_param);
	if (ret < 0) {
		LOG_ERR("Failed to configure PDM: %d", ret);
		return ret;
	}
#endif

    i2s_config_param.word_size = SAMPLE_BIT_WIDTH;
    i2s_config_param.channels = NUMBER_OF_CHANNELS;
    i2s_config_param.format = I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED;
    i2s_config_param.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
    i2s_config_param.frame_clk_freq = SAMPLE_FREQUENCY;
    i2s_config_param.mem_slab = &mem_slab;
    i2s_config_param.block_size = BLOCK_SIZE;
    i2s_config_param.timeout = TIMEOUT;

	LOG_INF("I2S sample rate: %u, block_size: %u, block window %d ms",
		i2s_config_param.frame_clk_freq, i2s_config_param.block_size, SAMPLE_BLOCKS_MS);

    ret = i2s_configure(i2s_dev, I2S_DIR_TX, &i2s_config_param);

    if (ret < 0) {
		LOG_ERR("Failed to configure i2s TX: %d\n", ret);
		return 0;
	}

#if DMIC_INPUT_ENABLE
	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_START);
	if (ret < 0) {
		LOG_ERR("START trigger failed: %d", ret);
		return ret;
	}
#endif

    bool started = false;
    LOG_INF("start streams\n");

    while (1) {
        int ret;
        void *buffer;
        uint32_t size;

#if DMIC_INPUT_ENABLE
        ret = dmic_read(dmic_dev, 0, &buffer, &size, TIMEOUT);
        if (ret < 0) {
            LOG_ERR("Read failed: %d", ret);
            return ret;
        }

        if(size != BLOCK_SIZE) {
            LOG_ERR("Read data size %d and configured size %d are mismatched.\n", size, BLOCK_SIZE);
            return 0;
        }

        ret = i2s_write(i2s_dev, buffer, BLOCK_SIZE);
#else
        void *mem_block;
        ret = k_mem_slab_alloc(&mem_slab,
                    &mem_block, Z_TIMEOUT_TICKS(TIMEOUT));
        if (ret < 0) {
            LOG_ERR("Failed to allocate TX block\n");
            return 0;
        }
#if SINE_WAVE_DATA_IN
        static uint32_t ti = 0;
        const float M_PI = 3.14159265358979323846f;
        const uint32_t f = 2000;
        const uint32_t fs = i2s_config_param.frame_clk_freq;
    
        if(i2s_config_param.word_size == 16)
        {
            const uint32_t amp = 0x4000;
            int16_t *i2s_data_buf_16bit = (int16_t *)mem_block;
            for (size_t i = 0; i < SAMPLES_PER_BLOCK; i++)
            {
                i2s_data_buf_16bit[i] = (int16_t)(amp * sinf(2*M_PI*f*((ti++)%fs)/fs));
            }
        
        }
        else if((i2s_config_param.word_size == 24) || (i2s_config_param.word_size == 32))
        {
            const uint32_t amp = 0x400000;
            int32_t *i2s_data_buf_32bit = (int32_t *)mem_block;
            for (size_t i = 0; i < SAMPLES_PER_BLOCK; i++)
            {
                i2s_data_buf_32bit[i] = ((int32_t)(amp * sinf(2*M_PI*f*((ti++)%fs)/fs))) & 0xFFFFFF;
            }
        }
#elif SEQ_NUM_DATA_IN
        if (i2s_config_param.word_size == 16)
        {
            int16_t *i2s_data_buf_16bit = (int16_t *)mem_block;
            for (int i = 0; i < SAMPLES_PER_BLOCK; i++)
            {
                i2s_data_buf_16bit[i] = (i & 0xFF);
            }
        }
        else
        {
            int32_t *i2s_data_buf_32bit = (int32_t *)mem_block;
            for (int i = 0; i < SAMPLES_PER_BLOCK; i++)
            {
               i2s_data_buf_32bit[i] = (i & 0xFF) | 0xCD0000;
            }
        }
#endif
        ret = i2s_write(i2s_dev, mem_block, BLOCK_SIZE);
#endif

        if (ret < 0) {
            LOG_ERR("Failed to write data: %d\n", ret);
            break;
        }

        if (!started) {
            i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
            started = true;
        }
    }

    if (i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DROP) < 0) {
        LOG_ERR("Send I2S trigger DRAIN failed: %d", ret);
        return 0;
    }

    LOG_INF("Streams stopped\n");
    return 0;
}
