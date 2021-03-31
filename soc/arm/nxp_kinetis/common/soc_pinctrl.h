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

typedef struct nxp_kinetis_pinctrl pinctrl_dt_pin_spec_t;

#define NXP_KINETIS_PIN(i, node, n) \
	DT_PROP_BY_IDX(DT_PHANDLE_BY_IDX(node, pinctrl_##n, i), nxp_kinetis_port_pins, 0)

#define NXP_KINETIS_MUX(i, node, n) \
	DT_PROP_BY_IDX(DT_PHANDLE_BY_IDX(node, pinctrl_##n, i), nxp_kinetis_port_pins, 1)

#define NXP_KINETIS_GET_PORT_DEV(i, node, n) \
	DEVICE_DT_GET(DT_PARENT(DT_PHANDLE_BY_IDX(node, pinctrl_##n, i)))

#define NXP_KINETIS_PIN_ELEM(i, node, n)			\
	{							\
		.port = NXP_KINETIS_GET_PORT_DEV(i, node, n),	\
		.pin = NXP_KINETIS_PIN(i, node, n),		\
		.mux = NXP_KINETIS_MUX(i, node, n),		\
	},

#define PINCTRL_DT_SPEC_GET(node, n) \
{ UTIL_LISTIFY(DT_PROP_LEN(node, pinctrl_##n), NXP_KINETIS_PIN_ELEM, node, n) }

#ifdef __cplusplus
}
#endif

#endif /* SOC_NXP_KINETIS_SOC_PINCTRL_H_ */
