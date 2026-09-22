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
 * @driver_fake{rtc_interface,CONFIG_RTC_FAKE,zephyr\,fake-rtc}
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

/** @fake_of{rtc_driver_api::set_time} */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_set_time, const struct device *, const struct rtc_time *);
/** @fake_of{rtc_driver_api::get_time} */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_get_time, const struct device *, struct rtc_time *);

#if defined(CONFIG_RTC_ALARM) || defined(__DOXYGEN__)
/**
 * @fake_of{rtc_driver_api::alarm_get_supported_fields}
 * @kconfig_dep{CONFIG_RTC_ALARM}
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_get_supported_fields, const struct device *, uint16_t,
			uint16_t *);
/**
 * @fake_of{rtc_driver_api::alarm_set_time}
 * @kconfig_dep{CONFIG_RTC_ALARM}
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_set_time, const struct device *, uint16_t, uint16_t,
			const struct rtc_time *);
/**
 * @fake_of{rtc_driver_api::alarm_get_time}
 * @kconfig_dep{CONFIG_RTC_ALARM}
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_get_time, const struct device *, uint16_t, uint16_t *,
			struct rtc_time *);
/**
 * @fake_of{rtc_driver_api::alarm_is_pending}
 * @kconfig_dep{CONFIG_RTC_ALARM}
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_is_pending, const struct device *, uint16_t);
/**
 * @fake_of{rtc_driver_api::alarm_set_callback}
 * @kconfig_dep{CONFIG_RTC_ALARM}
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_alarm_set_callback, const struct device *, uint16_t,
			rtc_alarm_callback, void *);
#endif /* CONFIG_RTC_ALARM */

#if defined(CONFIG_RTC_UPDATE) || defined(__DOXYGEN__)
/**
 * @fake_of{rtc_driver_api::update_set_callback}
 * @kconfig_dep{CONFIG_RTC_UPDATE}
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_update_set_callback, const struct device *,
			rtc_update_callback, void *);
#endif /* CONFIG_RTC_UPDATE */

#if defined(CONFIG_RTC_CALIBRATION) || defined(__DOXYGEN__)
/**
 * @fake_of{rtc_driver_api::set_calibration}
 * @kconfig_dep{CONFIG_RTC_CALIBRATION}
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_set_calibration, const struct device *, int32_t);
/**
 * @fake_of{rtc_driver_api::get_calibration}
 * @kconfig_dep{CONFIG_RTC_CALIBRATION}
 */
DECLARE_FAKE_VALUE_FUNC(int, rtc_fake_get_calibration, const struct device *, int32_t *);
#endif /* CONFIG_RTC_CALIBRATION */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_FAKE_H_ */
