/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <ztest.h>

/* pin configuration for test device */
#define TEST_DEVICE DT_NODELABEL(test_device)
PINCTRL_DT_DEV_CONFIG_DECLARE(TEST_DEVICE);
static const struct pinctrl_dev_config *pcfg = PINCTRL_DT_DEV_CONFIG_GET(TEST_DEVICE);

static void test_dt_extract(void)
{
	const struct pinctrl_state *scfg;
	pinctrl_soc_pin_t pin;

	zassert_equal(pcfg->state_cnt, 1U, NULL);

	scfg = &pcfg->states[0];

	zassert_equal(scfg->id, PINCTRL_STATE_DEFAULT, NULL);
	zassert_equal(scfg->pin_cnt, 12U, NULL);

	pin = scfg->pins[0];
	zassert_equal(GD32_PORT_GET(pin), 0, NULL);
	zassert_equal(GD32_PIN_GET(pin), 0, NULL);
	zassert_equal(GD32_AF_GET(pin), GD32_AF0, NULL);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE, NULL);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP, NULL);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ, NULL);

	pin = scfg->pins[1];
	zassert_equal(GD32_PORT_GET(pin), 1, NULL);
	zassert_equal(GD32_PIN_GET(pin), 1, NULL);
	zassert_equal(GD32_AF_GET(pin), GD32_AF1, NULL);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE, NULL);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP, NULL);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ, NULL);

	pin = scfg->pins[2];
	zassert_equal(GD32_PORT_GET(pin), 2, NULL);
	zassert_equal(GD32_PIN_GET(pin), 2, NULL);
	zassert_equal(GD32_AF_GET(pin), GD32_AF2, NULL);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE, NULL);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP, NULL);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ, NULL);

	pin = scfg->pins[3];
	zassert_equal(GD32_PORT_GET(pin), 0, NULL);
	zassert_equal(GD32_PIN_GET(pin), 3, NULL);
	zassert_equal(GD32_AF_GET(pin), GD32_AF3, NULL);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE, NULL);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_OD, NULL);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ, NULL);

	pin = scfg->pins[4];
	zassert_equal(GD32_PORT_GET(pin), 1, NULL);
	zassert_equal(GD32_PIN_GET(pin), 4, NULL);
	zassert_equal(GD32_AF_GET(pin), GD32_AF4, NULL);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE, NULL);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP, NULL);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ, NULL);

	pin = scfg->pins[5];
	zassert_equal(GD32_PORT_GET(pin), 2, NULL);
	zassert_equal(GD32_PIN_GET(pin), 5, NULL);
	zassert_equal(GD32_AF_GET(pin), GD32_AF5, NULL);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_PULLUP, NULL);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP, NULL);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ, NULL);

	pin = scfg->pins[6];
	zassert_equal(GD32_PORT_GET(pin), 0, NULL);
	zassert_equal(GD32_PIN_GET(pin), 6, NULL);
	zassert_equal(GD32_AF_GET(pin), GD32_AF6, NULL);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_PULLDOWN, NULL);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP, NULL);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ, NULL);

	pin = scfg->pins[7];
	zassert_equal(GD32_PORT_GET(pin), 1, NULL);
	zassert_equal(GD32_PIN_GET(pin), 7, NULL);
	zassert_equal(GD32_AF_GET(pin), GD32_AF7, NULL);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE, NULL);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP, NULL);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ, NULL);

	pin = scfg->pins[8];
	zassert_equal(GD32_PORT_GET(pin), 2, NULL);
	zassert_equal(GD32_PIN_GET(pin), 8, NULL);
	zassert_equal(GD32_AF_GET(pin), GD32_AF8, NULL);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE, NULL);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP, NULL);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_25MHZ, NULL);

	pin = scfg->pins[9];
	zassert_equal(GD32_PORT_GET(pin), 0, NULL);
	zassert_equal(GD32_PIN_GET(pin), 9, NULL);
	zassert_equal(GD32_AF_GET(pin), GD32_AF9, NULL);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE, NULL);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP, NULL);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_50MHZ, NULL);

	pin = scfg->pins[10];
	zassert_equal(GD32_PORT_GET(pin), 1, NULL);
	zassert_equal(GD32_PIN_GET(pin), 10, NULL);
	zassert_equal(GD32_AF_GET(pin), GD32_AF10, NULL);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE, NULL);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP, NULL);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_200MHZ, NULL);

	pin = scfg->pins[11];
	zassert_equal(GD32_PORT_GET(pin), 2, NULL);
	zassert_equal(GD32_PIN_GET(pin), 11, NULL);
	zassert_equal(GD32_AF_GET(pin), GD32_ANALOG, NULL);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE, NULL);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP, NULL);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ, NULL);
}

void test_main(void)
{
	ztest_test_suite(pinctrl_gd32,
			 ztest_unit_test(test_dt_extract));
	ztest_run_test_suite(pinctrl_gd32);
}
