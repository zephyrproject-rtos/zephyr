/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/ztest.h>

/* pin configuration for test device */
#define TEST_DEVICE DT_NODELABEL(test_device)
PINCTRL_DT_DEV_CONFIG_DECLARE(TEST_DEVICE);
static const struct pinctrl_dev_config *pcfg = PINCTRL_DT_DEV_CONFIG_GET(TEST_DEVICE);

ZTEST(pinctrl_nrf, test_dt_extract)
{
	const struct pinctrl_state *scfg;

	zassert_equal(pcfg->reg, 0x0U);
	zassert_equal(pcfg->state_cnt, 1U);

	scfg = &pcfg->states[0];

	zassert_equal(scfg->id, PINCTRL_STATE_DEFAULT);
	zassert_equal(scfg->pin_cnt, 7U);

	zassert_equal(NRF_GET_FUN(scfg->pins[0]), NRF_FUN_UART_TX);
	zassert_equal(NRF_GET_LP(scfg->pins[0]), NRF_LP_DISABLE);
	zassert_equal(NRF_GET_DRIVE(scfg->pins[0]), NRF_DRIVE_S0S1);
	zassert_equal(NRF_GET_PULL(scfg->pins[0]), NRF_PULL_NONE);
	zassert_equal(NRF_GET_PIN(scfg->pins[0]), 1U);

	zassert_equal(NRF_GET_FUN(scfg->pins[1]), NRF_FUN_UART_RTS);
	zassert_equal(NRF_GET_LP(scfg->pins[1]), NRF_LP_DISABLE);
	zassert_equal(NRF_GET_DRIVE(scfg->pins[1]), NRF_DRIVE_S0S1);
	zassert_equal(NRF_GET_PULL(scfg->pins[1]), NRF_PULL_NONE);
	zassert_equal(NRF_GET_PIN(scfg->pins[1]), 2U);

	zassert_equal(NRF_GET_FUN(scfg->pins[2]), NRF_FUN_UART_RX);
	zassert_equal(NRF_GET_PIN(scfg->pins[2]), NRF_PIN_DISCONNECTED);

	zassert_equal(NRF_GET_FUN(scfg->pins[3]), NRF_FUN_UART_RX);
	zassert_equal(NRF_GET_LP(scfg->pins[3]), NRF_LP_DISABLE);
	zassert_equal(NRF_GET_DRIVE(scfg->pins[3]), NRF_DRIVE_H0S1);
	zassert_equal(NRF_GET_PULL(scfg->pins[3]), NRF_PULL_NONE);
	zassert_equal(NRF_GET_PIN(scfg->pins[3]), 3U);

	zassert_equal(NRF_GET_FUN(scfg->pins[4]), NRF_FUN_UART_RX);
	zassert_equal(NRF_GET_LP(scfg->pins[4]), NRF_LP_DISABLE);
	zassert_equal(NRF_GET_DRIVE(scfg->pins[4]), NRF_DRIVE_S0S1);
	zassert_equal(NRF_GET_PULL(scfg->pins[4]), NRF_PULL_UP);
	zassert_equal(NRF_GET_PIN(scfg->pins[4]), 4U);

	zassert_equal(NRF_GET_FUN(scfg->pins[5]), NRF_FUN_UART_RX);
	zassert_equal(NRF_GET_LP(scfg->pins[5]), NRF_LP_DISABLE);
	zassert_equal(NRF_GET_DRIVE(scfg->pins[5]), NRF_DRIVE_S0S1);
	zassert_equal(NRF_GET_PULL(scfg->pins[5]), NRF_PULL_DOWN);
	zassert_equal(NRF_GET_PIN(scfg->pins[5]), 5U);

	zassert_equal(NRF_GET_FUN(scfg->pins[6]), NRF_FUN_UART_CTS);
	zassert_equal(NRF_GET_LP(scfg->pins[6]), NRF_LP_ENABLE);
	zassert_equal(NRF_GET_DRIVE(scfg->pins[6]), NRF_DRIVE_S0S1);
	zassert_equal(NRF_GET_PULL(scfg->pins[6]), NRF_PULL_NONE);
	zassert_equal(NRF_GET_PIN(scfg->pins[6]), 38U);
}

ZTEST_SUITE(pinctrl_nrf, NULL, NULL, NULL, NULL, NULL);
