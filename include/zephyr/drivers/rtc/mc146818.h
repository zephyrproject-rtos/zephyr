/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_MC146818_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_MC146818_H_

#include <zephyr/drivers/rtc.h>

struct rtc_mc146818_data {
	struct k_spinlock lock;
	bool alarm_pending;
	rtc_alarm_callback cb;
	void *cb_data;
	rtc_update_callback update_cb;
	void *update_cb_data;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_MC146818_H_ */
