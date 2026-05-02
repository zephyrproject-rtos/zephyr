/*
 * Copyright (c) 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/rtc.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_rtc_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	K_OOPS(K_SYSCALL_DRIVER_RTC(dev, set_time));
	K_OOPS(K_SYSCALL_MEMORY_READ(timeptr, sizeof(struct rtc_time)));
	return z_impl_rtc_set_time(dev, timeptr);
}
#include <zephyr/syscalls/rtc_set_time_mrsh.c>

static inline int z_vrfy_rtc_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	K_OOPS(K_SYSCALL_DRIVER_RTC(dev, get_time));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(timeptr, sizeof(struct rtc_time)));
	return z_impl_rtc_get_time(dev, timeptr);
}
#include <zephyr/syscalls/rtc_get_time_mrsh.c>

#ifdef CONFIG_RTC_ALARM
static inline int z_vrfy_rtc_alarm_get_supported_fields(const struct device *dev, uint16_t id,
							uint16_t *mask)
{
	K_OOPS(K_SYSCALL_DRIVER_RTC(dev, alarm_get_supported_fields));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(mask, sizeof(uint16_t)));
	return z_impl_rtc_alarm_get_supported_fields(dev, id, mask);
}
#include <zephyr/syscalls/rtc_alarm_get_supported_fields_mrsh.c>

static inline int z_vrfy_rtc_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
					    const struct rtc_time *timeptr)
{
	K_OOPS(K_SYSCALL_DRIVER_RTC(dev, alarm_set_time));
	K_OOPS(K_SYSCALL_MEMORY_READ(timeptr, sizeof(struct rtc_time)));
	return z_impl_rtc_alarm_set_time(dev, id, mask, timeptr);
}
#include <zephyr/syscalls/rtc_alarm_set_time_mrsh.c>

static inline int z_vrfy_rtc_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
					    struct rtc_time *timeptr)
{
	K_OOPS(K_SYSCALL_DRIVER_RTC(dev, alarm_get_time));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(mask, sizeof(uint16_t)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(timeptr, sizeof(struct rtc_time)));
	return z_impl_rtc_alarm_get_time(dev, id, mask, timeptr);
}
#include <zephyr/syscalls/rtc_alarm_get_time_mrsh.c>

static inline int z_vrfy_rtc_alarm_is_pending(const struct device *dev, uint16_t id)
{
	K_OOPS(K_SYSCALL_DRIVER_RTC(dev, alarm_is_pending));
	return z_impl_rtc_alarm_is_pending(dev, id);
}
#include <zephyr/syscalls/rtc_alarm_is_pending_mrsh.c>
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_CALIBRATION
static inline int z_vrfy_rtc_set_calibration(const struct device *dev, int32_t calibration)
{
	K_OOPS(K_SYSCALL_DRIVER_RTC(dev, set_calibration));
	return z_impl_rtc_set_calibration(dev, calibration);
}

#include <zephyr/syscalls/rtc_set_calibration_mrsh.c>

static inline int z_vrfy_rtc_get_calibration(const struct device *dev, int32_t *calibration)
{
	K_OOPS(K_SYSCALL_DRIVER_RTC(dev, get_calibration));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(calibration, sizeof(int32_t)));
	return z_impl_rtc_get_calibration(dev, calibration);
}
#include <zephyr/syscalls/rtc_get_calibration_mrsh.c>
#endif /* CONFIG_RTC_CALIBRATION */
