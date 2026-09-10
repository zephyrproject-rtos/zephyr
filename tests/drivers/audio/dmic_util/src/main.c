/*
 * Copyright 2025 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/ztest.h>

ZTEST(dmic_util, test_io_cfg_get)
{
	const struct pdm_io_cfg cfg0 = PDM_DT_IO_CFG_GET(DT_NODELABEL(dmic_cfg0));
	const struct pdm_io_cfg cfg1 = PDM_DT_IO_CFG_GET(DT_NODELABEL(dmic_cfg1));
	const struct pdm_io_cfg cfg2 = PDM_DT_IO_CFG_GET(DT_NODELABEL(dmic_cfg2));

	zassert_equal(1000000, cfg0.min_pdm_clk_freq);
	zassert_equal(2000000, cfg0.max_pdm_clk_freq);
	zassert_equal(45, cfg0.min_pdm_clk_dc);
	zassert_equal(55, cfg0.max_pdm_clk_dc);

	zassert_equal(100000, cfg1.min_pdm_clk_freq);
	zassert_equal(400000, cfg1.max_pdm_clk_freq);
	zassert_equal(40, cfg1.min_pdm_clk_dc);
	zassert_equal(60, cfg1.max_pdm_clk_dc);

	zassert_equal(150000, cfg2.min_pdm_clk_freq);
	zassert_equal(450000, cfg2.max_pdm_clk_freq);
	zassert_equal(49, cfg2.min_pdm_clk_dc);
	zassert_equal(51, cfg2.max_pdm_clk_dc);
}

ZTEST(dmic_util, test_channel_query)
{
	zassert_true(PDM_DT_HAS_LEFT_CHANNEL(DT_NODELABEL(dmic_cfg0)));
	zassert_false(PDM_DT_HAS_RIGHT_CHANNEL(DT_NODELABEL(dmic_cfg0)));

	zassert_false(PDM_DT_HAS_LEFT_CHANNEL(DT_NODELABEL(dmic_cfg1)));
	zassert_true(PDM_DT_HAS_RIGHT_CHANNEL(DT_NODELABEL(dmic_cfg1)));

	zassert_true(PDM_DT_HAS_LEFT_CHANNEL(DT_NODELABEL(dmic_cfg2)));
	zassert_true(PDM_DT_HAS_RIGHT_CHANNEL(DT_NODELABEL(dmic_cfg2)));
}

ZTEST_SUITE(dmic_util, NULL, NULL, NULL, NULL, NULL);
