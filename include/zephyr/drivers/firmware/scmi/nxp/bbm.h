/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup scmi_nxp_bbm
 * @brief Header file for the NXP SCMI BBM Protocol.
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_NXP_BBM_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_NXP_BBM_H_

#include <zephyr/drivers/firmware/scmi/protocol.h>

/**
 * @brief NXP vendor-specific Battery-Backed Module management via SCMI
 * @defgroup scmi_nxp_bbm NXP BBM Protocol
 * @ingroup scmi_protocols
 * @{
 */

/** Protocol ID of the NXP BBM protocol */
#define SCMI_PROTOCOL_NXP_BBM 129

/** Version of the NXP SCMI BBM protocol supported by this driver */
#define SCMI_NXP_BBM_PROTOCOL_SUPPORTED_VERSION 0x10000

/** BBM button notification flag: enable button detection */
#define SCMI_BBM_NOTIFY_BUTTON_DETECT(x) (((x) & 0x1U) << 0U)

/**
 * @brief BBM protocol notification message IDs
 */
enum scmi_bbm_notification_message {
	/** RTC event notification */
	SCMI_BBM_PROTOCOL_RTC_EVENT = 0x0,
	/** Button event notification */
	SCMI_BBM_PROTOCOL_BUTTON_EVENT = 0x1,
};

/**
 * @brief BBM protocol command message IDs
 */
enum scmi_bbm_command_message {
	/** Set general purpose register */
	SCMI_BBM_GPR_SET = 0x3,
	/** Get general purpose register */
	SCMI_BBM_GPR_GET = 0x4,
	/** Get RTC attributes */
	SCMI_BBM_RTC_ATTRIBUTES = 0x5,
	/** Set RTC time */
	SCMI_BBM_RTC_TIME_SET = 0x6,
	/** Get RTC time */
	SCMI_BBM_RTC_TIME_GET = 0x7,
	/** Set RTC alarm */
	SCMI_BBM_RTC_ALARM_SET = 0x8,
	/** Get button state */
	SCMI_BBM_BUTTON_GET = 0x9,
	/** Enable RTC notifications */
	SCMI_BBM_RTC_NOTIFY = 0xA,
	/** Enable button notifications */
	SCMI_BBM_BUTTON_NOTIFY = 0xB,
	/** Get RTC state */
	SCMI_BBM_RTC_STATE = 0xC,
	/** Negotiate protocol version */
	SCMI_BBM_NEGOTIATE_PROTOCOL_VERSION = 0x10,
};

/**
 * @brief Send the BBM_BUTTON_NOTIFY command to enable/disable button notifications
 *
 * @param flags notification flags (see @ref SCMI_BBM_NOTIFY_BUTTON_DETECT)
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_bbm_button_notify(uint32_t flags);

/**
 * @brief Read a BBM button event notification from the RX channel
 *
 * @param flags pointer to store the received notification flags
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_bbm_button_event(uint32_t *flags);

/**
 * @brief BBM P2A notification callback invoked by the SCMI core
 *
 * @param proto pointer to the SCMI protocol instance
 * @param msg_id protocol-specific message ID of the received notification
 */
void scmi_bbm_event_protocol_cb(struct scmi_protocol *proto, uint32_t msg_id);

/**
 * @}
 */

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_NXP_BBM_H_ */
