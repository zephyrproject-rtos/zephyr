/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/audio/codec.h>
#include <errno.h>

#include "tlv320aic3104_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_configure_fail))

#define REG_CODEC_DATAPATH 0x07
#define REG_DAC_POWER      0x25
#define REG_HPLOUT_LEVEL   0x33
#define VAL_HPLOUT_LEVEL   0x09

static struct audio_codec_cfg s_cfg = {
	.mclk_freq = 12288000,
	.dai_type = AUDIO_DAI_TYPE_I2S,
	.dai_cfg.i2s.frame_clk_freq = 48000,
	.dai_cfg.i2s.word_size = 16,
};

static void *suite_setup(void)
{
	zassert_true(device_is_ready(CODEC), "codec device not ready");
	return NULL;
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	tlv320aic3104_emul_reset();
}

ZTEST_SUITE(tlv320_configure_fail, NULL, suite_setup, before_each, NULL, NULL);

ZTEST(tlv320_configure_fail, test_configure_ok_baseline)
{
	zassert_ok(audio_codec_configure(CODEC, &s_cfg), "configure must succeed without faults");
	zassert_equal(tlv320aic3104_emul_last_val(0, REG_HPLOUT_LEVEL), VAL_HPLOUT_LEVEL,
		      "init sequence did not reach HPLOUT_LEVEL");
}

ZTEST(tlv320_configure_fail, test_channel_mode_write_failure_propagates)
{
	tlv320aic3104_emul_fail_write_at(0, REG_CODEC_DATAPATH);

	zassert_equal(audio_codec_configure(CODEC, &s_cfg), -EIO,
		      "channel-mode write failure must propagate");

	zassert_equal(tlv320aic3104_emul_last_val(0, REG_HPLOUT_LEVEL), 0x00,
		      "writes after the failed register must not run");
}

ZTEST(tlv320_configure_fail, test_init_sequence_write_failure_propagates)
{
	tlv320aic3104_emul_fail_write_at(0, REG_DAC_POWER);

	zassert_equal(audio_codec_configure(CODEC, &s_cfg), -EIO,
		      "init-sequence write failure must propagate");
	zassert_equal(tlv320aic3104_emul_last_val(0, REG_HPLOUT_LEVEL), 0x00,
		      "writes after the failed register must not run");
}

ZTEST(tlv320_configure_fail, test_configure_recovers_after_fault_cleared)
{
	tlv320aic3104_emul_fail_write_at(0, REG_DAC_POWER);
	zassert_equal(audio_codec_configure(CODEC, &s_cfg), -EIO, "armed fault must fail");

	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_configure(CODEC, &s_cfg), "configure must succeed after recovery");
	zassert_equal(tlv320aic3104_emul_last_val(0, REG_HPLOUT_LEVEL), VAL_HPLOUT_LEVEL,
		      "recovered sequence did not reach HPLOUT_LEVEL");
}
