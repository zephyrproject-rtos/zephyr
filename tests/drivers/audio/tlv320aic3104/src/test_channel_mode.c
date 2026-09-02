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

#define CODEC DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_channel_mode))

ZTEST(tlv320_channel_mode, test_stereo_sets_datapath_0x0a)
{
	zassert_true(device_is_ready(CODEC));
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_HP));
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x07), 0x0A, "stereo datapath");
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x11), 0x0F, "stereo R17");
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x12), 0xF0, "stereo R18");
}

ZTEST(tlv320_channel_mode, test_mono_sets_datapath_0x1e)
{
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_FRONT_CENTER,
					    TLV320AIC3104_OUTPUT_HP));
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x07), 0x1E, "mono datapath");
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x11), 0x44, "mono R17");
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x12), 0x44, "mono R18");
}

ZTEST(tlv320_channel_mode, test_capture_start_powers_adc_and_routes_line2)
{
	tlv320aic3104_emul_reset();

	tlv320aic3104_emul_set_val(0, 0x13, 0x78);
	tlv320aic3104_emul_set_val(0, 0x16, 0x78);
	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_HP));
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x13), 0x7C, "Left-ADC powered (R19)");
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x16), 0x7C, "Right-ADC powered (R22)");
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x0F), 0x00, "Left PGA 0dB unmuted (R15)");
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x10), 0x00, "Right PGA 0dB unmuted (R16)");
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x11), 0x0F, "LINE2L->LADC (R17)");
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x12), 0xF0, "LINE2R->RADC (R18)");

	zassert_equal(tlv320aic3104_emul_last_val(0, 0x19), 0x00, "MICBIAS off by default (R25)");
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x0C), 0x00,
		      "ADC HPF disabled by default (R12)");
}

ZTEST(tlv320_channel_mode, test_channel_mode_preserves_clock_family_bit)
{

	tlv320aic3104_emul_reset();
	tlv320aic3104_emul_set_val(0, 0x07, 0x80);

	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_FRONT_CENTER,
					    TLV320AIC3104_OUTPUT_HP));
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x07), 0x9E, "family bit survives mono write");

	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_HP));
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x07), 0x8A,
		      "family bit survives stereo write");
}

ZTEST(tlv320_channel_mode, test_capture_stop_powers_down_adc)
{
	tlv320aic3104_emul_reset();

	tlv320aic3104_emul_set_val(0, 0x13, 0x78);
	tlv320aic3104_emul_set_val(0, 0x16, 0x78);
	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_RX));
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x13), 0x78, "Left-ADC off (R19)");
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x16), 0x78, "Right-ADC off (R22)");
	zassert_equal(tlv320aic3104_emul_last_val(0, 0x0F), 0x80, "Left PGA muted (R15)");
}

ZTEST_SUITE(tlv320_channel_mode, NULL, NULL, NULL, NULL, NULL);
