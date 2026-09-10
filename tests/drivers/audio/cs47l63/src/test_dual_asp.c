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
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define PARENT DEVICE_DT_GET(DT_NODELABEL(cs47l63_dual))
#define CHILD  DEVICE_DT_GET(DT_NODELABEL(cs47l63_dual_asp2))
#define EMUL   EMUL_DT_GET(DT_NODELABEL(cs47l63_dual))

#define SWAP_PARENT DEVICE_DT_GET(DT_NODELABEL(cs47l63_swap))
#define SWAP_CHILD  DEVICE_DT_GET(DT_NODELABEL(cs47l63_swap_asp1))
#define SWAP_EMUL   EMUL_DT_GET(DT_NODELABEL(cs47l63_swap))

#define OUT1L_VOL_MASK 0x000000FFU
#define VOL_CODE_M10DB 0x6CU
#define VOL_M10DB      (-10)

#define OTHER_MCLK_HZ (2U * CS47L63_EMUL_MCLK_HZ)
#define OTHER_RATE_HZ (CS47L63_EMUL_RATE_HZ / 2U)

/** Long enough for the start check to reach its deadline. */
#define START_WAIT_MS (CS47L63_START_TIMEOUT_MS + 100U)

#define CHECK_FINISH_WAIT_MS 100U

#define CHILD_STACK_SIZE 2048U

/* DS1249F2 register map. */
#define OUT1L_INPUT3       0x00008108U
#define OUT1L_INPUT4       0x0000810CU
#define OUT1L_INPUT_STRIDE (OUT1L_INPUT4 - OUT1L_INPUT3)
#define ASP2_ENABLES1      0x00006080U
#define ASP2_CONTROL1      0x00006084U
#define ASP2_CONTROL2      0x00006088U
#define ASP2TX1_INPUT1     0x00008300U
#define ASP2TX2_INPUT1     0x00008310U

#define MIXER_SRC_ASP1RX1 0x020U
#define MIXER_SRC_ASP1RX2 0x021U
#define MIXER_SRC_ASP2RX1 0x030U
#define MIXER_SRC_ASP2RX2 0x031U

#define IN1_EN_BOTH    (CS47L63_IN1L_EN | CS47L63_IN1R_EN)
#define IN2_EN_BOTH    (CS47L63_IN2L_EN | CS47L63_IN2R_EN)
#define IN_EN_ALL      (IN1_EN_BOTH | IN2_EN_BOTH)
#define ASP_TX_EN_BOTH (CS47L63_ASP1_TX1_EN | CS47L63_ASP1_TX2_EN)
#define ASP_RX_EN_BOTH (CS47L63_ASP1_RX1_EN | CS47L63_ASP1_RX2_EN)

/** The one terminal each way: the port's own, from its devicetree node. */
#define PORT_TERMINAL 0U

#define MIX_SLOT(src) (((uint32_t)CS47L63_OUT1LMIX_VOL_0DB << CS47L63_OUT1LMIX_VOL_SHIFT) | (src))
/** One of two routed slots: each at -6 dB, so their sum cannot clip. */
#define MIX_PAIR(src) (((uint32_t)CS47L63_OUT1LMIX_VOL_M6DB << CS47L63_OUT1LMIX_VOL_SHIFT) | (src))
/** One of three or four routed slots across both ports: each at -12 dB. */
#define MIX_QUAD(src) (((uint32_t)CS47L63_OUT1LMIX_VOL_M12DB << CS47L63_OUT1LMIX_VOL_SHIFT) | (src))

static atomic_t s_dual_cb_calls;
static atomic_t s_parent_errors;
static atomic_t s_child_errors;
static atomic_t s_other_dev_calls;
static atomic_t s_clear_ret;

static void dual_fault_cb(const struct device *dev, uint32_t errors)
{
	(void)atomic_inc(&s_dual_cb_calls);
	if (dev == PARENT) {
		(void)atomic_set(&s_parent_errors, (atomic_val_t)errors);
	} else if (dev == CHILD) {
		(void)atomic_set(&s_child_errors, (atomic_val_t)errors);
	} else {
		(void)atomic_inc(&s_other_dev_calls);
	}
}

static void clearing_cb(const struct device *dev, uint32_t errors)
{
	(void)atomic_set(&s_clear_ret, audio_codec_clear_errors(dev));
	dual_fault_cb(dev, errors);
}

static int configure_at(const struct device *dev, audio_route_t route, uint32_t mclk_hz,
			uint32_t rate_hz)
{
	struct audio_codec_cfg cfg = cs47l63_test_cfg(route);

	cfg.mclk_freq = mclk_hz;
	cfg.dai_cfg.i2s.frame_clk_freq = rate_hz;

	return audio_codec_configure(dev, &cfg);
}

static void configure_both(audio_route_t route)
{
	zassert_ok(cs47l63_test_configure(PARENT, route));
	zassert_ok(cs47l63_test_configure(CHILD, route));
}

/** Polls the part's fault flags through a parent output mute set that changes nothing. */
static void dual_poll_faults(void)
{
	audio_property_value_t unmute = {.mute = false};

	zassert_ok(audio_codec_set_property(PARENT, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL,
					    unmute));
	zassert_equal(atomic_get(&s_other_dev_calls), 0,
		      "the callback reports a device of this part");
}

static bool written_since(const struct emul *target, uint32_t mark, uint32_t addr)
{
	const uint32_t count = cs47l63_emul_xfer_count(target);
	struct cs47l63_emul_xfer xfer;

	for (uint32_t i = mark; i < count; i++) {
		zassert_true(cs47l63_emul_xfer_get(target, i, &xfer));
		if (xfer.write && (xfer.addr == addr)) {
			return true;
		}
	}

	return false;
}

/** No reset, boot poll, ID read or clock write since mark; a stream start or stop may
 *  still toggle SYSCLK_EN.
 */
static void assert_no_bringup_since(const struct emul *target, uint32_t mark)
{
	static const uint32_t k_clock[] = {
		CS47L63_FLL1_CONTROL1, CS47L63_FLL1_CONTROL2, CS47L63_FLL1_CONTROL3,
		CS47L63_FLL1_CONTROL4, CS47L63_SAMPLE_RATE1,
	};
	const uint32_t count = cs47l63_emul_xfer_count(target);
	struct cs47l63_emul_xfer xfer;
	uint32_t sysclk = 0U;

	for (uint32_t i = 0U; i < mark; i++) {
		zassert_true(cs47l63_emul_xfer_get(target, i, &xfer));
		if (xfer.write && (xfer.addr == CS47L63_SYSTEM_CLOCK1)) {
			sysclk = xfer.val;
		}
	}

	for (uint32_t i = mark; i < count; i++) {
		zassert_true(cs47l63_emul_xfer_get(target, i, &xfer));
		zassert_false((xfer.addr == CS47L63_IRQ1_EINT_2) || (xfer.addr == CS47L63_DEVID),
			      "the part was reset under a configured port");
		zassert_false(xfer.write && (xfer.addr == CS47L63_SYSTEM_CLOCK1) &&
				      ((xfer.val & ~CS47L63_SYSCLK_EN) !=
				       (sysclk & ~CS47L63_SYSCLK_EN)),
			      "SYSCLK reprogrammed under a configured port");
		for (size_t j = 0U; j < ARRAY_SIZE(k_clock); j++) {
			zassert_false(xfer.write && (xfer.addr == k_clock[j]),
				      "clock register 0x%05x written under a configured port",
				      k_clock[j]);
		}
	}
}

static void assert_inputs_silent(const struct emul *target, uint32_t first)
{
	zassert_equal(cs47l63_emul_get_reg(target, first), MIX_SLOT(CS47L63_MIXER_SRC_NONE),
		      "mixer input 0x%05x still feeds the output", first);
	zassert_equal(cs47l63_emul_get_reg(target, first + OUT1L_INPUT_STRIDE),
		      MIX_SLOT(CS47L63_MIXER_SRC_NONE), "mixer input 0x%05x still feeds the output",
		      first + OUT1L_INPUT_STRIDE);
}

static bool amp_on(const struct emul *target)
{
	return (cs47l63_emul_get_reg(target, CS47L63_OUTPUT_ENABLE_1) & CS47L63_OUT1L_EN) != 0U;
}

/** FLL1 locked and the amplifier reporting enabled, so a start check completes. */
static void clocks_up(const struct emul *target)
{
	cs47l63_emul_set_reg(target, CS47L63_IRQ1_STS_6, CS47L63_FLL1_LOCK_STS1);
	cs47l63_emul_set_reg(target, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS);
}

/** Output running on the parent, so a clock latch is a fault. */
static void parent_running(void)
{
	clocks_up(EMUL);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	(void)k_msleep((int32_t)START_WAIT_MS);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & CS47L63_OUT1L_MUTE, 0U,
		      "the premise: the start check confirmed the output");
}

static struct cs47l63_chip *dual_chip(const struct device *dev)
{
	return ((const struct cs47l63_config *)dev->config)->chip;
}

static void dual_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
	cs47l63_emul_reset(SWAP_EMUL);

	dual_chip(PARENT)->configured = 0U;
	dual_chip(SWAP_PARENT)->configured = 0U;

	(void)atomic_set(&s_dual_cb_calls, 0);
	(void)atomic_set(&s_parent_errors, 0);
	(void)atomic_set(&s_child_errors, 0);
	(void)atomic_set(&s_other_dev_calls, 0);
	(void)atomic_set(&s_clear_ret, -EAGAIN);
}

static void dual_after_each(void *fixture)
{
	const struct device *const k_devs[] = {PARENT, CHILD, SWAP_PARENT, SWAP_CHILD};

	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
	cs47l63_emul_reset(SWAP_EMUL);

	for (size_t i = 0U; i < ARRAY_SIZE(k_devs); i++) {
		audio_codec_stop_output(k_devs[i]);
		zassert_ok(audio_codec_stop(k_devs[i], AUDIO_DAI_DIR_RX));
		zassert_ok(audio_codec_register_error_callback(k_devs[i], NULL));
		zassert_ok(audio_codec_clear_errors(k_devs[i]));
	}
	zassert_equal(atomic_get(&s_other_dev_calls), 0,
		      "the callback reports a device of this part");
}

ZTEST_SUITE(cs47l63_dual, NULL, NULL, dual_before_each, dual_after_each, NULL);

static ZTEST(cs47l63_dual, test_both_ports_configure)
{
	uint32_t mark;

	zassert_ok(cs47l63_test_configure(PARENT, AUDIO_ROUTE_PLAYBACK_CAPTURE));
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_ok(cs47l63_test_configure(CHILD, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_true(written_since(EMUL, mark, ASP2_CONTROL2),
		     "the premise: the child's port was configured");
	assert_no_bringup_since(EMUL, mark);
	zassert_equal((cs47l63_emul_get_reg(EMUL, ASP2_CONTROL1) & CS47L63_ASP1_RATE_MASK) >>
			      CS47L63_ASP1_RATE_SHIFT,
		      CS47L63_ASP1_RATE_SEL_SAMPLE_RATE1,
		      "the child's port runs from the rate slot the parent set");
}

static ZTEST(cs47l63_dual, test_second_port_other_mclk_rejected_while_the_first_runs)
{
	uint32_t mark;

	zassert_ok(cs47l63_test_configure(PARENT, AUDIO_ROUTE_PLAYBACK));
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_equal(-ENOTSUP, configure_at(CHILD, AUDIO_ROUTE_PLAYBACK, OTHER_MCLK_HZ,
					     CS47L63_EMUL_RATE_HZ));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark, "a refused configure touches nothing");

	zassert_ok(configure_at(PARENT, AUDIO_ROUTE_PLAYBACK, OTHER_MCLK_HZ, CS47L63_EMUL_RATE_HZ),
		   "the premise: the part itself runs at that MCLK");
}

static ZTEST(cs47l63_dual, test_second_port_other_rate_rejected_while_the_first_runs)
{
	uint32_t mark;

	zassert_ok(cs47l63_test_configure(PARENT, AUDIO_ROUTE_PLAYBACK));
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_equal(-ENOTSUP, configure_at(CHILD, AUDIO_ROUTE_PLAYBACK, CS47L63_EMUL_MCLK_HZ,
					     OTHER_RATE_HZ));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark, "a refused configure touches nothing");

	zassert_ok(configure_at(PARENT, AUDIO_ROUTE_PLAYBACK, CS47L63_EMUL_MCLK_HZ, OTHER_RATE_HZ),
		   "the premise: the part itself runs at that rate");
}

/** Index in the transfer log of the first write to @p addr since @p mark, or -1. */
static int first_write_since(const struct emul *target, uint32_t mark, uint32_t addr)
{
	const uint32_t count = cs47l63_emul_xfer_count(target);
	struct cs47l63_emul_xfer xfer;

	for (uint32_t i = mark; i < count; i++) {
		zassert_true(cs47l63_emul_xfer_get(target, i, &xfer));
		if (xfer.write && (xfer.addr == addr)) {
			return (int)i;
		}
	}

	return -1;
}

static ZTEST(cs47l63_dual, test_stopped_sibling_does_not_hold_the_rate)
{
	zassert_ok(configure_at(PARENT, AUDIO_ROUTE_PLAYBACK, CS47L63_EMUL_MCLK_HZ,
				CS47L63_EMUL_RATE_HZ));
	zassert_ok(configure_at(CHILD, AUDIO_ROUTE_PLAYBACK, CS47L63_EMUL_MCLK_HZ,
				CS47L63_EMUL_RATE_HZ));

	zassert_ok(configure_at(PARENT, AUDIO_ROUTE_PLAYBACK, CS47L63_EMUL_MCLK_HZ, OTHER_RATE_HZ),
		   "a sibling that runs nothing does not pin the rate");
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_2), 2U,
		      "the new rate took a full bring-up");

	zassert_equal(-EINVAL, audio_codec_start(CHILD, AUDIO_DAI_DIR_TX),
		      "the reset left the sibling to configure again");
}

static ZTEST(cs47l63_dual, test_same_rate_keeps_a_stopped_sibling_configured)
{
	uint32_t mark;

	configure_both(AUDIO_ROUTE_PLAYBACK);
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_ok(cs47l63_test_configure(PARENT, AUDIO_ROUTE_PLAYBACK));

	assert_no_bringup_since(EMUL, mark);
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX), "the sibling is still configured");
}

static ZTEST(cs47l63_dual, test_new_rate_on_a_started_port_takes_the_full_path)
{
	configure_both(AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX | AUDIO_DAI_DIR_RX));
	zassert_true(amp_on(EMUL), "the premise: the parent plays");

	zassert_ok(configure_at(PARENT, AUDIO_ROUTE_PLAYBACK_CAPTURE, CS47L63_EMUL_MCLK_HZ,
				OTHER_RATE_HZ));

	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_2), 2U,
		      "a full bring-up");
	zassert_false(amp_on(EMUL), "the reset stopped the output");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP_TX_EN_BOTH, 0U,
		      "and the parent's transmit slots");
	zassert_equal(-EINVAL, audio_codec_start(CHILD, AUDIO_DAI_DIR_TX),
		      "the sibling is unconfigured");
}

static ZTEST(cs47l63_dual, test_failed_join_still_stops_both_directions)
{
	uint32_t mark;

	configure_both(AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX | AUDIO_DAI_DIR_RX));
	mark = cs47l63_emul_xfer_count(EMUL);
	cs47l63_emul_fail_write_at(EMUL, CS47L63_OUT1L_VOLUME_1);

	zassert_equal(-EIO, cs47l63_test_configure(PARENT, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_true(cs47l63_emul_fail_hits(EMUL) > 0U,
		     "the injection took effect: the output stop's mute failed");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP_TX_EN_BOTH, 0U,
		      "the capture stop still ran");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, 0U,
		      "and took the front end down");
	zassert_false(written_since(EMUL, mark, CS47L63_ASP1_CONTROL2),
		      "no format write after a failed stop");
	zassert_equal(-EINVAL, audio_codec_start(PARENT, AUDIO_DAI_DIR_TX),
		      "the port stays unconfigured");
}

static ZTEST(cs47l63_dual, test_join_clears_the_slots_before_the_format_changes)
{
	struct audio_codec_cfg cfg = cs47l63_test_cfg(AUDIO_ROUTE_PLAYBACK);
	uint32_t mark;
	int clear_idx = -1;
	struct cs47l63_emul_xfer xfer;

	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	mark = cs47l63_emul_xfer_count(EMUL);

	cfg.dai_cfg.i2s.word_size = 24U;
	zassert_ok(audio_codec_configure(PARENT, &cfg));

	for (uint32_t i = mark; i < cs47l63_emul_xfer_count(EMUL); i++) {
		zassert_true(cs47l63_emul_xfer_get(EMUL, i, &xfer));
		if (xfer.write && (xfer.addr == CS47L63_ASP1_ENABLES1) && (xfer.val == 0U)) {
			clear_idx = (int)i;
			break;
		}
	}
	zassert_true(first_write_since(EMUL, mark, CS47L63_ASP1_CONTROL2) >= 0,
		     "the premise: the new word size rewrote the format");
	zassert_true(clear_idx >= 0, "the port's slots were all disabled");
	zassert_true(clear_idx < first_write_since(EMUL, mark, CS47L63_ASP1_CONTROL2),
		     "before the format write");
	zassert_true(clear_idx < first_write_since(EMUL, mark, CS47L63_ASP1_DATA_CONTROL1));
	zassert_true(clear_idx < first_write_since(EMUL, mark, CS47L63_ASP1_DATA_CONTROL5));
	assert_no_bringup_since(EMUL, mark);
}

static ZTEST(cs47l63_dual, test_child_first_then_parent)
{
	uint32_t mark;

	zassert_ok(cs47l63_test_configure(CHILD, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_2), 1U,
		      "the child alone brings the part up");
	zassert_true(cs47l63_emul_write_count(EMUL, CS47L63_SAMPLE_RATE1) >= 1U);
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT3), MIX_SLOT(CS47L63_MIXER_SRC_NONE),
		      "the child's mixer inputs carry no source until it starts");
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT4), MIX_SLOT(CS47L63_MIXER_SRC_NONE));
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_OUT1L_INPUT1), 0U,
		      "the parent's mixer inputs are not the child's");
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_ok(cs47l63_test_configure(PARENT, AUDIO_ROUTE_PLAYBACK_CAPTURE));

	zassert_true(written_since(EMUL, mark, CS47L63_ASP1_CONTROL2),
		     "the premise: the parent's port was configured");
	assert_no_bringup_since(EMUL, mark);
	zassert_false(written_since(EMUL, mark, OUT1L_INPUT3), "the child's route stands");
}

static ZTEST(cs47l63_dual, test_parent_reconfigure_keeps_child)
{
	uint32_t mark;

	configure_both(AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	zassert_true(amp_on(EMUL));
	assert_inputs_silent(EMUL, CS47L63_OUT1L_INPUT1);
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_ok(cs47l63_test_configure(PARENT, AUDIO_ROUTE_PLAYBACK_CAPTURE));
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

static ZTEST(cs47l63_dual, test_only_the_started_port_feeds_the_output)
{
	configure_both(AUDIO_ROUTE_PLAYBACK);

	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));

	assert_inputs_silent(EMUL, CS47L63_OUT1L_INPUT1);
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT3), MIX_PAIR(MIXER_SRC_ASP2RX1));
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT4), MIX_PAIR(MIXER_SRC_ASP2RX2));

	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_QUAD(MIXER_SRC_ASP1RX1), "the parent's own start restores its route");
}

static void assert_mixer(const struct emul *target, uint32_t first, uint32_t slot1,
			 uint32_t slot2)
{
	zassert_equal(cs47l63_emul_get_reg(target, first), slot1,
		      "mixer input 0x%05x holds 0x%08x", first,
		      cs47l63_emul_get_reg(target, first));
	zassert_equal(cs47l63_emul_get_reg(target, first + OUT1L_INPUT_STRIDE), slot2,
		      "mixer input 0x%05x holds 0x%08x", first + OUT1L_INPUT_STRIDE,
		      cs47l63_emul_get_reg(target, first + OUT1L_INPUT_STRIDE));
}

static int last_write_index(uint32_t addr)
{
	const uint32_t n = cs47l63_emul_write_count(EMUL, addr);

	zassert_true(n > 0U);

	return cs47l63_emul_write_index(EMUL, addr, n - 1U);
}

static ZTEST(cs47l63_dual, test_mono_stream_joined_to_a_running_part_feeds_one_slot_at_0db)
{
	struct audio_codec_cfg cfg = cs47l63_test_cfg(AUDIO_ROUTE_PLAYBACK);

	zassert_ok(cs47l63_test_configure(PARENT, AUDIO_ROUTE_PLAYBACK));
	cfg.dai_cfg.i2s.channels = 1U;
	zassert_ok(audio_codec_configure(CHILD, &cfg));

	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));

	assert_mixer(EMUL, OUT1L_INPUT3, MIX_SLOT(MIXER_SRC_ASP2RX1),
		     MIX_SLOT(CS47L63_MIXER_SRC_NONE));
}

static ZTEST(cs47l63_dual, test_both_stereo_ports_share_the_headroom)
{
	configure_both(AUDIO_ROUTE_PLAYBACK);

	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	assert_mixer(EMUL, CS47L63_OUT1L_INPUT1, MIX_PAIR(MIXER_SRC_ASP1RX1),
		     MIX_PAIR(MIXER_SRC_ASP1RX2));

	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	assert_mixer(EMUL, CS47L63_OUT1L_INPUT1, MIX_QUAD(MIXER_SRC_ASP1RX1),
		     MIX_QUAD(MIXER_SRC_ASP1RX2));
	assert_mixer(EMUL, OUT1L_INPUT3, MIX_QUAD(MIXER_SRC_ASP2RX1), MIX_QUAD(MIXER_SRC_ASP2RX2));
	zassert_true(last_write_index(CS47L63_OUT1L_INPUT1) < last_write_index(OUT1L_INPUT3),
		     "the running port is turned down before the new sources come in");

	zassert_ok(audio_codec_stop(CHILD, AUDIO_DAI_DIR_TX));
	assert_inputs_silent(EMUL, OUT1L_INPUT3);
	assert_mixer(EMUL, CS47L63_OUT1L_INPUT1, MIX_PAIR(MIXER_SRC_ASP1RX1),
		     MIX_PAIR(MIXER_SRC_ASP1RX2));
	zassert_true(last_write_index(OUT1L_INPUT3) < last_write_index(CS47L63_OUT1L_INPUT1),
		     "the running port is turned up after the leaving sources are gone");
}

static ZTEST(cs47l63_dual, test_a_route_change_on_one_port_resizes_the_headroom_of_both)
{
	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));

	zassert_ok(audio_codec_route_output(CHILD, AUDIO_CHANNEL_FRONT_LEFT, PORT_TERMINAL));
	assert_mixer(EMUL, CS47L63_OUT1L_INPUT1, MIX_QUAD(MIXER_SRC_ASP1RX1),
		     MIX_QUAD(MIXER_SRC_ASP1RX2));
	assert_mixer(EMUL, OUT1L_INPUT3, MIX_QUAD(MIXER_SRC_ASP2RX1),
		     MIX_SLOT(CS47L63_MIXER_SRC_NONE));

	zassert_ok(audio_codec_route_output(PARENT, AUDIO_CHANNEL_FRONT_LEFT, PORT_TERMINAL));
	assert_mixer(EMUL, CS47L63_OUT1L_INPUT1, MIX_PAIR(MIXER_SRC_ASP1RX1),
		     MIX_SLOT(CS47L63_MIXER_SRC_NONE));
	assert_mixer(EMUL, OUT1L_INPUT3, MIX_PAIR(MIXER_SRC_ASP2RX1),
		     MIX_SLOT(CS47L63_MIXER_SRC_NONE));

	zassert_ok(audio_codec_route_output(PARENT, AUDIO_CHANNEL_ALL, PORT_TERMINAL));
	assert_mixer(EMUL, CS47L63_OUT1L_INPUT1, MIX_QUAD(MIXER_SRC_ASP1RX1),
		     MIX_QUAD(MIXER_SRC_ASP1RX2));
	assert_mixer(EMUL, OUT1L_INPUT3, MIX_QUAD(MIXER_SRC_ASP2RX1),
		     MIX_SLOT(CS47L63_MIXER_SRC_NONE));
}

static ZTEST(cs47l63_dual, test_a_port_stopped_last_does_not_feed_the_next_start)
{
	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_stop(PARENT, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_stop(CHILD, AUDIO_DAI_DIR_TX));

	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));

	assert_inputs_silent(EMUL, OUT1L_INPUT3);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_PAIR(MIXER_SRC_ASP1RX1));
}

static ZTEST(cs47l63_dual, test_a_port_whose_configure_failed_does_not_feed_the_output)
{
	uint32_t mark;

	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	mark = cs47l63_emul_xfer_count(EMUL);
	cs47l63_emul_fail_at(EMUL, CS47L63_ASP1_CONTROL2);

	zassert_equal(-EIO, cs47l63_test_configure(PARENT, AUDIO_ROUTE_PLAYBACK));
	zassert_equal(cs47l63_emul_fail_hits(EMUL), 1U,
		      "the injection took effect: the parent's port was never configured");

	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	assert_inputs_silent(EMUL, CS47L63_OUT1L_INPUT1);
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT3), MIX_PAIR(MIXER_SRC_ASP2RX1));
}

static ZTEST(cs47l63_dual, test_a_failed_start_under_a_live_amplifier_leaves_no_route)
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

static ZTEST(cs47l63_dual, test_child_output_routes_inputs_3_4)
{
	uint32_t mark;

	configure_both(AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_ok(audio_codec_route_output(CHILD, AUDIO_CHANNEL_FRONT_LEFT, PORT_TERMINAL));
	zassert_false(written_since(EMUL, mark, OUT1L_INPUT3),
		      "a stopped port's route is cached, not played on the parent's amplifier");
	zassert_false(written_since(EMUL, mark, OUT1L_INPUT4));

	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT3), MIX_QUAD(MIXER_SRC_ASP2RX1),
		      "three sources are running: each at -12 dB");
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT4), MIX_SLOT(CS47L63_MIXER_SRC_NONE),
		      "the child's start applies the cached route");
	assert_mixer(EMUL, CS47L63_OUT1L_INPUT1, MIX_QUAD(MIXER_SRC_ASP1RX1),
		     MIX_QUAD(MIXER_SRC_ASP1RX2));

	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2TX1_INPUT1), MIX_SLOT(CS47L63_MIXER_SRC_IN1L));
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2TX2_INPUT1), MIX_SLOT(CS47L63_MIXER_SRC_IN1R));

	zassert_false(written_since(EMUL, mark, CS47L63_ASP1TX1_INPUT1));
	zassert_false(written_since(EMUL, mark, CS47L63_ASP1TX2_INPUT1));
}

static ZTEST(cs47l63_dual, test_sysclk_stays_up_while_the_other_port_streams)
{
	configure_both(AUDIO_ROUTE_PLAYBACK_CAPTURE);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP_RX_EN_BOTH,
		      ASP_RX_EN_BOTH, "the parent's output start enabled its receive slots");
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2_ENABLES1) & ASP_RX_EN_BOTH, 0U,
		      "a capture start enables no receive slot");

	zassert_ok(audio_codec_stop(PARENT, AUDIO_DAI_DIR_TX));
	zassert_not_equal(cs47l63_emul_get_reg(EMUL, CS47L63_SYSTEM_CLOCK1) & CS47L63_SYSCLK_EN,
			  0U, "the child still captures");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1_ENABLES1) & ASP_RX_EN_BOTH, 0U);

	zassert_ok(audio_codec_stop(CHILD, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_SYSTEM_CLOCK1) & CS47L63_SYSCLK_EN, 0U,
		      "the last stop takes SYSCLK down");
}

static ZTEST(cs47l63_dual, test_amp_stays_on_until_last_output_stops)
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
	zassert_equal(cs47l63_emul_get_reg(EMUL, OUT1L_INPUT3), MIX_PAIR(MIXER_SRC_ASP2RX1));

	zassert_ok(audio_codec_stop(CHILD, AUDIO_DAI_DIR_TX));
	zassert_false(amp_on(EMUL), "the last stop takes the amplifier down");
}

static ZTEST(cs47l63_dual, test_fault_reaches_child_callback)
{
	configure_both(AUDIO_ROUTE_PLAYBACK);
	parent_running();
	zassert_ok(audio_codec_register_error_callback(CHILD, dual_fault_cb));

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	dual_poll_faults();

	zassert_equal(atomic_get(&s_dual_cb_calls), 1);
	zassert_equal((uint32_t)atomic_get(&s_child_errors),
		      (uint32_t)AUDIO_CODEC_ERROR_CLOCK);
}

static ZTEST(cs47l63_dual, test_fault_reaches_every_callback)
{
	configure_both(AUDIO_ROUTE_PLAYBACK);
	parent_running();
	zassert_ok(audio_codec_register_error_callback(PARENT, dual_fault_cb));
	zassert_ok(audio_codec_register_error_callback(CHILD, dual_fault_cb));

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	dual_poll_faults();

	zassert_equal(atomic_get(&s_dual_cb_calls), 2);
	zassert_equal((uint32_t)atomic_get(&s_parent_errors),
		      (uint32_t)AUDIO_CODEC_ERROR_CLOCK);
	zassert_equal(atomic_get(&s_child_errors), atomic_get(&s_parent_errors),
		      "both see the same flags");
}

static ZTEST(cs47l63_dual, test_a_callback_that_clears_errors_leaves_the_next_one_its_mask)
{
	configure_both(AUDIO_ROUTE_PLAYBACK);
	parent_running();
	zassert_ok(audio_codec_register_error_callback(PARENT, clearing_cb));
	zassert_ok(audio_codec_register_error_callback(CHILD, dual_fault_cb));

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	dual_poll_faults();

	zassert_equal(atomic_get(&s_dual_cb_calls), 2, "the premise: both callbacks ran");
	zassert_equal(atomic_get(&s_clear_ret), 0, "the clear from inside the callback succeeded");
	zassert_equal((uint32_t)atomic_get(&s_parent_errors),
		      (uint32_t)AUDIO_CODEC_ERROR_CLOCK);
	zassert_equal((uint32_t)atomic_get(&s_child_errors),
		      (uint32_t)AUDIO_CODEC_ERROR_CLOCK,
		      "the parent's clear must not empty the mask the child is told about");
}

static ZTEST(cs47l63_dual, test_child_clear_errors_clears_the_chip_latch)
{
	configure_both(AUDIO_ROUTE_PLAYBACK);
	parent_running();
	zassert_ok(audio_codec_register_error_callback(CHILD, dual_fault_cb));

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	dual_poll_faults();
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	dual_poll_faults();
	zassert_equal(atomic_get(&s_dual_cb_calls), 1,
		      "the premise: a latched fault is reported once");

	zassert_ok(audio_codec_clear_errors(CHILD));
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_1, CS47L63_SYSCLK_FAIL_EINT1);
	dual_poll_faults();
	zassert_equal(atomic_get(&s_dual_cb_calls), 2,
		      "the child's clear reached the part's latch");
}

static ZTEST(cs47l63_dual, test_child_properties_not_supported)
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

	for (size_t i = 0U; i < ARRAY_SIZE(k_props); i++) {
		zassert_equal(-ENOTSUP,
			      audio_codec_set_property(CHILD, k_props[i], AUDIO_CHANNEL_ALL, val),
			      "property %d is the parent's", k_props[i]);
	}
	zassert_equal(-ENOTSUP, audio_codec_apply_properties(CHILD));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark);
}

static ZTEST(cs47l63_dual, test_swapped_ports)
{
	uint32_t mark;

	zassert_ok(cs47l63_test_configure(SWAP_PARENT, AUDIO_ROUTE_PLAYBACK));
	mark = cs47l63_emul_xfer_count(SWAP_EMUL);

	zassert_ok(cs47l63_test_configure(SWAP_CHILD, AUDIO_ROUTE_PLAYBACK));
	zassert_true(written_since(SWAP_EMUL, mark, CS47L63_ASP1_CONTROL2),
		     "the premise: the child's port is ASP1");
	assert_no_bringup_since(SWAP_EMUL, mark);
	zassert_false(written_since(SWAP_EMUL, mark, ASP2_CONTROL2));

	zassert_ok(audio_codec_start(SWAP_PARENT, AUDIO_DAI_DIR_TX));
	zassert_equal(cs47l63_emul_get_reg(SWAP_EMUL, CS47L63_OUT1L_INPUT1),
		      MIX_PAIR(MIXER_SRC_ASP2RX1), "the parent on ASP2 feeds inputs 1-2");
	zassert_ok(audio_codec_start(SWAP_CHILD, AUDIO_DAI_DIR_TX));
	zassert_equal(cs47l63_emul_get_reg(SWAP_EMUL, OUT1L_INPUT3), MIX_QUAD(MIXER_SRC_ASP1RX1),
		      "the child on ASP1 feeds inputs 3-4");
	zassert_equal(cs47l63_emul_get_reg(SWAP_EMUL, OUT1L_INPUT4), MIX_QUAD(MIXER_SRC_ASP1RX2));

	zassert_ok(audio_codec_stop(SWAP_PARENT, AUDIO_DAI_DIR_TX));
	zassert_true(amp_on(SWAP_EMUL));
	zassert_ok(audio_codec_stop(SWAP_CHILD, AUDIO_DAI_DIR_TX));
	zassert_false(amp_on(SWAP_EMUL));
}

static ZTEST(cs47l63_dual, test_reset_forgets_every_port_started)
{
	zassert_ok(cs47l63_test_configure(CHILD, AUDIO_ROUTE_PLAYBACK));
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	zassert_ok(cs47l63_test_configure(CHILD, AUDIO_ROUTE_PLAYBACK));
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_2), 2U,
		      "the premise: the child alone reset the part again");

	zassert_ok(cs47l63_test_configure(PARENT, AUDIO_ROUTE_PLAYBACK));
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_stop(PARENT, AUDIO_DAI_DIR_TX));
	zassert_true(amp_on(EMUL), "the child's start after the reset was counted");

	zassert_ok(audio_codec_stop(CHILD, AUDIO_DAI_DIR_TX));
	zassert_false(amp_on(EMUL));
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	zassert_true(amp_on(EMUL), "the count came back to zero, not below it");
}

static ZTEST(cs47l63_dual, test_start_check_is_cancelled_by_the_last_stop)
{
	uint32_t mark;

	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));
	zassert_true(k_work_delayable_is_pending(&dual_chip(PARENT)->start_check),
		     "the premise: the parent's start armed the check");
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_stop(PARENT, AUDIO_DAI_DIR_TX));
	zassert_ok(audio_codec_stop(CHILD, AUDIO_DAI_DIR_TX));
	mark = cs47l63_emul_xfer_count(EMUL);

	(void)k_msleep((int32_t)START_WAIT_MS);

	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark,
		      "no start check ran against a stopped amplifier");
}

static ZTEST(cs47l63_dual, test_start_check_running_after_the_last_stop_writes_nothing)
{
	struct cs47l63_chip *chip = dual_chip(PARENT);
	uint32_t mark;
	uint32_t busy;
	int stop_ret;

	clocks_up(EMUL);
	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_TX));

	zassert_ok(k_mutex_lock(&chip->lock, K_FOREVER));
	(void)k_msleep((int32_t)START_WAIT_MS);
	busy = (uint32_t)k_work_delayable_busy_get(&chip->start_check);
	cs47l63_emul_set_reg(EMUL, CS47L63_OUTPUT_STATUS_1, 0U);
	stop_ret = audio_codec_stop(PARENT, AUDIO_DAI_DIR_TX);
	mark = cs47l63_emul_xfer_count(EMUL);
	zassert_ok(k_mutex_unlock(&chip->lock));

	zassert_not_equal(busy & (uint32_t)K_WORK_RUNNING, 0U, "the premise: the check is running");
	zassert_ok(stop_ret);
	(void)k_msleep((int32_t)CHECK_FINISH_WAIT_MS);

	zassert_equal(k_work_delayable_busy_get(&chip->start_check), 0, "the check has finished");
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark, "and found no output to confirm");
}

static int s_child_ret;

static void configure_child_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	s_child_ret = cs47l63_test_configure(CHILD, AUDIO_ROUTE_PLAYBACK);
}

static ZTEST(cs47l63_dual, test_concurrent_configures_bring_the_part_up_once)
{
	static K_THREAD_STACK_DEFINE(s_child_stack, CHILD_STACK_SIZE);
	static struct k_thread s_child_thread;

	s_child_ret = -EAGAIN;
	(void)k_thread_create(&s_child_thread, s_child_stack, K_THREAD_STACK_SIZEOF(s_child_stack),
			      configure_child_entry, NULL, NULL, NULL,
			      k_thread_priority_get(k_current_get()), 0U, K_NO_WAIT);

	zassert_ok(cs47l63_test_configure(PARENT, AUDIO_ROUTE_PLAYBACK));
	zassert_ok(k_thread_join(&s_child_thread, K_FOREVER));

	zassert_ok(s_child_ret);
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_IRQ1_EINT_2), 1U,
		      "one bring-up, not one per port");
	zassert_equal(dual_chip(PARENT)->configured, BIT_MASK(CS47L63_ASP_COUNT),
		      "both ports configured");
}

static ZTEST(cs47l63_dual, test_output_level_reaches_an_output_only_the_child_plays)
{
	audio_property_value_t vol = {.vol = VOL_M10DB};
	uint32_t val;

	clocks_up(EMUL);
	configure_both(AUDIO_ROUTE_PLAYBACK);
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	(void)k_msleep((int32_t)START_WAIT_MS);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1) & CS47L63_OUT1L_MUTE, 0U,
		      "the premise: the start check confirmed the child's output");
	assert_inputs_silent(EMUL, CS47L63_OUT1L_INPUT1);

	zassert_ok(audio_codec_set_property(PARENT, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL,
					    vol));

	val = cs47l63_emul_get_reg(EMUL, CS47L63_OUT1L_VOLUME_1);
	zassert_equal(val & OUT1L_VOL_MASK, VOL_CODE_M10DB);
	zassert_equal(val & CS47L63_OUT1L_MUTE, 0U);
}

static ZTEST(cs47l63_dual, test_input_front_end_stays_up_for_the_other_port)
{
	zassert_ok(cs47l63_test_configure(SWAP_PARENT, AUDIO_ROUTE_CAPTURE));
	zassert_ok(cs47l63_test_configure(SWAP_CHILD, AUDIO_ROUTE_CAPTURE));
	zassert_ok(audio_codec_start(SWAP_PARENT, AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_start(SWAP_CHILD, AUDIO_DAI_DIR_RX));

	zassert_ok(audio_codec_stop(SWAP_CHILD, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_get_reg(SWAP_EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH,
		      IN2_EN_BOTH, "the parent still captures from the line pair");
	zassert_equal(cs47l63_emul_get_reg(SWAP_EMUL, ASP2_ENABLES1) & ASP_TX_EN_BOTH,
		      ASP_TX_EN_BOTH);
	zassert_equal(cs47l63_emul_get_reg(SWAP_EMUL, CS47L63_ASP1_ENABLES1) & ASP_TX_EN_BOTH, 0U);

	zassert_ok(audio_codec_stop(SWAP_PARENT, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_get_reg(SWAP_EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, 0U);
}

static ZTEST(cs47l63_dual, test_input_terminal_is_chosen_per_port)
{
	configure_both(AUDIO_ROUTE_CAPTURE);
	zassert_ok(audio_codec_start(PARENT, AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_RX));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_ASP1TX1_INPUT1),
		      MIX_SLOT(CS47L63_MIXER_SRC_IN2L));
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2TX1_INPUT1), MIX_SLOT(CS47L63_MIXER_SRC_IN1L));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN_EN_ALL, IN_EN_ALL);

	zassert_ok(audio_codec_stop(CHILD, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN_EN_ALL, IN2_EN_BOTH,
		      "only the front end nobody uses comes down");
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_MICBIAS_CTRL5) & CS47L63_MICB1B_EN, 0U);

	zassert_ok(audio_codec_stop(PARENT, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, CS47L63_INPUT_CONTROL) & IN2_EN_BOTH, 0U);
}

static ZTEST(cs47l63_dual, test_child_start_refused_until_configured)
{
	uint32_t mark;

	zassert_ok(cs47l63_test_configure(PARENT, AUDIO_ROUTE_PLAYBACK_CAPTURE));
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_equal(-ENOTSUP, cs47l63_test_configure(CHILD, AUDIO_ROUTE_BYPASS));
	zassert_equal(-EINVAL, audio_codec_start(CHILD, AUDIO_DAI_DIR_RX));
	zassert_equal(-EINVAL, audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	audio_codec_start_output(CHILD);
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark, "an unconfigured port touches nothing");
}

static ZTEST(cs47l63_dual, test_child_start_refused_outside_its_route)
{
	uint32_t mark;

	zassert_ok(cs47l63_test_configure(PARENT, AUDIO_ROUTE_PLAYBACK_CAPTURE));
	zassert_ok(cs47l63_test_configure(CHILD, AUDIO_ROUTE_PLAYBACK));
	mark = cs47l63_emul_xfer_count(EMUL);
	zassert_equal(-EINVAL, audio_codec_start(CHILD, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark);

	zassert_ok(cs47l63_test_configure(CHILD, AUDIO_ROUTE_CAPTURE));
	mark = cs47l63_emul_xfer_count(EMUL);
	zassert_equal(-EINVAL, audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	audio_codec_start_output(CHILD);
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark, "no playback on a capture route");
}

static ZTEST(cs47l63_dual, test_child_failed_configure_leaves_nothing_startable)
{
	uint32_t mark;

	configure_both(AUDIO_ROUTE_PLAYBACK_CAPTURE);
	mark = cs47l63_emul_xfer_count(EMUL);
	cs47l63_emul_fail_at(EMUL, ASP2_CONTROL2);

	zassert_equal(-EIO, cs47l63_test_configure(CHILD, AUDIO_ROUTE_PLAYBACK_CAPTURE));
	zassert_equal(cs47l63_emul_fail_hits(EMUL), 1U,
		      "the injection took effect: the child's port was never configured");
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_equal(-EINVAL, audio_codec_start(CHILD, AUDIO_DAI_DIR_RX));
	zassert_equal(-EINVAL, audio_codec_start(CHILD, AUDIO_DAI_DIR_TX));
	audio_codec_start_output(CHILD);
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark, "a half-configured port stays stopped");
}

static ZTEST(cs47l63_dual, test_child_route_input_only_caches_while_stopped)
{
	uint32_t mark;

	configure_both(AUDIO_ROUTE_CAPTURE);
	mark = cs47l63_emul_xfer_count(EMUL);

	zassert_ok(audio_codec_route_input(CHILD, AUDIO_CHANNEL_FRONT_LEFT, PORT_TERMINAL));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), mark, "a stopped input is not written");

	zassert_ok(audio_codec_start(CHILD, AUDIO_DAI_DIR_RX));
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2TX1_INPUT1), MIX_SLOT(CS47L63_MIXER_SRC_IN1L),
		      "the cached route is applied by the start");
	zassert_equal(cs47l63_emul_get_reg(EMUL, ASP2TX2_INPUT1), MIX_SLOT(CS47L63_MIXER_SRC_IN1L),
		      "a mono route reaches both slots");
}
