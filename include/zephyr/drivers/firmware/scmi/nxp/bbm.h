/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCMI NXP BBNSM domain protocol helpers
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_BBM_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_BBM_H_

#include <zephyr/drivers/firmware/scmi/protocol.h>

/**
 * @brief SCMI NXP BBM protocol ID
 */
#define SCMI_PROTOCOL_NXP_BBM	129

/**
 * @brief SCMI NXP BBM protocol supported version
 */
#define SCMI_NXP_BBM_PROTOCOL_SUPPORTED_VERSION	0x10000

/**
 * @name SCMI NXP BBM button notification flags
 * @{
 */

/**
 * @brief Enable button notifications
 * @param x Enable flag (0 = disable, 1 = enable)
 */
#define SCMI_BBM_NOTIFY_BUTTON_DETECT(x) (((x) & 0x1U) << 0U)

/** @} */

/**
 * @brief BBM protocol command message IDs
 */
enum scmi_bbm_message {
	/** RTC event notification */
	SCMI_PROTO_BBM_PROTOCOL_RTC_EVENT = 0x0,
	/** Button event notification */
	SCMI_PROTO_BBM_PROTOCOL_BUTTON_EVENT = 0x1,
	/** Set general purpose register */
	SCMI_PROTO_BBM_BBM_GPR_SET = 0x3,
	/** Get general purpose register */
	SCMI_PROTO_BBM_BBM_GPR_GET = 0x4,
	/** Get RTC attributes */
	SCMI_PROTO_BBM_BBM_RTC_ATTRIBUTES = 0x5,
	/** Set RTC time */
	SCMI_PROTO_BBM_BBM_RTC_TIME_SET = 0x6,
	/** Get RTC time */
	SCMI_PROTO_BBM_BBM_RTC_TIME_GET = 0x7,
	/** Set RTC alarm */
	SCMI_PROTO_BBM_BBM_RTC_ALARM_SET = 0x8,
	/** Get button state */
	SCMI_PROTO_BBM_BBM_BUTTON_GET = 0x9,
	/** Enable RTC notifications */
	SCMI_PROTO_BBM_BBM_RTC_NOTIFY = 0xA,
	/** Enable button notifications */
	SCMI_PROTO_BBM_BBM_BUTTON_NOTIFY = 0xB,
	/** Get RTC state */
	SCMI_PROTO_BBM_BBM_RTC_STATE = 0xC,
	/** Negotiate protocol version */
	SCMI_PROTO_BBM_NEGOTIATE_PROTOCOL_VERSION = 0x10,
};

/**
 * @brief Set up BBM button notify message
 *
 * @param flags to enable or disable button notify for agent
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_bbm_button_notify(uint32_t flags);

/**
 * @brief Read BBM button event message
 *
 * @param flags to get notification event from agent
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_bbm_button_event(uint32_t *flags);

/**
 * @brief BBM event notification callback
 *
 * @param msg_id to determine and handle the corresponding event.
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_bbm_event_protocol_cb(int32_t msg_id);

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_BBM_H_ */
