/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_SCMI_SYSTEM_H_
#define ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_SCMI_SYSTEM_H_

/**
 * @file
 * @brief SCMI System Power Management Protocol
 */

#include <zephyr/device.h>
#include <zephyr/drivers/firmware/scmi/protocol.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System protocol command message IDs
 */
enum scmi_system_message {
	SCMI_SYSTEM_MSG_PROTOCOL_VERSION = 0x0,
	SCMI_SYSTEM_MSG_PROTOCOL_ATTRIBUTES = 0x1,
	SCMI_SYSTEM_MSG_MESSAGE_ATTRIBUTES = 0x2,
	SCMI_SYSTEM_MSG_POWER_STATE_SET = 0x3,
	SCMI_SYSTEM_MSG_POWER_STATE_NOTIFY = 0x5,
	SCMI_SYSTEM_MSG_NEGOTIATE_PROTOCOL_VERSION = 0x10,
};

#define SCMI_SYSTEM_POWER_PROTOCOL_SUPPORTED_VERSION 0x20001

/**
 * @name Standard SCMI System Power States
 * @{
 */

/**< Shutdown off */
#define SCMI_SYSTEM_POWER_STATE_SHUTDOWN      0x00000000U
/**< Cold reset */
#define SCMI_SYSTEM_POWER_STATE_COLD_RESET    0x00000001U
/**< Warm reset */
#define SCMI_SYSTEM_POWER_STATE_WARM_RESET    0x00000002U
/**< Power up */
#define SCMI_SYSTEM_POWER_STATE_POWER_UP      0x00000003U
/**< Suspend */
#define SCMI_SYSTEM_POWER_STATE_SUSPEND       0x00000004U

/** @} */

/**
 * @name System Power State flags
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define SCMI_SYSTEM_POWER_FLAG_SHIFT	(0)
/** @endcond */

/**< Forceful request */
#define SCMI_SYSTEM_POWER_FLAG_FORCEFUL	(0 << SCMI_SYSTEM_POWER_FLAG_SHIFT)
/**< Graceful request */
#define SCMI_SYSTEM_POWER_FLAG_GRACEFUL	(1 << SCMI_SYSTEM_POWER_FLAG_SHIFT)

/** @} */


/*!
 * @name SCMI system message attributes
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define SCMI_SYSTEM_MSG_ATTR_SUSPEND_SHIFT	(30U)
#define SCMI_SYSTEM_MSG_ATTR_WARM_RESET_SHIFT	(31U)
/** @endcond */

/*! System suspend support */
#define SCMI_SYSTEM_MSG_ATTR_SUSPEND	(1 << SCMI_SYSTEM_MSG_ATTR_SUSPEND_SHIFT)
/*! System warm reset support */
#define SCMI_SYSTEM_MSG_ATTR_WARM_RESET	(1 << SCMI_SYSTEM_MSG_ATTR_WARM_RESET_SHIFT)

/** @} */

/**
 * @struct scmi_system_power_state_config
 * @brief System power state configuration
 */
struct scmi_system_power_state_config {
	uint32_t flags;
	uint32_t system_state;
};

/**
 * @brief Get protocol version
 *
 * @param version Protocol version
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_system_protocol_version(uint32_t *version);

/**
 * @brief Get protocol attributes
 *
 * @param attributes Protocol attributes
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_system_protocol_attributes(uint32_t *attributes);

/**
 * @brief Get protocol message attributes
 *
 * @param message_id Message ID of the message
 * @param attributes Message attributes
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_system_protocol_message_attributes(uint32_t message_id, uint32_t *attributes);

/**
 * @brief Negotiate protocol version
 *
 * @param version desired protocol version
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_system_protocol_version_negotiate(uint32_t version);

/**
 * @brief Set system power state
 *
 * @param cfg pointer to power state configuration
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_system_power_state_set(struct scmi_system_power_state_config *cfg);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_SCMI_SYSTEM_H_ */
