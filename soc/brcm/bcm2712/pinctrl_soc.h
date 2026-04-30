/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_BRCM_BCM2712_PINCTRL_SOC_H_
#define ZEPHYR_SOC_BRCM_BCM2712_PINCTRL_SOC_H_

#include <stdint.h>
#include <zephyr/drivers/pinctrl/pinctrl_bcm2712_common.h>
#include <zephyr/drivers/pinctrl/pinctrl_rp1_common.h>
#include <zephyr/dt-bindings/pinctrl/bcm2712-pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/rp1-pinctrl.h>

struct bcm2712_composit_pinctrl_soc_pin {
	int type;
	union {
		struct brcm_bcm2712_pinctrl_pinconfig brcm_bcm2712_pinctrl;
		struct raspberrypi_rp1_pinctrl_pinconfig raspberrypi_rp1_pinctrl;
	};
};

typedef struct bcm2712_composit_pinctrl_soc_pin pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	{IF_ENABLED(DT_SAME_NODE(DT_PARENT(DT_PARENT(node_id)), DT_NODELABEL(pinctrl)), (          \
		.type = DT_DEP_ORD(DT_PARENT(DT_PARENT(node_id))),                                 \
		.brcm_bcm2712_pinctrl = BRCM_BCM2712_PINCTRL_STATE_PIN_INIT(node_id, prop, idx),   \
	))                                                                                         \
	 IF_ENABLED(DT_SAME_NODE(DT_PARENT(DT_PARENT(node_id)), DT_NODELABEL(rp1_pinctrl)), (      \
		.type = DT_DEP_ORD(DT_PARENT(DT_PARENT(node_id))),                                 \
		.raspberrypi_rp1_pinctrl =                                                         \
			RASPBERRYPI_RP1_PINCTRL_STATE_PIN_INIT(node_id, prop, idx),                \
	)) },

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)}

#endif /* ZEPHYR_SOC_BRCM_BCM2712_PINCTRL_SOC_H_ */
