/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_PINCTRL_SOC_H_
#define _NUVOTON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

/**
 * @brief Pinctrl node types in NPCM series
 */
enum npcm_pinctrl_type {
	NPCM_PINCTRL_TYPE_PERIPH,
	NPCM_PINCTRL_TYPE_DEVICE_CTRL,
	NPCM_PINCTRL_TYPE_RESERVED,
};

/**
 * @brief Suppoerted peripheral device configuration type in NPCM series
 */
enum npcm_periph_type {
	NPCM_PINCTRL_TYPE_PERIPH_PINMUX,
	NPCM_PINCTRL_TYPE_PERIPH_PUPD,
};

/**
 * @brief Suppoerted IO bias type in NPCM series
 */
enum npcm_io_bias_type {
	NPCM_BIAS_TYPE_NONE,
	NPCM_BIAS_TYPE_PULL_DOWN,
	NPCM_BIAS_TYPE_PULL_UP,
};

/**
 * @brief NPCM peripheral device configuration structure
 *
 * Used to indicate the peripheral device's corresponding register/bit for
 * pin-muxing, pull-up/down and so on.
 */
struct npcm_periph {
	/** Related register group for peripheral device. */
	uint16_t group: 8;
	/** Related register bit for peripheral device. */
	uint16_t bit: 3;
	/** The polarity for peripheral device functionality. */
	bool inverted: 1;
	/** The type of peripheral device configuration. */
	enum npcm_periph_type type: 2;
	/** Reserved field. */
	uint16_t reserved: 2;
} __packed;

/**
 * @brief NPCM device control structure
 *
 * Used to indicate the device's corresponding register/field for its io
 * characteristics such as tri-state, power supply type selection, and so on.
 */
struct npcm_dev_ctl {
	/** Related register offset for device configuration. */
	uint32_t offest: 8;
	/** Related register field offset for device control. */
	uint32_t field_offset: 6;
	/** Related register field size for device control. */
	uint32_t field_size: 6;
	/** field value */
	uint32_t field_value: 12;
} __packed;

/**
 * @brief Type for NPCM pin configuration. Please make sure the size of this
 *        structure is 4 bytes in case the impact of ROM usage.
 */
struct npcm_pinctrl {
	union {
		struct npcm_periph periph;
		struct npcm_dev_ctl dev_ctl;
		uint16_t cfg_word;
	} cfg;
	struct {
		/** Indicates the current pinctrl type. */
		enum npcm_pinctrl_type type :2;
		/** Properties used for pinmuxing. */
		bool pinmux_gpio :1;
		/** Properties used for io-pad. */
		enum npcm_io_bias_type io_bias_type :2;
		uint16_t reserved :11;
	} flags;
} __packed;

typedef struct npcm_pinctrl pinctrl_soc_pin_t;

/** Helper macros for NPCM pinctrl configurations. */
#define  Z_PINCTRL_NPCM_BIAS_TYPE(node_id)					\
	COND_CODE_1(DT_PROP(node_id, bias_pull_up), (NPCM_BIAS_TYPE_PULL_UP),	\
		(COND_CODE_1(DT_PROP(node_id, bias_pull_down),			\
		(NPCM_BIAS_TYPE_PULL_DOWN), (NPCM_BIAS_TYPE_NONE))))

#define Z_PINCTRL_NPCM_HAS_PUPD_PROP(node_id)		\
	UTIL_OR(DT_PROP(node_id, bias_pull_down),	\
		DT_PROP(node_id, bias_pull_up))

/**
 * @brief Utility macro to initialize a periphral pinmux configuration.
 *
 * @param node_id Node identifier.
 * @param prop Property name for pinmux configuration. (i.e. 'pinmux')
 */
#define Z_PINCTRL_NPCM_PERIPH_PINMUX_INIT(node_id, prop)				\
	{										\
		.flags.type = NPCM_PINCTRL_TYPE_PERIPH,					\
		.flags.pinmux_gpio = DT_PROP(node_id, pinmux_gpio),			\
		.cfg.periph.type = NPCM_PINCTRL_TYPE_PERIPH_PINMUX,			\
		.cfg.periph.group = DT_PHA(DT_PROP(node_id, prop), alts, group),	\
		.cfg.periph.bit = DT_PHA(DT_PROP(node_id, prop), alts, bit),		\
		.cfg.periph.inverted = DT_PHA(DT_PROP(node_id, prop), alts, inv),	\
	},

/**
 * @brief Utility macro to initialize a periphral pinmux configuration.
 *
 * @param node_id Node identifier.
 * @param prop Property name for pinmux configuration. (i.e. 'pinmux')
 */
#define Z_PINCTRL_NPCM_DEVICE_CONTROL_INIT(node_id, prop)			\
	{									\
		.flags.type = NPCM_PINCTRL_TYPE_DEVICE_CTRL,			\
		.cfg.dev_ctl.offest = DT_PROP_BY_IDX(node_id, prop, 0),		\
		.cfg.dev_ctl.field_offset = DT_PROP_BY_IDX(node_id, prop, 1),	\
		.cfg.dev_ctl.field_size = DT_PROP_BY_IDX(node_id, prop, 2),	\
		.cfg.dev_ctl.field_value = DT_PROP_BY_IDX(node_id, prop, 3),	\
	},

/**
 * @brief Utility macro to initialize a periphral pull-up/down configuration.
 *
 * @param node_id Node identifier.
 * @param prop Property name for pull-up/down configuration. (i.e. 'periph-pupd')
 */
#define Z_PINCTRL_NPCM_PERIPH_PUPD_INIT(node_id, prop)				\
	{									\
		.flags.type = NPCM_PINCTRL_TYPE_PERIPH,				\
		.flags.io_bias_type = Z_PINCTRL_NPCM_BIAS_TYPE(node_id),	\
		.cfg.periph.type = NPCM_PINCTRL_TYPE_PERIPH_PUPD,		\
		.cfg.periph.group = DT_PROP_BY_IDX(node_id, prop,  0),		\
		.cfg.periph.bit = DT_PROP_BY_IDX(node_id, prop,  1),		\
	},

/**
 * @brief Utility macro to initialize all peripheral confiurations for each pin.
 *
 * @param node_id Node identifier.
 * @param prop Pinctrl state property name. (i.e. 'pinctrl-0/1/2')
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)					\
	COND_CODE_1(Z_PINCTRL_NPCM_HAS_PUPD_PROP(DT_PROP_BY_IDX(node_id, prop, idx)),	\
		(Z_PINCTRL_NPCM_PERIPH_PUPD_INIT(					\
			DT_PROP_BY_IDX(node_id, prop, idx), periph_pupd)), ())		\
	COND_CODE_1(DT_NODE_HAS_PROP(DT_PROP_BY_IDX(node_id, prop, idx), dev_ctl),	\
		(Z_PINCTRL_NPCM_DEVICE_CONTROL_INIT(					\
			DT_PROP_BY_IDX(node_id, prop, idx), dev_ctl)), ())		\
	COND_CODE_1(DT_NODE_HAS_PROP(DT_PROP_BY_IDX(node_id, prop, idx), pinmux),	\
		(Z_PINCTRL_NPCM_PERIPH_PINMUX_INIT(					\
			DT_PROP_BY_IDX(node_id, prop, idx), pinmux)), ())

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)	\
	{DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT)}

#endif /* _NUVOTON_PINCTRL_SOC_H_ */
