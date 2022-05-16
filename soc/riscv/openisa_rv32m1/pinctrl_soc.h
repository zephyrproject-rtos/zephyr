/*
 * Copyright (c) 2022 Henrik Brix Andersen <henrik@brixandersen.dk>
 * Copyright (c) 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * OpenISA RV32M1 SOC specific helpers for pinctrl driver
 */

#ifndef ZEPHYR_SOC_RISCV_OPENISA_RV32M1_PINCTRL_SOC_H_
#define ZEPHYR_SOC_RISCV_OPENISA_RV32M1_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t pinctrl_soc_pin_t;

#define Z_PINCTRL_RV32M1_PINCFG(node_id)					\
	(PORT_PCR_PS(DT_PROP(node_id, bias_pull_up)) |				\
	PORT_PCR_PE(DT_PROP(node_id, bias_pull_up)) |				\
	PORT_PCR_PE(DT_PROP(node_id, bias_pull_down)) |				\
	PORT_PCR_ODE(DT_PROP(node_id, drive_open_drain)) |			\
	PORT_PCR_SRE(DT_ENUM_IDX(node_id, slew_rate)) |				\
	PORT_PCR_PFE(DT_PROP(node_id, openisa_passive_filter)))

#define Z_PINCTRL_RV32M1_PCR_MASK						\
	(PORT_PCR_MUX_MASK | PORT_PCR_ODE_MASK | PORT_PCR_PFE_MASK |		\
	PORT_PCR_SRE_MASK | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK)

#define Z_PINCTRL_STATE_PIN_INIT(group, pin_prop, idx)				\
	DT_PROP_BY_IDX(group, pin_prop, idx) | Z_PINCTRL_RV32M1_PINCFG(group),

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)				\
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),			\
		DT_FOREACH_PROP_ELEM, pinmux, Z_PINCTRL_STATE_PIN_INIT)};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_RISCV_OPENISA_RV32M1_PINCTRL_SOC_H_ */
