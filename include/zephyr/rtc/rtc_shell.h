/*
 * Copyright (c) 2026 Boston Engineering
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RTC_RTC_SHELL_H_
#define ZEPHYR_INCLUDE_RTC_RTC_SHELL_H_

/**
 * @brief RTC shell command types, used for specifying which command type to set the callback for.
 */
typedef enum {
    RTC_SHELL_CMD_SET,
    RTC_SHELL_CMD_GET,
} rtc_shell_cmd_type_t;

/**
 * @brief RTC shell command event callback.
 */
typedef void(* rtc_shell_cmd_cb) (void* user_data);

/**
 * @brief Sets the RTC shell callback for a specific command type. The callback is called every time
 * the RTC is set or gotten via the RTC shell.
 *
 * @param cmd_type The command type for which to set the callback
 * @param callback The callback function to be called when the RTC is set/get via the RTC shell
 * @param user_data User data to be passed to the callback when it is called
 */
int rtc_shell_cmd_set_callback(rtc_shell_cmd_type_t cmd_type, rtc_shell_cmd_cb callback, void* user_data);

#endif /* ZEPHYR_INCLUDE_RTC_RTC_SHELL_H_ */
