/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCMI power domain protocol helpers
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_POWER_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_POWER_H_

#include <zephyr/drivers/firmware/scmi/protocol.h>

#define SCMI_POWER_STATE_SET_FLAGS_ASYNC BIT(0)

/**
 * @struct scmi_power_state_config
 *
 * @brief Describes the parameters for the POWER_STATE_SET
 * command
 */
struct scmi_power_state_config {
	uint32_t flags;
	uint32_t domain_id;
	uint32_t power_state;
};

/**
 * @brief Power domain protocol command message IDs
 */
enum scmi_power_domain_message {
	SCMI_POWER_DOMAIN_MSG_PROTOCOL_VERSION = 0x0,
	SCMI_POWER_DOMAIN_MSG_PROTOCOL_ATTRIBUTES = 0x1,
	SCMI_POWER_DOMAIN_MSG_PROTOCOL_MESSAGE_ATTRIBUTES = 0x2,
	SCMI_POWER_DOMAIN_MSG_POWER_DOMAIN_ATTRIBUTES = 0x3,
	SCMI_POWER_DOMAIN_MSG_POWER_STATE_SET = 0x4,
	SCMI_POWER_DOMAIN_MSG_POWER_STATE_GET = 0x5,
	SCMI_POWER_DOMAIN_MSG_POWER_STATE_NOTIFY = 0x6,
	SCMI_POWER_DOMAIN_MSG_POWER_STATE_CHANGE_REQEUSTED_NOTIFY = 0x7,
	SCMI_POWER_DOMAIN_MSG_POWER_DOMAIN_NAME_GET = 0x8,
	SCMI_POWER_DOMAIN_MSG_NEGOTIATE_PROTOCOL_VERSION = 0x10,
};

/**
 * @brief Send the POWER_STATE_SET command and get its reply
 *
 * @param cfg pointer to structure containing configuration
 * to be set
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_power_state_set(struct scmi_power_state_config *cfg);
/**
 * @brief Query the power domain state
 *
 * @param domain_id ID of the power domain for which the query is done
 * @param power_state pointer to be set via this command
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_power_state_get(uint32_t domain_id, uint32_t *power_state);

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_POWER_H_ */
