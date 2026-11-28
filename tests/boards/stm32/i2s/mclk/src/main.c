/*
 * Copyright (c) 2026 Mario Paja
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/i2s.h>

#define I2S_NODE  DT_ALIAS(i2s)
#define MCLK_NODE DT_ALIAS(i2s_mclk)

#define FRAME_RATE 44100U
#define MCLK_RATE  (256U * FRAME_RATE)

K_MEM_SLAB_DEFINE_STATIC(tx_slab, 64, 2, 32);

ZTEST(i2s_mclk, test_mclk_rate)
{
	const struct device *sai = DEVICE_DT_GET(I2S_NODE);
	const struct device *mclk = DEVICE_DT_GET(MCLK_NODE);
	const struct i2s_config cfg = {
		.word_size = 16,
		.channels = 2,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = 0, /* controller for both bit and frame clocks */
		.frame_clk_freq = FRAME_RATE,
		.mem_slab = &tx_slab,
		.block_size = 64,
		.timeout = 0,
	};
	uint32_t rate = 0;

	zassert_true(device_is_ready(sai));
	zassert_true(device_is_ready(mclk));

	/* MCLK is only configured by i2s_configure() */
	zassert_ok(clock_control_on(mclk, NULL));
	zassert_equal(clock_control_get_rate(mclk, NULL, &rate), -ENOTSUP);

	zassert_ok(i2s_configure(sai, I2S_DIR_TX, &cfg));
	zassert_ok(clock_control_on(mclk, NULL));
	zassert_ok(clock_control_get_rate(mclk, NULL, &rate));

	/* MCKDIV is an integer, so allow 2% error */
	zassert_within(rate, MCLK_RATE, MCLK_RATE / 50U, "mclk rate %u", rate);
}

ZTEST_SUITE(i2s_mclk, NULL, NULL, NULL, NULL, NULL);
