/*
 * Copyright (c) 2017 comsuisse AG
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/sys/iterable_sections.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2s_add, LOG_LEVEL_INF);

#define I2S_DEV_NODE DT_ALIAS(i2s_node0)

#define WORD_SIZE 16U
#define NUMBER_OF_CHANNELS 2
#define FRAME_CLK_FREQ 44100

#define NUM_BLOCKS 4
#define TIMEOUT 1000

#define SAMPLES_COUNT 64

/* The data_l represent a sine wave */
static int16_t data_l[SAMPLES_COUNT] = {
	  3211,   6392,   9511,  12539,  15446,  18204,  20787,  23169,
	 25329,  27244,  28897,  30272,  31356,  32137,  32609,  32767,
	 32609,  32137,  31356,  30272,  28897,  27244,  25329,  23169,
	 20787,  18204,  15446,  12539,   9511,   6392,   3211,      0,
	 -3212,  -6393,  -9512, -12540, -15447, -18205, -20788, -23170,
	-25330, -27245, -28898, -30273, -31357, -32138, -32610, -32767,
	-32610, -32138, -31357, -30273, -28898, -27245, -25330, -23170,
	-20788, -18205, -15447, -12540,  -9512,  -6393,  -3212,     -1,
};

/* The data_r represent a sine wave shifted by 90 deg to data_l sine wave */
static int16_t data_r[SAMPLES_COUNT] = {
	 32609,  32137,  31356,  30272,  28897,  27244,  25329,  23169,
	 20787,  18204,  15446,  12539,   9511,   6392,   3211,      0,
	 -3212,  -6393,  -9512, -12540, -15447, -18205, -20788, -23170,
	-25330, -27245, -28898, -30273, -31357, -32138, -32610, -32767,
	-32610, -32138, -31357, -30273, -28898, -27245, -25330, -23170,
	-20788, -18205, -15447, -12540,  -9512,  -6393,  -3212,     -1,
	  3211,   6392,   9511,  12539,  15446,  18204,  20787,  23169,
	 25329,  27244,  28897,  30272,  31356,  32137,  32609,  32767,
};

#define BLOCK_SIZE (2 * sizeof(data_l))

#ifdef CONFIG_NOCACHE_MEMORY
	#define MEM_SLAB_CACHE_ATTR __nocache
#else
	#define MEM_SLAB_CACHE_ATTR
#endif /* CONFIG_NOCACHE_MEMORY */

/*
 * NUM_BLOCKS is the number of blocks used by the test. Some of the drivers,
 * permanently keep ownership of a few RX buffers. Add a two more
 * RX blocks to satisfy this requirement
 */
static char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_rx_0_mem_slab[(NUM_BLOCKS + 2) * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, rx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(rx_0_mem_slab, _k_mem_slab_buf_rx_0_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS + 2);

static char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_tx_0_mem_slab[(NUM_BLOCKS) * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, tx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(tx_0_mem_slab, _k_mem_slab_buf_tx_0_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS);

static const struct device *dev_i2s;

static const struct i2s_config default_i2s_cfg = {
	.word_size = WORD_SIZE,
	.channels = NUMBER_OF_CHANNELS,
	.format = I2S_FMT_DATA_FORMAT_I2S,
	.frame_clk_freq = FRAME_CLK_FREQ,
	.block_size = BLOCK_SIZE,
	.timeout = TIMEOUT,
#if defined(CONFIG_I2S_TEST_USE_GPIO_LOOPBACK)
	.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER,
#else
	.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER | I2S_OPT_LOOPBACK,
#endif
	.mem_slab = &tx_0_mem_slab,
};

#if (CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET > 0)
/* Data offset may differ when test uses I2S with different configuration. */
static int offset;
static int16_t word_size_bytes;
static int16_t sample_in_bytes;
#endif

/* Fill in TX buffer with test samples. */
static void fill_buf(int16_t *tx_block, uint8_t word_size)
{

	/* Technically, this is correct for word_size of 16 bits only
	 * (incorrect for word_size of 8, 24 and 32 bit).
	 * However, tests checks if received bytes are identical to
	 * the transmitted ones. Meaning of transmitted data is irrelevant.
	 */
	if (word_size == 24) {
		int8_t *tx_block_8bit = (int8_t *) tx_block;
		int8_t *data_l_8bit = (int8_t *) &data_l;
		int8_t *data_r_8bit = (int8_t *) &data_r;
		int16_t tx_cnt = 0;
		int16_t l_cnt = 0;
		int16_t r_cnt = 0;

		while (tx_cnt < BLOCK_SIZE) {
			tx_block_8bit[tx_cnt++] = data_l_8bit[l_cnt++];
			tx_block_8bit[tx_cnt++] = data_l_8bit[l_cnt++];
			tx_block_8bit[tx_cnt++] = data_l_8bit[l_cnt++];
			tx_block_8bit[tx_cnt++] = 0;
			tx_block_8bit[tx_cnt++] = data_r_8bit[r_cnt++];
			tx_block_8bit[tx_cnt++] = data_r_8bit[r_cnt++];
			tx_block_8bit[tx_cnt++] = data_r_8bit[r_cnt++];
			tx_block_8bit[tx_cnt++] = 0;
		}
	} else {
		for (int i = 0; i < SAMPLES_COUNT; i++) {
			tx_block[2 * i] = data_l[i];
			tx_block[2 * i + 1] = data_r[i];
		}
	}
}

static int verify_buf(int16_t *rx_block, uint8_t word_size, uint8_t channels)
{
	int sample_no = SAMPLES_COUNT;
	bool same = true;

/* Find offset.
 * This doesn't handle correctly situation when
 * word_size is 8 bit and offset is odd.
 */
#if (CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET > 0)
	/* Offset -1 means that offset has to be detected. */
	if (offset < 0) {
		LOG_HEXDUMP_DBG(rx_block, BLOCK_SIZE, "Received");

		/* When word_size is:
		 * 8 bit, it occupies 8/8 = 1 byte,
		 * 16 bit, it occupies 16/8 = 2 bytes,
		 * 24 bit, it occupies 4 bytes,
		 * 32 bit, it occupies 32/8 = 4 bytes,
		 * in TX/RX buffers.
		 */
		word_size_bytes = (word_size == 24) ? 4 : word_size / 8;
		LOG_DBG("word_size_bytes = %u", word_size_bytes);

		/* Offset is in 'samples'.
		 *
		 * One 'sample' is data for all channels:
		 * two channels, 8 bit word -> sample is 2 bytes
		 * two channels, 16 bit word -> sample is 4 bytes
		 * two channels, 24 bit word -> sample is 8 bytes (24 bit extended to 32 bit)
		 * two channels, 32 bit word -> sample is 8 bytes
		 */
		sample_in_bytes = channels * word_size_bytes;
		LOG_DBG("sample_in_bytes = %u", sample_in_bytes);

		do {
			++offset;
			if (offset > CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET) {
				TC_PRINT("Allowed data offset (%d) exceeded\n",
					CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET);
				return -TC_FAIL;
			}
		} while (rx_block[offset * sample_in_bytes / 2] != data_l[0]);
		TC_PRINT("Using data offset: %d (%u bytes)\n", offset,
			offset * sample_in_bytes);
	}

	/* Divided by 2 because rx_block is uint16 */
	rx_block += (offset * sample_in_bytes / 2);
	sample_no -= (offset * word_size_bytes / 2);
	LOG_DBG("sample_no = %u", sample_no);
#endif

	/* Compare received data with sent values. */
	if (word_size == 24) {
		int8_t *rx_block_8bit = (int8_t *) rx_block;
		int8_t *data_l_8bit = (int8_t *) &data_l;
		int8_t *data_r_8bit = (int8_t *) &data_r;
		int16_t rx_cnt = 0;
		int16_t temp = 0;
		int8_t expected = 0;

		while (rx_cnt < (BLOCK_SIZE - offset * sample_in_bytes)) {
/* Map byte number from RX array to channel array
 *
 * rx_cnt | l_index | r_index || rx_cnt/8 | rx_cnt%4 | (rx_cnt/8)*3+(rx_cnt%4)
 *    0   |    0    |         ||     0    |    0     |    0*3+0 = 0  data_l
 *    1   |    1    |         ||     0    |    1     |    0*3+1 = 1  data_l
 *    2   |    2    |         ||     0    |    2     |    0*3+2 = 2  data_l
 *    3   |    -    |         ||     0    |    3     |    0*3+3 = 3  ignore
 *    4   |         |     0   ||     0    |    0     |    0*3+0 = 0  data_r
 *    5   |         |     1   ||     0    |    1     |    0*3+1 = 1  data_r
 *    6   |         |     2   ||     0    |    2     |    0*3+2 = 2  data_r
 *    7   |         |     -   ||     0    |    3     |    0*3+3 = 3  ignore
 *
 *    8   |    3    |         ||     1    |    0     |    1*3+0 = 3  data_l
 *    9   |    4    |         ||     1    |    1     |    1*3+1 = 4  data_l
 *   10   |    5    |         ||     1    |    2     |    1*3+2 = 5  data_l
 *   11   |    -    |         ||     1    |    3     |    1*3+3 = 6  ignore
 *   12   |         |     3   ||     1    |    0     |    1*3+0 = 3  data_r
 *   13   |         |     4   ||     1    |    1     |    1*3+1 = 4  data_r
 *   14   |         |     5   ||     1    |    2     |    1*3+2 = 5  data_r
 *   15   |         |     -   ||     1    |    3     |    1*3+3 = 6  ignore
 *
 *   16   |    6    |         ||     2    |    0     |    2*3+0 = 6  data_l
 *   ...
 */
			temp = ((rx_cnt / 8) * 3) + (rx_cnt % 4);

			if ((rx_cnt % 8) < 4) {
				/* Compare with left channel. */
				expected = data_l_8bit[temp];
			} else {
				/* Compare with right channel. */
				expected = data_r_8bit[temp];
			}

			if ((rx_cnt % 4) == 3) {
				/* Ignore every fourth byte */
			} else {
				/* Compare received data with expected value. */
				if (rx_block_8bit[rx_cnt] != expected) {
					TC_PRINT("Index %d, expected 0x%x, actual 0x%x\n",
						 rx_cnt, expected, rx_block_8bit[rx_cnt]);
					same = false;
				}
			}
			/* Move to next received byte. */
			rx_cnt++;
		}

	} else {
		for (int i = 0; i < sample_no; i++) {
			if (rx_block[2 * i] != data_l[i]) {
				TC_PRINT("data_l, index %d, expected 0x%x, actual 0x%x\n",
					 i, data_l[i], rx_block[2 * i]);
				same = false;
			}
			if (rx_block[2 * i + 1] != data_r[i]) {
				TC_PRINT("data_r, index %d, expected 0x%x, actual 0x%x\n",
					 i, data_r[i], rx_block[2 * i + 1]);
				same = false;
			}
		}
	}

	if (!same) {
		return -TC_FAIL;
	} else {
		return TC_PASS;
	}
}

static int configure_stream(const struct device *dev, enum i2s_dir dir,
	struct i2s_config *i2s_cfg)
{
	int ret;

	if (dir == I2S_DIR_TX) {
		/* Configure the Transmit port as Master */
		i2s_cfg->options = I2S_OPT_FRAME_CLK_MASTER
				| I2S_OPT_BIT_CLK_MASTER;
	} else if (dir == I2S_DIR_RX) {
		/* Configure the Receive port as Slave */
		i2s_cfg->options = I2S_OPT_FRAME_CLK_SLAVE
				| I2S_OPT_BIT_CLK_SLAVE;
	} else { /* dir == I2S_DIR_BOTH */
		i2s_cfg->options = I2S_OPT_FRAME_CLK_MASTER
				| I2S_OPT_BIT_CLK_MASTER;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		i2s_cfg->mem_slab = &tx_0_mem_slab;
		ret = i2s_configure(dev, I2S_DIR_TX, i2s_cfg);
		if (ret < 0) {
			TC_PRINT("Failed to configure I2S TX stream (%d)\n",
				 ret);
			return -TC_FAIL;
		}
	}

	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		i2s_cfg->mem_slab = &rx_0_mem_slab;
		ret = i2s_configure(dev, I2S_DIR_RX, i2s_cfg);
		if (ret < 0) {
			TC_PRINT("Failed to configure I2S RX stream (%d)\n",
				 ret);
			return -TC_FAIL;
		}
	}

	return TC_PASS;
}

static void i2s_dir_both_transfer_long(struct i2s_config *i2s_cfg)
{
	void *rx_block[NUM_BLOCKS];
	void *tx_block[NUM_BLOCKS];
	size_t rx_size;
	int tx_idx;
	int rx_idx = 0;
	int num_verified;
	int ret;

	/* Configure I2S Dir Both transfer. */
	ret = configure_stream(dev_i2s, I2S_DIR_BOTH, i2s_cfg);
	zassert_equal(ret, TC_PASS);

	/* Prepare TX data blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[tx_idx],
				       K_FOREVER);
		zassert_equal(ret, 0);
		fill_buf((uint16_t *)tx_block[tx_idx], i2s_cfg->word_size);
	}

	LOG_HEXDUMP_DBG(tx_block[0], BLOCK_SIZE, "transmitted");

	tx_idx = 0;

	/* Prefill TX queue */
	ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
	zassert_equal(ret, 0);

	ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
	zassert_equal(ret, 0);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	while (tx_idx < NUM_BLOCKS) {
		ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
		zassert_equal(ret, 0);

		ret = i2s_read(dev_i2s, &rx_block[rx_idx++], &rx_size);
		zassert_equal(ret, 0, "Got unexpected %d", ret);
		zassert_equal(rx_size, BLOCK_SIZE);
	}

	/* All data written, drain TX queue and stop both streams. */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	ret = i2s_read(dev_i2s, &rx_block[rx_idx++], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	ret = i2s_read(dev_i2s, &rx_block[rx_idx++], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	TC_PRINT("%d TX blocks sent\n", tx_idx);
	TC_PRINT("%d RX blocks received\n", rx_idx);

	/* Verify received data */
	num_verified = 0;
	for (rx_idx = 0; rx_idx < NUM_BLOCKS; rx_idx++) {
		ret = verify_buf((uint16_t *)rx_block[rx_idx],
			i2s_cfg->word_size, i2s_cfg->channels);
		if (ret != 0) {
			TC_PRINT("%d RX block invalid\n", rx_idx);
		} else {
			num_verified++;
		}
		k_mem_slab_free(&rx_0_mem_slab, rx_block[rx_idx]);
	}
	zassert_equal(num_verified, NUM_BLOCKS, "Invalid RX blocks received");
}

/** @brief Test I2S transfer with word_size_8bit
 */
ZTEST(i2s_additional, test_01a_word_size_08bit)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.word_size = 8;

#if defined(CONFIG_I2S_TEST_WORD_SIZE_8_BIT_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with word_size_16bit
 */
ZTEST(i2s_additional, test_01b_word_size_16bit)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.word_size = 16;

#if defined(CONFIG_I2S_TEST_WORD_SIZE_16_BIT_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with word_size_24bit
 */
ZTEST(i2s_additional, test_01c_word_size_24bit)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.word_size = 24;

#if defined(CONFIG_I2S_TEST_WORD_SIZE_24_BIT_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with word_size_32bit
 */
ZTEST(i2s_additional, test_01d_word_size_32bit)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.word_size = 32;

#if defined(CONFIG_I2S_TEST_WORD_SIZE_32_BIT_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with word_size_48bit
 */
ZTEST(i2s_additional, test_01e_word_size_48bit)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.word_size = 48;

#if defined(CONFIG_I2S_TEST_WORD_SIZE_48_BIT_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with one channel.
 */
ZTEST(i2s_additional, test_02a_one_channel)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.channels = 1;

#if defined(CONFIG_I2S_TEST_ONE_CHANNEL_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with four channels.
 */
ZTEST(i2s_additional, test_02b_four_channels)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.channels = 4;

#if defined(CONFIG_I2S_TEST_FOUR_CHANNELS_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else /* CONFIG_I2S_TEST_FOUR_CHANNELS_UNSUPPORTED */

	/* Select format that supports four channels. */
#if !defined(CONFIG_I2S_TEST_DATA_FORMAT_PCM_LONG_UNSUPPORTED)
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_PCM_LONG;
	TC_PRINT("Selected format is I2S_FMT_DATA_FORMAT_PCM_LONG\n");
#elif !defined(CONFIG_I2S_TEST_DATA_FORMAT_PCM_SHORT_UNSUPPORTED)
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_PCM_SHORT;
	TC_PRINT("Selected format is I2S_FMT_DATA_FORMAT_PCM_SHORT\n");
#else
#error "Don't know what format supports four channels."
#endif

	i2s_dir_both_transfer_long(&i2s_cfg);
#endif /* CONFIG_I2S_TEST_FOUR_CHANNELS_UNSUPPORTED */
}

/** @brief Test I2S transfer with eight channels, 16 bit and 44.1 kHz.
 */
ZTEST(i2s_additional, test_02c_eight_channels_default)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.channels = 8;

#if defined(CONFIG_I2S_TEST_EIGHT_CHANNELS_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else /* CONFIG_I2S_TEST_EIGHT_CHANNELS_UNSUPPORTED */

	/* Select format that supports eight channels. */
#if !defined(CONFIG_I2S_TEST_DATA_FORMAT_PCM_LONG_UNSUPPORTED)
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_PCM_LONG;
	TC_PRINT("Selected format is I2S_FMT_DATA_FORMAT_PCM_LONG\n");
#elif !defined(CONFIG_I2S_TEST_DATA_FORMAT_PCM_SHORT_UNSUPPORTED)
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_PCM_SHORT;
	TC_PRINT("Selected format is I2S_FMT_DATA_FORMAT_PCM_SHORT\n");
#else
#error "Don't know what format supports eight channels."
#endif

	i2s_dir_both_transfer_long(&i2s_cfg);
#endif /* CONFIG_I2S_TEST_EIGHT_CHANNELS_UNSUPPORTED */
}

/** @brief Test I2S transfer with eight channels, 32 bit and 48 kHz.
 */
ZTEST(i2s_additional, test_02d_eight_channels_high_throughput)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.channels = 8;
	i2s_cfg.word_size = 32;
	i2s_cfg.frame_clk_freq = 48000;

#if defined(CONFIG_I2S_TEST_EIGHT_CHANNELS_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else /* CONFIG_I2S_TEST_EIGHT_CHANNELS_UNSUPPORTED */

#if defined(CONFIG_I2S_TEST_EIGHT_CHANNELS_32B_48K_UNSUPPORTED)
	/* Skip this test if driver supports 8ch but fails in this configuration. */
	ztest_test_skip();
#endif

	/* Select format that supports eight channels. */
#if !defined(CONFIG_I2S_TEST_DATA_FORMAT_PCM_LONG_UNSUPPORTED)
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_PCM_LONG;
	TC_PRINT("Selected format is I2S_FMT_DATA_FORMAT_PCM_LONG\n");
#elif !defined(CONFIG_I2S_TEST_DATA_FORMAT_PCM_SHORT_UNSUPPORTED)
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_PCM_SHORT;
	TC_PRINT("Selected format is I2S_FMT_DATA_FORMAT_PCM_SHORT\n");
#else
#error "Don't know what format supports eight channels."
#endif

	i2s_dir_both_transfer_long(&i2s_cfg);
#endif /* CONFIG_I2S_TEST_EIGHT_CHANNELS_UNSUPPORTED */
}

/** @brief Test I2S transfer with format I2S_FMT_DATA_FORMAT_I2S
 */
ZTEST(i2s_additional, test_03a_format_i2s)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;

#if defined(CONFIG_I2S_TEST_DATA_FORMAT_I2S_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with format I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED
 */
ZTEST(i2s_additional, test_03b_format_left_justified)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.format = I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED;

#if defined(CONFIG_I2S_TEST_DATA_FORMAT_LEFT_JUSTIFIED_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with format I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED
 */
ZTEST(i2s_additional, test_03c_format_right_justified)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.format = I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED;

#if defined(CONFIG_I2S_TEST_DATA_FORMAT_RIGHT_JUSTIFIED_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with format I2S_FMT_DATA_FORMAT_PCM_LONG
 */
ZTEST(i2s_additional, test_03d_format_pcm_long)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.format = I2S_FMT_DATA_FORMAT_PCM_LONG;

#if defined(CONFIG_I2S_TEST_DATA_FORMAT_PCM_LONG_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with format I2S_FMT_DATA_FORMAT_PCM_SHORT
 */
ZTEST(i2s_additional, test_03e_format_pcm_short)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.format = I2S_FMT_DATA_FORMAT_PCM_SHORT;

#if defined(CONFIG_I2S_TEST_DATA_FORMAT_PCM_SHORT_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with format I2S_FMT_DATA_ORDER_MSB
 */
ZTEST(i2s_additional, test_04a_format_data_order_MSB)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.format |= I2S_FMT_DATA_ORDER_MSB;

#if defined(CONFIG_I2S_TEST_DATA_ORDER_MSB_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with format I2S_FMT_DATA_ORDER_LSB
 */
ZTEST(i2s_additional, test_04b_format_data_order_LSB)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.format |= I2S_FMT_DATA_ORDER_LSB;

#if defined(CONFIG_I2S_TEST_DATA_ORDER_LSB_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with format I2S_FMT_BIT_CLK_INV
 */
ZTEST(i2s_additional, test_05a_format_bit_clk_inv)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.format |= I2S_FMT_BIT_CLK_INV;

#if defined(CONFIG_I2S_TEST_BIT_CLK_INV_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with format I2S_FMT_FRAME_CLK_INV
 */
ZTEST(i2s_additional, test_05b_format_frame_clk_inv)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.format |= I2S_FMT_FRAME_CLK_INV;

#if defined(CONFIG_I2S_TEST_FRAME_CLK_INV_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with block_size set to 6.
 */
ZTEST(i2s_additional, test_06_block_size_6)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.block_size = 6;

#if defined(CONFIG_I2S_TEST_BLOCK_SIZE_6_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with I2S_OPT_BIT_CLK_CONT.
 */
ZTEST(i2s_additional, test_07a_options_bit_clk_cont)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.options |= I2S_OPT_BIT_CLK_CONT;

#if defined(CONFIG_I2S_TEST_OPTIONS_BIT_CLK_CONT_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with I2S_OPT_BIT_CLK_GATED.
 */
ZTEST(i2s_additional, test_07b_options_bit_clk_gated)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.options |= I2S_OPT_BIT_CLK_GATED;

#if defined(CONFIG_I2S_TEST_OPTIONS_BIT_CLK_GATED_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Check error when I2S is configured with unsupported
 *  combination of bit CLK and frame CLK options.
 */
ZTEST(i2s_additional, test_08_options_bit_frame_clk_mixed)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	int ret;

	i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_SLAVE;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);

	i2s_cfg.options = I2S_OPT_FRAME_CLK_SLAVE | I2S_OPT_BIT_CLK_MASTER;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
}

/** @brief Test I2S transfer with I2S_OPT_LOOPBACK.
 */
ZTEST(i2s_additional, test_09a_options_loopback)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.options |= I2S_OPT_LOOPBACK;

#if defined(CONFIG_I2S_TEST_OPTIONS_LOOPBACK_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

/** @brief Test I2S transfer with I2S_OPT_PINGPONG.
 */
ZTEST(i2s_additional, test_09b_options_pingpong)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.options |= I2S_OPT_PINGPONG;

#if defined(CONFIG_I2S_TEST_OPTIONS_LOOPBACK_UNSUPPORTED)
	int ret;

	ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
#else
	i2s_dir_both_transfer_long(&i2s_cfg);
#endif
}

static void *suite_setup(void)
{
	/* Check I2S Device. */
	dev_i2s = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE);
	zassert_not_null(dev_i2s, "I2S device not found");
	zassert(device_is_ready(dev_i2s), "I2S device not ready");

	LOG_HEXDUMP_DBG(&data_l, 2 * SAMPLES_COUNT, "data_l");
	LOG_HEXDUMP_DBG(&data_r, 2 * SAMPLES_COUNT, "data_r");
	TC_PRINT("===================================================================\n");

	return 0;
}

static void before(void *not_used)
{
	ARG_UNUSED(not_used);

#if (CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET > 0)
	/* Data offset may differ when test uses I2S
	 * with different configuration.
	 * Force offset callculation for every test.
	 */
	offset = -1;
#endif
}

ZTEST_SUITE(i2s_additional, NULL, suite_setup, before, NULL, NULL);
