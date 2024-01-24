/*
 * Copyright (c) 2023 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_SOC_ARM_RENESAS_RCAR_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_RENESAS_RCAR_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-rcar-common.h>
#include <stdint.h>
#include <zephyr/sys/util_macro.h>

struct rcar_pin_func {
	uint8_t bank:5;      /* bank number 0 - 18 */
	uint8_t shift:5;     /* bit shift 0 - 28 */
	uint8_t func:4;      /* choice from 0x0 to 0xF */
};

/** Pull-up, pull-down, or bias disable is requested */
#define RCAR_PIN_FLAGS_PULL_SET BIT(0)
/** Performs on/off control of the pull resistors */
#define RCAR_PIN_FLAGS_PUEN     BIT(1)
/** Select pull-up resistor if set pull-down otherwise */
#define RCAR_PIN_FLAGS_PUD      BIT(2)
/** Alternate function for the pin is requested */
#define RCAR_PIN_FLAGS_FUNC_SET BIT(3)

#define RCAR_PIN_PULL_UP      (RCAR_PIN_FLAGS_PULL_SET | RCAR_PIN_FLAGS_PUEN | RCAR_PIN_FLAGS_PUD)
#define RCAR_PIN_PULL_DOWN    (RCAR_PIN_FLAGS_PULL_SET | RCAR_PIN_FLAGS_PUEN)
#define RCAR_PIN_PULL_DISABLE  RCAR_PIN_FLAGS_PULL_SET

/** Type for R-Car pin. */
typedef struct pinctrl_soc_pin {
	uint16_t pin;
	struct rcar_pin_func func;
	uint8_t flags;
	uint8_t drive_strength;
	uint8_t voltage;
} pinctrl_soc_pin_t;

#define RCAR_IPSR(node_id) DT_PROP_BY_IDX(node_id, pin, 1)
#define RCAR_HAS_IPSR(node_id) DT_PROP_HAS_IDX(node_id, pin, 1)

/* Offsets are defined in dt-bindings pinctrl-rcar-common.h */
#define RCAR_PIN_FUNC(node_id)			       \
	{					       \
		((RCAR_IPSR(node_id) >> 10U) & 0x1FU), \
		((RCAR_IPSR(node_id) >> 4U) & 0x1FU),  \
		((RCAR_IPSR(node_id) & 0xFU))	       \
	}

#define RCAR_PIN_FLAGS(node_id)						       \
	DT_PROP(node_id, bias_pull_up)   * RCAR_PIN_PULL_UP |		       \
	DT_PROP(node_id, bias_pull_down) * RCAR_PIN_PULL_DOWN |		       \
	DT_PROP(node_id, bias_disable)   * RCAR_PIN_PULL_DISABLE |	       \
	RCAR_HAS_IPSR(node_id) * RCAR_PIN_FLAGS_FUNC_SET

#define RCAR_DT_PIN(node_id)						       \
	{								       \
		.pin = DT_PROP_BY_IDX(node_id, pin, 0),			       \
		.func = COND_CODE_1(RCAR_HAS_IPSR(node_id),		       \
				    (RCAR_PIN_FUNC(node_id)), {0}),	       \
		.flags = RCAR_PIN_FLAGS(node_id),			       \
		.drive_strength =					       \
			COND_CODE_1(DT_NODE_HAS_PROP(node_id, drive_strength), \
				    (DT_PROP(node_id, drive_strength)), (0)),  \
		.voltage = COND_CODE_1(DT_NODE_HAS_PROP(node_id,	       \
							power_source),	       \
				       (DT_PROP(node_id, power_source)),       \
				       (PIN_VOLTAGE_NONE)),		       \
	},

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param state_prop State property name.
 * @param idx State property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx) \
	RCAR_DT_PIN(DT_PROP_BY_IDX(node_id, state_prop, idx))

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop) \
	{ DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT) }

struct pfc_drive_reg_field {
	uint16_t pin;
	uint8_t offset;
	uint8_t size;
};

struct pfc_drive_reg {
	uint32_t reg;
	const struct pfc_drive_reg_field fields[8];
};

struct pfc_bias_reg {
	uint32_t puen;		/** Pull-enable or pull-up control register */
	uint32_t pud;		/** Pull-up/down or pull-down control register */
	const uint16_t pins[32];
};

/**
 * @brief Utility macro to check if a pin is GPIO capable
 *
 * @param pin
 * @return true if pin is GPIO capable false otherwise
 */
#define RCAR_IS_GP_PIN(pin) (pin < PIN_NOGPSR_START)

#endif /* ZEPHYR_SOC_ARM_RENESAS_RCAR_COMMON_PINCTRL_SOC_H_ */
