/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Interrupt demux, power button and GPIO of the TPS65214/TPS65215 against
 * the register emulator. Expected behaviour is taken from SLVSHK7A and
 * SLVSHQ9.
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mfd/tps6521x.h>
#include <zephyr/input/input.h>
#include <zephyr/ztest.h>

#include "emul_tps6521x.h"

#define PMIC_NODE   DT_NODELABEL(pmic)
#define IS_TPS65215 DT_NODE_HAS_COMPAT(PMIC_NODE, ti_tps65215)

#define REG_GENERAL_CONFIG 0x1eU
#define REG_MFP_1_CONFIG   0x1fU
#define REG_MFP_2_CONFIG   0x20U
#define REG_MASK_CONFIG    0x25U
#define REG_MFP_CTRL       0x29U
#define REG_INT_LDO_2      0x2cU
#define REG_INT_LDO_1      0x2dU
#define REG_INT_BUCK_3     0x2eU
#define REG_INT_BUCK_1_2   0x2fU
#define REG_INT_SYSTEM     0x30U
#define REG_INT_PB         0x33U

#define PB_FALL   BIT(0)
#define PB_RISE   BIT(1)
#define PB_STATUS BIT(2)

/* emulator defaults, see emul_tps6521x_init() */
#define GENERAL_CONFIG_DEFAULT (BIT(5) | BIT(1))
#define MFP_2_CONFIG_DEFAULT   (IS_TPS65215 ? 0x10 : 0x00)

#define PIN_GPO1 0U
#define PIN_GPO2 1U
#define PIN_GPIO 2U

/* longer than one pass, shorter than the 100 ms recheck */
#define SETTLE K_MSEC(20)

static const struct emul *emul = EMUL_DT_GET(PMIC_NODE);
static const struct device *const pmic = DEVICE_DT_GET(PMIC_NODE);
static const struct device *const pgpio = DEVICE_DT_GET(DT_NODELABEL(pmic_gpio));
static const struct device *const button = DEVICE_DT_GET(DT_NODELABEL(power_button));

struct hit {
	struct mfd_tps6521x_int_callback cb;
	const struct device *dev;
	int count;
	uint8_t value;
	/* raised from the handler on the first call */
	uint8_t raise_bits;
};

static struct input_event events[8];
static int n_events;

static void input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	if (evt->type == INPUT_EV_KEY && n_events < ARRAY_SIZE(events)) {
		events[n_events] = *evt;
		n_events++;
	}
}
INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(power_button)), input_cb, NULL);

static void hit_handler(const struct device *dev, struct mfd_tps6521x_int_callback *cb,
			uint8_t value)
{
	struct hit *h = CONTAINER_OF(cb, struct hit, cb);

	/* runs in the workqueue: record, assert in the test */
	h->dev = dev;
	h->count++;
	h->value = value;
	if (h->raise_bits != 0U && h->count == 1) {
		emul_tps6521x_raise(emul, h->cb.reg, h->raise_bits);
	}
}

static struct hit *registered[8];

static void hit_add(struct hit *h, uint8_t reg, uint8_t mask)
{
	*h = (struct hit){.cb = {.handler = hit_handler, .reg = reg, .mask = mask}};
	zassert_ok(mfd_tps6521x_add_int_callback(pmic, &h->cb));

	for (size_t i = 0; i < ARRAY_SIZE(registered); i++) {
		if (registered[i] == NULL) {
			registered[i] = h;
			return;
		}
	}
	zassert_unreachable("too many callbacks");
}

static void hit_remove(struct hit *h)
{
	zassert_ok(mfd_tps6521x_remove_int_callback(pmic, &h->cb));

	for (size_t i = 0; i < ARRAY_SIZE(registered); i++) {
		if (registered[i] == h) {
			registered[i] = NULL;
		}
	}
}

static void check_locked(void)
{
	struct emul_tps6521x_stats stats;

	emul_tps6521x_stats(emul, &stats);
	zassert_equal(stats.blocked_writes, 0, "a protected write hit a locked register");
	if (!IS_TPS65215) {
		zassert_true(emul_tps6521x_locked(emul), "registers left unlocked");
	}
}

/* state seen right after boot, before any test touched the emulator */
static struct {
	uint8_t int_pb;
	uint8_t int_buck_3;
	uint8_t mask_config;
	bool nint;
	int events;
} boot;

static void *suite_setup(void)
{
	boot.int_pb = emul_tps6521x_get(emul, REG_INT_PB);
	boot.int_buck_3 = emul_tps6521x_get(emul, REG_INT_BUCK_3);
	boot.mask_config = emul_tps6521x_get(emul, REG_MASK_CONFIG);
	boot.nint = emul_tps6521x_nint_asserted(emul);
	boot.events = n_events;

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	emul_tps6521x_set(emul, REG_GENERAL_CONFIG, GENERAL_CONFIG_DEFAULT);
	emul_tps6521x_set(emul, REG_MFP_1_CONFIG, 0x00);
	emul_tps6521x_set(emul, REG_MFP_2_CONFIG, MFP_2_CONFIG_DEFAULT);
	emul_tps6521x_set(emul, REG_MFP_CTRL, 0x00);
	emul_tps6521x_stick(emul, REG_INT_BUCK_1_2, 0);
	emul_tps6521x_pb_hold(emul, false);
	n_events = 0;
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	for (size_t i = 0; i < ARRAY_SIZE(registered); i++) {
		if (registered[i] != NULL) {
			hit_remove(registered[i]);
		}
	}
	emul_tps6521x_int_reset(emul);
	emul_tps6521x_set(emul, REG_MASK_CONFIG, boot.mask_config);
}

ZTEST_SUITE(mfd_tps6521x, NULL, suite_setup, before, after, NULL);

/* Flags set before boot hold nINT low without an edge (7.3.10): init clears them. */
ZTEST(mfd_tps6521x, test_boot_flags_cleared)
{
	zassert_equal(boot.int_pb & (PB_FALL | PB_RISE), 0, "stale INT_PB edges not cleared");
	zassert_equal(boot.int_pb & PB_STATUS, PB_STATUS, "read-only PB status changed");
	zassert_equal(boot.int_buck_3, 0, "stale INT_BUCK_3 flag not cleared");
	zassert_false(boot.nint, "nINT still asserted after init");
	zassert_equal(boot.events, 0, "the power-on press was reported");
}

ZTEST(mfd_tps6521x, test_callback_validation)
{
	struct mfd_tps6521x_int_callback cb = {.reg = REG_INT_SYSTEM, .mask = 0x01};

	zassert_equal(mfd_tps6521x_add_int_callback(pmic, &cb), -EINVAL, "no handler accepted");
	zassert_equal(mfd_tps6521x_remove_int_callback(pmic, &cb), -EINVAL,
		      "unregistered callback removed");
}

ZTEST(mfd_tps6521x, test_dispatch_and_clear)
{
	static struct hit a;
	static struct hit other_bit;
	static struct hit other_reg;

	hit_add(&a, REG_INT_BUCK_1_2, BIT(1));
	hit_add(&other_bit, REG_INT_BUCK_1_2, BIT(7));
	hit_add(&other_reg, REG_INT_SYSTEM, 0xff);

	emul_tps6521x_raise(emul, REG_INT_BUCK_1_2, BIT(1));
	k_sleep(SETTLE);

	zassert_equal(a.count, 1);
	zassert_equal(a.dev, pmic);
	zassert_equal(a.value, BIT(1));
	zassert_equal(other_bit.count, 0, "callback for another bit ran");
	zassert_equal(other_reg.count, 0, "callback for another register ran");
	zassert_equal(emul_tps6521x_get(emul, REG_INT_BUCK_1_2), 0, "flag not cleared");
	zassert_false(emul_tps6521x_nint_asserted(emul));
	check_locked();

	hit_remove(&a);
	emul_tps6521x_raise(emul, REG_INT_BUCK_1_2, BIT(1));
	k_sleep(SETTLE);
	zassert_equal(a.count, 1, "removed callback ran");
	zassert_equal(emul_tps6521x_get(emul, REG_INT_BUCK_1_2), 0);

	hit_remove(&other_bit);
	hit_remove(&other_reg);
}

/*
 * INT_SOURCE bit 6 is INT_LDO_2 (0x2c) on TPS65215 and reserved on TPS65214,
 * where bit 5 points at INT_LDO_1_2 (0x2d) for both LDOs.
 */
ZTEST(mfd_tps6521x, test_ldo_register_map)
{
	static struct hit ldo2;
	static struct hit ldo1;
	uint8_t reg = IS_TPS65215 ? REG_INT_LDO_2 : REG_INT_LDO_1;
	/* LDO2 short to ground: 0x2c bit 0 on TPS65215, 0x2d bit 3 on TPS65214 */
	uint8_t bit = IS_TPS65215 ? BIT(0) : BIT(3);

	hit_add(&ldo2, REG_INT_LDO_2, 0xff);
	hit_add(&ldo1, REG_INT_LDO_1, 0xff);

	emul_tps6521x_raise(emul, reg, bit);
	k_sleep(SETTLE);

	zassert_equal(IS_TPS65215 ? ldo2.count : ldo1.count, 1, "LDO2 fault not dispatched");
	zassert_equal(IS_TPS65215 ? ldo1.count : ldo2.count, 0, "dispatched to the wrong register");
	zassert_equal(emul_tps6521x_get(emul, reg), 0, "flag not cleared");
	zassert_false(emul_tps6521x_nint_asserted(emul));

	if (IS_TPS65215) {
		/* bit 5 points at INT_LDO_1 (0x2d), LDO1 short to ground */
		emul_tps6521x_raise(emul, REG_INT_LDO_1, BIT(0));
		k_sleep(SETTLE);
		zassert_equal(ldo1.count, 1, "LDO1 fault not dispatched");
		zassert_equal(ldo2.count, 1, "LDO1 fault dispatched to INT_LDO_2");
		zassert_equal(emul_tps6521x_get(emul, REG_INT_LDO_1), 0);
	}

	hit_remove(&ldo2);
	hit_remove(&ldo1);
}

/* A flag nobody watches is cleared too, or nINT would stay low (7.3.10). */
ZTEST(mfd_tps6521x, test_unwatched_flag_cleared)
{
	emul_tps6521x_raise(emul, REG_INT_SYSTEM, BIT(7));
	k_sleep(SETTLE);

	zassert_equal(emul_tps6521x_get(emul, REG_INT_SYSTEM), 0);
	zassert_false(emul_tps6521x_nint_asserted(emul));
	check_locked();
}

/* An event during a handler comes after the clear: nINT falls again. */
ZTEST(mfd_tps6521x, test_event_during_handler)
{
	static struct hit h;

	hit_add(&h, REG_INT_BUCK_3, 0x0f);
	h.raise_bits = BIT(1);

	emul_tps6521x_raise(emul, REG_INT_BUCK_3, BIT(0));
	k_sleep(SETTLE);

	zassert_equal(h.count, 2, "second event lost");
	zassert_equal(h.value, BIT(1));
	zassert_false(emul_tps6521x_nint_asserted(emul));

	hit_remove(&h);
}

/*
 * A flag set between the read and the clear must survive the clear, and is
 * found again because nINT stays low.
 */
ZTEST(mfd_tps6521x, test_flag_after_read_survives)
{
	static struct hit h;

	hit_add(&h, REG_INT_BUCK_1_2, 0xff);
	emul_tps6521x_raise_on_read(emul, REG_INT_BUCK_1_2, BIT(4));

	emul_tps6521x_raise(emul, REG_INT_BUCK_1_2, BIT(0));
	k_sleep(SETTLE);

	zassert_equal(h.count, 1);
	zassert_equal(h.value, BIT(0));
	zassert_equal(emul_tps6521x_get(emul, REG_INT_BUCK_1_2), BIT(4),
		      "a flag that was not read got cleared");
	zassert_true(emul_tps6521x_nint_asserted(emul));

	k_sleep(K_MSEC(150));
	zassert_equal(h.count, 2, "pending flag not picked up again");
	zassert_equal(h.value, BIT(4));
	zassert_false(emul_tps6521x_nint_asserted(emul));

	hit_remove(&h);
}

/* A persisting fault keeps its flag and nINT low (7.3.10): recheck, do not spin. */
ZTEST(mfd_tps6521x, test_persistent_fault)
{
	static struct hit h;
	int count;

	hit_add(&h, REG_INT_BUCK_1_2, BIT(2));
	emul_tps6521x_stick(emul, REG_INT_BUCK_1_2, BIT(2));
	emul_tps6521x_raise(emul, REG_INT_BUCK_1_2, BIT(2));

	/* passes at about 0, 100, 200 and 300 ms */
	k_sleep(K_MSEC(350));
	zassert_within(h.count, 4, 1, "%d passes in 350 ms", h.count);

	emul_tps6521x_stick(emul, REG_INT_BUCK_1_2, 0);
	k_sleep(K_MSEC(150));
	zassert_false(emul_tps6521x_nint_asserted(emul));
	count = h.count;
	k_sleep(K_MSEC(300));
	zassert_equal(h.count, count, "passes go on after the fault went away");

	hit_remove(&h);
}

#ifndef EMUL_TPS6521X_PIN_IS_EN

ZTEST(mfd_tps6521x, test_button_unmasks_pb)
{
	zassert_true(device_is_ready(button));
	zassert_equal(boot.mask_config, BIT(5), "MASK_CONFIG 0x%02x", boot.mask_config);
	check_locked();
}

ZTEST(mfd_tps6521x, test_button_press_release)
{
	emul_tps6521x_pb_hold(emul, true);
	emul_tps6521x_raise(emul, REG_INT_PB, PB_FALL);
	k_sleep(SETTLE);

	zassert_equal(n_events, 1);
	zassert_equal(events[0].code, INPUT_KEY_POWER);
	zassert_equal(events[0].value, 1, "press not reported as 1");
	zassert_true(events[0].sync);
	zassert_equal(emul_tps6521x_get(emul, REG_INT_PB), 0, "edge not cleared");

	emul_tps6521x_pb_hold(emul, false);
	emul_tps6521x_raise(emul, REG_INT_PB, PB_RISE);
	k_sleep(SETTLE);

	zassert_equal(n_events, 2);
	zassert_equal(events[1].code, INPUT_KEY_POWER);
	zassert_equal(events[1].value, 0, "release not reported as 0");
	zassert_equal(emul_tps6521x_get(emul, REG_INT_PB), PB_STATUS);
	zassert_false(emul_tps6521x_nint_asserted(emul));
	check_locked();
}

/* Both edges in one read: the real-time status says which was last. */
ZTEST(mfd_tps6521x, test_button_both_edges)
{
	/* released now: press, then release */
	emul_tps6521x_pb_hold(emul, false);
	emul_tps6521x_raise(emul, REG_INT_PB, PB_FALL | PB_RISE);
	k_sleep(SETTLE);

	zassert_equal(n_events, 2);
	zassert_equal(events[0].value, 1);
	zassert_equal(events[1].value, 0);

	/* held now: release, then press */
	emul_tps6521x_pb_hold(emul, true);
	emul_tps6521x_raise(emul, REG_INT_PB, PB_FALL | PB_RISE);
	k_sleep(SETTLE);

	zassert_equal(n_events, 4);
	zassert_equal(events[2].value, 0);
	zassert_equal(events[3].value, 1);
}

#else /* EMUL_TPS6521X_PIN_IS_EN */

ZTEST(mfd_tps6521x, test_button_refused_on_en_pin)
{
	zassert_false(device_is_ready(button));
	zassert_equal(boot.mask_config, BIT(7) | BIT(5), "MASK_CONFIG changed");
}

#endif /* EMUL_TPS6521X_PIN_IS_EN */

/* Runs first among the GPIO tests: nothing configured yet, nothing written. */
ZTEST(mfd_tps6521x, test_gpio_0_unconfigured_not_written)
{
	zassert_ok(gpio_port_set_masked_raw(pgpio, 0x7, 0x7));
	zassert_equal(emul_tps6521x_get(emul, REG_GENERAL_CONFIG), GENERAL_CONFIG_DEFAULT,
		      "pins written before being configured");
}

ZTEST(mfd_tps6521x, test_gpio_gpo1)
{
	uint8_t other = GENERAL_CONFIG_DEFAULT & ~BIT(PIN_GPO1);

	zassert_ok(gpio_pin_configure(pgpio, PIN_GPO1, GPIO_OUTPUT_HIGH | GPIO_OPEN_DRAIN));
	zassert_equal(emul_tps6521x_get(emul, REG_GENERAL_CONFIG), other | BIT(PIN_GPO1));
	zassert_equal(gpio_pin_get_raw(pgpio, PIN_GPO1), 1);

	zassert_ok(gpio_pin_set_raw(pgpio, PIN_GPO1, 0));
	zassert_equal(emul_tps6521x_get(emul, REG_GENERAL_CONFIG), other);
	zassert_equal(gpio_pin_get_raw(pgpio, PIN_GPO1), 0);

	zassert_ok(gpio_pin_toggle(pgpio, PIN_GPO1));
	zassert_equal(emul_tps6521x_get(emul, REG_GENERAL_CONFIG), other | BIT(PIN_GPO1));
	check_locked();
}

ZTEST(mfd_tps6521x, test_gpio_flags_rejected)
{
	struct emul_tps6521x_stats before;
	struct emul_tps6521x_stats after;

	emul_tps6521x_stats(emul, &before);
	zassert_equal(gpio_pin_configure(pgpio, PIN_GPO1, GPIO_OUTPUT_LOW), -ENOTSUP,
		      "push-pull accepted");
	zassert_equal(gpio_pin_configure(pgpio, PIN_GPO1, GPIO_OUTPUT_LOW | GPIO_OPEN_SOURCE),
		      -ENOTSUP, "open-source accepted");
	zassert_equal(gpio_pin_configure(pgpio, PIN_GPO1,
					 GPIO_OUTPUT_LOW | GPIO_OPEN_DRAIN | GPIO_PULL_UP),
		      -ENOTSUP, "pull-up accepted");
	zassert_equal(gpio_pin_configure(pgpio, PIN_GPO1, GPIO_INPUT), -ENOTSUP);
	emul_tps6521x_stats(emul, &after);
	zassert_equal(after.writes, before.writes, "rejected configuration wrote");
}

/* Pin 1 is GPO2 on TPS65215; on TPS65214 bit 1 is GPIO_CONFIG and pin 1 does not exist. */
ZTEST(mfd_tps6521x, test_gpio_gpo2)
{
	if (IS_TPS65215) {
		zassert_ok(gpio_pin_configure(pgpio, PIN_GPO2, GPIO_OUTPUT_LOW | GPIO_OPEN_DRAIN));
		zassert_equal(emul_tps6521x_get(emul, REG_GENERAL_CONFIG), BIT(5));
		zassert_ok(gpio_pin_set_raw(pgpio, PIN_GPO2, 1));
		zassert_equal(emul_tps6521x_get(emul, REG_GENERAL_CONFIG), BIT(5) | BIT(1));
		return;
	}

	zassert_equal(gpio_pin_configure(pgpio, PIN_GPO2, GPIO_OUTPUT_LOW | GPIO_OPEN_DRAIN),
		      -EINVAL);

	zassert_ok(gpio_pin_configure(pgpio, PIN_GPO1, GPIO_OUTPUT_LOW | GPIO_OPEN_DRAIN));
	zassert_ok(gpio_pin_configure(pgpio, PIN_GPIO, GPIO_OUTPUT_LOW | GPIO_OPEN_DRAIN));
	zassert_ok(gpio_port_set_masked_raw(pgpio, 0x7, 0x0));
	zassert_equal(emul_tps6521x_get(emul, REG_GENERAL_CONFIG), BIT(5) | BIT(1),
		      "GPIO_CONFIG changed by a pin write");
	check_locked();
}

ZTEST(mfd_tps6521x, test_gpio_keeps_level)
{
	emul_tps6521x_set(emul, REG_GENERAL_CONFIG, GENERAL_CONFIG_DEFAULT | BIT(PIN_GPO1));
	zassert_ok(gpio_pin_configure(pgpio, PIN_GPO1, GPIO_OUTPUT | GPIO_OPEN_DRAIN));
	zassert_equal(emul_tps6521x_get(emul, REG_GENERAL_CONFIG),
		      GENERAL_CONFIG_DEFAULT | BIT(PIN_GPO1));
}

/* The GPIO pin reads its real-time level from MFP_CTRL GPIO_STATUS. */
ZTEST(mfd_tps6521x, test_gpio_pin)
{
	if (IS_TPS65215) {
		/* single-device mode: an output, readable */
		zassert_equal(gpio_pin_configure(pgpio, PIN_GPIO, GPIO_INPUT), -ENOTSUP);
		zassert_ok(gpio_pin_configure(pgpio, PIN_GPIO,
					      GPIO_INPUT | GPIO_OUTPUT_HIGH | GPIO_OPEN_DRAIN));
		zassert_equal(emul_tps6521x_get(emul, REG_GENERAL_CONFIG),
			      GENERAL_CONFIG_DEFAULT | BIT(PIN_GPIO));
	} else {
		/* GPIO_CONFIG polarity is ambiguous in SLVSHK7A: never written */
		zassert_ok(gpio_pin_configure(pgpio, PIN_GPIO, GPIO_INPUT));
		zassert_equal(emul_tps6521x_get(emul, REG_GENERAL_CONFIG), GENERAL_CONFIG_DEFAULT,
			      "GPIO_CONFIG written");
	}

	emul_tps6521x_set(emul, REG_MFP_CTRL, BIT(4));
	zassert_equal(gpio_pin_get_raw(pgpio, PIN_GPIO), 1);
	emul_tps6521x_set(emul, REG_MFP_CTRL, (uint8_t)~BIT(4));
	zassert_equal(gpio_pin_get_raw(pgpio, PIN_GPIO), 0, "bits other than GPIO_STATUS read");

	if (!IS_TPS65215) {
		/* output: only GPIO_EN changes */
		zassert_ok(gpio_pin_configure(pgpio, PIN_GPIO, GPIO_OUTPUT_HIGH | GPIO_OPEN_DRAIN));
		zassert_equal(emul_tps6521x_get(emul, REG_GENERAL_CONFIG),
			      BIT(5) | BIT(1) | BIT(2));
	}
	check_locked();
}

ZTEST(mfd_tps6521x, test_gpio_nvm_direction)
{
	struct emul_tps6521x_stats before;
	struct emul_tps6521x_stats after;

	if (IS_TPS65215) {
		ztest_test_skip();
	}

	emul_tps6521x_set(emul, REG_GENERAL_CONFIG, BIT(5));
	emul_tps6521x_stats(emul, &before);
	zassert_equal(gpio_pin_configure(pgpio, PIN_GPIO, GPIO_OUTPUT_HIGH | GPIO_OPEN_DRAIN),
		      -ENOTSUP, "NVM input driven as output");
	zassert_ok(gpio_pin_configure(pgpio, PIN_GPIO, GPIO_INPUT));
	emul_tps6521x_stats(emul, &after);
	zassert_equal(after.writes, before.writes, "input-configured pin written");
	zassert_equal(emul_tps6521x_get(emul, REG_GENERAL_CONFIG), BIT(5));

	emul_tps6521x_set(emul, REG_GENERAL_CONFIG, BIT(5) | BIT(1));
	zassert_ok(gpio_pin_configure(pgpio, PIN_GPIO, GPIO_OUTPUT_HIGH | GPIO_OPEN_DRAIN));
	zassert_equal(emul_tps6521x_get(emul, REG_GENERAL_CONFIG), BIT(5) | BIT(1) | BIT(2));
	check_locked();
}

ZTEST(mfd_tps6521x, test_gpio_function_taken)
{
	struct emul_tps6521x_stats before;
	struct emul_tps6521x_stats after;

	/* TPS65214: GPIO_VSEL_CONFIG = VSEL; TPS65215: MULTI_DEVICE_ENABLE */
	emul_tps6521x_set(emul, REG_MFP_1_CONFIG, BIT(3));
	/* TPS65214: GPO_nWAKEUP_CONFIG = nWAKEUP; TPS65215: MODE_RESET_CONFIG */
	emul_tps6521x_set(emul, REG_MFP_2_CONFIG, MFP_2_CONFIG_DEFAULT | BIT(2));

	emul_tps6521x_stats(emul, &before);
	zassert_equal(gpio_pin_configure(pgpio, PIN_GPIO, GPIO_OUTPUT_LOW | GPIO_OPEN_DRAIN),
		      -ENOTSUP, "GPIO pin driven although taken");
	if (IS_TPS65215) {
		zassert_ok(gpio_pin_configure(pgpio, PIN_GPIO, GPIO_INPUT));
		/* MODE_RESET_CONFIG has nothing to do with GPO1 */
		zassert_ok(gpio_pin_configure(pgpio, PIN_GPO1, GPIO_OUTPUT | GPIO_OPEN_DRAIN));
	} else {
		zassert_equal(gpio_pin_configure(pgpio, PIN_GPIO, GPIO_INPUT), -ENOTSUP,
			      "VSEL pin configured");
		zassert_equal(
			gpio_pin_configure(pgpio, PIN_GPO1, GPIO_OUTPUT_LOW | GPIO_OPEN_DRAIN),
			-ENOTSUP, "nWAKEUP pin driven");
	}
	emul_tps6521x_stats(emul, &after);
	zassert_equal(after.writes, before.writes, "taken pin written");
	zassert_equal(emul_tps6521x_get(emul, REG_GENERAL_CONFIG), GENERAL_CONFIG_DEFAULT);
}
