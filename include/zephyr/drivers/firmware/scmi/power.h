/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup scmi_power
 * @brief Header file for the SCMI Power Domain Protocol.
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_POWER_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_POWER_H_

#include <zephyr/drivers/firmware/scmi/protocol.h>

/**
 * @brief Power domain state management via SCMI
 * @defgroup scmi_power Power Domain Protocol
 * @ingroup scmi_protocols
 * @{
 */

/** POWER_STATE_SET flag: perform the power state change asynchronously */
#define SCMI_POWER_STATE_SET_FLAGS_ASYNC BIT(0)

/** Version of the SCMI power domain protocol supported by this driver */
#define SCMI_POWER_DOMAIN_PROTOCOL_SUPPORTED_VERSION	0x30001

/**
 * @name SCMI power domain state parameters
 * @{
 */

/** Power state type field bit shift */
#define SCMI_POWER_STATE_TYPE_SHIFT	30U

/** Power state ID field mask */
#define SCMI_POWER_STATE_ID_MASK	(BIT(28) - 1)

/**
 * @brief Construct SCMI power state parameter
 *
 * @param type Power state type
 * @param id   Power state ID
 */
#define SCMI_POWER_STATE_PARAM(type, id) \
	((((type) & BIT(0)) << SCMI_POWER_STATE_TYPE_SHIFT) | \
	 ((id) & SCMI_POWER_STATE_ID_MASK))

/** @} */

/**
 * @name SCMI power domain generic states
 * @{
 */

/** Power domain is in ON state */
#define SCMI_POWER_STATE_GENERIC_ON	SCMI_POWER_STATE_PARAM(0, 0)

/** Power domain is in OFF state */
#define SCMI_POWER_STATE_GENERIC_OFF	SCMI_POWER_STATE_PARAM(1, 0)

/** @} */

/**
 * @struct scmi_power_state_config
 *
 * @brief Describes the parameters for the POWER_STATE_SET
 * command
 */
struct scmi_power_state_config {
	/** Request flags (see SCMI_POWER_STATE_SET_FLAGS_*) */
	uint32_t flags;
	/** ID of the power domain to configure */
	uint32_t domain_id;
	/** Power state to set (see @ref SCMI_POWER_STATE_PARAM) */
	uint32_t power_state;
};

/**
 * @brief Send the POWER_STATE_SET command and get its reply
 *
 * @param cfg Pointer to structure containing configuration to be set
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_power_state_set(struct scmi_power_state_config *cfg);
/**
 * @brief Query the power domain state
 *
 * @param domain_id ID of the power domain for which the query is done
 * @param power_state Pointer to be set via this command
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_power_state_get(uint32_t domain_id, uint32_t *power_state);

/**
 * @}
 */

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_POWER_H_ */
