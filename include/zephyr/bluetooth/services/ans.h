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
 * @brief ANS max text string size
 *
 * This is the max string length to be saved in a New Alert. Text longer than the max is truncated.
 */
#define BT_ANS_MAX_TEXT_STR_SIZE 18

/**
 * @brief ANS Category ID Type
 */
typedef uint8_t bt_ans_cat_t;

/**
 * @brief ANS Category ID Enum
 *
 * Enumeration for whether the category is supported.
 */
enum {
	bt_ans_cat_simple_alert,
	bt_ans_cat_email,
	bt_ans_cat_news,
	bt_ans_cat_call,
	bt_ans_cat_missed_call,
	bt_ans_cat_sms_mms,
	bt_ans_cat_voice_mail,
	bt_ans_cat_schedule,
	bt_ans_cat_high_pri_alert,
	bt_ans_cat_instant_message,
	/* Marker for number of categories */
	bt_ans_cat_num,
	/* 10–15 reserved */
};

/**
 * @brief Send a new alert to central
 *
 * The new alert is transmitted to the central if notifications are enabled. Each category will save
 * the latest call to this function in case an immediate replay is requested via the ANS control
 * point.
 *
 * @param category The bt_ans_cat_t category the notification is for
 * @param num_new Number of new alerts since last alert
 * @param text Text brief of alert
 * @param text_len String length of text param. If greater than MAX_TEXT_STR_SIZE, text is truncated
 *
 * @return 0 on success
 * @return negative error codes on failure
 */
int bt_ans_notify_new_alert(bt_ans_cat_t category, uint8_t num_new, const char *text, int text_len);

/**
 * @brief Set the total unread count for a given category
 *
 * The unread count is transmitted to the central if notifications are enabled. Each category will
 * save the latest call to this function in case an immediate replay is requested via the ANS
 * control point.
 *
 * @param category The bt_ans_cat_t category the unread count is for
 * @param unread Total number of unread alerts
 *
 * @return 0 on success
 * @return negative error codes on failure
 */
int bt_ans_set_unread_count(bt_ans_cat_t category, uint8_t unread);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_BT_ANS_H_ */
