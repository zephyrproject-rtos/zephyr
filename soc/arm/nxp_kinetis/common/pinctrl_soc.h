/*
 * Copyright (c) 2021 Linaro Limited.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Public APIs for pin control drivers
 */

#ifndef ZEPHYR_SOC_ARM_NXP_KINETIS_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_NXP_KINETIS_COMMON_PINCTRL_SOC_H_

/**
 * @brief Pin Controller Interface (NXP Kinetis)
 * @defgroup pinctrl_interface_nxp_kinetis Pin Controller Interface
 * @ingroup pinctrl_interface
 * @{
 */

#include <devicetree.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/** Type for NXP Kinetis pin. */
typedef struct pinctrl_soc_pin {
	PORT_Type *port_reg;
	uint8_t pin;
	uint16_t mux;
} pinctrl_soc_pin_t;

#define PINCTRL_SOC_PINS_ELEM_INIT(node_id)							\
{												\
	.port_reg = (PORT_Type *)DT_REG_ADDR(DT_PARENT(node_id)),				\
	.pin = DT_PROP_BY_IDX(node_id, nxp_kinetis_port_pins, 0),				\
	.mux = PORT_PCR_MUX(DT_PROP_BY_IDX(node_id, nxp_kinetis_port_pins, 1)) |		\
		(DT_PROP(node_id, bias_pull_up) & (PORT_PCR_PE_MASK | PORT_PCR_PS_MASK)) |	\
		(DT_PROP(node_id, bias_pull_down) & PORT_PCR_PS_MASK) |				\
		(DT_PROP(node_id, drive_open_drain) & PORT_PCR_ODE_MASK), 			\
},

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param state_prop State property name.
 * @param idx State property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx)		       \
	PINCTRL_SOC_PINS_ELEM_INIT(DT_PROP_BY_IDX(node_id, state_prop, idx))

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			       \
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

/** @endcond */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_SOC_ARM_NXP_KINETIS_COMMON_PINCTRL_SOC_H_ */
