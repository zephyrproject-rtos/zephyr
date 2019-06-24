/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @brief Real-time clock control based on the DS3231 counter API. */
#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_DS3231_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_DS3231_H_

#include <zephyr/types.h>
#include <time.h>
#include <counter.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Constants corresponding to bits in the DS3231 control and
 * status registers. */
enum RTC_DS3231_CTRLSTAT {
	/* Bit definitions removed for expository purposes */
	RTC_DS3231_CTRLSTAT_BIT0 = BIT(0),
};

/** @brief Control match on day of week versus day of month */
#define RTC_DS3231_ALARM_FLAGS_DOW BIT(4)

/** @brief Indicates whether the corresponding alarm is enabled */
#define RTC_DS3231_ALARM_FLAGS_ENABLED BIT(7)

/** @brief Signature for DS3231 alarm callbacks. */
typedef void (*rtc_ds3231_alarm_callback_handler_t)(struct device *dev,
						    u8_t ready,
						    void *user_data);

/** @brief Information defining the alarm configuration. */
struct rtc_ds3231_alarms {
	/** @brief Times for alarms. */
	time_t time[2];

	/** @brief Handler to be invoked when alarms are signalled. */
	rtc_ds3231_alarm_callback_handler_t handler;

	/** @brief User-provided pointer passed to alarm callback. */
	void *user_data;

	/** @brief Flags controlling configuration of alarms. */
	u8_t flags[2];
};

typedef int (*rtc_ds3231_api_get_ctrlstat)(struct device *dev);

typedef int (*rtc_ds3231_api_get_alarms)(struct device *dev,
					 struct rtc_ds3231_alarms *cfg);

typedef int (*rtc_ds3231_api_set_alarms)(struct device *dev,
					 const struct rtc_ds3231_alarms *cfg);

struct rtc_ds3231_driver_api {
	struct counter_driver_api counter_api;
	rtc_ds3231_api_get_ctrlstat get_ctrlstat;
	rtc_ds3231_api_get_alarms get_alarms;
	rtc_ds3231_api_set_alarms set_alarms;
};

/** @brief Read the DS3231 control and status.
 *
 * The control register (0x0E) and status register (0x0F) of the
 * DS3231 are accessed and mutated through a combined 16-bit word,
 * where the lower byte is the control register and the upper byte is
 * the status register.  The fields of this word are defined in
 * RTC_DS3231_CTRLSTAT.

 * @param dev the DS3231 device pointer.
 *
 * @return a non-negative value corresponding to a combined
 * control/status word, or a negative error.
 */
__syscall int rtc_ds3231_get_ctrlstat(struct device *dev);

static inline int z_impl_rtc_ds3231_get_ctrlstat(struct device *dev)
{
	const struct rtc_ds3231_driver_api *api =
		(const struct rtc_ds3231_driver_api *)dev->driver_api;

	return api->get_ctrlstat(dev);
}

/** @brief Read the DS3231 alarm configuration. */
__syscall int rtc_ds3231_get_alarms(struct device *dev,
				    struct rtc_ds3231_alarms *cfg);

static inline int z_impl_rtc_ds3231_get_alarms(struct device *dev,
					       struct rtc_ds3231_alarms *cfg)
{
	const struct rtc_ds3231_driver_api *api =
		(const struct rtc_ds3231_driver_api *)dev->driver_api;

	return api->get_alarms(dev, cfg);
}

/** @brief Configure the DS3231 alarms.*/
__syscall int rtc_ds3231_set_alarms(struct device *dev,
				    const struct rtc_ds3231_alarms *cfg);

static inline int z_impl_rtc_ds3231_set_alarms(struct device *dev,
					       const struct rtc_ds3231_alarms *cfg)
{
	const struct rtc_ds3231_driver_api *api =
		(const struct rtc_ds3231_driver_api *)dev->driver_api;

	return api->set_alarms(dev, cfg);
}

#ifdef __cplusplus
}
#endif

/* @todo this should be syscalls/drivers/rtc/ds3231.h */
#include <syscalls/ds3231.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_DS3231_H_ */
