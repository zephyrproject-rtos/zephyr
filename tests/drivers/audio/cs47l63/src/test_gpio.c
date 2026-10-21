/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_gpio.c
 * @brief GPIO1-12 through the driver's private pin interface
 *
 * Register addresses and GPIOn_CTRL1 words are written out from DS1249F2 tables 4-56 and 4-57,
 * not derived from the driver's macros, so an offset error cannot cancel out.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_gpio))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_gpio))

#define CODEC_ASP2 DEVICE_DT_GET(DT_NODELABEL(cs47l63_gpio_asp2))
#define EMUL_ASP2  EMUL_DT_GET(DT_NODELABEL(cs47l63_gpio_asp2))

#define CODEC_DUAL DEVICE_DT_GET(DT_NODELABEL(cs47l63_gpio_dual))
#define EMUL_DUAL  EMUL_DT_GET(DT_NODELABEL(cs47l63_gpio_dual))
#define ASP2_DUAL  DEVICE_DT_GET(DT_NODELABEL(cs47l63_gpio_dual_asp2))

/* GPIOn_CTRL1 is 4 bytes apart from R3080 (GPIO1) to R3124 (GPIO12); n = pin + 1. */
#define CTRL1(pin)  (0x00000C08U + (4U * (pin)))
#define GPIO_STATUS 0x00000C00U

#define PINS 12U

/* GPIOn_CTRL1 words: DIR bit 31, PU 30, PD 29, DRV_STR 24, LVL 15, OP_CFG 14, FN 9:0. */
#define WORD_OUT_HIGH    0x01008001U
#define WORD_OUT_LOW     0x01000001U
#define WORD_OD_HIGH     0x0100C001U
#define WORD_OD_LOW      0x01004001U
#define WORD_IN_FLOAT    0x81000001U
#define WORD_IN_PULL_UP  0xC1000001U
#define WORD_IN_PULL_DN  0xA1000001U
#define WORD_RESET       0xE1000001U

/* GPIO10 is pin 9, GPIO11 pin 10. */
#define PIN_A 9U
#define PIN_B 10U

#define THREAD_STACK_SIZE 2048U

static struct cs47l63_chip *chip_of(const struct device *dev)
{
	return ((const struct cs47l63_config *)dev->config)->chip;
}

static uint32_t last_write(const struct emul *target, uint32_t addr)
{
	const uint32_t count = cs47l63_emul_write_count(target, addr);
	uint32_t val = 0U;

	zassert_true(count > 0U, "no write to 0x%05x", addr);
	zassert_true(cs47l63_emul_nth_write(target, addr, count - 1U, &val));

	return val;
}

static void disconnect_all(const struct device *dev)
{
	for (gpio_pin_t pin = 0U; pin < PINS; pin++) {
		const int ret = cs47l63_gpio_pin_configure(dev, pin, GPIO_DISCONNECTED);

		zassert_true((ret == 0) || (ret == -EBUSY), "pin %u: %d", pin, ret);
	}
}

/* The chip state outlives a reset of the emulator, so each case starts from no pin in use and
 * an empty log.
 */
static void gpio_before_each(const struct device *dev, const struct emul *target)
{
	cs47l63_emul_reset(target);
	disconnect_all(dev);
	cs47l63_emul_reset(target);
	chip_of(dev)->configured = 0U;
}

static void main_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	gpio_before_each(CODEC, EMUL);
}

static void asp2_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	gpio_before_each(CODEC_ASP2, EMUL_ASP2);
}

static void dual_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	gpio_before_each(CODEC_DUAL, EMUL_DUAL);
}

/* A failed configure() clears booted, so the next pin call has to bring the part up again. */
static void force_unbooted(void)
{
	cs47l63_emul_reset(EMUL);
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_2, 0U);
	zassert_equal(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK), -ETIMEDOUT);
	zassert_false(chip_of(CODEC)->booted);
	cs47l63_emul_reset(EMUL);
}

ZTEST_SUITE(cs47l63_gpio, NULL, NULL, main_before_each, NULL, NULL);
ZTEST_SUITE(cs47l63_gpio_asp2, NULL, NULL, asp2_before_each, NULL, NULL);
ZTEST_SUITE(cs47l63_gpio_dual, NULL, NULL, dual_before_each, NULL, NULL);

static ZTEST(cs47l63_gpio, test_output_is_one_write_of_the_whole_word)
{
	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_OUTPUT_HIGH));

	zassert_equal(cs47l63_emul_write_count(EMUL, 0x00000C2CU), 1U);
	zassert_equal(last_write(EMUL, 0x00000C2CU), WORD_OUT_HIGH);
	zassert_equal(cs47l63_emul_read_count(EMUL, 0x00000C2CU), 0U, "LVL cannot be read back");
}

static ZTEST(cs47l63_gpio, test_input_pull_flags)
{
	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_INPUT));
	zassert_equal(last_write(EMUL, CTRL1(PIN_A)), WORD_IN_FLOAT);

	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_INPUT | GPIO_PULL_UP));
	zassert_equal(last_write(EMUL, CTRL1(PIN_A)), WORD_IN_PULL_UP);

	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_INPUT | GPIO_PULL_DOWN));
	zassert_equal(last_write(EMUL, CTRL1(PIN_A)), WORD_IN_PULL_DN);
}

static ZTEST(cs47l63_gpio, test_open_drain_sets_op_cfg)
{
	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_OUTPUT_HIGH | GPIO_OPEN_DRAIN));
	zassert_equal(last_write(EMUL, CTRL1(PIN_A)), WORD_OD_HIGH);

	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_OUTPUT_LOW | GPIO_OPEN_DRAIN));
	zassert_equal(last_write(EMUL, CTRL1(PIN_A)), WORD_OD_LOW);
}

static ZTEST(cs47l63_gpio, test_input_level_comes_from_status)
{
	gpio_port_value_t val = 0U;

	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_B, GPIO_INPUT));
	cs47l63_emul_set_reg(EMUL, GPIO_STATUS, BIT(PIN_B));

	zassert_ok(cs47l63_gpio_port_get_raw(CODEC, &val));
	zassert_equal(val, BIT(PIN_B), "only pin 10 is high");
	zassert_equal(cs47l63_emul_write_count(EMUL, GPIO_STATUS), 0U);
}

static ZTEST(cs47l63_gpio, test_output_level_comes_from_the_shadow)
{
	gpio_port_value_t val = 0U;

	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_OUTPUT_HIGH));
	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_B, GPIO_OUTPUT_LOW));

	cs47l63_emul_set_reg(EMUL, GPIO_STATUS, BIT(PIN_B));
	zassert_ok(cs47l63_gpio_port_get_raw(CODEC, &val));
	zassert_equal(val, BIT(PIN_A), "the status bits of an output pin are ignored");
}

static ZTEST(cs47l63_gpio, test_set_clear_toggle_masked)
{
	const gpio_pin_t low = 8U;
	gpio_port_value_t val = 0U;
	uint32_t writes_low;
	uint32_t writes_a;
	uint32_t writes_b;

	zassert_ok(cs47l63_gpio_pin_configure(CODEC, low, GPIO_OUTPUT_LOW));
	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_OUTPUT_LOW));
	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_B, GPIO_INPUT));
	writes_low = cs47l63_emul_write_count(EMUL, CTRL1(low));
	writes_a = cs47l63_emul_write_count(EMUL, CTRL1(PIN_A));
	writes_b = cs47l63_emul_write_count(EMUL, CTRL1(PIN_B));

	/* Pin 8 changes, pin 9 is already low, pin 10 is an input. */
	zassert_ok(cs47l63_gpio_port_set_masked_raw(CODEC, BIT(low) | BIT(PIN_A) | BIT(PIN_B),
						    BIT(low) | BIT(PIN_B)));
	zassert_equal(cs47l63_emul_write_count(EMUL, CTRL1(low)), writes_low + 1U);
	zassert_equal(last_write(EMUL, CTRL1(low)), WORD_OUT_HIGH);
	zassert_equal(cs47l63_emul_write_count(EMUL, CTRL1(PIN_A)), writes_a);
	zassert_equal(cs47l63_emul_write_count(EMUL, CTRL1(PIN_B)), writes_b);

	zassert_ok(cs47l63_gpio_port_set_masked_raw(CODEC, BIT(low), 0U));
	zassert_equal(last_write(EMUL, CTRL1(low)), WORD_OUT_LOW);

	zassert_ok(cs47l63_gpio_port_toggle_bits(CODEC, BIT(low) | BIT(PIN_A) | BIT(PIN_B)));
	zassert_equal(last_write(EMUL, CTRL1(low)), WORD_OUT_HIGH);
	zassert_equal(last_write(EMUL, CTRL1(PIN_A)), WORD_OUT_HIGH);
	zassert_equal(cs47l63_emul_write_count(EMUL, CTRL1(PIN_B)), writes_b);

	zassert_ok(cs47l63_gpio_port_toggle_bits(CODEC, BIT(low) | BIT(PIN_A) | BIT(PIN_B)));
	zassert_equal(last_write(EMUL, CTRL1(low)), WORD_OUT_LOW, "two toggles are the identity");
	zassert_equal(last_write(EMUL, CTRL1(PIN_A)), WORD_OUT_LOW);

	zassert_ok(cs47l63_gpio_port_get_raw(CODEC, &val));
	zassert_equal(val, 0U);
}

static ZTEST(cs47l63_gpio, test_failed_write_leaves_state)
{
	gpio_port_value_t val = 0U;

	cs47l63_emul_fail_write_at(EMUL, 0x00000C2CU);
	zassert_equal(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_OUTPUT_LOW), -EIO);
	zassert_equal(cs47l63_emul_fail_hits(EMUL), 1U, "the premise: the write was refused");

	/* Were the pin known as an output, the shadow would answer 0. */
	cs47l63_emul_set_reg(EMUL, GPIO_STATUS, BIT(PIN_A));
	zassert_ok(cs47l63_gpio_port_get_raw(CODEC, &val));
	zassert_equal(val, BIT(PIN_A), "the pin is read from the status register");

	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK));
	zassert_equal(cs47l63_emul_fail_hits(EMUL), 1U, "a replay did not write the pin");
}

static ZTEST(cs47l63_gpio, test_flags_refused_without_traffic)
{
	static const struct {
		gpio_pin_t pin;
		gpio_flags_t flags;
		int err;
	} k_refused[] = {
		{PIN_A, GPIO_OUTPUT | GPIO_OPEN_SOURCE, -ENOTSUP},
		{PIN_A, GPIO_INPUT | GPIO_OUTPUT, -ENOTSUP},
		{PIN_A, GPIO_INPUT | GPIO_PULL_UP | GPIO_PULL_DOWN, -ENOTSUP},
		{PIN_A, GPIO_INPUT | GPIO_INT_ENABLE, -ENOTSUP},
		{PINS, GPIO_INPUT, -EINVAL},
	};

	for (size_t i = 0U; i < ARRAY_SIZE(k_refused); i++) {
		const uint32_t before = cs47l63_emul_xfer_count(EMUL);

		zassert_equal(cs47l63_gpio_pin_configure(CODEC, k_refused[i].pin,
							 k_refused[i].flags),
			      k_refused[i].err, "case %zu", i);
		zassert_equal(cs47l63_emul_xfer_count(EMUL), before, "case %zu moved the bus", i);
	}
}

static ZTEST(cs47l63_gpio, test_disconnected_restores_reset_word)
{
	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_OUTPUT_HIGH));
	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_DISCONNECTED));
	zassert_equal(last_write(EMUL, CTRL1(PIN_A)), WORD_RESET);

	cs47l63_emul_reset(EMUL);
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK));
	zassert_equal(cs47l63_emul_write_count(EMUL, CTRL1(PIN_A)), 0U,
		      "a disconnected pin is not written back");
}

static ZTEST(cs47l63_gpio, test_first_use_boots_the_part)
{
	int boot_done;
	int devid;
	int ctrl;

	force_unbooted();
	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_OUTPUT_HIGH));
	zassert_true(chip_of(CODEC)->booted);

	boot_done = cs47l63_emul_read_index(EMUL, CS47L63_IRQ1_EINT_2, 0U);
	devid = cs47l63_emul_read_index(EMUL, CS47L63_DEVID, 0U);
	ctrl = cs47l63_emul_write_index(EMUL, CTRL1(PIN_A), 0U);
	zassert_true(boot_done >= 0, "boot-done is polled");
	zassert_true(devid > boot_done, "the part is identified once boot-done asserted");
	zassert_true(ctrl > devid, "and only then is the pin written");

	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_B, GPIO_INPUT));
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_DEVID), 1U, "booted once");
}

static ZTEST(cs47l63_gpio, test_first_use_on_a_part_that_never_boots)
{
	force_unbooted();
	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_2, 0U);

	zassert_equal(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_OUTPUT_HIGH), -ETIMEDOUT);
	zassert_equal(cs47l63_emul_write_count(EMUL, CTRL1(PIN_A)), 0U);
	zassert_false(chip_of(CODEC)->booted);
}

static ZTEST(cs47l63_gpio, test_failed_configure_makes_the_next_pin_call_boot_again)
{
	cs47l63_emul_reset(EMUL);
	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK));
	zassert_true(chip_of(CODEC)->booted);

	cs47l63_emul_set_reg(EMUL, CS47L63_IRQ1_EINT_2, 0U);
	zassert_equal(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK), -ETIMEDOUT);
	cs47l63_emul_reset(EMUL);

	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_INPUT));
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_DEVID), 1U,
		      "hw_reset() cleared booted, so the part is brought up again");
}

static ZTEST(cs47l63_gpio, test_replay_failure_fails_bringup)
{
	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_OUTPUT_HIGH));

	cs47l63_emul_reset(EMUL);
	cs47l63_emul_fail_write_at(EMUL, CTRL1(PIN_A));
	zassert_equal(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK), -EIO);
	zassert_false(chip_of(CODEC)->booted);
	zassert_equal(cs47l63_emul_write_count(EMUL, CS47L63_SAMPLE_RATE1), 0U,
		      "the bring-up stopped at the failed replay");

	cs47l63_emul_reset(EMUL);
	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_B, GPIO_INPUT));
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_DEVID), 1U, "the part boots again");
	zassert_equal(last_write(EMUL, CTRL1(PIN_A)), WORD_OUT_HIGH, "and the pin is written back");
}

static ZTEST(cs47l63_gpio, test_output_level_survives_output_reconfigure)
{
	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_OUTPUT_HIGH));

	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_OUTPUT | GPIO_OPEN_DRAIN));
	zassert_equal(last_write(EMUL, CTRL1(PIN_A)), WORD_OD_HIGH);

	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_INPUT));
	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_OUTPUT));
	zassert_equal(last_write(EMUL, CTRL1(PIN_A)), WORD_OUT_LOW, "an input carries no level");
}

static ZTEST(cs47l63_gpio, test_port_op_stops_at_first_failure)
{
	const gpio_pin_t pins[] = {8U, 9U, 10U};
	const gpio_port_pins_t all = BIT(8) | BIT(9) | BIT(10);
	gpio_port_value_t val = 0U;
	uint32_t writes_third;

	for (size_t i = 0U; i < ARRAY_SIZE(pins); i++) {
		zassert_ok(cs47l63_gpio_pin_configure(CODEC, pins[i], GPIO_OUTPUT_LOW));
	}
	writes_third = cs47l63_emul_write_count(EMUL, CTRL1(10U));

	cs47l63_emul_fail_write_at(EMUL, CTRL1(9U));
	zassert_equal(cs47l63_gpio_port_set_masked_raw(CODEC, all, all), -EIO);

	zassert_equal(last_write(EMUL, CTRL1(8U)), WORD_OUT_HIGH);
	zassert_equal(cs47l63_emul_write_count(EMUL, CTRL1(10U)), writes_third, "pin 10 untouched");

	zassert_ok(cs47l63_gpio_port_get_raw(CODEC, &val));
	zassert_equal(val, BIT(8), "pin 8 changed, the others did not");
}

static ZTEST(cs47l63_gpio, test_audio_configure_replays_the_pins)
{
	int devid;
	int key_lock;

	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_OUTPUT_HIGH));
	zassert_ok(cs47l63_gpio_pin_configure(CODEC, PIN_B, GPIO_INPUT | GPIO_PULL_UP));

	cs47l63_emul_reset(EMUL);
	cs47l63_emul_set_reg(EMUL, CTRL1(PIN_A), WORD_RESET);
	cs47l63_emul_set_reg(EMUL, CTRL1(PIN_B), WORD_RESET);

	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK));

	zassert_equal(cs47l63_emul_get_reg(EMUL, CTRL1(PIN_A)), WORD_OUT_HIGH);
	zassert_equal(cs47l63_emul_get_reg(EMUL, CTRL1(PIN_B)), WORD_IN_PULL_UP);
	zassert_equal(cs47l63_emul_write_count(EMUL, CTRL1(PIN_A)), 1U);
	zassert_equal(cs47l63_emul_write_count(EMUL, CTRL1(PIN_B)), 1U);
	zassert_equal(cs47l63_emul_write_count(EMUL, CTRL1(11U)), 0U, "a pin never set up");

	devid = cs47l63_emul_read_index(EMUL, CS47L63_DEVID, 0U);
	key_lock = cs47l63_emul_write_index(EMUL, CS47L63_USER_KEY_CTRL,
					    (2U * CS47L63_KEY_CODES) - 1U);
	zassert_true(key_lock > devid, "the premise: the trim block was written");
	zassert_true(cs47l63_emul_write_index(EMUL, CTRL1(PIN_A), 0U) > key_lock,
		     "the pins come back after the trim");
	zassert_true(cs47l63_emul_write_index(EMUL, CTRL1(PIN_B), 0U) > key_lock);
}

static ZTEST(cs47l63_gpio_dual, test_shared_configure_does_not_replay)
{
	static const uint32_t k_asp2_pads[] = {0x00000C18U, 0x00000C1CU, 0x00000C20U, 0x00000C24U};
	struct cs47l63_emul_xfer xfer;
	uint32_t mark;

	for (gpio_pin_t pin = 8U; pin < PINS; pin++) {
		zassert_ok(cs47l63_gpio_pin_configure(CODEC_DUAL, pin, GPIO_OUTPUT_HIGH));
	}
	zassert_ok(cs47l63_test_configure(CODEC_DUAL, AUDIO_ROUTE_PLAYBACK));
	mark = cs47l63_emul_xfer_count(EMUL_DUAL);

	zassert_ok(cs47l63_test_configure(ASP2_DUAL, AUDIO_ROUTE_PLAYBACK));
	zassert_true(cs47l63_emul_xfer_count(EMUL_DUAL) > mark, "the premise: the join wrote");

	for (uint32_t i = mark; cs47l63_emul_xfer_get(EMUL_DUAL, i, &xfer); i++) {
		bool pad = false;

		if (!xfer.write || (xfer.addr < CTRL1(0U)) || (xfer.addr > CTRL1(PINS - 1U))) {
			continue;
		}
		for (size_t p = 0U; p < ARRAY_SIZE(k_asp2_pads); p++) {
			pad = pad || (xfer.addr == k_asp2_pads[p]);
		}
		zassert_true(pad, "0x%05x written by a join that must not replay", xfer.addr);
	}
}

static ZTEST(cs47l63_gpio, test_asp_pads_are_refused)
{
	for (gpio_pin_t pin = 0U; pin < PINS; pin++) {
		const int ret = cs47l63_gpio_pin_configure(CODEC, pin, GPIO_INPUT);

		zassert_equal(ret, (pin < 4U) ? -EBUSY : 0, "pin %u: %d", pin, ret);
	}
}

static ZTEST(cs47l63_gpio_asp2, test_asp_pads_are_refused)
{
	static const uint32_t k_asp2_pads[] = {0x00000C18U, 0x00000C1CU, 0x00000C20U, 0x00000C24U};

	for (gpio_pin_t pin = 0U; pin < PINS; pin++) {
		const bool pad = (pin >= 4U) && (pin < 8U);
		const int ret = cs47l63_gpio_pin_configure(CODEC_ASP2, pin, GPIO_INPUT);

		zassert_equal(ret, pad ? -EBUSY : 0, "pin %u: %d", pin, ret);
	}
	for (size_t i = 0U; i < ARRAY_SIZE(k_asp2_pads); i++) {
		zassert_equal(cs47l63_emul_write_count(EMUL_ASP2, k_asp2_pads[i]), 0U);
		zassert_equal(cs47l63_emul_read_count(EMUL_ASP2, k_asp2_pads[i]), 0U);
	}
}

static ZTEST(cs47l63_gpio_dual, test_asp_pads_are_refused)
{
	for (gpio_pin_t pin = 0U; pin < PINS; pin++) {
		const int ret = cs47l63_gpio_pin_configure(CODEC_DUAL, pin, GPIO_INPUT);

		zassert_equal(ret, (pin < 8U) ? -EBUSY : 0, "pin %u: %d", pin, ret);
	}
}

static K_THREAD_STACK_DEFINE(s_pin_stack, THREAD_STACK_SIZE);
static struct k_thread s_pin_thread;
static volatile bool s_pin_waiting;
static int s_pin_ret;

static void pin_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	s_pin_waiting = true;
	s_pin_ret = cs47l63_gpio_pin_configure(CODEC, PIN_A, GPIO_OUTPUT_HIGH);
}

/* Runs inside a write of configure(), which holds the part's lock. */
static void start_pin_thread(void *arg)
{
	ARG_UNUSED(arg);

	(void)k_thread_create(&s_pin_thread, s_pin_stack, K_THREAD_STACK_SIZEOF(s_pin_stack),
			      pin_thread_entry, NULL, NULL, NULL,
			      k_thread_priority_get(k_current_get()), 0U, K_NO_WAIT);
	/* Let it run now, into the lock configure() holds, rather than after configure(). */
	k_yield();
}

static ZTEST(cs47l63_gpio, test_gpio_call_waits_for_configure)
{
	struct cs47l63_emul_xfer last;
	uint32_t count;

	s_pin_waiting = false;
	s_pin_ret = -EAGAIN;
	cs47l63_emul_once_on_write(EMUL, CS47L63_USER_KEY_CTRL, start_pin_thread, NULL);

	zassert_ok(cs47l63_test_configure(CODEC, AUDIO_ROUTE_PLAYBACK));
	zassert_true(s_pin_waiting, "the premise: the other thread ran during configure()");
	zassert_ok(k_thread_join(&s_pin_thread, K_FOREVER));
	zassert_ok(s_pin_ret);

	count = cs47l63_emul_xfer_count(EMUL);
	zassert_true(cs47l63_emul_xfer_get(EMUL, count - 1U, &last));
	zassert_equal(last.addr, CTRL1(PIN_A), "the pin write came after every configure write");
	zassert_equal(cs47l63_emul_write_count(EMUL, CTRL1(PIN_A)), 1U);
	zassert_equal(cs47l63_emul_read_count(EMUL, CS47L63_DEVID), 1U,
		      "the pin call did not bring the part up in the middle of configure()");
}

static ZTEST(cs47l63_gpio_dual, test_gpio_child_does_not_disturb_audio)
{
	struct cs47l63_chip *chip = chip_of(CODEC_DUAL);

	zassert_true(device_is_ready(ASP2_DUAL));
	zassert_ok(cs47l63_test_configure(CODEC_DUAL, AUDIO_ROUTE_PLAYBACK));
	zassert_ok(cs47l63_test_configure(ASP2_DUAL, AUDIO_ROUTE_PLAYBACK));

	zassert_equal(chip->configured, BIT_MASK(CS47L63_ASP_COUNT), "both ports configured");
	for (size_t i = 0U; i < ARRAY_SIZE(chip->ports); i++) {
		zassert_not_null(chip->ports[i]);
	}
}
