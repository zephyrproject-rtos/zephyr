/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_asp2.c
 * @brief cirrus,asp = <2>: every serial-port access moves to ASP2
 *
 * ASP2 addresses are written out from DS1249F2 sections 4.7-4.8, not derived from the
 * ASP1 names, so a wrong driver offset cannot cancel out. ASP1 must stay untouched.
 */

#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_asp2))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_asp2))

#define WORD_BITS 16U

/* ASP2 registers, DS1249F2 register map. */
#define ASP2_ENABLES1      0x00006080U
#define ASP2_CONTROL1      0x00006084U
#define ASP2_CONTROL2      0x00006088U
#define ASP2_DATA_CONTROL1 0x000060B0U
#define ASP2_DATA_CONTROL5 0x000060C0U
#define ASP2TX1_INPUT1     0x00008300U
#define ASP2TX2_INPUT1     0x00008310U
#define GPIO5_CTRL1        0x00000C18U
#define GPIO6_CTRL1        0x00000C1CU
#define GPIO7_CTRL1        0x00000C20U
#define GPIO8_CTRL1        0x00000C24U

#define MIXER_SRC_ASP2RX1 0x030U
#define MIXER_SRC_ASP2RX2 0x031U

#define ASP_RX_EN_BOTH (CS47L63_ASP1_RX1_EN | CS47L63_ASP1_RX2_EN)
#define ASP_TX_EN_BOTH (CS47L63_ASP1_TX1_EN | CS47L63_ASP1_TX2_EN)

#define MIX_SLOT(src) (((uint32_t)CS47L63_OUT1LMIX_VOL_0DB << CS47L63_OUT1LMIX_VOL_SHIFT) | (src))

/** Both slot widths, the format code, and every master and inversion bit low. */
#define EXPECT_CONTROL2                                                                            \
	((WORD_BITS << CS47L63_ASP1_RX_WIDTH_SHIFT) | (WORD_BITS << CS47L63_ASP1_TX_WIDTH_SHIFT) | \
	 ((uint32_t)CS47L63_ASP1_FMT_I2S << CS47L63_ASP1_FMT_SHIFT))

static const uint32_t k_asp1_only[] = {
	CS47L63_ASP1_ENABLES1,      CS47L63_ASP1_CONTROL1,      CS47L63_ASP1_CONTROL2,
	CS47L63_ASP1_DATA_CONTROL1, CS47L63_ASP1_DATA_CONTROL5, CS47L63_ASP1TX1_INPUT1,
	CS47L63_ASP1TX2_INPUT1,     CS47L63_GPIO1_CTRL1,        CS47L63_GPIO2_CTRL1,
	CS47L63_GPIO3_CTRL1,        CS47L63_GPIO4_CTRL1,
};

static void assert_asp1_untouched(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(k_asp1_only); i++) {
		zassert_equal(cs47l63_emul_write_count(EMUL, k_asp1_only[i]), 0,
			      "ASP1 register 0x%05x written on an ASP2 board", k_asp1_only[i]);
		zassert_equal(cs47l63_emul_read_count(EMUL, k_asp1_only[i]), 0,
			      "ASP1 register 0x%05x read on an ASP2 board", k_asp1_only[i]);
	}
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));
}

ZTEST_SUITE(cs47l63_asp2, NULL, NULL, before_each, NULL, NULL);

ZTEST(cs47l63_asp2, test_configure_programs_the_asp2_port_block)
{
	zassert_equal((cs47l63_emul_get_reg(EMUL, ASP2_CONTROL1) & CS47L63_ASP1_RATE_MASK) >>
			      CS47L63_ASP1_RATE_SHIFT,
		      CS47L63_ASP1_RATE_SEL_SAMPLE_RATE1);
	zassert_true(cs47l63_emul_write_count(EMUL, ASP2_CONTROL2) >= 1);
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2_CONTROL2), EXPECT_CONTROL2);
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2_DATA_CONTROL1) & CS47L63_ASP1_TX_WL_MASK,
		      WORD_BITS);
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2_DATA_CONTROL5) & CS47L63_ASP1_RX_WL_MASK,
		      WORD_BITS);
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2_ENABLES1), ASP_RX_EN_BOTH,
		      "both receive slots of ASP2 carry the stereo stream");

	assert_asp1_untouched();
}

ZTEST(cs47l63_asp2, test_configure_muxes_gpio5_to_8)
{
	static const struct {
		uint32_t addr;
		uint32_t val;
	} k_pads[] = {
		{GPIO5_CTRL1, CS47L63_GP_CTRL1_ASP_PAD},
		{GPIO6_CTRL1, CS47L63_GP_CTRL1_ASP_PAD | CS47L63_GP_DIR_INPUT},
		{GPIO7_CTRL1, CS47L63_GP_CTRL1_ASP_PAD | CS47L63_GP_DIR_INPUT},
		{GPIO8_CTRL1, CS47L63_GP_CTRL1_ASP_PAD | CS47L63_GP_DIR_INPUT},
	};

	/* GPIO5 (DOUT) is an output; DIN, BCLK and FSYNC are inputs. */
	for (size_t i = 0; i < ARRAY_SIZE(k_pads); i++) {
		uint32_t val;

		zassert_true(cs47l63_emul_nth_write(EMUL, k_pads[i].addr, 0, &val),
			     "pad 0x%05x must be muxed to its ASP function", k_pads[i].addr);
		zassert_equal(val, k_pads[i].val, "pad 0x%05x got the wrong configuration",
			      k_pads[i].addr);
	}

	assert_asp1_untouched();
}

ZTEST(cs47l63_asp2, test_output_mixer_takes_the_asp2_receive_channels)
{
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(MIXER_SRC_ASP2RX1));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT2),
		      MIX_SLOT(MIXER_SRC_ASP2RX2));

	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_FRONT_LEFT, CS47L63_OUTPUT_HP));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(MIXER_SRC_ASP2RX1));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT2),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));

	zassert_ok(audio_codec_route_output(CODEC, AUDIO_CHANNEL_FRONT_RIGHT, CS47L63_OUTPUT_HP));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(MIXER_SRC_ASP2RX2));

	assert_asp1_untouched();
}

ZTEST(cs47l63_asp2, test_input_start_and_stop_use_the_asp2_transmit_side)
{
	uint32_t val;

	zassert_ok(audio_codec_start(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2L));
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2R));
	val = cs47l63_emul_get_reg(EMUL, ASP2_ENABLES1);
	zassert_equal(val & ASP_TX_EN_BOTH, ASP_TX_EN_BOTH, "the ASP2 transmit slots come up");
	zassert_equal(val & ASP_RX_EN_BOTH, ASP_RX_EN_BOTH, "and its receive slots survive it");

	zassert_ok(audio_codec_stop(CODEC, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2_ENABLES1), ASP_RX_EN_BOTH);
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2TX2_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));

	assert_asp1_untouched();
}

ZTEST(cs47l63_asp2, test_input_hpf_does_not_depend_on_the_port)
{
	static const uint32_t k_control1[] = {
		CS47L63_IN1L_CONTROL1,
		CS47L63_IN1R_CONTROL1,
		CS47L63_IN2L_CONTROL1,
		CS47L63_IN2R_CONTROL1,
	};

	for (size_t i = 0; i < ARRAY_SIZE(k_control1); i++) {
		zassert_not_equal(cs47l63_emul_get_reg(EMUL, k_control1[i]) & CS47L63_IN_HPF, 0,
				  "HPF enabled on 0x%05x on an ASP2 board too", k_control1[i]);
	}
}
