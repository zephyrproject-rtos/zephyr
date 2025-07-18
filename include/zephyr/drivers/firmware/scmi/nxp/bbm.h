/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCMI BBNSM domain protocol helpers
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_BBM_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_BBM_H_

#include <zephyr/drivers/firmware/scmi/protocol.h>

/*!
 * @name SCMI BBM button notification flags
 */
/** @{ */
/*! Enable button notifications */
#define SCMI_BBM_NOTIFY_BUTTON_DETECT(x)  (((x) & 0x1U) << 0U)
/** @} */

/**
 * @brief Bbm protocol command message IDs
 */
enum scmi_bbm_message {
	SCMI_PROTO_BBM_PROTOCOL_RTC_EVENT = 0x0,
	SCMI_PROTO_BBM_PROTOCOL_BUTTON_EVENT = 0x1,
	SCMI_PROTO_BBM_BBM_GPR_SET = 0x3,
	SCMI_PROTO_BBM_BBM_GPR_GET = 0x4,
	SCMI_PROTO_BBM_BBM_RTC_ATTRIBUTES = 0x5,
	SCMI_PROTO_BBM_BBM_RTC_TIME_SET = 0x6,
	SCMI_PROTO_BBM_BBM_RTC_TIME_GET = 0x7,
	SCMI_PROTO_BBM_BBM_RTC_ALARM_SET = 0x8,
	SCMI_PROTO_BBM_BBM_BUTTON_GET = 0x9,
	SCMI_PROTO_BBM_BBM_RTC_NOTIFY = 0xA,
	SCMI_PROTO_BBM_BBM_BUTTON_NOTIFY = 0xB,
	SCMI_PROTO_BBM_BBM_RTC_STATE = 0xC,
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

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_BBM_H_ */
