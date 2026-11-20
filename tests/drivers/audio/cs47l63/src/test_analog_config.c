/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_analog_config.c
 * @brief Analog wiring taken from devicetree: the IN2 pair and the headphone load
 *
 * cs47l63_in (test_input_path.c) sets cirrus,in2-single-ended; cs47l63_diff leaves it out.
 * cs47l63_hp_low sets cirrus,hp-load-below-20-ohm; the nodes of the other suites leave it out.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define DIFF_CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_diff))
#define DIFF_EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_diff))
#define HP_CODEC   DEVICE_DT_GET(DT_NODELABEL(cs47l63_hp_low))
#define HP_EMUL    EMUL_DT_GET(DT_NODELABEL(cs47l63_hp_low))

#define START_WAIT_MS (CS47L63_START_TIMEOUT_MS + 100U)

/** DAC_IF_TEST_1 as the trim of OTP variant 8 writes it, HP1L_CFG = 010. */
#define DAC_IF_TEST_1_TRIMMED 0x700249B8U

/** The same with HP1L_CFG = 011 and every other bit as the trim left it. */
#define DAC_IF_TEST_1_LT20 0x700269B8U

static uint32_t in2_src(const struct emul *target, uint32_t addr)
{
	return (cs47l63_emul_get_reg(target, addr) & CS47L63_IN2_SRC_MASK) >> CS47L63_IN2_SRC_SHIFT;
}

static void diff_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(DIFF_EMUL);
	zassert_ok(cs47l63_test_configure(DIFF_CODEC, AUDIO_ROUTE_PLAYBACK_CAPTURE));
}

ZTEST_SUITE(cs47l63_diff, NULL, NULL, diff_before_each, NULL, NULL);

static ZTEST(cs47l63_diff, test_in2_without_the_property_is_differential)
{
	zassert_ok(audio_codec_start(DIFF_CODEC, AUDIO_DAI_DIR_RX));

	zassert_true(cs47l63_emul_write_count(DIFF_EMUL, CS47L63_IN2L_CONTROL1) > 0U,
		     "the premise: the start wrote the source of IN2L");
	zassert_true(cs47l63_emul_write_count(DIFF_EMUL, CS47L63_IN2R_CONTROL1) > 0U,
		     "and of IN2R");
	zassert_equal(in2_src(DIFF_EMUL, CS47L63_IN2L_CONTROL1), CS47L63_IN2_SRC_DIFFERENTIAL);
	zassert_equal(in2_src(DIFF_EMUL, CS47L63_IN2R_CONTROL1), CS47L63_IN2_SRC_DIFFERENTIAL);
}

static void hp_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(HP_EMUL);
}

ZTEST_SUITE(cs47l63_hp_low, NULL, NULL, hp_before_each, NULL, NULL);

static void arm_hp_write_failure(void *arg)
{
	ARG_UNUSED(arg);

	cs47l63_emul_fail_write_at(HP_EMUL, CS47L63_DAC_IF_TEST_1);
}

static ZTEST(cs47l63_hp_low, test_load_under_20_ohm_sets_only_hp1l_cfg_after_the_trim)
{
	uint32_t n;
	int amp_idx = -1;
	uint32_t val;

	zassert_ok(cs47l63_test_configure(HP_CODEC, AUDIO_ROUTE_PLAYBACK));

	zassert_equal(cs47l63_emul_get_reg(HP_EMUL, CS47L63_DAC_IF_TEST_1), DAC_IF_TEST_1_LT20,
		      "HP1L_CFG is 011 and the bits the trim set are untouched");

	n = cs47l63_emul_write_count(HP_EMUL, CS47L63_DAC_IF_TEST_1);
	zassert_equal(n, 2U, "the trim, then one update");
	zassert_true(cs47l63_emul_nth_write(HP_EMUL, CS47L63_DAC_IF_TEST_1, 0U, &val));
	zassert_equal(val, DAC_IF_TEST_1_TRIMMED, "the first write is the trim's");

	cs47l63_emul_set_reg(HP_EMUL, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(HP_EMUL, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS);
	audio_codec_start_output(HP_CODEC);
	(void)k_msleep((int32_t)START_WAIT_MS);
	audio_codec_stop_output(HP_CODEC);

	n = cs47l63_emul_write_count(HP_EMUL, CS47L63_OUTPUT_ENABLE_1);
	for (uint32_t i = 0U; i < n; i++) {
		zassert_true(cs47l63_emul_nth_write(HP_EMUL, CS47L63_OUTPUT_ENABLE_1, i, &val));
		if ((val & CS47L63_OUT1L_EN) != 0U) {
			amp_idx = cs47l63_emul_write_index(HP_EMUL, CS47L63_OUTPUT_ENABLE_1, i);
			break;
		}
	}

	zassert_true(amp_idx >= 0, "the premise: the start enabled the amplifier");
	zassert_true(cs47l63_emul_write_index(HP_EMUL, CS47L63_DAC_IF_TEST_1, 1U) >
			     cs47l63_emul_write_index(HP_EMUL, CS47L63_DAC_IF_TEST_1, 0U),
		     "the update comes after the trim, which rewrites the whole register");
	zassert_true(amp_idx > cs47l63_emul_write_index(HP_EMUL, CS47L63_DAC_IF_TEST_1, 1U),
		     "and before the amplifier is enabled");
}

static ZTEST(cs47l63_hp_low, test_failed_hp1l_cfg_update_fails_configure)
{
	cs47l63_emul_once_on_write(HP_EMUL, CS47L63_DAC_IF_TEST_1, arm_hp_write_failure, NULL);

	zassert_equal(-EIO, cs47l63_test_configure(HP_CODEC, AUDIO_ROUTE_PLAYBACK));

	zassert_equal(cs47l63_emul_fail_hits(HP_EMUL), 1U,
		      "the injection took effect: the update after the trim was refused");
	zassert_equal(cs47l63_emul_get_reg(HP_EMUL, CS47L63_OUTPUT_ENABLE_1) & CS47L63_OUT1L_EN, 0U,
		      "a failed configure leaves the amplifier disabled");
	zassert_equal(-EINVAL, audio_codec_start(HP_CODEC, AUDIO_DAI_DIR_TX),
		      "and the port unconfigured");
}
