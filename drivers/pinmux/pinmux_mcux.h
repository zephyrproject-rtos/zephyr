/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_PINMUX_PINMUX_MCUX_H_
#define ZEPHYR_DRIVERS_PINMUX_PINMUX_MCUX_H_

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

struct soc_pinctrl {
	const struct device *port;
	uint8_t pin;
	uint8_t mux;
};

void k_pincfg(const struct soc_pinctrl *pins, size_t num_pins);



#define NXP_PIN(i, node) \
	DT_PROP_BY_IDX(DT_PHANDLE_BY_IDX(node, pinctrl_0, i), nxp_kinetis_port_pins, 0)

#define NXP_MUX(i, node) \
	DT_PROP_BY_IDX(DT_PHANDLE_BY_IDX(node, pinctrl_0, i), nxp_kinetis_port_pins, 1)

#define PIN_BAR(i, node) \
	pinmux_pin_set(GET_PORT_DEV(i, node), \
			NXP_PIN(i, node), \
			PORT_PCR_MUX(NXP_MUX(i, node)));

#define PIN_FOO(node) \
	UTIL_LISTIFY(DT_PROP_LEN(node, pinctrl_0), PIN_BAR, node)

#define GET_PORT_DEV(i, node) \
	DEVICE_DT_GET(DT_PARENT(DT_PHANDLE_BY_IDX(node, pinctrl_0, i)))

#define NXP_DT_INST_PIN(i, inst) NXP_PIN(i, DT_DRV_INST(inst))

#define NXP_DT_INST_MUX(i, inst) NXP_MUX(i, DT_DRV_INST(inst))

#define NXP_K_DT_PIN_ELEM(i, inst)				\
	{							\
		.port = GET_PORT_DEV(i, DT_DRV_INST(inst)),	\
		.pin = NXP_DT_INST_PIN(i, inst),		\
		.mux = NXP_DT_INST_MUX(i, inst),		\
	},

#define NXP_K_DT_INST_PINS(inst)					\
{ UTIL_LISTIFY(DT_INST_PROP_LEN(inst, pinctrl_0), NXP_K_DT_PIN_ELEM, inst) }


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_PINMUX_PINMUX_MCUX_H_ */
