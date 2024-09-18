/*
 * Copyright (c) 2023 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_fake_rtc

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/rtc/rtc_fake.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#endif /* CONFIG_ZTEST */

DEFINE_FAKE_VALUE_FUNC(int, rtc_fake_set_time, const struct device *, const struct rtc_time *);
DEFINE_FAKE_VALUE_FUNC(int, rtc_fake_get_time, const struct device *, struct rtc_time *);

#ifdef CONFIG_RTC_ALARM
DEFINE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_get_supported_fields, const struct device *, uint16_t,
		       uint16_t *);
DEFINE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_set_time, const struct device *, uint16_t, uint16_t,
		       const struct rtc_time *);
DEFINE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_get_time, const struct device *, uint16_t, uint16_t *,
		       struct rtc_time *);
DEFINE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_is_pending, const struct device *, uint16_t);
DEFINE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_set_callback, const struct device *, uint16_t,
		       rtc_alarm_callback, void *);
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
DEFINE_FAKE_VALUE_FUNC(int, rtc_fake_update_set_callback, const struct device *,
		       rtc_update_callback, void *);
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_CALIBRATION
DEFINE_FAKE_VALUE_FUNC(int, rtc_fake_set_calibration, const struct device *, int32_t);
DEFINE_FAKE_VALUE_FUNC(int, rtc_fake_get_calibration, const struct device *, int32_t *);
#endif /* CONFIG_RTC_CALIBRATION */

#ifdef CONFIG_ZTEST
static void fake_rtc_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);

	RESET_FAKE(rtc_fake_set_time);
	RESET_FAKE(rtc_fake_get_time);

#ifdef CONFIG_RTC_ALARM
	RESET_FAKE(rtc_fake_alarm_get_supported_fields);
	RESET_FAKE(rtc_fake_alarm_set_time);
	RESET_FAKE(rtc_fake_alarm_get_time);
	RESET_FAKE(rtc_fake_alarm_is_pending);
	RESET_FAKE(rtc_fake_alarm_set_callback);
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
	RESET_FAKE(rtc_fake_update_set_callback);
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_CALIBRATION
	RESET_FAKE(rtc_fake_set_calibration);
	RESET_FAKE(rtc_fake_get_calibration);
#endif /* CONFIG_RTC_CALIBRATION */
}

ZTEST_RULE(fake_rtc_reset_rule, fake_rtc_reset_rule_before, NULL);
#endif /* CONFIG_ZTEST */

static const struct rtc_driver_api rtc_fake_driver_api = {
	.set_time = rtc_fake_set_time,
	.get_time = rtc_fake_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_fake_alarm_get_supported_fields,
	.alarm_set_time = rtc_fake_alarm_set_time,
	.alarm_get_time = rtc_fake_alarm_get_time,
	.alarm_is_pending = rtc_fake_alarm_is_pending,
	.alarm_set_callback = rtc_fake_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	.update_set_callback = rtc_fake_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = rtc_fake_set_calibration,
	.get_calibration = rtc_fake_get_calibration,
#endif /* CONFIG_RTC_CALIBRATION */
};

#define RTC_FAKE_DEVICE_INIT(inst)                                                                 \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY, \
			      &rtc_fake_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_FAKE_DEVICE_INIT);
