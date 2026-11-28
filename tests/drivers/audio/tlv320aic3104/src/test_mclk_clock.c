/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * MCLK from the devicetree "mclk" clock against audio_codec_cfg.mclk_freq.
 * A fixed-clock (12.288 MHz) is read at build time. The test provider of
 * fake_clock.c has no clock-frequency property, so the driver switches it on
 * and reads it through the clock control API.
 *
 * Register values follow SLAS509G:
 *   R2   ADC/DAC fs = fsref / NCODEC (Table 10-10), 0x00 = fsref / 1.
 *   R3   D7 PLL enable, D6-D3 Q, D2-D0 P (Table 10-11). 12.288 MHz at 48 kHz
 *        is Q = 2 without the PLL: 0x10. 44.1 kHz needs the PLL: D7 set.
 *   R101 D0 CODEC_CLKIN: 1 = CLKDIV_OUT, 0 = PLLDIV_OUT (Table 10-110).
 */

#include <errno.h>

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/audio/codec.h>

#include "tlv320aic3104_priv.h"

#include "tlv320aic3104_emul.h"
#include "fake_clock.h"

#define AIC_DT_MCLK DEVICE_DT_GET(DT_NODELABEL(tlv320aic3106_mclk))
#define AIC_NO_MCLK DEVICE_DT_GET(DT_NODELABEL(tlv320aic3106_default))
#define AIC_FAKE_CLK DEVICE_DT_GET(DT_NODELABEL(tlv320aic3106_fake_mclk))

#define FAKE_CLOCK_CELL 7U

#define R2_NCODEC     0x02U
#define R3_PLL_A      0x03U
#define R101_CLKIN    0x65U
#define R3_Q2_NO_PLL  0x10U
#define R3_PLL_ENABLE 0x80U

static int configure(const struct device *dev, uint32_t mclk_freq, uint32_t fs)
{
	struct audio_codec_cfg cfg = {
		.mclk_freq = mclk_freq,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = fs,
		.dai_cfg.i2s.word_size = 16,
	};

	tlv320aic3104_emul_reset();
	return audio_codec_configure(dev, &cfg);
}

static void assert_48k_divider_regs(void)
{
	zassert_equal(tlv320aic3104_emul_last_val(0, R2_NCODEC), 0x00U);
	zassert_equal(tlv320aic3104_emul_last_val(0, R3_PLL_A), R3_Q2_NO_PLL);
	zassert_equal(tlv320aic3104_emul_last_val(0, R101_CLKIN), 0x01U);
}

/* The echo sample leaves mclk_freq unset: the devicetree rate must be used. */
ZTEST(tlv320_aic3104_mclk, test_dt_clock_used_when_cfg_rate_is_zero)
{
	zassert_ok(configure(AIC_DT_MCLK, 0U, 48000U));
	assert_48k_divider_regs();
}

ZTEST(tlv320_aic3104_mclk, test_dt_clock_overrides_cfg_rate)
{
	/* 11.2896 MHz would need the PLL for 48 kHz; the board clock wins. */
	zassert_ok(configure(AIC_DT_MCLK, 11289600U, 48000U));
	assert_48k_divider_regs();
}

ZTEST(tlv320_aic3104_mclk, test_dt_clock_44k1_uses_pll)
{
	zassert_ok(configure(AIC_DT_MCLK, 0U, 44100U));
	zassert_equal(tlv320aic3104_emul_last_val(0, R3_PLL_A) & R3_PLL_ENABLE, R3_PLL_ENABLE);
	zassert_equal(tlv320aic3104_emul_last_val(0, R101_CLKIN), 0x00U);
}

ZTEST(tlv320_aic3104_mclk, test_cfg_rate_without_dt_clock)
{
	zassert_ok(configure(AIC_NO_MCLK, 12288000U, 48000U));
	assert_48k_divider_regs();
}

ZTEST(tlv320_aic3104_mclk, test_no_rate_at_all_rejected)
{
	zassert_equal(configure(AIC_NO_MCLK, 0U, 48000U), -EINVAL);
	zassert_false(tlv320aic3104_emul_was_written(0, R3_PLL_A), "R3 written");
}

ZTEST(tlv320_aic3104_mclk, test_provider_switched_on_then_rate_read)
{
	zassert_ok(configure(AIC_FAKE_CLK, 0U, 48000U));
	assert_48k_divider_regs();
	zassert_equal(fake_clock_on_count(), 1U);
	zassert_equal(fake_clock_last_subsys(), (uintptr_t)FAKE_CLOCK_CELL);
}

ZTEST(tlv320_aic3104_mclk, test_provider_already_on_is_not_an_error)
{
	fake_clock_set_on_result(-EALREADY);
	zassert_ok(configure(AIC_FAKE_CLK, 0U, 48000U));
	assert_48k_divider_regs();
}

ZTEST(tlv320_aic3104_mclk, test_provider_enable_failure_rejected)
{
	fake_clock_set_on_result(-EIO);
	zassert_equal(configure(AIC_FAKE_CLK, 0U, 48000U), -EIO);
	zassert_equal(fake_clock_on_count(), 1U);
	zassert_false(tlv320aic3104_emul_was_written(0, R3_PLL_A), "R3 written");
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	fake_clock_reset();
}

ZTEST_SUITE(tlv320_aic3104_mclk, NULL, NULL, before_each, NULL, NULL);
