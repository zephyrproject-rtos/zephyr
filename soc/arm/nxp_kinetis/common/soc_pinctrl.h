/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SOC_NXP_KINETIS_SOC_PINCTRL_H_
#define SOC_NXP_KINETIS_SOC_PINCTRL_H_

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

struct nxp_kinetis_pinctrl {
	const struct device *port;
	uint8_t pin;
	uint8_t mux;
};

typedef struct nxp_kinetis_pinctrl pinctrl_soc_pins_t;

#define PINCTRL_SOC_PINS_ELEM_INIT(node)			\
{								\
	.port = DEVICE_DT_GET(DT_PARENT(node)),			\
	.pin = DT_PROP_BY_IDX(node, nxp_kinetis_port_pins, 0),	\
	.mux = DT_PROP_BY_IDX(node, nxp_kinetis_port_pins, 1),	\
},

#ifdef __cplusplus
}
#endif

#endif /* SOC_NXP_KINETIS_SOC_PINCTRL_H_ */
