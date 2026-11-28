/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * TLV320AIC3106 (ti,tlv320aic3106) against the TLV320AIC3104 register model.
 *
 * Register values follow the datasheets: SLAS509G (TLV320AIC3106) and
 * SLAS510G (TLV320AIC3104).
 *   R17/R18 MIC3L/R to left/right ADC (3106 Tables 10-25/10-26), MIC2/LINE2
 *           on the 3104 (Tables 10-23/10-24): D7-D4 left pin, D3-D0 right
 *           pin, 0000 = 0 dB, 1111 = not connected.
 *   R19/R22 LINE1L/LINE1R to left/right ADC (3106 Tables 10-27/10-30):
 *           D6-D3 level (1111 = not connected), D2 ADC channel power.
 *   R20/R23 LINE2L/LINE2R to left/right ADC (3106 Tables 10-28/10-31):
 *           D7 differential, D6-D3 level (1111 = not connected), D2 weak
 *           common-mode bias. Reserved, "do not write", on the 3104
 *           (Tables 10-26/10-29).
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/audio/codec.h>

#include "tlv320aic3104_priv.h"

#include "tlv320aic3104_emul.h"

#define AIC3104       DEVICE_DT_GET(DT_NODELABEL(tlv320aic3104_variant))
#define AIC3106       DEVICE_DT_GET(DT_NODELABEL(tlv320aic3106_default))
#define AIC3106_EVM   DEVICE_DT_GET(DT_NODELABEL(tlv320aic3106_line2_mic3))
#define AIC3106_LINE1 DEVICE_DT_GET(DT_NODELABEL(tlv320aic3106_line1))

#define R17_MIC3_TO_LADC   0x11U
#define R18_MIC3_TO_RADC   0x12U
#define R19_LINE1L_TO_LADC 0x13U
#define R20_LINE2L_TO_LADC 0x14U
#define R22_LINE1R_TO_RADC 0x16U
#define R23_LINE2R_TO_RADC 0x17U

#define MIC3_OFF        0xFFU
#define MIC3L_LEFT_0DB  0x0FU
#define MIC3R_RIGHT_0DB 0xF0U
#define MIC3_MONO_MIX   0x44U
#define LEVEL_OFF       0x78U
#define ADC_POWER       0x04U

static const struct audio_codec_cfg codec_cfg = {
	.mclk_freq = 12288000,
	.dai_type = AUDIO_DAI_TYPE_I2S,
	.dai_cfg.i2s.frame_clk_freq = 48000,
	.dai_cfg.i2s.word_size = 16,
};

static void configure(const struct device *dev)
{
	struct audio_codec_cfg cfg = codec_cfg;

	zassert_ok(audio_codec_configure(dev, &cfg));
}

static void assert_reg(uint8_t addr, uint8_t expected, const char *what)
{
	uint8_t val = tlv320aic3104_emul_last_val(0, addr);

	zassert_equal(val, expected, "%s: R%u = 0x%02x, expected 0x%02x", what, addr, val,
		      expected);
}

static void assert_input_regs_untouched(void)
{
	static const uint8_t regs[] = {R17_MIC3_TO_LADC,   R18_MIC3_TO_RADC,   R19_LINE1L_TO_LADC,
				       R20_LINE2L_TO_LADC, R22_LINE1R_TO_RADC, R23_LINE2R_TO_RADC};

	for (size_t i = 0; i < ARRAY_SIZE(regs); i++) {
		zassert_false(tlv320aic3104_emul_was_written(0, regs[i]), "R%u written", regs[i]);
	}
}

/* TLV320AIC3104: R20/R23 are reserved, so LINE2 to ADC is not an input. */

ZTEST(tlv320_aic3106, test_aic3104_rejects_aic3106_line2)
{
	tlv320aic3104_emul_reset();
	zassert_equal(
		audio_codec_route_input(AIC3104, AUDIO_CHANNEL_ALL, TLV320AIC3106_INPUT_LINE2),
		-ENOTSUP);
	assert_input_regs_untouched();
}

ZTEST(tlv320_aic3106, test_aic3104_never_writes_reserved_registers)
{
	tlv320aic3104_emul_reset();
	configure(AIC3104);
	zassert_ok(audio_codec_route_input(AIC3104, AUDIO_CHANNEL_ALL, TLV320AIC3104_INPUT_LINE1));
	zassert_ok(audio_codec_route_input(AIC3104, AUDIO_CHANNEL_FRONT_LEFT,
					   TLV320AIC3104_INPUT_LINE2));
	zassert_ok(audio_codec_route_output(AIC3104, AUDIO_CHANNEL_FRONT_CENTER,
					    TLV320AIC3104_OUTPUT_HP));
	zassert_ok(audio_codec_route_output(AIC3104, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_LOP));
	zassert_ok(audio_codec_start(AIC3104, AUDIO_DAI_DIR_TXRX));
	zassert_ok(audio_codec_set_property(AIC3104, AUDIO_PROPERTY_OUTPUT_VOLUME,
					    AUDIO_CHANNEL_ALL,
					    (audio_property_value_t){.vol = -6}));
	zassert_ok(audio_codec_set_property(AIC3104, AUDIO_PROPERTY_INPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    (audio_property_value_t){.vol = 60}));
	zassert_ok(audio_codec_stop(AIC3104, AUDIO_DAI_DIR_TXRX));
	zassert_ok(audio_codec_route_output(AIC3104, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_HP));

	zassert_equal(tlv320aic3104_emul_reserved_writes(), 0U, "reserved register written");
	zassert_false(tlv320aic3104_emul_was_written(0, R20_LINE2L_TO_LADC), "R20 written");
	zassert_false(tlv320aic3104_emul_was_written(0, R23_LINE2R_TO_RADC), "R23 written");
}

/*
 * Same configure() on both parts: identical page 0 writes, except that the
 * TLV320AIC3106 also disconnects LINE2 from the ADCs (R20/R23 = 0x78).
 */
ZTEST(tlv320_aic3106, test_configure_matches_aic3104_except_line2)
{
	uint8_t regs_3104[128];
	bool written_3104[128];

	/* Same output state on both nodes; other tests may have left LOP selected. */
	zassert_ok(audio_codec_route_output(AIC3104, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_HP));
	zassert_ok(audio_codec_route_output(AIC3106, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_HP));

	tlv320aic3104_emul_reset();
	configure(AIC3104);
	for (uint8_t r = 1; r < 128; r++) {
		regs_3104[r] = tlv320aic3104_emul_last_val(0, r);
		written_3104[r] = tlv320aic3104_emul_was_written(0, r);
	}

	tlv320aic3104_emul_reset();
	configure(AIC3106);

	for (uint8_t r = 1; r < 128; r++) {
		if (r == R20_LINE2L_TO_LADC || r == R23_LINE2R_TO_RADC) {
			continue;
		}
		zassert_equal(tlv320aic3104_emul_was_written(0, r), written_3104[r],
			      "R%u written on one part only", r);
		zassert_equal(tlv320aic3104_emul_last_val(0, r), regs_3104[r],
			      "R%u differs between the parts", r);
	}
	assert_reg(R20_LINE2L_TO_LADC, LEVEL_OFF, "LINE2L off the left ADC");
	assert_reg(R23_LINE2R_TO_RADC, LEVEL_OFF, "LINE2R off the right ADC");
	assert_reg(R17_MIC3_TO_LADC, MIC3L_LEFT_0DB, "default MIC3L to left ADC");
	assert_reg(R18_MIC3_TO_RADC, MIC3R_RIGHT_0DB, "default MIC3R to right ADC");
}


ZTEST(tlv320_aic3106, test_route_line2_all)
{
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_route_input(AIC3106, AUDIO_CHANNEL_ALL, TLV320AIC3106_INPUT_LINE2));

	assert_reg(R20_LINE2L_TO_LADC, 0x00, "LINE2L to left ADC 0 dB");
	assert_reg(R23_LINE2R_TO_RADC, 0x00, "LINE2R to right ADC 0 dB");
	assert_reg(R17_MIC3_TO_LADC, MIC3_OFF, "MIC3 off the left ADC");
	assert_reg(R18_MIC3_TO_RADC, MIC3_OFF, "MIC3 off the right ADC");
	assert_reg(R19_LINE1L_TO_LADC, LEVEL_OFF, "LINE1L off");
	assert_reg(R22_LINE1R_TO_RADC, LEVEL_OFF, "LINE1R off");
}

ZTEST(tlv320_aic3106, test_route_line2_left_leaves_right_alone)
{
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_route_input(AIC3106, AUDIO_CHANNEL_FRONT_LEFT,
					   TLV320AIC3106_INPUT_LINE2));

	assert_reg(R20_LINE2L_TO_LADC, 0x00, "LINE2L to left ADC 0 dB");
	assert_reg(R17_MIC3_TO_LADC, MIC3_OFF, "MIC3 off the left ADC");
	assert_reg(R19_LINE1L_TO_LADC, LEVEL_OFF, "LINE1L off");
	zassert_false(tlv320aic3104_emul_was_written(0, R18_MIC3_TO_RADC), "R18 written");
	zassert_false(tlv320aic3104_emul_was_written(0, R22_LINE1R_TO_RADC), "R22 written");
	zassert_false(tlv320aic3104_emul_was_written(0, R23_LINE2R_TO_RADC), "R23 written");
}

ZTEST(tlv320_aic3106, test_line2_level_keeps_differential_and_bias_bits)
{
	tlv320aic3104_emul_reset();
		tlv320aic3104_emul_set_val(0, R20_LINE2L_TO_LADC, 0x84);
	tlv320aic3104_emul_set_val(0, R23_LINE2R_TO_RADC, 0x80);

	zassert_ok(audio_codec_route_input(AIC3106, AUDIO_CHANNEL_ALL, TLV320AIC3106_INPUT_LINE2));
	assert_reg(R20_LINE2L_TO_LADC, 0x84, "LINE2L connected, D7/D2 kept");
	assert_reg(R23_LINE2R_TO_RADC, 0x80, "LINE2R connected, D7 kept");

	zassert_ok(audio_codec_route_input(AIC3106, AUDIO_CHANNEL_ALL, TLV320AIC3106_INPUT_MIC3));
	assert_reg(R20_LINE2L_TO_LADC, 0xFC, "LINE2L off, D7/D2 kept");
	assert_reg(R23_LINE2R_TO_RADC, 0xF8, "LINE2R off, D7 kept");
	assert_reg(R17_MIC3_TO_LADC, MIC3L_LEFT_0DB, "MIC3L to left ADC");
	assert_reg(R18_MIC3_TO_RADC, MIC3R_RIGHT_0DB, "MIC3R to right ADC");
}

ZTEST(tlv320_aic3106, test_route_mic3_right_for_jack_microphone)
{
	tlv320aic3104_emul_reset();
	tlv320aic3104_emul_set_val(0, R23_LINE2R_TO_RADC, 0x00);
	zassert_ok(audio_codec_route_input(AIC3106, AUDIO_CHANNEL_FRONT_RIGHT,
					   TLV320AIC3106_INPUT_MIC3));

	assert_reg(R18_MIC3_TO_RADC, MIC3R_RIGHT_0DB, "MIC3R to right ADC 0 dB");
	assert_reg(R23_LINE2R_TO_RADC, LEVEL_OFF, "LINE2R off the right ADC");
	assert_reg(R22_LINE1R_TO_RADC, LEVEL_OFF, "LINE1R off");
	zassert_false(tlv320aic3104_emul_was_written(0, R17_MIC3_TO_LADC), "R17 written");
}

ZTEST(tlv320_aic3106, test_route_line1_disconnects_line2)
{
	tlv320aic3104_emul_reset();
	zassert_ok(audio_codec_route_input(AIC3106, AUDIO_CHANNEL_ALL, TLV320AIC3106_INPUT_LINE2));
	zassert_ok(audio_codec_route_input(AIC3106, AUDIO_CHANNEL_ALL, TLV320AIC3106_INPUT_LINE1));

	assert_reg(R19_LINE1L_TO_LADC, 0x00, "LINE1L to left ADC 0 dB");
	assert_reg(R22_LINE1R_TO_RADC, 0x00, "LINE1R to right ADC 0 dB");
	assert_reg(R20_LINE2L_TO_LADC, LEVEL_OFF, "LINE2L off");
	assert_reg(R23_LINE2R_TO_RADC, LEVEL_OFF, "LINE2R off");
	assert_reg(R17_MIC3_TO_LADC, MIC3_OFF, "MIC3 off the left ADC");
	assert_reg(R18_MIC3_TO_RADC, MIC3_OFF, "MIC3 off the right ADC");
}

ZTEST(tlv320_aic3106, test_unsupported_input_writes_nothing)
{
	tlv320aic3104_emul_reset();
	zassert_equal(
		audio_codec_route_input(AIC3106, AUDIO_CHANNEL_ALL, TLV320AIC3106_INPUT_LINE2 + 1),
		-ENOTSUP);
	assert_input_regs_untouched();
}

ZTEST(tlv320_aic3106, test_rx_start_keeps_line2)
{
	tlv320aic3104_emul_reset();
	configure(AIC3106);
	zassert_ok(audio_codec_route_input(AIC3106, AUDIO_CHANNEL_ALL, TLV320AIC3106_INPUT_LINE2));
	zassert_ok(audio_codec_start(AIC3106, AUDIO_DAI_DIR_RX));

	assert_reg(R20_LINE2L_TO_LADC, 0x00, "LINE2L stays on the left ADC");
	assert_reg(R23_LINE2R_TO_RADC, 0x00, "LINE2R stays on the right ADC");
	assert_reg(R17_MIC3_TO_LADC, MIC3_OFF, "MIC3 stays off the left ADC");
	assert_reg(R18_MIC3_TO_RADC, MIC3_OFF, "MIC3 stays off the right ADC");
	assert_reg(R19_LINE1L_TO_LADC, LEVEL_OFF | ADC_POWER, "left ADC on, LINE1L off");
	assert_reg(R22_LINE1R_TO_RADC, LEVEL_OFF | ADC_POWER, "right ADC on, LINE1R off");
}

ZTEST(tlv320_aic3106, test_mono_has_no_line2_mix)
{
	tlv320aic3104_emul_reset();
	configure(AIC3106);
	zassert_ok(audio_codec_route_input(AIC3106, AUDIO_CHANNEL_ALL, TLV320AIC3106_INPUT_LINE2));
	zassert_ok(audio_codec_route_output(AIC3106, AUDIO_CHANNEL_FRONT_CENTER,
					    TLV320AIC3104_OUTPUT_HP));
	zassert_ok(audio_codec_start(AIC3106, AUDIO_DAI_DIR_RX));

	assert_reg(R17_MIC3_TO_LADC, MIC3_OFF, "mono: MIC3 off the left ADC");
	assert_reg(R18_MIC3_TO_RADC, MIC3_OFF, "mono: MIC3 off the right ADC");
	assert_reg(R20_LINE2L_TO_LADC, 0x00, "mono: LINE2L stays on the left ADC");
	assert_reg(R23_LINE2R_TO_RADC, 0x00, "mono: LINE2R stays on the right ADC");

	/* Leave the shared node in stereo for the other tests. */
	zassert_ok(audio_codec_route_output(AIC3106, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_HP));
}

ZTEST(tlv320_aic3106, test_mono_mixes_mic3_like_aic3104_line2)
{
	tlv320aic3104_emul_reset();
	configure(AIC3106);
	zassert_ok(audio_codec_route_output(AIC3106, AUDIO_CHANNEL_FRONT_CENTER,
					    TLV320AIC3104_OUTPUT_HP));

	assert_reg(R17_MIC3_TO_LADC, MIC3_MONO_MIX, "MIC3L and MIC3R at -6 dB to the left ADC");
	assert_reg(R18_MIC3_TO_RADC, MIC3_MONO_MIX, "MIC3L and MIC3R at -6 dB to the right ADC");

	zassert_ok(audio_codec_route_output(AIC3106, AUDIO_CHANNEL_ALL, TLV320AIC3104_OUTPUT_HP));
}


ZTEST(tlv320_aic3106, test_dt_default_line2_left_mic3_right)
{
	tlv320aic3104_emul_reset();
	configure(AIC3106_EVM);

	assert_reg(R20_LINE2L_TO_LADC, 0x00, "LINE2L to left ADC");
	assert_reg(R17_MIC3_TO_LADC, MIC3_OFF, "MIC3 off the left ADC");
	assert_reg(R19_LINE1L_TO_LADC, LEVEL_OFF, "LINE1L off");
	assert_reg(R18_MIC3_TO_RADC, MIC3R_RIGHT_0DB, "MIC3R to right ADC");
	assert_reg(R23_LINE2R_TO_RADC, LEVEL_OFF, "LINE2R off");
	assert_reg(R22_LINE1R_TO_RADC, LEVEL_OFF, "LINE1R off");

	zassert_ok(audio_codec_start(AIC3106_EVM, AUDIO_DAI_DIR_RX));
	assert_reg(R20_LINE2L_TO_LADC, 0x00, "start keeps LINE2L");
	assert_reg(R17_MIC3_TO_LADC, MIC3_OFF, "start keeps MIC3 off the left ADC");
	assert_reg(R18_MIC3_TO_RADC, MIC3R_RIGHT_0DB, "start keeps MIC3R");
	assert_reg(R19_LINE1L_TO_LADC, LEVEL_OFF | ADC_POWER, "left ADC powered");
	assert_reg(R22_LINE1R_TO_RADC, LEVEL_OFF | ADC_POWER, "right ADC powered");
}

ZTEST(tlv320_aic3106, test_configure_restores_dt_default)
{
	tlv320aic3104_emul_reset();
	configure(AIC3106_EVM);
	zassert_ok(
		audio_codec_route_input(AIC3106_EVM, AUDIO_CHANNEL_ALL, TLV320AIC3106_INPUT_LINE1));
	configure(AIC3106_EVM);

	assert_reg(R20_LINE2L_TO_LADC, 0x00, "LINE2L back on the left ADC");
	assert_reg(R18_MIC3_TO_RADC, MIC3R_RIGHT_0DB, "MIC3R back on the right ADC");
	assert_reg(R19_LINE1L_TO_LADC, LEVEL_OFF, "LINE1L off again");
	assert_reg(R22_LINE1R_TO_RADC, LEVEL_OFF, "LINE1R off again");
}

ZTEST(tlv320_aic3106, test_dt_default_line1)
{
	tlv320aic3104_emul_reset();
	configure(AIC3106_LINE1);

	assert_reg(R19_LINE1L_TO_LADC, 0x00, "LINE1L to left ADC");
	assert_reg(R22_LINE1R_TO_RADC, 0x00, "LINE1R to right ADC");
	assert_reg(R17_MIC3_TO_LADC, MIC3_OFF, "MIC3 off the left ADC");
	assert_reg(R18_MIC3_TO_RADC, MIC3_OFF, "MIC3 off the right ADC");
	assert_reg(R20_LINE2L_TO_LADC, LEVEL_OFF, "LINE2L off");
	assert_reg(R23_LINE2R_TO_RADC, LEVEL_OFF, "LINE2R off");

	zassert_ok(audio_codec_start(AIC3106_LINE1, AUDIO_DAI_DIR_RX));
	assert_reg(R19_LINE1L_TO_LADC, ADC_POWER, "LINE1L kept, left ADC powered");
	assert_reg(R22_LINE1R_TO_RADC, ADC_POWER, "LINE1R kept, right ADC powered");
}

ZTEST_SUITE(tlv320_aic3106, NULL, NULL, NULL, NULL, NULL);
