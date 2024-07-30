/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 * Copyright (c) 2024 Andrew Featherstone
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_RTC_UTILS_H_
#define ZEPHYR_DRIVERS_RTC_RTC_UTILS_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/rtc.h>

/**
 * @brief Validate a datetime with a mask
 *
 * Ensure that any fields selected by mask contain a valid value.
 *
 * @param timeptr The time to set
 * @param mask Mask of fields to validate
 *
 * @return true if the required fields are valid.
 */
bool rtc_utils_validate_rtc_time(const struct rtc_time *timeptr, uint16_t mask);

#endif /*  ZEPHYR_DRIVERS_RTC_RTC_UTILS_H_ */
