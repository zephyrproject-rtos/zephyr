/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_SOC_RISCV_RISCV_PRIVILEGE_HPMICRO_PINCTRL_SOC_H_
#define ZEPHYR_SOC_RISCV_RISCV_PRIVILEGE_HPMICRO_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <stdint.h>
#include <zephyr/drivers/pinctrl/pinctrl_hpmicro_common.h>

#ifdef __cplusplus
extern "C" {
#endif


struct pinctrl_soc_pin {
	/** Pinmux settings (pin, direction and signal). */
	uint32_t pinmux;
	/** Pincfg settings (bias). */
	uint32_t pincfg;
};

typedef struct pinctrl_soc_pin pinctrl_soc_pin_t;

#define Z_PINCTRL_HPMICRO_PINCFG_INIT(node_id)	\
	(((HPMICRO_OPEN_DRAIN & DT_PROP(node_id, drive_open_drain)) \
	<< HPMICRO_OPEN_DRAIN_SHIFT) |	\
	((HPMICRO_NO_PULL & DT_PROP(node_id, bias_disable)) \
	<< HPMICRO_NO_PULL_SHIFT) |	\
	((HPMICRO_PULL_DOWN & DT_PROP(node_id, bias_pull_down)) \
	<< HPMICRO_PULL_DOWN_SHIFT) |	\
	((HPMICRO_PULL_UP & DT_PROP(node_id, bias_pull_up)) \
	<< HPMICRO_PULL_UP_SHIFT) |	\
	((HPMICRO_FORCE_INPUT & DT_PROP(node_id, input_enable)) \
	<< HPMICRO_FORCE_INPUT_SHIFT) |	\
	((HPMICRO_DRIVER_STRENGTH & DT_ENUM_IDX(node_id, drive_strength)) \
	<< HPMICRO_DRIVER_STRENGTH_SHIFT) |	\
	((HPMICRO_DRIVER_STRENGTH & DT_ENUM_IDX(node_id, power_source)) \
	<< HPMICRO_DRIVER_STRENGTH_SHIFT) |	\
	((HPMICRO_SCHMITT_ENABLE & DT_PROP(node_id, input_schmitt_enable)) \
	<< HPMICRO_SCHMITT_ENABLE_SHIFT))

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)	\
	{										\
		.pinmux = DT_PROP_BY_IDX(node_id, prop, idx),	\
		.pincfg = Z_PINCTRL_HPMICRO_PINCFG_INIT(node_id) \
	},

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)	\
	{	\
		DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop),	\
		DT_FOREACH_PROP_ELEM, pinmux,	\
		Z_PINCTRL_STATE_PIN_INIT)	\
	}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_RISCV_RISCV_PRIVILEGE_HPMICRO_PINCTRL_SOC_H_ */
