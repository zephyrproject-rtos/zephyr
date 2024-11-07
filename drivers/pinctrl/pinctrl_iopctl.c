/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_iopctl

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/init.h>

#define OFFSET(mux) (((mux) & 0xFFF00000) >> 20)
#define INDEX(mux)  (((mux) & 0xF0000) >> 16)
#define Z_PINCTRL_IOPCTL_PIN_MASK 0xFFF

/* IOPCTL register addresses. */
static uint32_t *iopctl[] = {
#if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(iopctl0)))
	(uint32_t *)DT_REG_ADDR(DT_NODELABEL(iopctl0)),
#else
	NULL,
#endif
#if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(iopctl1)))
	(uint32_t *)DT_REG_ADDR(DT_NODELABEL(iopctl1)),
#else
	NULL,
#endif
#if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(iopctl2)))
	(uint32_t *)DT_REG_ADDR(DT_NODELABEL(iopctl2)),
#else
	NULL,
#endif
};

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	for (uint8_t i = 0; i < pin_cnt; i++) {
		uint32_t pin_mux = pins[i];
		uint32_t index = INDEX(pin_mux);
		uint32_t offset = OFFSET(pin_mux);

		if (index < ARRAY_SIZE(iopctl)) {
			/* Set pinmux */
			*(iopctl[index] + offset) = (pin_mux & Z_PINCTRL_IOPCTL_PIN_MASK);
		} else {
			return -EINVAL;
		}
	}

	return 0;
}
