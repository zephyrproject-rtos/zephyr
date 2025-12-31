/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include "pinctrl_soc.h"

LOG_MODULE_REGISTER(brcm_bcm2712, CONFIG_SOC_LOG_LEVEL);

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	int ret = 0;

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		if (pins[i].type == DT_DEP_ORD(DT_NODELABEL(pinctrl))) {
			ret = brcm_bcm2712_pinctrl_configure_pin(&pins[i].brcm_bcm2712_pinctrl);

			if (ret != 0) {
				break;
			}
		} else if (pins[i].type == DT_DEP_ORD(DT_NODELABEL(rp1_pinctrl))) {
			ret = raspberrypi_rp1_pinctrl_configure_pin(
				&pins[i].raspberrypi_rp1_pinctrl);

			if (ret != 0) {
				break;
			}
		}
	}

	return ret;
}
