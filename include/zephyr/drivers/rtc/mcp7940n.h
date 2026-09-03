/*
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_MCP7940N_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_MCP7940N_H_

#include <time.h>

#include <zephyr/device.h>

/** @brief Set the RTC to a given Unix time
 *
 * The RTC advances one tick per second with no access to sub-second
 * precision. This function will convert the given unix_time into seconds,
 * minutes, hours, day of the week, day of the month, month and year.
 * A Unix time of '0' means a timestamp of 00:00:00 UTC on Thursday 1st January
 * 1970.
 *
 * @param dev the MCP7940N device pointer.
 * @param unix_time Unix time to set the rtc to.
 *
 * @retval return 0 on success, or a negative error code from an I2C
 * transaction or invalid parameter.
 */
int mcp7940n_rtc_set_time(const struct device *dev, time_t unix_time);

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_MCP7940N_H_ */
