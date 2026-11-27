/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/sys/iterable_sections.h>

#define SAMPLE_NO 64

/* The data represent a sine wave */
static int16_t data[SAMPLE_NO] = {
	  3211,   6392,   9511,  12539,  15446,  18204,  20787,  23169,
	 25329,  27244,  28897,  30272,  31356,  32137,  32609,  32767,
	 32609,  32137,  31356,  30272,  28897,  27244,  25329,  23169,
	 20787,  18204,  15446,  12539,   9511,   6392,   3211,      0,
	 -3212,  -6393,  -9512, -12540, -15447, -18205, -20788, -23170,
	-25330, -27245, -28898, -30273, -31357, -32138, -32610, -32767,
	-32610, -32138, -31357, -30273, -28898, -27245, -25330, -23170,
	-20788, -18205, -15447, -12540,  -9512,  -6393,  -3212,     -1,
};

struct stream_format {
	const char *name;
	uint32_t format;
	uint8_t channels;
};

static const struct stream_format formats[] = {
	{"I2S", I2S_FMT_DATA_FORMAT_I2S, 2U},
#ifdef CONFIG_SAMPLE_I2S_FORMAT_PCM_SHORT
	{"PCM short frame", I2S_FMT_DATA_FORMAT_PCM_SHORT, 1U},
#endif
#ifdef CONFIG_SAMPLE_I2S_FORMAT_PCM_LONG
	{"PCM long frame", I2S_FMT_DATA_FORMAT_PCM_LONG, 1U},
#endif
};

/* Fill buffer with sine wave on left channel, and sine wave shifted by
 * 90 degrees on right channel. With one channel, fill it with the sine wave
 * only. "att" represents a power of two to attenuate the samples by
 */
static void fill_buf(int16_t *tx_block, int att, uint8_t channels)
{
	int r_idx;

	if (channels == 1U) {
		for (int i = 0; i < 2 * SAMPLE_NO; i++) {
			tx_block[i] = data[i % SAMPLE_NO] / (1 << att);
		}
		return;
	}

	for (int i = 0; i < SAMPLE_NO; i++) {
		/* Left channel is sine wave */
		tx_block[2 * i] = data[i] / (1 << att);
		/* Right channel is same sine wave, shifted by 90 degrees */
		r_idx = (i + (ARRAY_SIZE(data) / 4)) % ARRAY_SIZE(data);
		tx_block[2 * i + 1] = data[r_idx] / (1 << att);
	}
}

#define NUM_BLOCKS 20
#define BLOCK_SIZE (2 * sizeof(data))

/* Longer than the transmission of NUM_BLOCKS blocks with one channel */
#define DRAIN_TIME_MS 200

#ifdef CONFIG_NOCACHE_MEMORY
	#define MEM_SLAB_CACHE_ATTR __nocache
#else
	#define MEM_SLAB_CACHE_ATTR
#endif /* CONFIG_NOCACHE_MEMORY */

static char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_tx_0_mem_slab[(NUM_BLOCKS) * WB_UP(BLOCK_SIZE)];

static STRUCT_SECTION_ITERABLE(k_mem_slab, tx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(tx_0_mem_slab, _k_mem_slab_buf_tx_0_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS);

static int send_stream(const struct device *dev_i2s, const struct stream_format *format)
{
	void *tx_block[NUM_BLOCKS];
	struct i2s_config i2s_cfg;
	int ret;
	uint32_t tx_idx;

	printf("Sending %s, %u channel(s)\n", format->name, format->channels);

	/* Configure I2S stream */
	i2s_cfg.word_size = 16U;
	i2s_cfg.channels = format->channels;
	i2s_cfg.format = format->format;
	i2s_cfg.frame_clk_freq = 44100;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.timeout = 2000;
	/* Configure the Transmit port as Controller */
	i2s_cfg.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER
#ifdef CONFIG_SAMPLE_I2S_OPT_BIT_CLK_GATED
			  | I2S_OPT_BIT_CLK_GATED
#endif
		;
	i2s_cfg.mem_slab = &tx_0_mem_slab;
	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	if (ret < 0) {
		printf("Failed to configure I2S stream\n");
		return ret;
	}

	/* Prepare all TX blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[tx_idx],
				       K_FOREVER);
		if (ret < 0) {
			printf("Failed to allocate TX block\n");
			return ret;
		}
		fill_buf((uint16_t *)tx_block[tx_idx], tx_idx % 3, format->channels);
	}

	tx_idx = 0;
	/* Send first block */
	ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
	if (ret < 0) {
		printf("Could not write TX buffer %d\n", tx_idx);
		return ret;
	}
	/* Trigger the I2S transmission */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret < 0) {
		printf("Could not trigger I2S tx\n");
		return ret;
	}

	for (; tx_idx < NUM_BLOCKS; ) {
		ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
		if (ret < 0) {
			printf("Could not write TX buffer %d\n", tx_idx);
			return ret;
		}
	}
	/* Drain TX queue */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	if (ret < 0) {
		printf("Could not trigger I2S tx\n");
		return ret;
	}
	return 0;
}

int main(void)
{
	const struct device *dev_i2s = DEVICE_DT_GET(DT_ALIAS(i2s_tx));
	int ret;

	if (!device_is_ready(dev_i2s)) {
		printf("I2S device not ready\n");
		return -ENODEV;
	}

	ARRAY_FOR_EACH(formats, i) {
		ret = send_stream(dev_i2s, &formats[i]);
		if (ret < 0) {
			return ret;
		}

		/* The stream can be configured again once the drain has finished */
		k_msleep(DRAIN_TIME_MS);
	}

	printf("All I2S blocks written\n");
	return 0;
}
