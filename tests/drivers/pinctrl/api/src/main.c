/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_device.h"

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, pinctrl_configure_pins, const pinctrl_soc_pin_t *, uint8_t, uintptr_t);

/* test device 0 */
#define TEST_DEVICE0 DT_NODELABEL(test_device0)
PINCTRL_DT_DEV_CONFIG_DECLARE(TEST_DEVICE0);
static struct pinctrl_dev_config *pcfg0 = PINCTRL_DT_DEV_CONFIG_GET(TEST_DEVICE0);

/* test device 1 */
#define TEST_DEVICE1 DT_NODELABEL(test_device1)
PINCTRL_DT_DEV_CONFIG_DECLARE(TEST_DEVICE1);
static struct pinctrl_dev_config *pcfg1 = PINCTRL_DT_DEV_CONFIG_GET(TEST_DEVICE1);

/**
 * @brief Test if configuration for device 0 has been stored as expected.
 *
 * Device 0 is also used to verify that certain states are skipped
 * automatically, e.g. "sleep" if CONFIG_PM_DEVICE is not active.
 *
 * This test together with test_config_dev1 is used to verify that the whole
 * set of macros used to define and initialize pin control config from
 * Devicetree works as expected.
 */
ZTEST(pinctrl_api, test_config_dev0)
{
	const struct pinctrl_state *scfg;

	zassert_equal(pcfg0->state_cnt, 1);
#ifdef CONFIG_PINCTRL_STORE_REG
	zassert_equal(pcfg0->reg, 0);
#endif

	scfg = &pcfg0->states[0];
	zassert_equal(scfg->id, PINCTRL_STATE_DEFAULT);
	zassert_equal(TEST_GET_PIN(scfg->pins[0]), 0);
	zassert_equal(TEST_GET_PULL(scfg->pins[0]), TEST_PULL_UP);
	zassert_equal(TEST_GET_PIN(scfg->pins[1]), 1);
	zassert_equal(TEST_GET_PULL(scfg->pins[1]), TEST_PULL_DOWN);
}

/**
 * @brief Test if configuration for device 1 has been stored as expected.
 *
 * Device 1 is also used to verify that custom states can be defined.
 *
 * @see test_config_dev0()
 */
ZTEST(pinctrl_api, test_config_dev1)
{
	const struct pinctrl_state *scfg;

	zassert_equal(pcfg1->state_cnt, 2);
#ifdef CONFIG_PINCTRL_STORE_REG
	zassert_equal(pcfg1->reg, 1);
#endif

	scfg = &pcfg1->states[0];
	zassert_equal(scfg->id, PINCTRL_STATE_DEFAULT);
	zassert_equal(scfg->pin_cnt, 3);
	zassert_equal(TEST_GET_PIN(scfg->pins[0]), 10);
	zassert_equal(TEST_GET_PULL(scfg->pins[0]), TEST_PULL_DISABLE);
	zassert_equal(TEST_GET_PIN(scfg->pins[1]), 11);
	zassert_equal(TEST_GET_PULL(scfg->pins[1]), TEST_PULL_DISABLE);
	zassert_equal(TEST_GET_PIN(scfg->pins[2]), 12);
	zassert_equal(TEST_GET_PULL(scfg->pins[2]), TEST_PULL_DISABLE);

	scfg = &pcfg1->states[1];
	zassert_equal(scfg->id, PINCTRL_STATE_MYSTATE);
	zassert_equal(scfg->pin_cnt, 3);
	zassert_equal(TEST_GET_PIN(scfg->pins[0]), 10);
	zassert_equal(TEST_GET_PULL(scfg->pins[0]), TEST_PULL_DISABLE);
	zassert_equal(TEST_GET_PIN(scfg->pins[1]), 11);
	zassert_equal(TEST_GET_PULL(scfg->pins[1]), TEST_PULL_UP);
	zassert_equal(TEST_GET_PIN(scfg->pins[2]), 12);
	zassert_equal(TEST_GET_PULL(scfg->pins[2]), TEST_PULL_DOWN);
}

/**
 * @brief Test that pinctrl_lookup_state() works as expected
 */
ZTEST(pinctrl_api, test_lookup_state)
{
	int ret;
	const struct pinctrl_state *scfg;

	ret = pinctrl_lookup_state(pcfg0, PINCTRL_STATE_DEFAULT, &scfg);
	zassert_equal(ret, 0);
	zassert_equal_ptr(scfg, &pcfg0->states[0], NULL);

	ret = pinctrl_lookup_state(pcfg0, PINCTRL_STATE_SLEEP, &scfg);
	zassert_equal(ret, -ENOENT);
}

/**
 * @brief Test that pinctrl_apply_state() works as expected.
 */
ZTEST(pinctrl_api, test_apply_state)
{
	zassert_ok(pinctrl_apply_state(pcfg0, PINCTRL_STATE_DEFAULT));
	zassert_equal(1, pinctrl_configure_pins_fake.call_count);
	zassert_equal(pcfg0->states[0].pins, pinctrl_configure_pins_fake.arg0_val);
	zassert_equal(pcfg0->states[0].pin_cnt, pinctrl_configure_pins_fake.arg1_val);
#ifdef CONFIG_PINCTRL_STORE_REG
	zassert_equal(0, pinctrl_configure_pins_fake.arg2_val);
#else
	zassert_equal(PINCTRL_REG_NONE, pinctrl_configure_pins_fake.arg2_val);
#endif
}

/** Test device 0 alternative pins for default state */
PINCTRL_DT_STATE_PINS_DEFINE(DT_PATH(zephyr_user), test_device0_alt_default);
/** Test device 0 alternative pins for sleep state */
PINCTRL_DT_STATE_PINS_DEFINE(DT_PATH(zephyr_user), test_device0_alt_sleep);

/** Test device 0 alternative states. */
static const struct pinctrl_state test_device0_alt[] = {
	PINCTRL_DT_STATE_INIT(test_device0_alt_default, PINCTRL_STATE_DEFAULT),
};

/** Test device 0 invalid alternative states. */
static const struct pinctrl_state test_device0_alt_invalid[] = {
	PINCTRL_DT_STATE_INIT(test_device0_alt_default, PINCTRL_STATE_DEFAULT),
	/* sleep state is skipped (no CONFIG_PM_DEVICE), so it is invalid */
	PINCTRL_DT_STATE_INIT(test_device0_alt_sleep, PINCTRL_STATE_SLEEP),
};

/**
 * @brief This test checks if pinctrl_update_states() works as expected.
 */
ZTEST(pinctrl_api, test_update_states)
{
	int ret;
	const struct pinctrl_state *scfg;

	ret = pinctrl_update_states(pcfg0, test_device0_alt, ARRAY_SIZE(test_device0_alt));
	zassert_equal(ret, 0);

	scfg = &pcfg0->states[0];
	zassert_equal(TEST_GET_PIN(scfg->pins[0]), 2);
	zassert_equal(TEST_GET_PULL(scfg->pins[0]), TEST_PULL_DOWN);
	zassert_equal(TEST_GET_PIN(scfg->pins[1]), 3);
	zassert_equal(TEST_GET_PULL(scfg->pins[1]), TEST_PULL_UP);

	ret = pinctrl_update_states(pcfg0, test_device0_alt_invalid,
				    ARRAY_SIZE(test_device0_alt_invalid));
	zassert_equal(ret, -EINVAL);
}

static void pinctrl_api_before(void *f)
{
	ARG_UNUSED(f);
	RESET_FAKE(pinctrl_configure_pins);
	FFF_RESET_HISTORY();
}

ZTEST_SUITE(pinctrl_api, NULL, NULL, pinctrl_api_before, NULL, NULL);
