/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ALIF_ENSEMBLE_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ALIF_ENSEMBLE_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>

typedef uint32_t pinctrl_soc_pin_t;

/* Pinmux encoding (Also, matches the hardware pad configuration register):
 * bits 0:2   [AF] - Alternate function (0-7)
 * bits 3:5   [PIN] - Pin number within port (0-7)
 * bits 6:10  [PORT] - Port number (0-17, port 15 is LPGPIO)
 * bits 11:15 - Reserved
 * bit 16     [REN] - Input enable: 1=enable, 0=disable
 * bit 17     [SMT] - Schmitt trigger: 1=enable, 0=disable
 * bit 18     [SR] - Slew rate: 1=fast, 0=slow
 * bits 19:20 [DSC] - Bias control: 0=high-Z, 1=pull-up, 2=pull-down, 3=bus-keeper
 * bits 21:22 [ODS] - Drive strength: 0=2mA, 1=4mA, 2=8mA, 3=12mA
 * bit 23     [DRV] - Driver type: 0=push-pull, 1=open-drain
 * bits 24:31 - Reserved
 */
#define REN_BIT_POS		16
#define SMT_BIT_POS		17
#define SR_BIT_POS		18
#define DSC_BIT_POS		19
#define ODS_BIT_POS		21
#define DRV_BIT_POS		23

#define ALIF_PAD_CONF_REN(x) (((x) & BIT_MASK(1)) << REN_BIT_POS)
#define ALIF_PAD_CONF_SMT(x) (((x) & BIT_MASK(1)) << SMT_BIT_POS)
#define ALIF_PAD_CONF_SR(x)  (((x) & BIT_MASK(1)) << SR_BIT_POS)
#define ALIF_PAD_CONF_DSC(x) (((x) & BIT_MASK(2)) << DSC_BIT_POS)
#define ALIF_PAD_CONF_ODS(x) (((x) & BIT_MASK(2)) << ODS_BIT_POS)
#define ALIF_PAD_CONF_DRV(x) (((x) & BIT_MASK(1)) << DRV_BIT_POS)

/* Prepare bias control field value: 0=high-Z, 1=pull-up, 2=pull-down, 3=bus-hold */
#define ALIF_PINCTRL_BIAS_CFG(node_id)						\
	(DT_PROP(node_id, bias_high_impedance) ? 0 :				\
	 DT_PROP(node_id, bias_pull_up) ? 1 :					\
	 DT_PROP(node_id, bias_pull_down) ? 2 :					\
	 DT_PROP(node_id, bias_bus_hold) ? 3 : 0)

/* Prepare drive strength field value: 0=2mA, 1=4mA, 2=8mA, 3=12mA */
#define ALIF_PINCTRL_DRIVE_STRENGTH_CFG(node_id) \
	DT_ENUM_IDX(node_id, drive_strength)

/* Returns register ready pad configuration value */
#define ALIF_PINCTRL_PADCONF(node_id)						\
	(ALIF_PAD_CONF_REN(DT_PROP(node_id, input_enable)) |			\
	ALIF_PAD_CONF_SMT(DT_PROP(node_id, input_schmitt_enable)) |		\
	ALIF_PAD_CONF_SR(DT_ENUM_IDX(node_id, slew_rate)) |			\
	ALIF_PAD_CONF_DSC(ALIF_PINCTRL_BIAS_CFG(node_id)) |			\
	ALIF_PAD_CONF_ODS(ALIF_PINCTRL_DRIVE_STRENGTH_CFG(node_id)) |		\
	ALIF_PAD_CONF_DRV(DT_PROP(node_id, drive_open_drain)))

/**
 * @brief Initialize a single pin configuration from devicetree
 * @param group Devicetree node for the pin group
 * @param pin_prop Property name containing pin configurations
 * @param idx Index of the pin within the property array
 */
#define Z_PINCTRL_STATE_PIN_INIT(group, pin_prop, idx)			\
	DT_PROP_BY_IDX(group, pin_prop, idx) | ALIF_PINCTRL_PADCONF(group),

/**
 * @brief Initialize all pins for a pinctrl state from devicetree
 * @param node_id Devicetree node ID
 * @param prop Property name (typically "pinctrl-0", "pinctrl-1", etc.)
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			\
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),		\
		DT_FOREACH_PROP_ELEM, pinmux, Z_PINCTRL_STATE_PIN_INIT)};

#endif /* ZEPHYR_SOC_ALIF_ENSEMBLE_PINCTRL_SOC_H_ */
