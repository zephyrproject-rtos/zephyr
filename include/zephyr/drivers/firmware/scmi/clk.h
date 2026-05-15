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

#define SCMI_CLK_CONFIG_DISABLE_ENABLE_MASK GENMASK(1, 0)
#define SCMI_CLK_CONFIG_ENABLE_DISABLE(x)\
	((uint32_t)(x) & SCMI_CLK_CONFIG_DISABLE_ENABLE_MASK)

#define SCMI_CLK_ATTRIBUTES_CLK_NUM(x) ((x) & GENMASK(15, 0))

#define SCMI_CLK_RATE_SET_FLAGS_ASYNC                BIT(0)
#define SCMI_CLK_RATE_SET_FLAGS_IGNORE_DELEAYED_RESP BIT(1)
#define SCMI_CLK_RATE_SET_FLAGS_ROUNDS_UP_DOWN       BIT(2)
#define SCMI_CLK_RATE_SET_FLAGS_ROUNDS_AUTO          BIT(3)

#define SCMI_CLK_PROTOCOL_SUPPORTED_VERSION	0x30000

/** clock name length (short version) */
#define SCMI_CLK_NAME_LEN 16

/** get the clock's enabled status based on given attributes */
#define SCMI_CLK_ENABLED(attributes) ((attributes) & BIT(0))

/**
 * @struct scmi_clock_config
 *
 * @brief Describes the parameters for the CLOCK_CONFIG_SET
 * command
 */
struct scmi_clock_config {
	uint32_t clk_id;
	uint32_t attributes;
	uint32_t extended_cfg_val;
};

/**
 * @struct scmi_clock_rate_config
 *
 * @brief Describes the parameters for the CLOCK_RATE_SET
 * command
 */
struct scmi_clock_rate_config {
	uint32_t flags;
	uint32_t clk_id;
	uint32_t rate[2];
};

/**
 * @struct scmi_clock_attributes
 *
 * @brief Describes the content of the CLOCK_ATTRIBUTES command reply
 */
struct scmi_clock_attributes {
	/** reply status */
	int32_t status;
	/** clock attributes */
	uint32_t attributes;
	/** clock name */
	uint8_t clock_name[SCMI_CLK_NAME_LEN];
	/** clock enable delay incurred by platform */
	uint32_t clock_enable_delay;
} __packed;

/**
 * @brief Send the CLOCK_CONFIG_SET command and get its reply
 *
 * @param proto pointer to SCMI clock protocol data
 * @param cfg pointer to structure containing configuration
 * to be set
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_clock_config_set(struct scmi_protocol *proto,
			  struct scmi_clock_config *cfg);
/**
 * @brief Query the rate of a clock
 *
 * @param proto pointer to SCMI clock protocol data
 * @param clk_id ID of the clock for which the query is done
 * @param rate pointer to rate to be set via this command
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_clock_rate_get(struct scmi_protocol *proto,
			uint32_t clk_id, uint32_t *rate);

/**
 * @brief Send the CLOCK_RATE_SET command and get its reply
 *
 * @param proto pointer to SCMI clock protocol data
 * @param cfg pointer to structure containing configuration
 * to be set
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_clock_rate_set(struct scmi_protocol *proto, struct scmi_clock_rate_config *cfg);

/**
 * @brief Query the parent of a clock
 *
 * @param proto pointer to SCMI clock protocol data
 * @param clk_id ID of the clock for which the query is done
 * @param parent_id pointer to be set via this command
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_clock_parent_get(struct scmi_protocol *proto, uint32_t clk_id, uint32_t *parent_id);

/**
 * @brief Send the CLOCK_PARENT_SET command and get its reply
 *
 * @param proto pointer to SCMI clock protocol data
 * @param clk_id ID of the clock for which the query is done
 * @param parent_id to be set via this command
 * to be set
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_clock_parent_set(struct scmi_protocol *proto, uint32_t clk_id, uint32_t parent_id);

/**
 * @brief Send the CLOCK_ATTRIBUTES command and get its reply
 *
 * @param proto pointer to SCMI clock protocol data
 * @param clk_id ID of the clock for which the query is done
 * @param attributes clock attributes returned by the command
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_clock_attributes(struct scmi_protocol *proto, uint32_t clk_id,
			  struct scmi_clock_attributes *attributes);
/**
 * @}
 */

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_CLK_H_ */
