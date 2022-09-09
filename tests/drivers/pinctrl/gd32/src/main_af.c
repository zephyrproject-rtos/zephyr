/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/ztest.h>

/* pin configuration for test device */
#define TEST_DEVICE DT_NODELABEL(test_device)
PINCTRL_DT_DEV_CONFIG_DECLARE(TEST_DEVICE);
static const struct pinctrl_dev_config *pcfg = PINCTRL_DT_DEV_CONFIG_GET(TEST_DEVICE);

ZTEST(pinctrl_gd32, test_dt_extract)
{
	const struct pinctrl_state *scfg;
	pinctrl_soc_pin_t pin;

	zassert_equal(pcfg->state_cnt, 1U);

	scfg = &pcfg->states[0];

	zassert_equal(scfg->id, PINCTRL_STATE_DEFAULT);
	zassert_equal(scfg->pin_cnt, 12U);

	pin = scfg->pins[0];
	zassert_equal(GD32_PORT_GET(pin), 0);
	zassert_equal(GD32_PIN_GET(pin), 0);
	zassert_equal(GD32_AF_GET(pin), GD32_AF0);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ);

	pin = scfg->pins[1];
	zassert_equal(GD32_PORT_GET(pin), 1);
	zassert_equal(GD32_PIN_GET(pin), 1);
	zassert_equal(GD32_AF_GET(pin), GD32_AF1);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ);

	pin = scfg->pins[2];
	zassert_equal(GD32_PORT_GET(pin), 2);
	zassert_equal(GD32_PIN_GET(pin), 2);
	zassert_equal(GD32_AF_GET(pin), GD32_AF2);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ);

	pin = scfg->pins[3];
	zassert_equal(GD32_PORT_GET(pin), 0);
	zassert_equal(GD32_PIN_GET(pin), 3);
	zassert_equal(GD32_AF_GET(pin), GD32_AF3);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_OD);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ);

	pin = scfg->pins[4];
	zassert_equal(GD32_PORT_GET(pin), 1);
	zassert_equal(GD32_PIN_GET(pin), 4);
	zassert_equal(GD32_AF_GET(pin), GD32_AF4);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ);

	pin = scfg->pins[5];
	zassert_equal(GD32_PORT_GET(pin), 2);
	zassert_equal(GD32_PIN_GET(pin), 5);
	zassert_equal(GD32_AF_GET(pin), GD32_AF5);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_PULLUP);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ);

	pin = scfg->pins[6];
	zassert_equal(GD32_PORT_GET(pin), 0);
	zassert_equal(GD32_PIN_GET(pin), 6);
	zassert_equal(GD32_AF_GET(pin), GD32_AF6);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_PULLDOWN);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ);

	pin = scfg->pins[7];
	zassert_equal(GD32_PORT_GET(pin), 1);
	zassert_equal(GD32_PIN_GET(pin), 7);
	zassert_equal(GD32_AF_GET(pin), GD32_AF7);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ);

	pin = scfg->pins[8];
	zassert_equal(GD32_PORT_GET(pin), 2);
	zassert_equal(GD32_PIN_GET(pin), 8);
	zassert_equal(GD32_AF_GET(pin), GD32_AF8);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_25MHZ);

	pin = scfg->pins[9];
	zassert_equal(GD32_PORT_GET(pin), 0);
	zassert_equal(GD32_PIN_GET(pin), 9);
	zassert_equal(GD32_AF_GET(pin), GD32_AF9);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_50MHZ);

	pin = scfg->pins[10];
	zassert_equal(GD32_PORT_GET(pin), 1);
	zassert_equal(GD32_PIN_GET(pin), 10);
	zassert_equal(GD32_AF_GET(pin), GD32_AF10);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_MAX);

	pin = scfg->pins[11];
	zassert_equal(GD32_PORT_GET(pin), 2);
	zassert_equal(GD32_PIN_GET(pin), 11);
	zassert_equal(GD32_AF_GET(pin), GD32_ANALOG);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ);
}

ZTEST_SUITE(pinctrl_gd32, NULL, NULL, NULL, NULL, NULL);
