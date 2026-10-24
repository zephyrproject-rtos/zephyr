/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NUVOTON_NPCM_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_NUVOTON_NPCM_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>
#include <zephyr/types.h>

/** Kinds of pin configuration entry */
#define NPCM_PINCTRL_TYPE_FUNC   0 /**< Select a pin function, with optional bias and voltage */
#define NPCM_PINCTRL_TYPE_DEVCTL 1 /**< Write one SCFG bit verbatim */

/** Bias requests. Also the direction a pin function's resistor pulls. */
#define NPCM_BIAS_NONE      0
#define NPCM_BIAS_PULL_UP   1
#define NPCM_BIAS_PULL_DOWN 2
#define NPCM_BIAS_DISABLE   3

/** Voltage level requests */
#define NPCM_VOLT_NONE 0
#define NPCM_VOLT_3V3  1
#define NPCM_VOLT_1V8  2

/** Width of the pin-function index and maximum number of table entries. */
#define NPCM_PINCTRL_FUNC_BITS 11
#define NPCM_PINCTRL_MAX_FUNCS BIT(NPCM_PINCTRL_FUNC_BITS)

/**
 * @brief One NPCM pin configuration entry.
 *
 * Selects a pin function or writes an SCFG bit from a
 * nuvoton,device-control property.
 */
struct npcm_pinctrl {
	uint16_t type: 1;
	uint16_t bias: 2;
	uint16_t volt: 2;
	uint16_t fn: NPCM_PINCTRL_FUNC_BITS; /**< FUNC: index into the pin function table */
	uint8_t ctl_reg;                     /**< DEVCTL: SCFG register offset */
	uint8_t ctl_bit: 3;                  /**< DEVCTL: bit within the register */
	uint8_t ctl_val: 1;                  /**< DEVCTL: value to write */
	uint8_t reserved: 4;
};

typedef struct npcm_pinctrl pinctrl_soc_pin_t;

BUILD_ASSERT(sizeof(pinctrl_soc_pin_t) == 4, "NPCM pin configuration must stay 4 bytes");

#define Z_PINCTRL_NPCM_BIAS(node_id)                                                               \
	COND_CODE_1(DT_PROP(node_id, bias_pull_up), (NPCM_BIAS_PULL_UP),                           \
		    (COND_CODE_1(DT_PROP(node_id, bias_pull_down), (NPCM_BIAS_PULL_DOWN),          \
				 (COND_CODE_1(DT_PROP(node_id, bias_disable), (NPCM_BIAS_DISABLE), \
					      (NPCM_BIAS_NONE))))))

#define Z_PINCTRL_NPCM_VOLT(node_id)                                                               \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, nuvoton_voltage_level_mv),                           \
		    ((DT_PROP(node_id, nuvoton_voltage_level_mv) == 1800) ? NPCM_VOLT_1V8          \
									  : NPCM_VOLT_3V3),        \
		    (NPCM_VOLT_NONE))

#define Z_PINCTRL_NPCM_FUNC_INIT(node_id)                                                          \
	{                                                                                          \
		.type = NPCM_PINCTRL_TYPE_FUNC,                                                    \
		.bias = Z_PINCTRL_NPCM_BIAS(node_id),                                              \
		.volt = Z_PINCTRL_NPCM_VOLT(node_id),                                              \
		.fn = DT_NODE_CHILD_IDX(DT_PHANDLE(node_id, pinmux)),                              \
	},

#define Z_PINCTRL_NPCM_DEVCTL_INIT(node_id)                                                        \
	{                                                                                          \
		.type = NPCM_PINCTRL_TYPE_DEVCTL,                                                  \
		.ctl_reg = DT_PROP_BY_IDX(node_id, nuvoton_device_control, 0),                     \
		.ctl_bit = DT_PROP_BY_IDX(node_id, nuvoton_device_control, 1),                     \
		.ctl_val = DT_PROP_BY_IDX(node_id, nuvoton_device_control, 2),                     \
	},

/* Expand a pin node into its function selection and optional device control. */
#define Z_PINCTRL_NPCM_PIN_INIT(node_id)                                                           \
	Z_PINCTRL_NPCM_FUNC_INIT(node_id)                                                          \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, nuvoton_device_control),                              \
		   (Z_PINCTRL_NPCM_DEVCTL_INIT(node_id)))

/* Expand each pin configuration in a group named by a pinctrl-N property. */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                               \
	DT_FOREACH_CHILD(DT_PROP_BY_IDX(node_id, prop, idx), Z_PINCTRL_NPCM_PIN_INIT)

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)                      \
	}

#endif /* ZEPHYR_SOC_NUVOTON_NPCM_COMMON_PINCTRL_SOC_H_ */
