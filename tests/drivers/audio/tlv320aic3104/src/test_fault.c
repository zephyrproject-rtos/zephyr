/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>

#include "tlv320aic3104_fault.h"

#include "tlv320aic3104_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_fault))

#define R_HP_OUTPUT_DRIVER_CTRL  0x26
#define R_STICKY_INTERRUPT_FLAGS 0x60

#define HP_OUTPUT_DRIVER_CTRL_EXPECT 0x06

#define STICKY_INTERRUPT_FLAGS_HPLOUT_SHORT 0x80

static uint32_t s_cb_errors;
static int s_cb_calls;

static void fault_cb(const struct device *dev, uint32_t errors)
{
	zassert_equal(dev, CODEC, "callback must receive the codec device");
	s_cb_errors = errors;
	s_cb_calls++;
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	tlv320aic3104_emul_reset();
	s_cb_errors = 0;
	s_cb_calls = 0;

	struct audio_codec_cfg cfg = {
		.mclk_freq = 12288000,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.frame_clk_freq = 48000,
		.dai_cfg.i2s.word_size = 16,
	};
	zassert_ok(audio_codec_configure(CODEC, &cfg));
}

ZTEST_SUITE(tlv320_fault, NULL, NULL, before_each, NULL, NULL);

ZTEST(tlv320_fault, test_init_enables_short_circuit_protection)
{
	zassert_equal(tlv320aic3104_emul_last_val(0, R_HP_OUTPUT_DRIVER_CTRL),
		      HP_OUTPUT_DRIVER_CTRL_EXPECT, "R38 D2|D1 must be set by configure()");
}

ZTEST(tlv320_fault, test_register_callback_present_in_api)
{
	zassert_ok(audio_codec_register_error_callback(CODEC, fault_cb));
}

ZTEST(tlv320_fault, test_clear_errors_present_in_api)
{
	zassert_ok(audio_codec_clear_errors(CODEC));
}

ZTEST(tlv320_fault, test_short_circuit_via_start_output_reports_overcurrent)
{
	zassert_ok(audio_codec_register_error_callback(CODEC, fault_cb));

	tlv320aic3104_emul_set_val(0, R_STICKY_INTERRUPT_FLAGS,
				   STICKY_INTERRUPT_FLAGS_HPLOUT_SHORT);
	audio_codec_start_output(CODEC);

	zassert_equal(s_cb_calls, 1, "callback must fire exactly once for the raised fault");
	zassert_equal(s_cb_errors, AUDIO_CODEC_ERROR_OVERCURRENT,
		      "callback must report AUDIO_CODEC_ERROR_OVERCURRENT");
}

ZTEST(tlv320_fault, test_short_circuit_via_set_property_reports_overcurrent)
{
	audio_property_value_t val = {.vol = 0};

	zassert_ok(audio_codec_register_error_callback(CODEC, fault_cb));

	tlv320aic3104_emul_set_val(0, R_STICKY_INTERRUPT_FLAGS,
				   STICKY_INTERRUPT_FLAGS_HPLOUT_SHORT);
	zassert_ok(audio_codec_set_property(CODEC, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    val));

	zassert_equal(s_cb_calls, 1, "callback must fire exactly once for the raised fault");
	zassert_equal(s_cb_errors, AUDIO_CODEC_ERROR_OVERCURRENT,
		      "callback must report AUDIO_CODEC_ERROR_OVERCURRENT");
}

ZTEST(tlv320_fault, test_no_short_circuit_no_callback)
{
	uint32_t errors;

	zassert_ok(audio_codec_register_error_callback(CODEC, fault_cb));

	audio_codec_start_output(CODEC);

	zassert_equal(s_cb_calls, 0, "no fault raised, callback must not fire");
	zassert_ok(tlv320aic3104_fault_get_errors(CODEC, &errors));
	zassert_equal(errors, 0, "no fault raised, sticky state must stay clear");
}

ZTEST(tlv320_fault, test_sticky_flag_survives_status_read)
{
	uint32_t errors;

	tlv320aic3104_emul_set_val(0, R_STICKY_INTERRUPT_FLAGS,
				   STICKY_INTERRUPT_FLAGS_HPLOUT_SHORT);
	audio_codec_start_output(CODEC);

	zassert_ok(tlv320aic3104_fault_get_errors(CODEC, &errors));
	zassert_equal(errors, AUDIO_CODEC_ERROR_OVERCURRENT, "flag must be set after the fault");

	zassert_ok(tlv320aic3104_fault_get_errors(CODEC, &errors));
	zassert_equal(errors, AUDIO_CODEC_ERROR_OVERCURRENT,
		      "flag must still be set after a status read");

	tlv320aic3104_emul_set_val(0, R_STICKY_INTERRUPT_FLAGS, 0x00);
	audio_codec_start_output(CODEC);
	zassert_ok(tlv320aic3104_fault_get_errors(CODEC, &errors));
	zassert_equal(errors, AUDIO_CODEC_ERROR_OVERCURRENT,
		      "flag must still be set once the register clears on its own");
}

ZTEST(tlv320_fault, test_clear_errors_clears_the_sticky_flag)
{
	uint32_t errors;

	tlv320aic3104_emul_set_val(0, R_STICKY_INTERRUPT_FLAGS,
				   STICKY_INTERRUPT_FLAGS_HPLOUT_SHORT);
	audio_codec_start_output(CODEC);
	zassert_ok(tlv320aic3104_fault_get_errors(CODEC, &errors));
	zassert_equal(errors, AUDIO_CODEC_ERROR_OVERCURRENT, "flag must be set before clearing");

	zassert_ok(audio_codec_clear_errors(CODEC));

	zassert_ok(tlv320aic3104_fault_get_errors(CODEC, &errors));
	zassert_equal(errors, 0, "clear_errors() must be what clears the sticky flag");
}

ZTEST(tlv320_fault, test_get_errors_null_arg)
{
	zassert_equal(tlv320aic3104_fault_get_errors(CODEC, NULL), -EINVAL,
		      "NULL out_errors must return -EINVAL");
}
