/*
 * Copyright (c) 2026 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_SOC_ARM_RENESAS_RCAR_X5H_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_RENESAS_RCAR_X5H_COMMON_PINCTRL_SOC_H_

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>

struct rcar_pin_func {
	uint8_t group:4; /* GPIO group 0 - 10 */
	uint8_t bit:5;   /* Bit i.e. pin 0 - 31 */
	uint8_t func:4;  /* Alternate function 0 - 15 */
};

/* Pull-up, pull-down, or bias disable is requested */
#define RCAR_PIN_FLAGS_PULL_SET BIT(0)
/* Perform on/off control of the pull resistors */
#define RCAR_PIN_FLAGS_PUEN     BIT(1)
/* Select pull-up resistor if set, pull-down otherwise */
#define RCAR_PIN_FLAGS_PUD      BIT(2)
/* Alternate function for the pin is requested */
#define RCAR_PIN_FLAGS_FUNC_SET BIT(3)

#define RCAR_PIN_PULL_UP      (RCAR_PIN_FLAGS_PULL_SET | RCAR_PIN_FLAGS_PUEN | RCAR_PIN_FLAGS_PUD)
#define RCAR_PIN_PULL_DOWN    (RCAR_PIN_FLAGS_PULL_SET | RCAR_PIN_FLAGS_PUEN)
#define RCAR_PIN_PULL_DISABLE RCAR_PIN_FLAGS_PULL_SET

/* Type for R-Car pin */
typedef struct pinctrl_soc_pin {
	uint16_t pin;
	struct rcar_pin_func func;
	uint8_t flags;
	uint16_t drive_strength_microamp;
} pinctrl_soc_pin_t;

#define RCAR_ALTSEL(node_id) DT_PROP_BY_IDX(node_id, pin, 1)
#define RCAR_HAS_ALTSEL(node_id) DT_PROP_HAS_IDX(node_id, pin, 1)

/* Offsets are defined in dt-bindings pinctrl-rcar-common.h */
#define RCAR_PIN_FUNC(node_id)					\
	{							\
		((RCAR_ALTSEL(node_id) >> 10U) & 0xFU),		\
		((RCAR_ALTSEL(node_id) >> 14U) & 0x1FU),	\
		((RCAR_ALTSEL(node_id) & 0xFU))			\
	}

#define RCAR_PIN_FLAGS(node_id)						\
	DT_PROP(node_id, bias_pull_up)   * RCAR_PIN_PULL_UP |		\
	DT_PROP(node_id, bias_pull_down) * RCAR_PIN_PULL_DOWN |		\
	DT_PROP(node_id, bias_disable)   * RCAR_PIN_PULL_DISABLE |	\
	RCAR_HAS_ALTSEL(node_id) * RCAR_PIN_FLAGS_FUNC_SET

#define RCAR_DT_PIN(node_id)								\
	{										\
		.pin = DT_PROP_BY_IDX(node_id, pin, 0),					\
		.func = COND_CODE_1(RCAR_HAS_ALTSEL(node_id),				\
				    (RCAR_PIN_FUNC(node_id)), ({0})),			\
		.flags = RCAR_PIN_FLAGS(node_id),					\
		.drive_strength_microamp =						\
			COND_CODE_1(DT_NODE_HAS_PROP(node_id, drive_strength_microamp),	\
				    (DT_PROP(node_id, drive_strength_microamp)), (0)),	\
	},

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id    Node identifier.
 * @param state_prop State property name.
 * @param idx        State property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, state_prop, idx) \
	RCAR_DT_PIN(DT_PROP_BY_IDX(node_id, state_prop, idx))

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop    Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop) \
	{ DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT) }

struct pfc_drive_reg {
	uint32_t drvctrl0; /* Driving control register 0 */
	uint32_t drvctrl1; /* Driving control register 1 */
	uint32_t drvctrl2; /* Driving control register 2 */
	const uint16_t pins[32];
};

struct pfc_bias_reg {
	uint32_t puen; /* Pull-enable register */
	uint32_t pud;  /* Pull-up/down control register */
	const uint16_t pins[32];
};

#endif /* ZEPHYR_SOC_ARM_RENESAS_RCAR_X5H_COMMON_PINCTRL_SOC_H_ */
