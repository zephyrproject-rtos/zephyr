/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * hp-single-ended: page 0 register 14 (SLAS509G Table 10-22, SLAS510G
 * Table 10-20). D7 = 1 ac-coupled high-power outputs, D6 = 1 stereo fully
 * differential output configuration, D3 = 1 pseudo-differential (never set
 * together with D6), D5/D4 read-only flags, D2-D0 reserved (write zeros).
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/audio/codec.h>

#include "tlv320aic3104_priv.h"

#include "tlv320aic3104_emul.h"

#define AIC_HP_SE   DEVICE_DT_GET(DT_NODELABEL(tlv320aic3106_hp_se))
#define AIC_HP_DIFF DEVICE_DT_GET(DT_NODELABEL(tlv320aic3106_default))

#define R14_HEADSET_B 0x0EU

static void configure(const struct device *dev)
{
	struct audio_codec_cfg cfg = {
		.mclk_freq = 12288000U,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = 48000U,
		.dai_cfg.i2s.word_size = 16,
	};

	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_configure(dev, &cfg));
}

ZTEST(tlv320_aic3104_hp_se, test_single_ended_clears_fully_differential)
{
	configure(AIC_HP_SE);
	zassert_equal(tlv320aic3104_emul_last_val(0, R14_HEADSET_B), 0x80U);
}

ZTEST(tlv320_aic3104_hp_se, test_default_stays_fully_differential)
{
	configure(AIC_HP_DIFF);
	zassert_equal(tlv320aic3104_emul_last_val(0, R14_HEADSET_B), 0xC0U);
}

ZTEST(tlv320_aic3104_hp_se, test_single_ended_survives_output_start_stop)
{
	configure(AIC_HP_SE);
	zassert_ok(audio_codec_start(AIC_HP_SE, AUDIO_DAI_DIR_TXRX));
	zassert_ok(audio_codec_stop(AIC_HP_SE, AUDIO_DAI_DIR_TXRX));
	zassert_equal(tlv320aic3104_emul_last_val(0, R14_HEADSET_B), 0x80U);
}

ZTEST(tlv320_aic3104_hp_se, test_r14_written_before_dac_power_and_output_stage)
{
	configure(AIC_HP_SE);
	zassert_true(tlv320aic3104_emul_write_seq(0, R14_HEADSET_B) <
		     tlv320aic3104_emul_write_seq(0, DAC_POWER_DRV));
	zassert_true(tlv320aic3104_emul_write_seq(0, R14_HEADSET_B) <
		     tlv320aic3104_emul_write_seq(0, HP_OUTPUT_STAGE));

	configure(AIC_HP_DIFF);
	zassert_true(tlv320aic3104_emul_write_seq(0, R14_HEADSET_B) <
		     tlv320aic3104_emul_write_seq(0, DAC_POWER_DRV));
}

ZTEST_SUITE(tlv320_aic3104_hp_se, NULL, NULL, NULL, NULL, NULL);
