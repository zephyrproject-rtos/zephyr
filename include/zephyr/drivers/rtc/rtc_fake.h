/*
 * Copyright (c) 2023 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fake RTC driver API functions.
 * @ingroup rtc_fake
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_FAKE_H_

#include <zephyr/drivers/rtc.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fake RTC driver
 * @defgroup rtc_fake Fake RTC
 * @ingroup io_emulators
 * @ingroup rtc_interface
 *
 * The fake RTC driver implements every RTC API callback as a Fake Function
 * Framework (FFF) fake. It is enabled by @kconfig{CONFIG_RTC_FAKE} and
 * instantiated from @dtcompatible{zephyr,fake-rtc} devicetree nodes.
 *
 * Each fake is named after the API function it backs, with `rtc_` followed by
 * `fake_` (`rtc_fake_set_time()` for `rtc_set_time()`, and so on), and is paired
 * with an FFF control structure carrying an additional `_fake` suffix
 * (`rtc_fake_set_time_fake`). Test suites include this header to set return
 * values, install custom fakes, or inspect call counts and captured arguments.
 * See @rstref{mocking-fff}.
 *
 * The alarm, update and calibration fakes are only declared when
 * @kconfig{CONFIG_RTC_ALARM}, @kconfig{CONFIG_RTC_UPDATE} and
 * @kconfig{CONFIG_RTC_CALIBRATION} are enabled, respectively.
 *
 * When @kconfig{CONFIG_ZTEST} is enabled, a ztest rule resets all fakes before
 * each test case.
 *
 * @code{.c}
 * const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(fake_rtc));
 * struct rtc_time tm = {0};
 *
 * rtc_fake_set_time_fake.return_val = -EINVAL;
 *
 * zassert_equal(-EINVAL, rtc_set_time(dev, &tm));
 * zassert_equal(1, rtc_fake_set_time_fake.call_count);
 * zassert_equal(dev, rtc_fake_set_time_fake.arg0_val);
 * @endcode
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */

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

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_FAKE_H_ */
