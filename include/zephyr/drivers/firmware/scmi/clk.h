/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCMI clock protocol helpers
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_CLK_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_CLK_H_

#include <zephyr/drivers/firmware/scmi/protocol.h>

#define SCMI_CLK_CONFIG_DISABLE_ENABLE_MASK GENMASK(1, 0)
#define SCMI_CLK_CONFIG_ENABLE_DISABLE(x)\
	((uint32_t)(x) & SCMI_CLK_CONFIG_DISABLE_ENABLE_MASK)

#define SCMI_CLK_ATTRIBUTES_CLK_NUM(x) ((x) & GENMASK(15, 0))

#define SCMI_CLK_RATE_SET_FLAGS_ASYNC                BIT(0)
#define SCMI_CLK_RATE_SET_FLAGS_IGNORE_DELEAYED_RESP BIT(1)
#define SCMI_CLK_RATE_SET_FLAGS_ROUNDS_UP_DOWN       BIT(2)
#define SCMI_CLK_RATE_SET_FLAGS_ROUNDS_AUTO          BIT(3)

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
 * @brief Clock protocol command message IDs
 */
enum scmi_clock_message {
	SCMI_CLK_MSG_PROTOCOL_VERSION = 0x0,
	SCMI_CLK_MSG_PROTOCOL_ATTRIBUTES = 0x1,
	SCMI_CLK_MSG_PROTOCOL_MESSAGE_ATTRIBUTES = 0x2,
	SCMI_CLK_MSG_CLOCK_ATTRIBUTES = 0x3,
	SCMI_CLK_MSG_CLOCK_DESCRIBE_RATES = 0x4,
	SCMI_CLK_MSG_CLOCK_RATE_SET = 0x5,
	SCMI_CLK_MSG_CLOCK_RATE_GET = 0x6,
	SCMI_CLK_MSG_CLOCK_CONFIG_SET = 0x7,
	SCMI_CLK_MSG_CLOCK_NAME_GET = 0x8,
	SCMI_CLK_MSG_CLOCK_RATE_NOTIFY = 0x9,
	SCMI_CLK_MSG_CLOCK_RATE_CHANGE_REQUESTED_NOTIFY = 0xa,
	SCMI_CLK_MSG_CLOCK_CONFIG_GET = 0xb,
	SCMI_CLK_MSG_CLOCK_POSSIBLE_PARENTS_GET = 0xc,
	SCMI_CLK_MSG_CLOCK_PARENT_SET = 0xd,
	SCMI_CLK_MSG_CLOCK_PARENT_GET = 0xe,
	SCMI_CLK_MSG_CLOCK_GET_PERMISSIONS = 0xf,
	SCMI_CLK_MSG_NEGOTIATE_PROTOCOL_VERSION = 0x10,
};

/**
 * @brief Send the PROTOCOL_ATTRIBUTES command and get its reply
 *
 * @param proto pointer to SCMI clock protocol data
 * @param attributes pointer to attributes to be set via
 * this command
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_clock_protocol_attributes(struct scmi_protocol *proto,
				   uint32_t *attributes);

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

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_CLK_H_ */
