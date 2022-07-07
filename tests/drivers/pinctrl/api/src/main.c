/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <ztest.h>

#include "test_device.h"

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
static void test_config_dev0(void)
{
	const struct pinctrl_state *scfg;

	zassert_equal(pcfg0->state_cnt, 1, NULL);
#ifdef CONFIG_PINCTRL_STORE_REG
	zassert_equal(pcfg0->reg, 0, NULL);
#endif

	scfg = &pcfg0->states[0];
	zassert_equal(scfg->id, PINCTRL_STATE_DEFAULT, NULL);
	zassert_equal(TEST_GET_PIN(scfg->pins[0]), 0, NULL);
	zassert_equal(TEST_GET_PULL(scfg->pins[0]), TEST_PULL_UP, NULL);
	zassert_equal(TEST_GET_PIN(scfg->pins[1]), 1, NULL);
	zassert_equal(TEST_GET_PULL(scfg->pins[1]), TEST_PULL_DOWN, NULL);
}

/**
 * @brief Test if configuration for device 1 has been stored as expected.
 *
 * Device 1 is also used to verify that custom states can be defined.
 *
 * @see test_config_dev0()
 */
static void test_config_dev1(void)
{
	const struct pinctrl_state *scfg;

	zassert_equal(pcfg1->state_cnt, 2, NULL);
#ifdef CONFIG_PINCTRL_STORE_REG
	zassert_equal(pcfg1->reg, 1, NULL);
#endif

	scfg = &pcfg1->states[0];
	zassert_equal(scfg->id, PINCTRL_STATE_DEFAULT, NULL);
	zassert_equal(scfg->pin_cnt, 3, NULL);
	zassert_equal(TEST_GET_PIN(scfg->pins[0]), 10, NULL);
	zassert_equal(TEST_GET_PULL(scfg->pins[0]), TEST_PULL_DISABLE, NULL);
	zassert_equal(TEST_GET_PIN(scfg->pins[1]), 11, NULL);
	zassert_equal(TEST_GET_PULL(scfg->pins[1]), TEST_PULL_DISABLE, NULL);
	zassert_equal(TEST_GET_PIN(scfg->pins[2]), 12, NULL);
	zassert_equal(TEST_GET_PULL(scfg->pins[2]), TEST_PULL_DISABLE, NULL);

	scfg = &pcfg1->states[1];
	zassert_equal(scfg->id, PINCTRL_STATE_MYSTATE, NULL);
	zassert_equal(scfg->pin_cnt, 3, NULL);
	zassert_equal(TEST_GET_PIN(scfg->pins[0]), 10, NULL);
	zassert_equal(TEST_GET_PULL(scfg->pins[0]), TEST_PULL_DISABLE, NULL);
	zassert_equal(TEST_GET_PIN(scfg->pins[1]), 11, NULL);
	zassert_equal(TEST_GET_PULL(scfg->pins[1]), TEST_PULL_UP, NULL);
	zassert_equal(TEST_GET_PIN(scfg->pins[2]), 12, NULL);
	zassert_equal(TEST_GET_PULL(scfg->pins[2]), TEST_PULL_DOWN, NULL);
}

/**
 * @brief Test that pinctrl_lookup_state() works as expected
 */
static void test_lookup_state(void)
{
	int ret;
	const struct pinctrl_state *scfg;

	ret = pinctrl_lookup_state(pcfg0, PINCTRL_STATE_DEFAULT, &scfg);
	zassert_equal(ret, 0, NULL);
	zassert_equal_ptr(scfg, &pcfg0->states[0], NULL);

	ret = pinctrl_lookup_state(pcfg0, PINCTRL_STATE_SLEEP, &scfg);
	zassert_equal(ret, -ENOENT, NULL);
}

/**
 * @brief Test that pinctrl_apply_state() works as expected.
 */
static void test_apply_state(void)
{
	int ret;

	ztest_expect_data(pinctrl_configure_pins, pins, pcfg0->states[0].pins);
	ztest_expect_value(pinctrl_configure_pins, pin_cnt,
			   pcfg0->states[0].pin_cnt);
#ifdef CONFIG_PINCTRL_STORE_REG
	ztest_expect_value(pinctrl_configure_pins, reg, 0);
#else
	ztest_expect_value(pinctrl_configure_pins, reg, PINCTRL_REG_NONE);
#endif

	ret = pinctrl_apply_state(pcfg0, PINCTRL_STATE_DEFAULT);
	zassert_equal(ret, 0, NULL);
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
static void test_update_states(void)
{
	int ret;
	const struct pinctrl_state *scfg;

	ret = pinctrl_update_states(pcfg0, test_device0_alt,
				    ARRAY_SIZE(test_device0_alt));
	zassert_equal(ret, 0, NULL);

	scfg = &pcfg0->states[0];
	zassert_equal(TEST_GET_PIN(scfg->pins[0]), 2, NULL);
	zassert_equal(TEST_GET_PULL(scfg->pins[0]), TEST_PULL_DOWN, NULL);
	zassert_equal(TEST_GET_PIN(scfg->pins[1]), 3, NULL);
	zassert_equal(TEST_GET_PULL(scfg->pins[1]), TEST_PULL_UP, NULL);

	ret = pinctrl_update_states(pcfg0, test_device0_alt_invalid,
				    ARRAY_SIZE(test_device0_alt_invalid));
	zassert_equal(ret, -EINVAL, NULL);
}

void test_main(void)
{
	ztest_test_suite(pinctrl_api,
			 ztest_unit_test(test_config_dev0),
			 ztest_unit_test(test_config_dev1),
			 ztest_unit_test(test_lookup_state),
			 ztest_unit_test(test_apply_state),
			 ztest_unit_test(test_update_states));
	ztest_run_test_suite(pinctrl_api);
}
