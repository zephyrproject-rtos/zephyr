/*
 * Copyright 2022, 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * NXP Kinetis SOC specific helpers for pinctrl driver
 */


#ifndef ZEPHYR_SOC_ARM_NXP_KINETIS_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_NXP_KINETIS_COMMON_PINCTRL_SOC_H_

/** @cond INTERNAL_HIDDEN */

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef uint32_t pinctrl_soc_pin_t;

/* Kinetis KW/KL/KE series does not support open drain. Define macros to have no effect
 * Note: KW22 and KW24 do support open drain, rest of KW series does not
 */
/* clang-format off */
#if (defined(CONFIG_SOC_SERIES_KINETIS_KWX) &&                                                     \
	!(defined(CONFIG_SOC_MKW24D5) || defined(CONFIG_SOC_MKW22D5))) ||                          \
	defined(CONFIG_SOC_SERIES_KINETIS_KL2X) || defined(CONFIG_SOC_SERIES_KINETIS_KE1XF) ||     \
	defined(CONFIG_SOC_SERIES_KE1XZ)
#define PORT_PCR_ODE(x)   0x0
#define PORT_PCR_ODE_MASK 0x0
#endif
/* clang-format on */

/* Kinetis KE series does not support slew rate. Define macros to have no effect */
#if defined(CONFIG_SOC_SERIES_KINETIS_KE1XF) || defined(CONFIG_SOC_SERIES_KE1XZ)
#define PORT_PCR_SRE(x)   0x0
#define PORT_PCR_SRE_MASK 0x0
#endif

#if !(defined(CONFIG_SOC_SERIES_MCXA))
#define PORT_PCR_IBE(x)   0x0
#define PORT_PCR_IBE_MASK 0x0
#endif

#define Z_PINCTRL_KINETIS_PINCFG(node_id)                                                          \
	(PORT_PCR_DSE(DT_ENUM_IDX(node_id, drive_strength)) |                                      \
	 PORT_PCR_PS(DT_PROP(node_id, bias_pull_up)) |                                             \
	 PORT_PCR_PE(DT_PROP(node_id, bias_pull_up)) |                                             \
	 PORT_PCR_PE(DT_PROP(node_id, bias_pull_down)) |                                           \
	 PORT_PCR_ODE(DT_PROP(node_id, drive_open_drain)) |                                        \
	 PORT_PCR_SRE(DT_ENUM_IDX(node_id, slew_rate)) |                                           \
	 PORT_PCR_IBE(DT_PROP(node_id, input_enable)) |                                            \
	 PORT_PCR_PFE(DT_PROP(node_id, nxp_passive_filter)))

#define Z_PINCTRL_KINETIS_PCR_MASK                                                                 \
	(PORT_PCR_MUX_MASK | PORT_PCR_DSE_MASK | PORT_PCR_ODE_MASK | PORT_PCR_PFE_MASK |           \
	 PORT_PCR_IBE_MASK | PORT_PCR_SRE_MASK | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK)

#define Z_PINCTRL_STATE_PIN_INIT(group, pin_prop, idx)                                             \
	DT_PROP_BY_IDX(group, pin_prop, idx) | Z_PINCTRL_KINETIS_PINCFG(group),

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, pinmux,           \
				Z_PINCTRL_STATE_PIN_INIT)};

#ifdef __cplusplus
}
#endif

/** @endcond */

#endif /* ZEPHYR_SOC_ARM_NXP_KINETIS_COMMON_PINCTRL_SOC_H_ */
