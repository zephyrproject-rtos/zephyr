/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pinctrl/pinctrl_bcm2712_common.h>

int brcm_bcm2712_pinctrl_configure_pin(const struct brcm_bcm2712_pinctrl_pinconfig *pin)
{
	ARG_UNUSED(pin);
	return 0;
}
