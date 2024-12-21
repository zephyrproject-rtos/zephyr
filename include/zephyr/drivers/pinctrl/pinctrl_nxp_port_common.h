/*
 * Copyright 2022, 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * NXP PORT SOC specific helpers for pinctrl driver
 */


#ifndef ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_NXP_PORT_COMMON_H_
#define ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_NXP_PORT_COMMON_H_

/** @cond INTERNAL_HIDDEN */

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

/* Include SOC headers, so we get definitions for PCR bitmasks */
#include <soc.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Some PORT IP instantiations lack certain features, include input buffers,
 * open drain, and slew rate. If masks aren't defined for these bitfields,
 * define them to have no effect
 */
#ifndef PORT_PCR_IBE_MASK /* Input buffer enable */
#define PORT_PCR_IBE_MASK 0x0
#define PORT_PCR_IBE(x) 0x0
#endif

#ifndef PORT_PCR_SRE_MASK /* Slew rate */
#define PORT_PCR_SRE_MASK 0x0
#define PORT_PCR_SRE(x) 0x0
#endif

#ifndef PORT_PCR_ODE_MASK /* Open drain */
#define PORT_PCR_ODE_MASK 0x0
#define PORT_PCR_ODE(x) 0x0
#endif


typedef uint32_t pinctrl_soc_pin_t;

#define Z_PINCTRL_NXP_PORT_PINCFG(node_id)                                                         \
	(PORT_PCR_DSE(DT_ENUM_IDX(node_id, drive_strength)) |                                      \
	 PORT_PCR_PS(DT_PROP(node_id, bias_pull_up)) |                                             \
	 PORT_PCR_PE(DT_PROP(node_id, bias_pull_up)) |                                             \
	 PORT_PCR_PE(DT_PROP(node_id, bias_pull_down)) |                                           \
	 PORT_PCR_ODE(DT_PROP(node_id, drive_open_drain)) |                                        \
	 PORT_PCR_SRE(DT_ENUM_IDX(node_id, slew_rate)) |                                           \
	 PORT_PCR_IBE(DT_PROP(node_id, input_enable)) |                                            \
	 PORT_PCR_PFE(DT_PROP(node_id, nxp_passive_filter)))

#define Z_PINCTRL_NXP_PORT_PCR_MASK                                                                \
	(PORT_PCR_MUX_MASK | PORT_PCR_DSE_MASK | PORT_PCR_ODE_MASK | PORT_PCR_PFE_MASK |           \
	 PORT_PCR_IBE_MASK | PORT_PCR_SRE_MASK | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK)

#define Z_PINCTRL_STATE_PIN_INIT(group, pin_prop, idx)                                             \
	DT_PROP_BY_IDX(group, pin_prop, idx) | Z_PINCTRL_NXP_PORT_PINCFG(group),

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)};

#ifdef __cplusplus
}
#endif

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_NXP_PORT_COMMON_H_ */
