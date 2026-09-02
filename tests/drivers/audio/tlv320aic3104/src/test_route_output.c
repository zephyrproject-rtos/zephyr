/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>

#include "tlv320aic3104_out.h"

#include "tlv320aic3104_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_route_output))

#define DAC_L1_TO_HPLOUT_VOL    0x2F
#define DAC_R1_TO_HPROUT_VOL    0x40
#define DAC_L1_TO_LEFT_LOP_VOL  0x52
#define DAC_R1_TO_RIGHT_LOP_VOL 0x5C

ZTEST(tlv320_route_output, test_hp_front_left)
{
	tlv320aic3104_emul_reset();
	zassert_ok(
		audio_codec_route_output(CODEC, AUDIO_CHANNEL_FRONT_LEFT, TLV320AIC3104_OUTPUT_HP));
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0x80, "HP routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_LEFT_LOP_VOL), 0x00,
		      "LOP not routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_HPROUT_VOL), 0x00,
		      "right untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_RIGHT_LOP_VOL), 0x00,
		      "right untouched");
}

ZTEST(tlv320_route_output, test_lop_front_right)
{
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_FRONT_RIGHT,
					    TLV320AIC3104_OUTPUT_LOP));
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_RIGHT_LOP_VOL), 0x80, "LOP routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_HPROUT_VOL), 0x00, "HP not routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0x00, "left untouched");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_LEFT_LOP_VOL), 0x00,
		      "left untouched");
}

ZTEST(tlv320_route_output, test_hp_all)
{
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_HP));
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0x80, "left HP routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_HPROUT_VOL), 0x80,
		      "right HP routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_LEFT_LOP_VOL), 0x00,
		      "left LOP not routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_RIGHT_LOP_VOL), 0x00,
		      "right LOP not routed");
}

ZTEST(tlv320_route_output, test_hp_front_center_routes_like_all)
{
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_FRONT_CENTER,
					    TLV320AIC3104_OUTPUT_HP));
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0x80, "left HP routed");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_HPROUT_VOL), 0x80,
		      "right HP routed");
}

ZTEST(tlv320_route_output, test_unsupported_output_writes_nothing)
{
	tlv320aic3104_emul_reset();
	zassert_equal(audio_codec_route_output(CODEC, AUDIO_CHANNEL_FRONT_LEFT, 99), -ENOTSUP);
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0x00,
		      "no write on rejection");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_LEFT_LOP_VOL), 0x00,
		      "no write on rejection");
}

ZTEST(tlv320_route_output, test_unsupported_channel_writes_nothing)
{
	tlv320aic3104_emul_reset();
	zassert_equal(audio_codec_route_output(CODEC, AUDIO_CHANNEL_LFE, TLV320AIC3104_OUTPUT_HP),
		      -ENOTSUP);
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_L1_TO_HPLOUT_VOL), 0x00,
		      "no write on rejection");
	zassert_equal(tlv320aic3104_emul_last_val(0, DAC_R1_TO_HPROUT_VOL), 0x00,
		      "no write on rejection");
}

ZTEST_SUITE(tlv320_route_output, NULL, NULL, NULL, NULL, NULL);
