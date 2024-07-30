/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_PINCTRL_SOC_H_
#define _NUVOTON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

/**
 * @brief Pinctrl node types in NPCX series
 */
enum npcx_pinctrl_type {
	NPCX_PINCTRL_TYPE_PERIPH,
	NPCX_PINCTRL_TYPE_DEVICE_CTRL,
	NPCX_PINCTRL_TYPE_PSL_IN,
	NPCX_PINCTRL_TYPE_RESERVED,
};

/**
 * @brief Suppoerted peripheral device configuration type in NPCX series
 */
enum npcx_periph_type {
	NPCX_PINCTRL_TYPE_PERIPH_PINMUX,
	NPCX_PINCTRL_TYPE_PERIPH_PUPD,
	NPCX_PINCTRL_TYPE_PERIPH_DRIVE,
};

/**
 * @brief Suppoerted IO bias type in NPCX series
 */
enum npcx_io_bias_type {
	NPCX_BIAS_TYPE_NONE,
	NPCX_BIAS_TYPE_PULL_DOWN,
	NPCX_BIAS_TYPE_PULL_UP,
};

/**
 * @brief Suppoerted IO drive type in NPCX series
 */
enum npcx_io_drive_type {
	NPCX_DRIVE_TYPE_PUSH_PULL,
	NPCX_DRIVE_TYPE_OPEN_DRAIN,
};

/**
 * @brief Suppoerted PSL input detection mode in NPCX series
 */
enum npcx_psl_in_mode {
	NPCX_PSL_IN_MODE_LEVEL,
	NPCX_PSL_IN_MODE_EDGE,
};

/**
 * @brief Suppoerted PSL input detection polarity in NPCX series
 */
enum npcx_psl_in_pol {
	NPCX_PSL_IN_POL_LOW,
	NPCX_PSL_IN_POL_HIGH,
};

/**
 * @brief NPCX peripheral device configuration structure
 *
 * Used to indicate the peripheral device's corresponding register/bit for
 * pin-muxing, pull-up/down and so on.
 */
struct npcx_periph {
	/** Related register group for peripheral device. */
	uint16_t group: 8;
	/** Related register bit for peripheral device. */
	uint16_t bit: 3;
	/** The polarity for peripheral device functionality. */
	bool inverted: 1;
	/** The type of peripheral device configuration. */
	enum npcx_periph_type type: 2;
	/** Reserved field. */
	uint16_t reserved: 2;
} __packed;

/**
 * @brief NPCX device control structure
 *
 * Used to indicate the device's corresponding register/field for its io
 * characteristics such as tri-state, power supply type selection, and so on.
 */
struct npcx_dev_ctl {
	/** Related register offset for device configuration. */
	uint16_t offest: 5;
	/** Related register field offset for device control. */
	uint16_t field_offset: 3;
	/** Related register field size for device control. */
	uint16_t field_size: 3;
	/** field value */
	uint16_t field_value: 5;
} __packed;

/**
 * @brief NPCX Power Switch Logic (PSL) input pad configuration structure
 *
 * Used to indicate a Power Switch Logic (PSL) input detection configuration
 * such as detection polarity, port number, and so on.
 */
struct npcx_psl_input {
	/** Indicate a PSL input port number. */
	uint16_t port: 5;
	/** Related register group for detection polarity of PSL input. */
	uint16_t pol_group: 8;
	/** Related register bit for detection polarity of PSL input. */
	uint16_t pol_bit: 3;
} __packed;

/**
 * @brief Type for NPCX pin configuration. Please make sure the size of this
 *        structure is 4 bytes in case the impact of ROM usage.
 */
struct npcx_pinctrl {
	union {
		struct npcx_periph periph;
		struct npcx_dev_ctl dev_ctl;
		struct npcx_psl_input psl_in;
		uint16_t cfg_word;
	} cfg;
	struct {
		/** Indicates the current pinctrl type. */
		enum npcx_pinctrl_type type :2;
		/** Properties used for pinmuxing. */
		bool pinmux_lock :1;
		bool pinmux_gpio :1;
		/** Properties used for io-pad. */
		enum npcx_io_bias_type io_bias_type :2;
		enum npcx_io_drive_type io_drive_type :1;
		/** Properties used for PSL input. */
		enum npcx_psl_in_mode psl_in_mode :1;
		enum npcx_psl_in_pol psl_in_polarity :1;
		uint16_t reserved :7;
	} flags;
} __packed;

typedef struct npcx_pinctrl pinctrl_soc_pin_t;

/** Helper macros for NPCX pinctrl configurations. */
#define  Z_PINCTRL_NPCX_BIAS_TYPE(node_id)					\
	COND_CODE_1(DT_PROP(node_id, bias_pull_up), (NPCX_BIAS_TYPE_PULL_UP),	\
		(COND_CODE_1(DT_PROP(node_id, bias_pull_down),			\
		(NPCX_BIAS_TYPE_PULL_DOWN), (NPCX_BIAS_TYPE_NONE))))

#define  Z_PINCTRL_NPCX_DRIVE_TYPE(node_id)					\
	COND_CODE_1(DT_PROP(node_id, drive_open_drain),				\
		(NPCX_DRIVE_TYPE_OPEN_DRAIN), (NPCX_DRIVE_TYPE_PUSH_PULL))

#define Z_PINCTRL_NPCX_HAS_PUPD_PROP(node_id)		\
	UTIL_OR(DT_PROP(node_id, bias_pull_down),	\
		DT_PROP(node_id, bias_pull_up))

#define Z_PINCTRL_NPCX_HAS_DRIVE_PROP(node_id, node_periph)	\
	UTIL_AND(DT_PROP(node_id, drive_open_drain),		\
		DT_NODE_HAS_PROP(node_periph, pwm_channel))

#define Z_PINCTRL_NPCX_HAS_PSL_IN_PROP(node_id)			\
	UTIL_AND(DT_NODE_HAS_PROP(node_id, psl_in_pol),		\
		DT_NODE_HAS_PROP(node_id, psl_in_mode))

/**
 * @brief Utility macro to initialize a periphral pinmux configuration.
 *
 * @param node_id Node identifier.
 * @param prop Property name for pinmux configuration. (i.e. 'pinmux')
 */
#define Z_PINCTRL_NPCX_PERIPH_PINMUX_INIT(node_id, prop)				\
	{										\
		.flags.type = NPCX_PINCTRL_TYPE_PERIPH,					\
		.flags.pinmux_lock = DT_PROP(node_id, pinmux_locked),			\
		.flags.pinmux_gpio = DT_PROP(node_id, pinmux_gpio),			\
		.cfg.periph.type = NPCX_PINCTRL_TYPE_PERIPH_PINMUX,			\
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
#define Z_PINCTRL_NPCX_DEVICE_CONTROL_INIT(node_id, prop)			\
	{									\
		.flags.type = NPCX_PINCTRL_TYPE_DEVICE_CTRL,			\
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
#define Z_PINCTRL_NPCX_PERIPH_PUPD_INIT(node_id, prop)				\
	{									\
		.flags.type = NPCX_PINCTRL_TYPE_PERIPH,				\
		.flags.io_bias_type = Z_PINCTRL_NPCX_BIAS_TYPE(node_id),	\
		.cfg.periph.type = NPCX_PINCTRL_TYPE_PERIPH_PUPD,		\
		.cfg.periph.group = DT_PROP_BY_IDX(node_id, prop,  0),		\
		.cfg.periph.bit = DT_PROP_BY_IDX(node_id, prop,  1),		\
	},

/**
 * @brief Utility macro to initialize a periphral drive mode configuration.
 *
 * @param node_id Node identifier.
 * @param node_periph Peripheral node identifier.
 */
#define Z_PINCTRL_NPCX_PERIPH_DRIVE_INIT(node_id, node_periph)			\
	{									\
		.flags.type = NPCX_PINCTRL_TYPE_PERIPH,				\
		.flags.io_drive_type = Z_PINCTRL_NPCX_DRIVE_TYPE(node_id),	\
		.cfg.periph.type = NPCX_PINCTRL_TYPE_PERIPH_DRIVE,		\
		.cfg.periph.group = DT_PROP(node_periph, pwm_channel),		\
	},

/*
 * @brief Utility macro to initialize a Power Switch Logic (PSL) input detection
 *        configurations.
 *
 * @param node_id Node identifier.
 * @param prop Property name for pull-up/down configuration. (i.e. 'polarity')
 */
#define Z_PINCTRL_NPCX_PSL_IN_DETECT_CONF_INIT(node_id, prop)				\
	{										\
		.flags.type = NPCX_PINCTRL_TYPE_PSL_IN,					\
		.flags.psl_in_mode = DT_ENUM_IDX(node_id, psl_in_mode),			\
		.flags.psl_in_polarity = DT_ENUM_IDX(node_id, psl_in_pol),		\
		.cfg.psl_in.port = DT_PROP(node_id, psl_offset),			\
		.cfg.psl_in.pol_group = DT_PHA(DT_PROP(node_id, prop), alts, group),	\
		.cfg.psl_in.pol_bit = DT_PHA(DT_PROP(node_id, prop), alts, bit),	\
	},

/**
 * @brief Utility macro to initialize all peripheral confiurations for each pin.
 *
 * @param node_id Node identifier.
 * @param prop Pinctrl state property name. (i.e. 'pinctrl-0/1/2')
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)					\
	COND_CODE_1(Z_PINCTRL_NPCX_HAS_DRIVE_PROP(					\
			DT_PROP_BY_IDX(node_id, prop, idx), node_id),			\
		(Z_PINCTRL_NPCX_PERIPH_DRIVE_INIT(					\
			DT_PROP_BY_IDX(node_id, prop, idx), node_id)), ())		\
	COND_CODE_1(Z_PINCTRL_NPCX_HAS_PUPD_PROP(DT_PROP_BY_IDX(node_id, prop, idx)),	\
		(Z_PINCTRL_NPCX_PERIPH_PUPD_INIT(					\
			DT_PROP_BY_IDX(node_id, prop, idx), periph_pupd)), ())		\
	COND_CODE_1(Z_PINCTRL_NPCX_HAS_PSL_IN_PROP(DT_PROP_BY_IDX(node_id, prop, idx)),	\
		(Z_PINCTRL_NPCX_PSL_IN_DETECT_CONF_INIT(				\
			DT_PROP_BY_IDX(node_id, prop, idx), psl_polarity)), ())		\
	COND_CODE_1(DT_NODE_HAS_PROP(DT_PROP_BY_IDX(node_id, prop, idx), dev_ctl),	\
		(Z_PINCTRL_NPCX_DEVICE_CONTROL_INIT(					\
			DT_PROP_BY_IDX(node_id, prop, idx), dev_ctl)), ())		\
	COND_CODE_1(DT_NODE_HAS_PROP(DT_PROP_BY_IDX(node_id, prop, idx), pinmux),	\
		(Z_PINCTRL_NPCX_PERIPH_PINMUX_INIT(					\
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
