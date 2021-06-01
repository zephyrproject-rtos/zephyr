/*
 * Copyright (c) 2021 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pinctrl.h>
#include <soc.h>

int z_impl_pinctrl_pin_configure(const pinctrl_dt_pin_spec_t *pin_spec)
{
	soc_gpio_configure((const struct soc_gpio_pin *)pin_spec);

	return 0;
}
