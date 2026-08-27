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
	zassert_equal(scfg->pin_cnt, 14U);

	pin = scfg->pins[0];
	zassert_equal(GD32_PORT_GET(pin), 0);
	zassert_equal(GD32_PIN_GET(pin), 0);
	zassert_equal(GD32_MODE_GET(pin), GD32_MODE_ANALOG);
	zassert_equal(GD32_REMAP_GET(pin), GD32_NORMP);

	pin = scfg->pins[1];
	zassert_equal(GD32_PORT_GET(pin), 1);
	zassert_equal(GD32_PIN_GET(pin), 1);
	zassert_equal(GD32_MODE_GET(pin), GD32_MODE_ALTERNATE);
	zassert_equal(GD32_REMAP_GET(pin), GD32_NORMP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ);

	pin = scfg->pins[2];
	zassert_equal(GD32_PORT_GET(pin), 2);
	zassert_equal(GD32_PIN_GET(pin), 2);
	zassert_equal(GD32_MODE_GET(pin), GD32_MODE_GPIO_IN);
	zassert_equal(GD32_REMAP_GET(pin), GD32_NORMP);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);

	pin = scfg->pins[3];
	zassert_equal(GD32_PORT_GET(pin), 0);
	zassert_equal(GD32_PIN_GET(pin), 3);
	zassert_equal(GD32_MODE_GET(pin), GD32_MODE_GPIO_IN);
	zassert_equal(GD32_REMAP_REG_GET(GD32_REMAP_GET(pin)), 0);
	zassert_equal(GD32_REMAP_POS_GET(GD32_REMAP_GET(pin)), 0);
	zassert_equal(GD32_REMAP_MSK_GET(GD32_REMAP_GET(pin)), 0x1);
	zassert_equal(GD32_REMAP_VAL_GET(GD32_REMAP_GET(pin)), 1);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);

	pin = scfg->pins[4];
	zassert_equal(GD32_PORT_GET(pin), 1);
	zassert_equal(GD32_PIN_GET(pin), 4);
	zassert_equal(GD32_MODE_GET(pin), GD32_MODE_ALTERNATE);
	zassert_equal(GD32_REMAP_REG_GET(GD32_REMAP_GET(pin)), 0);
	zassert_equal(GD32_REMAP_POS_GET(GD32_REMAP_GET(pin)), 0);
	zassert_equal(GD32_REMAP_MSK_GET(GD32_REMAP_GET(pin)), 0x1);
	zassert_equal(GD32_REMAP_VAL_GET(GD32_REMAP_GET(pin)), 1);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ);

	pin = scfg->pins[5];
	zassert_equal(GD32_PORT_GET(pin), 2);
	zassert_equal(GD32_PIN_GET(pin), 5);
	zassert_equal(GD32_MODE_GET(pin), GD32_MODE_GPIO_IN);
	zassert_equal(GD32_REMAP_GET(pin), GD32_NORMP);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);

	pin = scfg->pins[6];
	zassert_equal(GD32_PORT_GET(pin), 0);
	zassert_equal(GD32_PIN_GET(pin), 6);
	zassert_equal(GD32_MODE_GET(pin), GD32_MODE_GPIO_IN);
	zassert_equal(GD32_REMAP_GET(pin), GD32_NORMP);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_OD);

	pin = scfg->pins[7];
	zassert_equal(GD32_PORT_GET(pin), 1);
	zassert_equal(GD32_PIN_GET(pin), 7);
	zassert_equal(GD32_MODE_GET(pin), GD32_MODE_GPIO_IN);
	zassert_equal(GD32_REMAP_GET(pin), GD32_NORMP);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_NONE);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);

	pin = scfg->pins[8];
	zassert_equal(GD32_PORT_GET(pin), 2);
	zassert_equal(GD32_PIN_GET(pin), 8);
	zassert_equal(GD32_MODE_GET(pin), GD32_MODE_GPIO_IN);
	zassert_equal(GD32_REMAP_GET(pin), GD32_NORMP);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_PULLUP);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);

	pin = scfg->pins[9];
	zassert_equal(GD32_PORT_GET(pin), 0);
	zassert_equal(GD32_PIN_GET(pin), 9);
	zassert_equal(GD32_MODE_GET(pin), GD32_MODE_GPIO_IN);
	zassert_equal(GD32_REMAP_GET(pin), GD32_NORMP);
	zassert_equal(GD32_PUPD_GET(pin), GD32_PUPD_PULLDOWN);
	zassert_equal(GD32_OTYPE_GET(pin), GD32_OTYPE_PP);

	pin = scfg->pins[10];
	zassert_equal(GD32_PORT_GET(pin), 1);
	zassert_equal(GD32_PIN_GET(pin), 10);
	zassert_equal(GD32_MODE_GET(pin), GD32_MODE_ALTERNATE);
	zassert_equal(GD32_REMAP_GET(pin), GD32_NORMP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_2MHZ);

	pin = scfg->pins[11];
	zassert_equal(GD32_PORT_GET(pin), 2);
	zassert_equal(GD32_PIN_GET(pin), 11);
	zassert_equal(GD32_MODE_GET(pin), GD32_MODE_ALTERNATE);
	zassert_equal(GD32_REMAP_GET(pin), GD32_NORMP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_10MHZ);

	pin = scfg->pins[12];
	zassert_equal(GD32_PORT_GET(pin), 0);
	zassert_equal(GD32_PIN_GET(pin), 12);
	zassert_equal(GD32_MODE_GET(pin), GD32_MODE_ALTERNATE);
	zassert_equal(GD32_REMAP_GET(pin), GD32_NORMP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_50MHZ);

	pin = scfg->pins[13];
	zassert_equal(GD32_PORT_GET(pin), 1);
	zassert_equal(GD32_PIN_GET(pin), 13);
	zassert_equal(GD32_MODE_GET(pin), GD32_MODE_ALTERNATE);
	zassert_equal(GD32_REMAP_GET(pin), GD32_NORMP);
	zassert_equal(GD32_OSPEED_GET(pin), GD32_OSPEED_MAX);
}

ZTEST_SUITE(pinctrl_gd32, NULL, NULL, NULL, NULL, NULL);
