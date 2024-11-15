/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/audio/codec.h>
#include <string.h>


#define I2S_CODEC_TX  DT_ALIAS(i2s_codec_tx)

#define SAMPLE_FREQUENCY    CONFIG_SAMPLE_FREQ
#define SAMPLE_BIT_WIDTH    (16U)
#define BYTES_PER_SAMPLE    sizeof(int16_t)
#if CONFIG_USE_DMIC
#define NUMBER_OF_CHANNELS  CONFIG_DMIC_CHANNELS
#else
#define NUMBER_OF_CHANNELS (2U)
#endif
/* Such block length provides an echo with the delay of 100 ms. */
#define SAMPLES_PER_BLOCK   ((SAMPLE_FREQUENCY / 10) * NUMBER_OF_CHANNELS)
#define INITIAL_BLOCKS      CONFIG_I2S_INIT_BUFFERS
#define TIMEOUT             (2000U)

#define BLOCK_SIZE  (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)
#define BLOCK_COUNT (INITIAL_BLOCKS + 32)
K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);

static bool configure_tx_streams(const struct device *i2s_dev, struct i2s_config *config)
{
	int ret;

	ret = i2s_configure(i2s_dev, I2S_DIR_TX, config);
	if (ret < 0) {
		printk("Failed to configure codec stream: %d\n", ret);
		return false;
	}

	return true;
}

static bool trigger_command(const struct device *i2s_dev_codec,
			    enum i2s_trigger_cmd cmd)
{
	int ret;

	ret = i2s_trigger(i2s_dev_codec, I2S_DIR_TX, cmd);
	if (ret < 0) {
		printk("Failed to trigger command %d on TX: %d\n", cmd, ret);
		return false;
	}

	return true;
}

int main(void)
{
	const struct device *const i2s_dev_codec = DEVICE_DT_GET(I2S_CODEC_TX);
#if CONFIG_USE_DMIC
	const struct device *const dmic_dev = DEVICE_DT_GET(DT_NODELABEL(dmic_dev));
#endif
	const struct device *const codec_dev = DEVICE_DT_GET(DT_NODELABEL(audio_codec));
	struct i2s_config config;
	struct audio_codec_cfg audio_cfg;
	int ret;

#if CONFIG_USE_DMIC
	struct pcm_stream_cfg stream = {
		.pcm_width = SAMPLE_BIT_WIDTH,
		.mem_slab  = &mem_slab,
	};
	struct dmic_cfg cfg = {
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
#endif
	printk("codec sample\n");

#if CONFIG_USE_DMIC
	if (!device_is_ready(dmic_dev)) {
		printk("%s is not ready", dmic_dev->name);
		return 0;
	}
#endif

	if (!device_is_ready(i2s_dev_codec)) {
		printk("%s is not ready\n", i2s_dev_codec->name);
		return 0;
	}


	if (!device_is_ready(codec_dev)) {
		printk("%s is not ready", codec_dev->name);
		return 0;
	}
	audio_cfg.dai_route = AUDIO_ROUTE_PLAYBACK;
	audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	audio_cfg.dai_cfg.i2s.word_size = SAMPLE_BIT_WIDTH;
	audio_cfg.dai_cfg.i2s.channels =  2;
	audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
	audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_MASTER;
	audio_cfg.dai_cfg.i2s.frame_clk_freq = SAMPLE_FREQUENCY;
	audio_cfg.dai_cfg.i2s.mem_slab = &mem_slab;
	audio_cfg.dai_cfg.i2s.block_size = BLOCK_SIZE;
	audio_codec_configure(codec_dev, &audio_cfg);
	k_msleep(1000);

#if CONFIG_USE_DMIC
	cfg.channel.req_num_chan = 2;
	cfg.channel.req_chan_map_lo =
		dmic_build_channel_map(0, 0, PDM_CHAN_LEFT) |
		dmic_build_channel_map(1, 0, PDM_CHAN_RIGHT);
	cfg.streams[0].pcm_rate = SAMPLE_FREQUENCY;
	cfg.streams[0].block_size = BLOCK_SIZE;

	printk("PCM output rate: %u, channels: %u\n",
		cfg.streams[0].pcm_rate, cfg.channel.req_num_chan);

	ret = dmic_configure(dmic_dev, &cfg);
	if (ret < 0) {
		printk("Failed to configure the driver: %d", ret);
		return ret;
	}
#endif

	config.word_size = SAMPLE_BIT_WIDTH;
	config.channels = NUMBER_OF_CHANNELS;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
	config.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
	config.frame_clk_freq = SAMPLE_FREQUENCY;
	config.mem_slab = &mem_slab;
	config.block_size = BLOCK_SIZE;
	config.timeout = TIMEOUT;
	if (!configure_tx_streams(i2s_dev_codec, &config)) {
		printk("failure to config streams\n");
		return 0;
	}

	printk("start streams\n");
	for (;;) {
		bool started = false;
#if CONFIG_USE_DMIC
		ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_START);
		if (ret < 0) {
			printk("START trigger failed: %d", ret);
			return ret;
		}
#endif
		while (1) {
			void *mem_block;
			uint32_t block_size = BLOCK_SIZE;
			int ret;
			int i;

			for (i = 0; i < 2; i++) {
#if CONFIG_USE_DMIC
				ret = dmic_read(dmic_dev, 0,
								&mem_block, &block_size, TIMEOUT);
				if (ret < 0) {
					printk("read failed: %d", ret);
					break;
				}
#else
				ret = k_mem_slab_alloc(&mem_slab,
							&mem_block, Z_TIMEOUT_TICKS(TIMEOUT));
				if (ret < 0) {
					printk("Failed to allocate TX block\n");
					return 0;
				}
#endif
				ret = i2s_write(i2s_dev_codec, mem_block, block_size);
				if (ret < 0) {
					printk("Failed to write data: %d\n", ret);
					break;
				}
			}
			if (ret < 0) {
				printk("error %d\n", ret);
				break;
			}
			if (!started) {
				i2s_trigger(i2s_dev_codec, I2S_DIR_TX, I2S_TRIGGER_START);
				started = true;
			}
		}
		if (!trigger_command(i2s_dev_codec,
				     I2S_TRIGGER_DROP)) {
			printk("Send I2S trigger DRAIN failed: %d", ret);
			return 0;
		}
#if CONFIG_USE_DMIC
		ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_STOP);
		if (ret < 0) {
			printk("STOP trigger failed: %d", ret);
			return 0;
		}
#endif
		printk("Streams stopped\n");
		return 0;
	}
}
