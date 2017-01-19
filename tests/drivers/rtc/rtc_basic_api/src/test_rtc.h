/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RTC cases header file
 *
 * Header file for RTC cases
 */

#ifndef __TEST_RTC_H__
#define __TEST_RTC_H__

#include <rtc.h>
#include <zephyr.h>
#include <ztest.h>

#define RTC_DEVICE_NAME CONFIG_RTC_0_NAME

void test_rtc_calendar(void);
void test_rtc_alarm(void);

#endif /* __TEST_RTC_H__ */
