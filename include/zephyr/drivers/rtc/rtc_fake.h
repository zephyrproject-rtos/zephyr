/*
 * Copyright (c) 2023 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_FAKE_H_

#include <zephyr/drivers/rtc.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_set_time, const struct device *, const struct rtc_time *);
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_get_time, const struct device *, struct rtc_time *);

#ifdef CONFIG_RTC_ALARM
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_get_supported_fields, const struct device *, uint16_t,
			uint16_t *);
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_set_time, const struct device *, uint16_t, uint16_t,
			const struct rtc_time *);
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_get_time, const struct device *, uint16_t, uint16_t *,
			struct rtc_time *);
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_is_pending, const struct device *, uint16_t);
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_set_callback, const struct device *, uint16_t,
			rtc_alarm_callback, void *);
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_update_set_callback, const struct device *,
			rtc_update_callback, void *);
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_CALIBRATION
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_set_calibration, const struct device *, int32_t);
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_get_calibration, const struct device *, int32_t *);
#endif /* CONFIG_RTC_CALIBRATION */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_FAKE_H_ */
