/*
 * Copyright (c) 2025 Sean Kyer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_ANS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_ANS_H_

/**
 * @brief Alert Notification Service (ANS)
 * @defgroup bt_ans Alert Notification Service (ANS)
 * @ingroup bluetooth
 * @{
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Command not supported error code
 */
#define BT_ATT_ERR_CMD_NOT_SUP 0xa0

/**
 * @brief ANS max text string size
 *
 * This is the max string length to be saved in a New Alert. Text longer than the max is truncated.
 */
#define BT_ANS_MAX_TEXT_STR_SIZE 18

/**
 * @brief ANS Category ID Enum
 *
 * Enumeration for whether the category is supported.
 */
enum bt_ans_category {
	BT_ANS_CAT_SIMPLE_ALERT,
	BT_ANS_CAT_EMAIL,
	BT_ANS_CAT_NEWS,
	BT_ANS_CAT_CALL,
	BT_ANS_CAT_MISSED_CALL,
	BT_ANS_CAT_SMS_MMS,
	BT_ANS_CAT_VOICE_MAIL,
	BT_ANS_CAT_SCHEDULE,
	BT_ANS_CAT_HIGH_PRI_ALERT,
	BT_ANS_CAT_INSTANT_MESSAGE,
	/* Marker for number of categories */
	BT_ANS_CAT_NUM,
	/* 10–15 reserved */
};

/**
 * @brief Set the support for a given new alert category
 *
 * @param category The category to set support for
 * @param enable True if enabled, false is disabled
 *
 * @return 0 on success
 * @return negative error codes on failure
 */
int bt_ans_set_new_alert_support_category(enum bt_ans_category category, bool enable);

/**
 * @brief Set the support for a given unread new alert category
 *
 * @param category The category to set support for
 * @param enable True if enabled, false is disabled
 *
 * @return 0 on success
 * @return negative error codes on failure
 */
int bt_ans_set_unread_support_category(enum bt_ans_category category, bool enable);

/**
 * @brief Send a new alert to remote devices
 *
 * The new alert is transmitted to the remote devices if notifications are enabled. Each category
 * will save the latest call to this function in case an immediate replay is requested via the ANS
 * control point.
 *
 * @param category The category the notification is for
 * @param num_new Number of new alerts since last alert
 * @param text Text brief of alert
 * @param text_len String length of text param. If greater than MAX_TEXT_STR_SIZE, text is truncated
 *
 * @return 0 on success
 * @return negative error codes on failure
 */
int bt_ans_notify_new_alert(enum bt_ans_category category, uint8_t num_new, const char *text,
			    int text_len);

/**
 * @brief Set the total unread count for a given category
 *
 * The unread count is transmitted to the remote devices if notifications are enabled. Each category
 * will save the latest call to this function in case an immediate replay is requested via the ANS
 * control point.
 *
 * @param category The category the unread count is for
 * @param unread Total number of unread alerts
 *
 * @return 0 on success
 * @return negative error codes on failure
 */
int bt_ans_set_unread_count(enum bt_ans_category category, uint8_t unread);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_BT_ANS_H_ */
