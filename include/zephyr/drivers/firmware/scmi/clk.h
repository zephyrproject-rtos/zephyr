/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup scmi_clk
 * @brief Header file for the SCMI Clock Protocol.
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_CLK_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_CLK_H_

#include <zephyr/drivers/firmware/scmi/protocol.h>

/**
 * @brief Clock management operations via SCMI
 * @defgroup scmi_clk Clock Protocol
 * @ingroup scmi_protocols
 * @{
 */

/** Mask of the enable/disable field in a CLOCK_CONFIG_SET attributes word */
#define SCMI_CLK_CONFIG_DISABLE_ENABLE_MASK GENMASK(1, 0)
/** Extract the enable/disable field from a CLOCK_CONFIG_SET value @p x */
#define SCMI_CLK_CONFIG_ENABLE_DISABLE(x)\
	((uint32_t)(x) & SCMI_CLK_CONFIG_DISABLE_ENABLE_MASK)

/** Extract the number of clocks from a PROTOCOL_ATTRIBUTES reply @p x */
#define SCMI_CLK_ATTRIBUTES_CLK_NUM(x) ((x) & GENMASK(15, 0))

/** CLOCK_RATE_SET flag: perform the rate change asynchronously */
#define SCMI_CLK_RATE_SET_FLAGS_ASYNC                BIT(0)
/** CLOCK_RATE_SET flag: do not send a delayed response for async requests */
#define SCMI_CLK_RATE_SET_FLAGS_IGNORE_DELEAYED_RESP BIT(1)
/** CLOCK_RATE_SET flag: round the rate up (set) or down (cleared) */
#define SCMI_CLK_RATE_SET_FLAGS_ROUNDS_UP_DOWN       BIT(2)
/** CLOCK_RATE_SET flag: round the rate to the closest supported value */
#define SCMI_CLK_RATE_SET_FLAGS_ROUNDS_AUTO          BIT(3)

/** Version of the SCMI clock protocol supported by this driver */
#define SCMI_CLK_PROTOCOL_SUPPORTED_VERSION	0x30000

/** Clock name length (short version) */
/**
 * @brief Extract major version from SCMI protocol version
 *
 * @param v Combined 32-bit protocol version (major << 16 | minor)
 * @return Major version number
 */
#define SCMI_PROTO_VER_MAJOR(v) ((v) >> 16)

/**
 * @brief Extract minor version from SCMI protocol version
 *
 * @param v Combined 32-bit protocol version (major << 16 | minor)
 * @return Minor version number
 */
#define SCMI_PROTO_VER_MINOR(v) ((v) & 0xFFFF)

/** clock name length (short version) */
#define SCMI_CLK_NAME_LEN 16

/** Get the clock's enabled status based on given attributes */
#define SCMI_CLK_ENABLED(attributes) ((attributes) & BIT(0))

/** Check if a clock has restrictions based on given attributes */
#define SCMI_CLK_HAS_RESTRICTIONS(attributes) ((attributes) & BIT(1))

/** Check if clock allows gating/ungating based on its permissions */
#define SCMI_CLK_STATE_CONTROL_ALLOWED(permissions) ((permissions) & BIT(31))

/**
 * @struct scmi_clock_config
 *
 * @brief Describes the parameters for the CLOCK_CONFIG_SET
 * command
 */
struct scmi_clock_config {
	/** ID of the clock to configure */
	uint32_t clk_id;
	/** Configuration attributes (e.g. enable/disable, see SCMI_CLK_CONFIG_*) */
	uint32_t attributes;
	/** Extended configuration value (meaning depends on @ref attributes) */
	uint32_t extended_cfg_val;
};

/**
 * @struct scmi_clock_rate_config
 *
 * @brief Describes the parameters for the CLOCK_RATE_SET
 * command
 */
struct scmi_clock_rate_config {
	/** Rate set flags (see SCMI_CLK_RATE_SET_FLAGS_*) */
	uint32_t flags;
	/** ID of the clock whose rate is being set */
	uint32_t clk_id;
	/** 64-bit rate in Hz, split as { lower 32 bits, upper 32 bits } */
	uint32_t rate[2];
};

/**
 * @struct scmi_clock_attributes
 *
 * @brief Describes the content of the CLOCK_ATTRIBUTES command reply
 */
struct scmi_clock_attributes {
	/** clock attributes */
	uint32_t attributes;
	/** Clock name */
	uint8_t clock_name[SCMI_CLK_NAME_LEN];
	/** Clock enable delay incurred by platform */
	uint32_t clock_enable_delay;
};

/**
 * @brief Send the CLOCK_CONFIG_SET command and get its reply
 *
 * @param proto Pointer to SCMI clock protocol data
 * @param cfg Pointer to structure containing configuration
 * to be set
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_clock_config_set(struct scmi_protocol *proto,
			  struct scmi_clock_config *cfg);
/**
 * @brief Query the rate of a clock
 *
 * @param proto Pointer to SCMI clock protocol data
 * @param clk_id ID of the clock for which the query is done
 * @param rate Pointer to rate to be set via this command
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_clock_rate_get(struct scmi_protocol *proto,
			uint32_t clk_id, uint32_t *rate);

/**
 * @brief Send the CLOCK_RATE_SET command and get its reply
 *
 * @param proto Pointer to SCMI clock protocol data
 * @param cfg Pointer to structure containing configuration
 * to be set
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_clock_rate_set(struct scmi_protocol *proto, struct scmi_clock_rate_config *cfg);

/**
 * @brief Query the parent of a clock
 *
 * @param proto Pointer to SCMI clock protocol data
 * @param clk_id ID of the clock for which the query is done
 * @param parent_id Pointer to be set via this command
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_clock_parent_get(struct scmi_protocol *proto, uint32_t clk_id, uint32_t *parent_id);

/**
 * @brief Send the CLOCK_PARENT_SET command and get its reply
 *
 * @param proto Pointer to SCMI clock protocol data
 * @param clk_id ID of the clock for which the query is done
 * @param parent_id to be set via this command
 * to be set
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_clock_parent_set(struct scmi_protocol *proto, uint32_t clk_id, uint32_t parent_id);

/**
 * @brief Send the CLOCK_ATTRIBUTES command and get its reply
 *
 * @param proto Pointer to SCMI clock protocol data
 * @param clk_id ID of the clock for which the query is done
 * @param attributes Clock attributes returned by the command
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_clock_attributes(struct scmi_protocol *proto, uint32_t clk_id,
			  struct scmi_clock_attributes *attributes);

/**
 * @brief Send the CLOCK_GET_PERMISSIONS command and get its reply
 *
 * @param proto Pointer to SCMI clock protocol data
 * @param clk_id ID of the clock for which the query is done
 * @param permissions Clock permissions returned by the command
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_clock_get_permissions(struct scmi_protocol *proto, uint32_t clk_id,
			       uint32_t *permissions);
/**
 * @}
 */

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_CLK_H_ */
