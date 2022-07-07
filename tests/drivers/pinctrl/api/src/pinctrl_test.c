/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <ztest.h>

/* pinctrl test driver implemented as a mock */

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ztest_check_expected_data(pins, pin_cnt * sizeof(*pins));
	ztest_check_expected_value(pin_cnt);
	ztest_check_expected_value(reg);

	return 0;
}
