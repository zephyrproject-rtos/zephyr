/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/i2s.h>
#include "i2s_api_test.h"

#ifdef CONFIG_I2S_TEST_TDM_CHANNEL_MASKING

#define PATTERN_BASE 0x1000

static int configure_with_mask(const struct device *dev, enum i2s_dir dir,
			       uint32_t mask, size_t block_size)
{
	int ret;
	struct i2s_config i2s_cfg = {0};

	i2s_cfg.word_size = 16U;
	i2s_cfg.channels = 2U;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.frame_clk_freq = FRAME_CLK_FREQ;
	i2s_cfg.block_size = block_size;
	i2s_cfg.timeout = TIMEOUT;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER;
	if (!IS_ENABLED(CONFIG_I2S_TEST_USE_GPIO_LOOPBACK)) {
		i2s_cfg.options |= I2S_OPT_LOOPBACK;
	}
	i2s_cfg.tdm.channel_disable_mask = mask;

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		i2s_cfg.mem_slab = &tx_mem_slab;
		ret = i2s_configure(dev, I2S_DIR_TX, &i2s_cfg);
		if (ret < 0) {
			return ret;
		}
	}
	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		i2s_cfg.mem_slab = &rx_mem_slab;
		ret = i2s_configure(dev, I2S_DIR_RX, &i2s_cfg);
		if (ret < 0) {
			return ret;
		}
	}
	return 0;
}

static void fill_pattern(int16_t *buf, size_t word_count)
{
	for (size_t i = 0; i < word_count; i++) {
		buf[i] = (int16_t)(PATTERN_BASE + i);
	}
}

static int verify_pattern(int16_t *buf, size_t word_count)
{
	size_t offset = 0;

#if (CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET > 0)
	while (offset <= CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET) {
		if (buf[offset] == (int16_t)PATTERN_BASE) {
			break;
		}
		offset++;
	}
	if (offset > CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET) {
		TC_PRINT("RX pattern not found within allowed offset\n");
		return -TC_FAIL;
	}
#endif
	for (size_t i = 0; i + offset < word_count; i++) {
		int16_t expected = (int16_t)(PATTERN_BASE + i);

		if (buf[i + offset] != expected) {
			TC_PRINT("mismatch at word %u: expected 0x%04x got 0x%04x\n",
				 (unsigned int)i, (uint16_t)expected,
				 (uint16_t)buf[i + offset]);
			return -TC_FAIL;
		}
	}
	return TC_PASS;
}

/** @brief Symmetric mask on TX and RX.
 *
 * - Disabling the same slot on both directions returns success for each bit.
 * - RX content matches the TX pattern in the compacted single-channel layout.
 */
ZTEST_USER(tdm_channel_masking, test_channel_mask_symmetric)
{
	static const uint32_t masks[] = {BIT(0), BIT(1)};
	int ret;
	size_t rx_size;
	char tx_buf[BLOCK_SIZE];
	char rx_buf[BLOCK_SIZE];

	if (!dir_both_supported) {
		ztest_test_skip();
		return;
	}

	for (size_t m = 0; m < ARRAY_SIZE(masks); m++) {
		TC_PRINT("mask=0x%x\n", masks[m]);

		ret = configure_with_mask(dev_i2s, I2S_DIR_BOTH, masks[m], BLOCK_SIZE);
		zassert_equal(ret, 0, "configure failed");

		fill_pattern((int16_t *)tx_buf, BLOCK_SIZE / sizeof(int16_t));
		ret = i2s_buf_write(dev_i2s, tx_buf, BLOCK_SIZE);
		zassert_equal(ret, 0, "TX write failed");

		ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
		zassert_equal(ret, 0, "RX/TX START trigger failed");

		ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
		zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

		ret = i2s_buf_read(dev_i2s, rx_buf, &rx_size);
		zassert_equal(ret, 0, "RX read failed");
		zassert_equal(rx_size, BLOCK_SIZE, "unexpected RX size");

		ret = verify_pattern((int16_t *)rx_buf, BLOCK_SIZE / sizeof(int16_t));
		zassert_equal(ret, TC_PASS, "data verify failed");
	}
}

/** @brief Asymmetric TX/RX masks via two configure calls.
 *
 * - One I2S_DIR_TX configure with a mask and one I2S_DIR_RX configure without.
 * - START / DRAIN trigger sequence completes on the masked configuration.
 */
ZTEST_USER(tdm_channel_masking, test_channel_mask_asymmetric)
{
	int ret;
	size_t rx_size;
	char tx_buf[BLOCK_SIZE / 2] = {0};
	char rx_buf[BLOCK_SIZE];

	if (!dir_both_supported) {
		ztest_test_skip();
		return;
	}

	ret = configure_with_mask(dev_i2s, I2S_DIR_TX, BIT(0), BLOCK_SIZE / 2);
	zassert_equal(ret, 0, "TX configure with mask failed");
	ret = configure_with_mask(dev_i2s, I2S_DIR_RX, 0, BLOCK_SIZE);
	zassert_equal(ret, 0, "RX configure without mask failed");

	ret = i2s_buf_write(dev_i2s, tx_buf, sizeof(tx_buf));
	zassert_equal(ret, 0, "TX write failed");
	TC_PRINT("%d->OK\n", 1);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed");

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	ret = i2s_buf_read(dev_i2s, rx_buf, &rx_size);
	zassert_equal(ret, 0, "RX read failed");
	zassert_equal(rx_size, BLOCK_SIZE, "unexpected RX size");
	TC_PRINT("%d<-OK\n", 1);
}

/** @brief Disabling every channel must be rejected at START.
 *
 * - START trigger fails when no slots are enabled in at least one direction.
 */
ZTEST_USER(tdm_channel_masking, test_channel_mask_all_disabled_rejected)
{
	int ret;

	if (!dir_both_supported) {
		ztest_test_skip();
		return;
	}

	ret = configure_with_mask(dev_i2s, I2S_DIR_BOTH, BIT_MASK(2), BLOCK_SIZE);
	zassert_equal(ret, 0, "configure failed");

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_not_equal(ret, 0, "START with all channels disabled should fail");
}

/** @brief Mask with bits beyond channels must be rejected at configure.
 *
 * - i2s_configure() returns -EINVAL when channel_disable_mask has bits at or
 *   above i2s_config.channels.
 */
ZTEST_USER(tdm_channel_masking, test_channel_mask_out_of_range_rejected)
{
	int ret;

	ret = configure_with_mask(dev_i2s, I2S_DIR_BOTH, BIT(2), BLOCK_SIZE);
	zassert_equal(ret, -EINVAL, "out-of-range mask should return -EINVAL, got %d", ret);
}

/** @brief Reconfigure with a different mask cleans up prior state.
 *
 * - Back-to-back configures with different masks each return success.
 * - A transfer completes after each configure.
 */
ZTEST_USER(tdm_channel_masking, test_channel_mask_reconfigure_cycle)
{
	static const uint32_t masks[] = {BIT(0), BIT(1)};
	int ret;
	size_t rx_size;
	char tx_buf[BLOCK_SIZE] = {0};
	char rx_buf[BLOCK_SIZE];

	if (!dir_both_supported) {
		ztest_test_skip();
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(masks); i++) {
		TC_PRINT("step %u mask=0x%x\n", (unsigned int)i, masks[i]);

		ret = configure_with_mask(dev_i2s, I2S_DIR_BOTH, masks[i], BLOCK_SIZE);
		zassert_equal(ret, 0, "configure failed");

		ret = i2s_buf_write(dev_i2s, tx_buf, BLOCK_SIZE);
		zassert_equal(ret, 0, "TX write failed");

		ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
		zassert_equal(ret, 0, "RX/TX START trigger failed");

		ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
		zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

		ret = i2s_buf_read(dev_i2s, rx_buf, &rx_size);
		zassert_equal(ret, 0, "RX read failed");
		zassert_equal(rx_size, BLOCK_SIZE, "unexpected RX size");
	}
}

#endif /* CONFIG_I2S_TEST_TDM_CHANNEL_MASKING */
