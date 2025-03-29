/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NUVOTON_NPCM_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_NUVOTON_NPCM_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>
#include <zephyr/dt-bindings/pinctrl/npcm-pinctrl.h>

/**
 * @brief NPCM peripheral pull-up/down configuration structure
 *
 * Used to indicate the peripheral device's corresponding group/bit for
 * pull-up/down configuration.
 */
struct npcm_periph_pupd {
	/** Properties used for io-pad. */
	uint16_t io_bias_type: 2;
	/** Reserved field. */
	uint16_t reserved: 14;
} __packed;

/**
 * @brief NPCM device control structure
 *
 * Used to indicate the device's corresponding register/field for its io
 * characteristics such as tri-state, power supply type selection, and so on.
 */
struct npcm_dev_ctl {
	/** The index for device configuration. */
	uint16_t reg_id: 11;
	/** The set value for device control. */
	bool is_set: 1;
	/** Reserved field. */
	uint16_t reserved: 4;
} __packed;

/**
 * @brief NPCM device low-voltage configuration structure
 *
 * Used to indicate the device's corresponding register/field for low-voltage
 * configuration.
 */
struct npcm_lvol {
	/** The set value for low-voltage control. */
	uint16_t level: 12;
	/** Reserved field. */
	uint16_t reserved: 4;
} __packed;

/**
 * @brief Type for NPCM pin configuration. Please make sure the size of this
 *        structure is 4 bytes in case the impact of ROM usage.
 */
struct npcm_pinctrl {
	union {
		struct npcm_periph_pupd pupd;
		struct npcm_dev_ctl dev_ctl;
		struct npcm_lvol lvol;
		uint16_t cfg_word;
	} cfg;

	struct {
		/** Indicates the current pin cfg type. */
		uint16_t type: 2;
		/** Pin number. */
		uint16_t id: 7;
		/** The pin group selection. */
		uint16_t group: 3;
		/** Reserved field. */
		uint16_t reserved: 4;
	} props;
} __packed;

typedef struct npcm_pinctrl pinctrl_soc_pin_t;

/* Pinctrl node types in NPCM series */
#define NPCM_PINCTRL_TYPE_PERIPH_PINMUX 0
#define NPCM_PINCTRL_TYPE_PERIPH_PUPD   1
#define NPCM_PINCTRL_TYPE_DEVICE_CTRL   2
#define NPCM_PINCTRL_TYPE_LVOL          3

/* Supported IO bias type in NPCM series */
#define NPCM_BIAS_TYPE_NONE      0
#define NPCM_BIAS_TYPE_PULL_UP   1
#define NPCM_BIAS_TYPE_PULL_DOWN 2

/** Helper macros for NPCM pinctrl configurations. */
#define Z_PINCTRL_NPCM_BIAS_TYPE(node_id)                                                          \
	COND_CODE_1( \
		DT_PROP(node_id, bias_pull_up), \
		(NPCM_BIAS_TYPE_PULL_UP), \
		(COND_CODE_1( \
			DT_PROP(node_id, bias_pull_down), \
			(NPCM_BIAS_TYPE_PULL_DOWN), \
			(NPCM_BIAS_TYPE_NONE)) \
		))

#define Z_PINCTRL_NPCM_HAS_PUPD_PROP(node_id)                                                      \
	UTIL_OR(DT_PROP(node_id, bias_pull_down), DT_PROP(node_id, bias_pull_up))

#define Z_PINCTRL_NPCM_PIN_ID(node_id)    DT_PROP_BY_IDX(node_id, pinmux, 0)
#define Z_PINCTRL_NPCM_PIN_GROUP(node_id) DT_PROP_BY_IDX(node_id, pinmux, 1)

/**
 * @brief Utility macro to initialize a peripheral pinmux configuration.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_NPCM_PERIPH_PINMUX_INIT(node_id)                                                 \
	{                                                                                          \
		.props.type = NPCM_PINCTRL_TYPE_PERIPH_PINMUX,                                     \
		.props.id = Z_PINCTRL_NPCM_PIN_ID(node_id),                                        \
		.props.group = Z_PINCTRL_NPCM_PIN_GROUP(node_id),                                  \
		.cfg.cfg_word = 0,                                                                 \
	},

/**
 * @brief Utility macro to initialize a device low-voltage configuration.
 *
 * @param node_id Node identifier.
 * @param prop Property name for low voltage level configuration. (i.e. 'nuvoton,voltage-level-mv')
 */
#define Z_PINCTRL_NPCM_LVOL_INIT(node_id, prop)                                                    \
	{                                                                                          \
		.props.type = NPCM_PINCTRL_TYPE_LVOL,                                              \
		.props.id = Z_PINCTRL_NPCM_PIN_ID(node_id),                                        \
		.props.group = Z_PINCTRL_NPCM_PIN_GROUP(node_id),                                  \
		.cfg.lvol.level = DT_PROP(node_id, prop),                                          \
	},

/**
 * @brief Utility macro to initialize a peripheral pinmux configuration.
 *
 * @param node_id Node identifier.
 * @param prop Property name for device control configuration. (i.e. 'nuvoton,device-control')
 */
#define Z_PINCTRL_NPCM_DEVICE_CTRL_INIT(node_id, prop)                                             \
	{                                                                                          \
		.props.type = NPCM_PINCTRL_TYPE_DEVICE_CTRL,                                       \
		.props.id = Z_PINCTRL_NPCM_PIN_ID(node_id),                                        \
		.props.group = Z_PINCTRL_NPCM_PIN_GROUP(node_id),                                  \
		.cfg.dev_ctl.reg_id = DT_PROP_BY_IDX(node_id, prop, 0),                            \
		.cfg.dev_ctl.is_set = DT_PROP_BY_IDX(node_id, prop, 1),                            \
	},

/**
 * @brief Utility macro to initialize a peripheral pull-up/down configuration.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_NPCM_PERIPH_PUPD_INIT(node_id)                                                   \
	{                                                                                          \
		.props.type = NPCM_PINCTRL_TYPE_PERIPH_PUPD,                                       \
		.props.id = Z_PINCTRL_NPCM_PIN_ID(node_id),                                        \
		.props.group = Z_PINCTRL_NPCM_PIN_GROUP(node_id),                                  \
		.cfg.pupd.io_bias_type = Z_PINCTRL_NPCM_BIAS_TYPE(node_id),                        \
	},

#define Z_PINCTRL_STATE_PIN_INIT_EXT(node_id, prop)                                                \
	Z_PINCTRL_NPCM_PERIPH_PINMUX_INIT(node_id)                                                 \
	IF_ENABLED( \
		Z_PINCTRL_NPCM_HAS_PUPD_PROP(node_id), \
		(Z_PINCTRL_NPCM_PERIPH_PUPD_INIT(node_id)) \
		)                                                                              \
	IF_ENABLED( \
		DT_NODE_HAS_PROP(node_id, nuvoton_device_control), \
		(Z_PINCTRL_NPCM_DEVICE_CTRL_INIT(node_id, nuvoton_device_control)) \
		)                                                                              \
	IF_ENABLED( \
		DT_NODE_HAS_PROP(node_id, nuvoton_voltage_level_mv), \
		(Z_PINCTRL_NPCM_LVOL_INIT(node_id, nuvoton_voltage_level_mv)) \
		)

/**
 * @brief Utility macro to initialize all peripheral configurations for each pin.
 *
 * @param node_id Node identifier.
 * @param prop Pinctrl state property name.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop)                                                    \
	IF_ENABLED( \
		DT_NODE_HAS_PROP(node_id, pinmux), \
		(Z_PINCTRL_STATE_PIN_INIT_EXT(node_id, prop)))

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), Z_PINCTRL_STATE_PIN_INIT)}

#endif /* ZEPHYR_SOC_NUVOTON_NPCM_COMMON_PINCTRL_SOC_H_ */
