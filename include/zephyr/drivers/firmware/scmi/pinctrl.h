/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup scmi_pinctrl
 * @brief Header file for the SCMI Pin Control Protocol.
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_PINCTRL_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_PINCTRL_H_

#include <zephyr/drivers/firmware/scmi/protocol.h>

/**
 * @brief Pin configuration and control via SCMI
 * @defgroup scmi_pinctrl Pin Control Protocol
 * @ingroup scmi_protocols
 * @{
 */

/** Maximum number of config words carried by a PINCTRL_SETTINGS_CONFIGURE command */
#define ARM_SCMI_PINCTRL_MAX_CONFIG_SIZE (10 * 2)

/** Sentinel @ref scmi_pinctrl_settings::function value meaning "no function" */
#define SCMI_PINCTRL_NO_FUNCTION 0xFFFFFFFF

/**
 * @brief Build the attributes word of a PINCTRL_SETTINGS_CONFIGURE command
 *
 * @param fid_valid 1 if the function ID field is valid, 0 otherwise
 * @param cfg_num Number of configurations carried by the command
 * @param selector Selects what is being configured (see SCMI_PINCTRL_SELECTOR_*)
 */
#define SCMI_PINCTRL_CONFIG_ATTRIBUTES(fid_valid, cfg_num, selector)	\
	(SCMI_FIELD_MAKE(fid_valid, BIT(0), 10) |			\
	 SCMI_FIELD_MAKE(cfg_num, GENMASK(7, 0), 2) |			\
	 SCMI_FIELD_MAKE(selector, GENMASK(1, 0), 0))

/** Selector value: the settings apply to a single pin */
#define SCMI_PINCTRL_SELECTOR_PIN 0x0
/** Selector value: the settings apply to a group of pins */
#define SCMI_PINCTRL_SELECTOR_GROUP 0x1

/** Extract the number of configurations from an attributes word */
#define SCMI_PINCTRL_ATTRIBUTES_CONFIG_NUM(attributes)\
	(((attributes) & GENMASK(9, 2)) >> 2)

/**
 * Extract the fid_valid value (1 if the function ID field is valid, 0 otherwise)
 * from an attributes word
 */
#define SCMI_PINCTRL_ATTRIBUTES_FID_VALID(attributes)\
	FIELD_GET(BIT(10), attributes)

/** Version of the SCMI pin control protocol supported by this driver */
#define SCMI_PIN_CONTROL_PROTOCOL_SUPPORTED_VERSION	0x10000

/**
 * @brief Pinctrl configurations
 */
enum scmi_pinctrl_config {
	/** Default configuration */
	SCMI_PINCTRL_DEFAULT = 0,
	/** Bias the pin using a bus hold (keeper) circuit */
	SCMI_PINCTRL_BIAS_BUS_HOLD = 1,
	/** Disable all biasing */
	SCMI_PINCTRL_BIAS_DISABLE = 2,
	/** High-impedance (floating) bias */
	SCMI_PINCTRL_BIAS_HIGH_Z = 3,
	/** Pull the pin up */
	SCMI_PINCTRL_BIAS_PULL_UP = 4,
	/** Apply the default pull bias */
	SCMI_PINCTRL_BIAS_PULL_DEFAULT = 5,
	/** Pull the pin down */
	SCMI_PINCTRL_BIAS_PULL_DOWN = 6,
	/** Open-drain output drive */
	SCMI_PINCTRL_DRIVE_OPEN_DRAIN = 7,
	/** Open-source output drive */
	SCMI_PINCTRL_DRIVE_OPEN_SOURCE = 8,
	/** Push-pull output drive (note: name kept for ABI compatibility) */
	SCMI_PCINTRL_DRIVE_PUSH_PULL = 9,
	/** Output drive strength (note: name kept for ABI compatibility) */
	SCMI_PCINTRL_DRIVE_STRENGTH = 10,
	/** Input debounce time */
	SCMI_PINCTRL_INPUT_DEBOUNCE = 11,
	/** Input mode enable */
	SCMI_PINCTRL_INPUT_MODE = 12,
	/** Pull mode selection */
	SCMI_PINCTRL_PULL_MODE = 13,
	/** Input value */
	SCMI_PINCTRL_INPUT_VALUE = 14,
	/** Input Schmitt-trigger enable */
	SCMI_PINCTRL_INPUT_SCHMITT = 15,
	/** Low-power mode */
	SCMI_PINCTRL_LP_MODE = 16,
	/** Output mode enable */
	SCMI_PINCTRL_OUTPUT_MODE = 17,
	/** Output value */
	SCMI_PINCTRL_OUTPUT_VALUE = 18,
	/** Power source selection */
	SCMI_PINCTRL_POWER_SOURCE = 19,
	/** Output slew rate */
	SCMI_PINCTRL_SLEW_RATE = 20,
	/** First reserved configuration type */
	SCMI_PINCTRL_RESERVED_START = 21,
	/** Last reserved configuration type */
	SCMI_PINCTRL_RESERVED_END = 191,
	/** First vendor-specific configuration type */
	SCMI_PINCTRL_VENDOR_START = 192,
};

/**
 * @struct scmi_pinctrl_settings
 *
 * @brief Describes the parameters for the PINCTRL_SETTINGS_CONFIGURE
 * command
 */
struct scmi_pinctrl_settings {
	/** Identifier of the pin or group being configured */
	uint32_t id;
	/** Function to multiplex, or @ref SCMI_PINCTRL_NO_FUNCTION */
	uint32_t function;
	/** Attributes word (see @ref SCMI_PINCTRL_CONFIG_ATTRIBUTES) */
	uint32_t attributes;
	/** Array of {type, value} configuration pairs */
	uint32_t config[ARM_SCMI_PINCTRL_MAX_CONFIG_SIZE];
};

/**
 * @brief Send the PINCTRL_SETTINGS_CONFIGURE command and get its reply
 *
 * @param settings Pointer to settings to be applied
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_pinctrl_settings_configure(struct scmi_pinctrl_settings *settings);

/**
 * @}
 */

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_PINCTRL_H_ */
