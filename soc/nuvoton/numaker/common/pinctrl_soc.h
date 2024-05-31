/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NUMAKER_PINCTRL_SOC_H_
#define _NUVOTON_NUMAKER_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <stdint.h>

#define PORT_INDEX(pinmux)    (((pinmux)&0xF0000000) >> 28)
#define PIN_INDEX(pinmux)     (((pinmux)&0x0F000000) >> 24)
#define MFP_CFG(pinmux)       (((pinmux)&0x000000FF) << ((PIN_INDEX(pinmux) % 4) * 8))
#define NU_MFP_POS(pinindex)  ((pinindex % 4) * 8)
#define NU_MFP_MASK(pinindex) (0x1f << NU_MFP_POS(pinindex))

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pinctrl_soc_pin_t {
	uint32_t pin_mux;
	uint32_t open_drain: 1;
	uint32_t schmitt_enable: 1;
	uint32_t slew_rate: 2;
	uint32_t digital_disable: 1;
} pinctrl_soc_pin_t;

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)						\
	{											\
		.pin_mux = DT_PROP_BY_IDX(node_id, prop, idx),					\
		.open_drain = DT_PROP(node_id, drive_open_drain),				\
		.schmitt_enable = DT_PROP(node_id, input_schmitt_enable),			\
		.slew_rate = DT_ENUM_IDX(node_id, slew_rate),					\
		.digital_disable = DT_PROP(node_id, digital_path_disable),			\
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)						\
	{											\
		DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,	\
					Z_PINCTRL_STATE_PIN_INIT)				\
	}

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NUMAKER_PINCTRL_SOC_H_ */
