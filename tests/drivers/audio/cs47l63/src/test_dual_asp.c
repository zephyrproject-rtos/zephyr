/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_dual_asp.c
 * @brief Both serial ports of one part, each through its own audio_codec device
 *
 * The first configure brings the part up; part-wide resources stay up while either port
 * uses them. Addresses are written out from DS1249F2, as in test_asp2.c.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/audio/cs47l63.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define PARENT DEVICE_DT_GET(DT_NODELABEL(cs47l63_dual))
#define CHILD  DEVICE_DT_GET(DT_NODELABEL(cs47l63_dual_asp2))
#define EMUL   EMUL_DT_GET(DT_NODELABEL(cs47l63_dual))

#define SWAP_PARENT DEVICE_DT_GET(DT_NODELABEL(cs47l63_swap))
#define SWAP_CHILD  DEVICE_DT_GET(DT_NODELABEL(cs47l63_swap_asp1))
#define SWAP_EMUL   EMUL_DT_GET(DT_NODELABEL(cs47l63_swap))

#define MCLK_HZ 6144000U
#define RATE_HZ 48000U

/** Long enough for the deferred start check to have run. */
#define START_WAIT_MS (CS47L63_FLL_LOCK_SETTLE_MS + 100)

/* DS1249F2 register map. */
#define OUT1L_INPUT3   0x00008108U
#define OUT1L_INPUT4   0x0000810CU
#define ASP2_ENABLES1  0x00006080U
#define ASP2_CONTROL1  0x00006084U
#define ASP2_CONTROL2  0x00006088U
#define ASP2TX1_INPUT1 0x00008300U
#define ASP2TX2_INPUT1 0x00008310U

#define MIXER_SRC_ASP1RX1 0x020U
#define MIXER_SRC_ASP1RX2 0x021U
#define MIXER_SRC_ASP2RX1 0x030U
#define MIXER_SRC_ASP2RX2 0x031U

#define IN1_EN_BOTH    (CS47L63_IN1L_EN | CS47L63_IN1R_EN)
#define IN2_EN_BOTH    (CS47L63_IN2L_EN | CS47L63_IN2R_EN)
#define IN_EN_ALL      (IN1_EN_BOTH | IN2_EN_BOTH)
#define ASP_TX_EN_BOTH (CS47L63_ASP1_TX1_EN | CS47L63_ASP1_TX2_EN)

#define MIX_SLOT(src) (((uint32_t)CS47L63_OUT1LMIX_VOL_0DB << CS47L63_OUT1LMIX_VOL_SHIFT) | (src))

static const struct device *const k_devs[] = {PARENT, CHILD, SWAP_PARENT, SWAP_CHILD};

static int s_cb_calls;
static uint32_t s_parent_errors;
static uint32_t s_child_errors;

static void fault_cb(const struct device *dev, uint32_t errors)
{
	s_cb_calls++;
	if (dev == PARENT) {
		s_parent_errors = errors;
	} else if (dev == CHILD) {
		s_child_errors = errors;
	} else {
		zassert_unreachable("the callback reports a device of this part");
	}
}

static int configure_at(const struct device *dev, audio_route_t route, uint32_t mclk_hz,
			uint32_t rate_hz)
{
	struct audio_codec_cfg cfg = {
		.mclk_freq = mclk_hz,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_route = route,
		.dai_cfg.i2s = {
			.word_size = 16,
			.channels = 2,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET,
			.frame_clk_freq = rate_hz,
		},
	};

	return audio_codec_configure(dev, &cfg);
}

static int configure(const struct device *dev, audio_route_t route)
{
	return configure_at(dev, route, MCLK_HZ, RATE_HZ);
}

static void configure_both(audio_route_t route)
{
	zassert_ok(configure(PARENT, route));
	zassert_ok(configure(CHILD, route));
}

static bool written_since(const struct emul *target, uint32_t mark, uint32_t addr)
{
	struct cs47l63_emul_xfer xfer;

	for (uint32_t i = mark; cs47l63_emul_xfer_get(target, i, &xfer); i++) {
		if (xfer.write && xfer.addr == addr) {
			return true;
		}
	}

	return false;
}

/** No reset, boot poll, ID read or clock write since mark. */
static void assert_no_bringup_since(const struct emul *target, uint32_t mark)
{
	static const uint32_t k_clock[] = {
		CS47L63_FLL1_CONTROL1, CS47L63_FLL1_CONTROL2, CS47L63_FLL1_CONTROL3,
		CS47L63_FLL1_CONTROL4, CS47L63_SYSTEM_CLOCK1, CS47L63_SAMPLE_RATE1,
	};
	struct cs47l63_emul_xfer xfer;

	for (uint32_t i = mark; cs47l63_emul_xfer_get(target, i, &xfer); i++) {
		zassert_false(xfer.addr == CS47L63_IRQ1_EINT_2 || xfer.addr == CS47L63_DEVID,
			      "the part was reset under a configured port");
		for (size_t j = 0; j < ARRAY_SIZE(k_clock); j++) {
			zassert_false(xfer.write && xfer.addr == k_clock[j],
				      "clock register 0x%05x written under a configured port",
				      k_clock[j]);
		}
	}
}

static void assert_inputs_silent(const struct emul *target, uint32_t first)
{
	zassert_equal(cs47l63_emul_get_reg(target, first), MIX_SLOT(CS47L63_MIXER_SRC_NONE),
		      "mixer input 0x%05x still feeds the output", first);
	zassert_equal(cs47l63_emul_get_reg(target, first + 4U), MIX_SLOT(CS47L63_MIXER_SRC_NONE),
		      "mixer input 0x%05x still feeds the output", first + 4U);
}

static bool amp_on(const struct emul *target)
{
	return (cs47l63_emul_get_reg(target, CS47L63_OUTPUT_ENABLE_1) & CS47L63_OUT1L_EN) != 0;
}

/** FLL1 locked and the amplifier reporting enabled, so a start check completes. */
static void clocks_up(const struct emul *target)
{
	cs47l63_emul_set_reg(target, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(target, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS);
}

static struct cs47l63_chip *chip_of(const struct device *dev)
{
	return ((const struct cs47l63_config *)dev->config)->chip;
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
	cs47l63_emul_reset(SWAP_EMUL);

	chip_of(PARENT)->configured = 0U;
	chip_of(SWAP_PARENT)->configured = 0U;

	s_cb_calls = 0;
	s_parent_errors = 0U;
	s_child_errors = 0U;
}

static void after_each(void *fixture)
{
	ARG_UNUSED(fixture);

	for (size_t i = 0; i < ARRAY_SIZE(k_devs); i++) {
		audio_codec_stop_output(k_devs[i]);
		(void)audio_codec_stop(k_devs[i], AUDIO_DAI_DIR_RX);
		(void)audio_codec_register_error_callback(k_devs[i], NULL);
		(void)audio_codec_clear_errors(k_devs[i]);
	}
}

ZTEST_SUITE(cs47l63_dual, NULL, NULL, before_each, after_each, NULL);

ZTEST(cs47l63_dual, test_both_ports_configure)
{
	uint32_t mark;

	zassert_ok(configure(PARENT, AUDIO_ROUTE_PLAYBACK_CAPTURE));
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_ok(configure(CHILD, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_true(written_since(EMUL, mark, ASP2_CONTROL2),
		     "the premise: the child's port was configured");
	assert_no_bringup_since(EMUL, mark);
	zassert_equal((cs47l63_emul_get_reg(EMUL, ASP2_CONTROL1) & CS47L63_ASP1_RATE_MASK) >>
			      CS47L63_ASP1_RATE_SHIFT,
		      CS47L63_ASP1_RATE_SEL_SAMPLE_RATE1,
		      "the child's port runs from the rate slot the parent set");
}

ZTEST(cs47l63_dual, test_second_port_other_mclk_rejected)
{
	uint32_t mark;

	zassert_ok(configure(PARENT, AUDIO_ROUTE_PLAYBACK));
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_equal(-ENOTSUP, configure_at(CHILD, AUDIO_ROUTE_PLAYBACK, 2 * MCLK_HZ, RATE_HZ));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark, "a refused configure touches nothing");

	zassert_ok(configure_at(PARENT, AUDIO_ROUTE_PLAYBACK, 2 * MCLK_HZ, RATE_HZ),
		   "the premise: the part itself runs at that MCLK");
}

ZTEST(cs47l63_dual, test_second_port_other_rate_rejected)
{
	uint32_t mark;

	zassert_ok(configure(PARENT, AUDIO_ROUTE_PLAYBACK));
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_equal(-ENOTSUP, configure_at(CHILD, AUDIO_ROUTE_PLAYBACK, MCLK_HZ, RATE_HZ / 2));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark, "a refused configure touches nothing");

	zassert_ok(configure_at(PARENT, AUDIO_ROUTE_PLAYBACK, MCLK_HZ, RATE_HZ / 2),
		   "the premise: the part itself runs at that rate");
}

ZTEST(cs47l63_dual, test_child_first_then_parent)
{
	uint32_t mark;

	zassert_ok(configure(CHILD, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_2), 1,
		      "the child alone brings the part up");
	zassert_true(cs47l63_emul_write_count(EMUL, CS47L63_SAMPLE_RATE1) >= 1);
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT3), MIX_SLOT(MIXER_SRC_ASP2RX1));
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT4), MIX_SLOT(MIXER_SRC_ASP2RX2));
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_INPUT1), 0,
		      "the parent's mixer inputs are not the child's");
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_ok(configure(PARENT, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_true(written_since(EMUL, mark, CS47L63_ASP1_CONTROL2),
		     "the premise: the parent's port was configured");
	assert_no_bringup_since(EMUL, mark);
	zassert_false(written_since(EMUL, mark, OUT1L_INPUT3), "the child's route stands");
}

ZTEST(cs47l63_dual, test_parent_reconfigure_keeps_child)
{
	uint32_t mark;

	configure_both(AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	zassert_true(amp_on(EMUL));
	assert_inputs_silent(EMUL, CS47L63_OUT1L_INPUT1);
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_ok(configure(PARENT, AUDIO_ROUTE_PLAYBACK_CAPTURE));
	assert_inputs_silent(EMUL, CS47L63_OUT1L_INPUT1);

	zassert_true(written_since(EMUL, mark, CS47L63_ASP1_ENABLES1),
		     "the premise: the parent's port was configured again");
	assert_no_bringup_since(EMUL, mark);
	zassert_false(written_since(EMUL, mark, CS47L63_OUTPUT_ENABLE_1));
	zassert_false(written_since(EMUL, mark, OUT1L_INPUT3));
	zassert_false(written_since(EMUL, mark, OUT1L_INPUT4));
	zassert_true(amp_on(EMUL), "the child keeps playing");

	zassert_ok(audio_codec_stop(CHILD, AUDIO_DAI_DIR_TX));
	zassert_false(amp_on(EMUL), "the child's stop is still the last one");
}

ZTEST(cs47l63_dual, test_only_the_started_port_feeds_the_output)
{
	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(MIXER_SRC_ASP1RX1), "the premise: the parent's configure routed it");

	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));

	assert_inputs_silent(EMUL, CS47L63_OUT1L_INPUT1);
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT3), MIX_SLOT(MIXER_SRC_ASP2RX1));
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT4), MIX_SLOT(MIXER_SRC_ASP2RX2));

	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(MIXER_SRC_ASP1RX1), "the parent's own start restores its route");
}

ZTEST(cs47l63_dual, test_a_port_stopped_last_does_not_feed_the_next_start)
{
	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_stop(PARENT, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_stop(CHILD, AUDIO_DAI_DIR_TX));

	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));

	assert_inputs_silent(EMUL, OUT1L_INPUT3);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(MIXER_SRC_ASP1RX1));
}

ZTEST(cs47l63_dual, test_a_port_whose_configure_failed_does_not_feed_the_output)
{
	uint32_t mark;

	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	mark = cs47l63_emul_xfer_count(EMUL);
	cs47l63_emul_fail_at(EMUL, CS47L63_ASP1_CONTROL2);

	zassert_equal(-EIO, configure(PARENT, AUDIO_ROUTE_PLAYBACK));
	zassert_false(written_since(EMUL, mark, CS47L63_ASP1_ENABLES1),
		      "the injection took effect: the parent's port was never configured");

	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	assert_inputs_silent(EMUL, CS47L63_OUT1L_INPUT1);
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT3), MIX_SLOT(MIXER_SRC_ASP2RX1));
}

ZTEST(cs47l63_dual, test_a_failed_start_under_a_live_amplifier_leaves_no_route)
{
	uint32_t mark;

	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	mark = cs47l63_emul_xfer_count(EMUL);
	cs47l63_emul_fail_at(EMUL, OUT1L_INPUT4);

	zassert_equal(-EIO, audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	zassert_true(written_since(EMUL, mark, OUT1L_INPUT3),
		     "the premise: the child's route was half written");
	zassert_false(written_since(EMUL, mark, OUT1L_INPUT4),
		      "the injection took effect: the second input was never written");

	zassert_true(amp_on(EMUL), "the parent still plays");
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT3), MIX_SLOT(CS47L63_MIXER_SRC_NONE),
		      "a port that failed to start feeds nothing");
}

ZTEST(cs47l63_dual, test_child_output_routes_inputs_3_4)
{
	uint32_t mark;

	configure_both(AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_ok(audio_codec_route_output(CHILD, AUDIO_CHANNEL_FRONT_LEFT, CS47L63_OUTPUT_HP));
	zassert_false(written_since(EMUL, mark, OUT1L_INPUT3),
		      "a stopped port's route is cached, not played on the parent's amplifier");
	zassert_false(written_since(EMUL, mark, OUT1L_INPUT4));

	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT3), MIX_SLOT(MIXER_SRC_ASP2RX1));
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT4), MIX_SLOT(CS47L63_MIXER_SRC_NONE),
		      "the child's start applies the cached route");

	zassert_ok(audio_codec_route_input(CHILD, AUDIO_CHANNEL_ALL, CS47L63_INPUT_PDM));
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2TX1_INPUT1), MIX_SLOT(CS47L63_MIXER_SRC_IN1L));
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2TX2_INPUT1), MIX_SLOT(CS47L63_MIXER_SRC_IN1R));

	zassert_false(written_since(EMUL, mark, CS47L63_OUT1L_INPUT1));
	zassert_false(written_since(EMUL, mark, CS47L63_OUT1L_INPUT2));
	zassert_false(written_since(EMUL, mark, CS47L63_ASP1TX1_INPUT1));
	zassert_false(written_since(EMUL, mark, CS47L63_ASP1TX2_INPUT1));
}

ZTEST(cs47l63_dual, test_amp_stays_on_until_last_output_stops)
{
	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));

	zassert_ok(audio_codec_stop(PARENT, AUDIO_DAI_DIR_TX));
	zassert_true(amp_on(EMUL), "the child still plays");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE), "the stopped port no longer feeds it");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT2),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE));
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT3), MIX_SLOT(MIXER_SRC_ASP2RX1));

	zassert_ok(audio_codec_stop(CHILD, AUDIO_DAI_DIR_TX));
	zassert_false(amp_on(EMUL), "the last stop takes the amplifier down");
}

ZTEST(cs47l63_dual, test_fault_reaches_child_callback)
{
	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_register_error_callback(CHILD, fault_cb));

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	zassert_ok(cs47l63_fault_check(PARENT));

	zassert_equal(s_cb_calls, 1);
	zassert_equal(s_child_errors, AUDIO_CODEC_ERROR_OVERCURRENT);
}

ZTEST(cs47l63_dual, test_fault_reaches_every_callback)
{
	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_register_error_callback(PARENT, fault_cb));
	zassert_ok(audio_codec_register_error_callback(CHILD, fault_cb));

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	zassert_ok(cs47l63_fault_check(PARENT));

	zassert_equal(s_cb_calls, 2);
	zassert_equal(s_parent_errors, AUDIO_CODEC_ERROR_OVERCURRENT);
	zassert_equal(s_child_errors, s_parent_errors, "both see the same flags");
}

ZTEST(cs47l63_dual, test_child_clear_errors_clears_the_chip_latch)
{
	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_register_error_callback(CHILD, fault_cb));

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	zassert_ok(cs47l63_fault_check(PARENT));
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	zassert_ok(cs47l63_fault_check(PARENT));
	zassert_equal(s_cb_calls, 1, "the premise: a latched fault is reported once");

	zassert_ok(audio_codec_clear_errors(CHILD));
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_OUT1L_SC_EINT1);
	zassert_ok(cs47l63_fault_check(PARENT));
	zassert_equal(s_cb_calls, 2, "the child's clear reached the part's latch");
}

ZTEST(cs47l63_dual, test_child_properties_not_supported)
{
	static const audio_property_t k_props[] = {
		AUDIO_PROPERTY_OUTPUT_VOLUME,
		AUDIO_PROPERTY_OUTPUT_MUTE,
		AUDIO_PROPERTY_INPUT_VOLUME,
		AUDIO_PROPERTY_INPUT_MUTE,
	};
	audio_property_value_t val = {0};
	uint32_t mark;

	configure_both(AUDIO_ROUTE_PLAYBACK_CAPTURE);
	mark = cs47l63_emul_xfer_count(EMUL);

	for (size_t i = 0; i < ARRAY_SIZE(k_props); i++) {
		zassert_equal(-ENOTSUP,
			      audio_codec_set_property(CHILD, k_props[i], AUDIO_CHANNEL_ALL, val),
			      "property %d is the parent's", k_props[i]);
	}
	zassert_equal(-ENOTSUP, audio_codec_apply_properties(CHILD));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark);
}

ZTEST(cs47l63_dual, test_swapped_ports)
{
	uint32_t mark;

	zassert_ok(configure(SWAP_PARENT, AUDIO_ROUTE_PLAYBACK));
	zassert_equal(cs47l63_emul_get_reg(SWAP_EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_SLOT(MIXER_SRC_ASP2RX1), "the parent on ASP2 feeds inputs 1-2");
	mark = cs47l63_emul_xfer_count(SWAP_EMUL);

	zassert_ok(configure(SWAP_CHILD, AUDIO_ROUTE_PLAYBACK));
	zassert_true(written_since(SWAP_EMUL, mark, CS47L63_ASP1_CONTROL2),
		     "the premise: the child's port is ASP1");
	assert_no_bringup_since(SWAP_EMUL, mark);
	zassert_false(written_since(SWAP_EMUL, mark, ASP2_CONTROL2));

	zassert_ok(audio_codec_start(SWAP_PARENT, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_start(SWAP_CHILD, AUDIO_DAI_DIR_TX));
	zassert_equal(cs47l63_emul_get_reg(SWAP_EMUL, OUT1L_INPUT3), MIX_SLOT(MIXER_SRC_ASP1RX1),
		      "the child on ASP1 feeds inputs 3-4");
	zassert_equal(cs47l63_emul_get_reg(SWAP_EMUL, OUT1L_INPUT4), MIX_SLOT(MIXER_SRC_ASP1RX2));

	zassert_ok(audio_codec_stop(SWAP_PARENT, AUDIO_DAI_DIR_TX));
	zassert_true(amp_on(SWAP_EMUL));
	zassert_ok(audio_codec_stop(SWAP_CHILD, AUDIO_DAI_DIR_TX));
	zassert_false(amp_on(SWAP_EMUL));
}

ZTEST(cs47l63_dual, test_reset_forgets_every_port_started)
{
	zassert_ok(configure(CHILD, AUDIO_ROUTE_PLAYBACK));
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	zassert_ok(configure(CHILD, AUDIO_ROUTE_PLAYBACK));
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_2), 2,
		      "the premise: the child alone reset the part again");

	zassert_ok(configure(PARENT, AUDIO_ROUTE_PLAYBACK));
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_stop(PARENT, AUDIO_DAI_DIR_TX));
	zassert_true(amp_on(EMUL), "the child's start after the reset was counted");

	zassert_ok(audio_codec_stop(CHILD, AUDIO_DAI_DIR_TX));
	zassert_false(amp_on(EMUL));
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	zassert_true(amp_on(EMUL), "the count came back to zero, not below it");
}

ZTEST(cs47l63_dual, test_start_check_is_cancelled_by_the_last_stop)
{
	uint32_t mark;

	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	zassert_true(k_work_delayable_is_pending(&chip_of(PARENT)->start_check),
		     "the premise: the parent's start armed the check");
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_stop(PARENT, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_stop(CHILD, AUDIO_DAI_DIR_TX));
	mark = cs47l63_emul_xfer_count(EMUL);

	k_msleep(START_WAIT_MS);

	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark,
		      "no start check ran against a stopped amplifier");
}

ZTEST(cs47l63_dual, test_start_check_running_after_the_last_stop_writes_nothing)
{
	struct cs47l63_chip *chip = chip_of(PARENT);
	uint32_t mark;

	clocks_up(EMUL);
	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));

	zassert_ok(k_mutex_lock(&chip->lock, K_FOREVER));
	k_msleep(START_WAIT_MS);
	zassert_not_equal(k_work_delayable_busy_get(&chip->start_check) & K_WORK_RUNNING, 0,
			  "the premise: the check is running");

	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, 0);
	zassert_ok(audio_codec_stop(PARENT, AUDIO_DAI_DIR_TX));
	mark = cs47l63_emul_xfer_count(EMUL);
	zassert_ok(k_mutex_unlock(&chip->lock));
	k_msleep(100);

	zassert_equal(k_work_delayable_busy_get(&chip->start_check), 0, "the check has finished");
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark, "and found no output to confirm");
}

static K_THREAD_STACK_DEFINE(s_child_stack, 2048);
static struct k_thread s_child_thread;
static int s_child_ret;

static void configure_child_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	s_child_ret = configure(CHILD, AUDIO_ROUTE_PLAYBACK);
}

ZTEST(cs47l63_dual, test_concurrent_configures_bring_the_part_up_once)
{
	s_child_ret = -EAGAIN;
	k_thread_create(&s_child_thread, s_child_stack, K_THREAD_STACK_SIZEOF(s_child_stack),
			configure_child_entry, NULL, NULL, NULL,
			k_thread_priority_get(k_current_get()), 0, K_NO_WAIT);

	zassert_ok(configure(PARENT, AUDIO_ROUTE_PLAYBACK));
	zassert_ok(k_thread_join(&s_child_thread, K_FOREVER));

	zassert_ok(s_child_ret);
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_2), 1,
		      "one bring-up, not one per port");
	zassert_equal(chip_of(PARENT)->configured, BIT(0) | BIT(1), "both ports configured");
}

ZTEST(cs47l63_dual, test_output_level_reaches_an_output_only_the_child_plays)
{
	audio_property_value_t vol = {.vol = -10};
	uint32_t val;

	clocks_up(EMUL);
	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	k_msleep(START_WAIT_MS);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & CS47L63_OUT1L_MUTE, 0,
		      "the premise: the start check confirmed the child's output");
	assert_inputs_silent(EMUL, CS47L63_OUT1L_INPUT1);

	zassert_ok(audio_codec_set_property(PARENT, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    vol));

	val = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
	zassert_equal(val & CS47L63_OUT1L_VOL_MASK, CS47L63_OUT1L_VOL_0DB - 20);
	zassert_equal(val & CS47L63_OUT1L_MUTE, 0);

	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, 0);
}

ZTEST(cs47l63_dual, test_input_front_end_stays_up_for_the_other_port)
{
	configure_both(AUDIO_ROUTE_CAPTURE);
	zassert_ok(audio_codec_route_input(PARENT, AUDIO_CHANNEL_ALL, CS47L63_INPUT_PDM));
	zassert_ok(audio_codec_route_input(CHILD, AUDIO_CHANNEL_ALL, CS47L63_INPUT_PDM));
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_RX));

	zassert_ok(audio_codec_stop(CHILD, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN1_EN_BOTH, IN1_EN_BOTH,
		      "the parent still captures from the microphone");
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_MICBIAS_CTRL5) & CS47L63_MICB1B_EN, 0,
			  "and its supply stays on");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP_TX_EN_BOTH,
		      ASP_TX_EN_BOTH);
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2_ENABLES1) & ASP_TX_EN_BOTH, 0);

	zassert_ok(audio_codec_stop(PARENT, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN1_EN_BOTH, 0);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_MICBIAS_CTRL5) & CS47L63_MICB1B_EN, 0);
}

ZTEST(cs47l63_dual, test_input_terminal_is_chosen_per_port)
{
	configure_both(AUDIO_ROUTE_CAPTURE);
	zassert_ok(audio_codec_route_input(CHILD, AUDIO_CHANNEL_ALL, CS47L63_INPUT_PDM));
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2L));
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2TX1_INPUT1), MIX_SLOT(CS47L63_MIXER_SRC_IN1L));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN_EN_ALL, IN_EN_ALL);

	zassert_ok(audio_codec_stop(CHILD, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN_EN_ALL, IN2_EN_BOTH,
		      "only the front end nobody uses comes down");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_MICBIAS_CTRL5) & CS47L63_MICB1B_EN, 0);

	zassert_ok(audio_codec_stop(PARENT, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, 0);
}

ZTEST(cs47l63_dual, test_child_start_refused_until_configured)
{
	uint32_t mark;

	zassert_ok(configure(PARENT, AUDIO_ROUTE_PLAYBACK_CAPTURE));
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_equal(-ENOTSUP, configure(CHILD, AUDIO_ROUTE_BYPASS));
	zassert_equal(-EINVAL, audio_codec_start(CHILD, AUDIO_DAI_DIR_RX));
	zassert_equal(-EINVAL, audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	audio_codec_start_output(CHILD);
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark, "an unconfigured port touches nothing");
}

ZTEST(cs47l63_dual, test_child_start_refused_outside_its_route)
{
	uint32_t mark;

	zassert_ok(configure(PARENT, AUDIO_ROUTE_PLAYBACK_CAPTURE));
	zassert_ok(configure(CHILD, AUDIO_ROUTE_PLAYBACK));
	mark = cs47l63_emul_xfer_count(EMUL);
	zassert_equal(-EINVAL, audio_codec_start(CHILD, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark);

	zassert_ok(configure(CHILD, AUDIO_ROUTE_CAPTURE));
	mark = cs47l63_emul_xfer_count(EMUL);
	zassert_equal(-EINVAL, audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	audio_codec_start_output(CHILD);
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark, "no playback on a capture route");
}

ZTEST(cs47l63_dual, test_child_failed_configure_leaves_nothing_startable)
{
	uint32_t mark;

	configure_both(AUDIO_ROUTE_PLAYBACK_CAPTURE);
	mark = cs47l63_emul_xfer_count(EMUL);
	cs47l63_emul_fail_at(EMUL, ASP2_CONTROL2);

	zassert_equal(-EIO, configure(CHILD, AUDIO_ROUTE_PLAYBACK_CAPTURE));
	zassert_false(written_since(EMUL, mark, ASP2_ENABLES1),
		      "the injection took effect: the child's port was never configured");
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_equal(-EINVAL, audio_codec_start(CHILD, AUDIO_DAI_DIR_RX));
	zassert_equal(-EINVAL, audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	audio_codec_start_output(CHILD);
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark, "a half-configured port stays stopped");
}

ZTEST(cs47l63_dual, test_child_route_input_only_caches_while_stopped)
{
	uint32_t mark;

	configure_both(AUDIO_ROUTE_CAPTURE);
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_ok(audio_codec_route_input(CHILD, AUDIO_CHANNEL_ALL, CS47L63_INPUT_PDM));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark, "a stopped input is not written");

	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2TX1_INPUT1), MIX_SLOT(CS47L63_MIXER_SRC_IN1L),
		      "the cached route is applied by the start");
}
