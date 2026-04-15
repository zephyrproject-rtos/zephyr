/*
 * Copyright (c) 2026 Texas Instruments.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_lpf3_rtc_timer

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/timeutil.h>

#include <inc/hw_rtc.h>
#include <inc/hw_memmap.h>
#include <inc/hw_systim.h>
#include <inc/hw_types.h>
#include <inc/hw_evtsvt.h>

#include <time.h>
#include <string.h>

#include "rtc_utils.h"

#define RTC_TI_LPF3_TOP_19_BITS_MASK 0xFFFFE000
#define RTC_TI_LPF3_TOP_16_BITS_MASK 0xFFFF0000

#define RTC_TI_LPF3_SUPPORTED_FIELDS                                                               \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_WEEKDAY)

/* This is maximum value of the RTC compare event in seconds (9.5hrs approx.) */
#define RTC_TI_LPF3_MAX_ALARM_DIFFERENCE_TIME (34360U)

/* RTC comparator event register (8us), CCH08U has a resolution of 8us.
 * This is a helper macro to convert seconds to 8 microsecond units.
 */

#define RTC_TI_LPF3_SECONDS_TO_8US(x) ((x) * 125000)

struct rtc_ti_lpf3_data {
	struct k_spinlock lock;
	uint64_t time_offset;
#if CONFIG_RTC_ALARM
	uint32_t alarm_mask;
	uint64_t sec_alarm_time;
	uint64_t alarm_setting_time;
	uint32_t alarm_compare_value;
	rtc_alarm_callback alarm_callback;
	int64_t net_compare_sec;
	void *alarm_callback_data;
	bool alarm_pending;
	bool alarm_enabled;
	struct rtc_time alarm_time;
	uint32_t next_alarm_8u;
#endif /* CONFIG_RTC_ALARM */

#if CONFIG_RTC_UPDATE
	bool update_enabled;
	rtc_update_callback update_callback;
	void *update_callback_data;
	uint32_t next_update_8u;
#endif /* CONFIG_RTC_UPDATE */
};

struct rtc_ti_lpf3_config {
	void (*irq_config)(void);
};

#if CONFIG_RTC_ALARM

static int64_t rtc_ti_lpf3_get_masked_alarm_timestamp(int64_t current_timestamp,
						      const struct rtc_time *alarm_time,
						      uint16_t mask)
{
	struct tm candidate;
	int64_t result;

	gmtime_r(&current_timestamp, &candidate);

	/*
	 * Mask of date/time fields excluding WEEKDAY, used to determine which
	 * sub-fields to zero.  WEEKDAY alarms preserve the current time-of-day
	 * so they must not trigger zeroing of hour/minute/second.
	 */
	uint16_t dt_mask = mask & ~RTC_ALARM_TIME_MASK_WEEKDAY;

	/*
	 * Zero sub-fields that are finer than the finest masked date/time
	 * field and are not themselves masked.  This prevents unmasked fields
	 * from inheriting stale values from the current time.
	 * Example: MONTHDAY-only mask → hour, minute, second should be 0.
	 * Example: MINUTE-only mask   → second should be 0.
	 * Example: SECOND|MONTHDAY   → nothing should be zeroed (SECOND is
	 *                               the finest masked field).
	 *
	 * Use the lowest set bit of dt_mask to find the finest masked field.
	 * Comparing against the full dt_mask would incorrectly zero finer
	 * fields when a coarser field (e.g. MONTHDAY) is also present.
	 */
	uint16_t finest_dt_field = dt_mask & (uint16_t)(-dt_mask);

	if (!(mask & RTC_ALARM_TIME_MASK_SECOND) &&
	    (finest_dt_field > RTC_ALARM_TIME_MASK_SECOND)) {
		candidate.tm_sec = 0;
	}
	if (!(mask & RTC_ALARM_TIME_MASK_MINUTE) &&
	    (finest_dt_field > RTC_ALARM_TIME_MASK_MINUTE)) {
		candidate.tm_min = 0;
	}
	if (!(mask & RTC_ALARM_TIME_MASK_HOUR) && (finest_dt_field > RTC_ALARM_TIME_MASK_HOUR)) {
		candidate.tm_hour = 0;
	}

	/* Apply masked fields from alarm_time onto the candidate time,
	 * then advance the next-coarser unit until the result is strictly
	 * in the future. The loop limit prevents an infinite loop if the
	 * mask combination is unsatisfiable (e.g. WEEKDAY + MONTHDAY conflict).
	 */
	for (int i = 0; i < 400; i++) {

		if (mask & RTC_ALARM_TIME_MASK_SECOND) {
			candidate.tm_sec = alarm_time->tm_sec;
		}
		if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
			candidate.tm_min = alarm_time->tm_min;
		}
		if (mask & RTC_ALARM_TIME_MASK_HOUR) {
			candidate.tm_hour = alarm_time->tm_hour;
		}
		if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
			candidate.tm_mday = alarm_time->tm_mday;
		}
		if (mask & RTC_ALARM_TIME_MASK_MONTH) {
			candidate.tm_mon = alarm_time->tm_mon;
		}

		/*
		 * Save the month we are attempting before normalisation.
		 * timeutil_timegm64 may roll an out-of-range day into the
		 * next month (e.g. April 31 → May 1), mutating tm_mon.
		 * The monthly advance must step from the *intended* month,
		 * not the normalised overflow month, to avoid skipping months
		 * that do contain the target day.
		 */
		int try_mon = candidate.tm_mon;

		/* mktime normalises out-of-range fields and fills tm_wday */
		result = timeutil_timegm64(&candidate);

		if (result < 0) {
			return -1;
		}

		/*
		 * If this candidate is strictly in the future, verify that
		 * all masked date fields still match after normalisation.
		 * timeutil_timegm64 may roll an out-of-range day (e.g. Feb 31)
		 * into the next month, producing a mismatched tm_mday/tm_mon.
		 * Similarly check the weekday constraint if active.
		 */
		if (result > current_timestamp) {
			struct tm check;

			gmtime_r(&result, &check);
			if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) &&
			    check.tm_mday != alarm_time->tm_mday) {
				/* day overflowed into a different month */
			} else if ((mask & RTC_ALARM_TIME_MASK_MONTH) &&
				   check.tm_mon != alarm_time->tm_mon) {
				/* month mismatch (e.g. leap-day in non-leap year) */
			} else if ((mask & RTC_ALARM_TIME_MASK_WEEKDAY) &&
				   check.tm_wday != alarm_time->tm_wday) {
				/* weekday mismatch */
			} else {
				return result;
			}
		}

		/* Candidate is not yet in the future (or a masked date field
		 * mismatched after normalisation).  Advance the next-coarser
		 * unit so the next iteration tries the equivalent alarm time
		 * in the following period.
		 */
		if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
			if (mask & RTC_ALARM_TIME_MASK_MONTH) {
				/* Annual alarm (month+day fixed): advance by 1 year */
				candidate.tm_year += 1;
				candidate.tm_mon = alarm_time->tm_mon;
			} else {
				/*
				 * Monthly alarm (day fixed): advance by 1 month
				 * from the intended month, not the normalised one.
				 * Without try_mon, a day overflow (e.g. Apr 31 →
				 * May 1) would leave tm_mon pointing at May, and
				 * the +1 here would jump to June, skipping May.
				 */
				candidate.tm_mon = try_mon + 1;
			}
			candidate.tm_mday = alarm_time->tm_mday;
		} else if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
			/* Finest masked field is weekday: advance by 1 day */
			candidate.tm_mday += 1;
		} else if (mask & RTC_ALARM_TIME_MASK_HOUR) {
			/* Finest masked field is hour: advance by 1 day */
			candidate.tm_mday += 1;
			candidate.tm_hour = alarm_time->tm_hour;
		} else if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
			/* Finest masked field is minute: advance by 1 hour */
			candidate.tm_hour += 1;
			candidate.tm_min = alarm_time->tm_min;
		} else if (mask & RTC_ALARM_TIME_MASK_SECOND) {
			/* Finest masked field is second: advance by 1 minute */
			candidate.tm_min += 1;
			candidate.tm_sec = alarm_time->tm_sec;
		} else {
			/* Empty mask - return current time + 1s as a safe default */
			return current_timestamp + 1;
		}
	}

	/* Could not find a matching future timestamp */
	return -1;
}

static int rtc_ti_lpf3_alarm_set_time_internal(const struct device *dev, uint16_t id, uint16_t mask,
					       const struct rtc_time *timeptr, bool set_in_isr);

#endif /* CONFIG_RTC_ALARM */

#if CONFIG_RTC_ALARM || CONFIG_RTC_UPDATE
/**
 * Program CH0 to fire at the earliest of the pending alarm and update
 * wakeup times. Must be called with data->lock held.
 *
 * When neither feature is active the channel is fully disarmed so no
 * spurious interrupts are generated.
 */
static void rtc_ti_lpf3_schedule_ch0_locked(struct rtc_ti_lpf3_data *data)
{
	bool have_event = false;
	uint32_t next = 0;

#if CONFIG_RTC_ALARM
	if (data->alarm_enabled) {
		next = data->next_alarm_8u;
		have_event = true;
	}
#endif /* CONFIG_RTC_ALARM */

#if CONFIG_RTC_UPDATE
	if (data->update_callback != NULL) {
		if (!have_event || (int32_t)(data->next_update_8u - next) < 0) {
			next = data->next_update_8u;
		}
		have_event = true;
	}
#endif /* CONFIG_RTC_UPDATE */

	if (!have_event) {
		sys_write32(RTC_ICLR_EV0_CLR, RTC_BASE + RTC_O_ICLR);
		sys_write32(RTC_IMCLR_EV0_CLR, RTC_BASE + RTC_O_IMCLR);
		sys_write32(RTC_ARMCLR_CH0_CLR, RTC_BASE + RTC_O_ARMCLR);
		return;
	}

	sys_write32(next, RTC_BASE + RTC_O_CH0CC8U);
	sys_write32(RTC_IMSET_EV0_SET, RTC_BASE + RTC_O_IMSET);
}
#endif /* CONFIG_RTC_ALARM || CONFIG_RTC_UPDATE */

static int rtc_ti_lpf3_init(const struct device *dev)
{
	struct rtc_ti_lpf3_data *data = dev->data;
	const struct rtc_ti_lpf3_config *config = dev->config;

	data->time_offset = 0;

	/* Configure RTC to halt when CPU stopped during debug */
	sys_write32(RTC_EMU_HALT_STOP, RTC_BASE + RTC_O_EMU);

	config->irq_config();

	return 0;
}

static uint64_t rtc_ti_lpf3_get_raw_time(struct rtc_ti_lpf3_data *data)
{
	uint64_t usec;
	uint32_t time524m;

#if CONFIG_SOC_SERIES_CC27XX
	uint32_t time1u;
#endif /* CONFIG_SOC_SERIES_CC27XX */

#if CONFIG_SOC_SERIES_CC23X0
	uint32_t time8u;
#endif /* CONFIG_SOC_SERIES_CC23X0 */

	while (true) {
#if CONFIG_SOC_SERIES_CC27XX
		time1u = sys_read32(RTC_BASE + RTC_O_TIME1U);
#elif CONFIG_SOC_SERIES_CC23X0
		time8u = sys_read32(RTC_BASE + RTC_O_TIME8U);
#else
#error RTC driver not implemented for this SoC
#endif

		time524m = sys_read32(RTC_BASE + RTC_O_TIME524M);

#if CONFIG_SOC_SERIES_CC23X0
		/**
		 * Check if TIME8U wrapped around.
		 * In such case read time again to get correct
		 * concatenated value.
		 */
		if ((time8u >> 31) == 1) {
			if ((sys_read32(RTC_BASE + RTC_O_TIME8U) >> 31) == 0) {
				continue;
			}
		}

#elif CONFIG_SOC_SERIES_CC27XX
		/**
		 * Check if TIME1U wrapped around.
		 * In such case read time again to get correct
		 * concatenated value.
		 */
		if ((time1u >> 31) == 1) {
			if ((sys_read32(RTC_BASE + RTC_O_TIME1U) >> 31) == 0) {
				continue;
			}
		}
#else
#error RTC driver not implemented for this SoC
#endif

		break;
	}

#if CONFIG_SOC_SERIES_CC27XX
	usec = (uint64_t)(time524m & RTC_TI_LPF3_TOP_19_BITS_MASK) << 19;
	usec |= time1u;
#endif

#if CONFIG_SOC_SERIES_CC23X0
	usec = (uint64_t)(time524m & RTC_TI_LPF3_TOP_16_BITS_MASK) << 19;
	usec |= (((uint64_t)time8u) << 3);
#endif

	usec = usec + data->time_offset;

	return usec;
}

static void rtc_ti_lpf3_set_dtime(struct rtc_ti_lpf3_data *data, int64_t delta_us)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	data->time_offset = (uint64_t)((int64_t)data->time_offset + (int64_t)delta_us);

	k_spin_unlock(&data->lock, key);
}

static int rtc_ti_lpf3_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_ti_lpf3_data *data = dev->data;
	int64_t sec_unix_target;
	int64_t usec_hw_target;
	uint64_t usec_hw_time;
	int64_t delta_us;
	k_spinlock_key_t key;
#if CONFIG_RTC_ALARM
	bool alarm_enabled;
	int ret;
#endif /* CONFIG_RTC_ALARM */

	if (!timeptr || (timeptr->tm_year < 0)) {
		return -EINVAL;
	}

	sec_unix_target = timeutil_timegm64((const struct tm *)timeptr);

	if (sec_unix_target > INT64_MAX / USEC_PER_SEC) {
		return -ERANGE;
	}

	usec_hw_target = sec_unix_target * USEC_PER_SEC + (timeptr->tm_nsec / 1000);

	key = k_spin_lock(&data->lock);

	usec_hw_time = rtc_ti_lpf3_get_raw_time(data);

#if CONFIG_RTC_ALARM
	alarm_enabled = data->alarm_enabled;
#endif /* CONFIG_RTC_ALARM */

	k_spin_unlock(&data->lock, key);

	delta_us = usec_hw_target - usec_hw_time;

	rtc_ti_lpf3_set_dtime(data, delta_us);

#if CONFIG_RTC_ALARM
	if (alarm_enabled) {
		ret = rtc_ti_lpf3_alarm_set_time_internal(dev, 0, data->alarm_mask,
							  &data->alarm_time, false);

		return ret;
	}
#endif /* CONFIG_RTC_ALARM */

	return 0;
}

static int rtc_ti_lpf3_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct rtc_ti_lpf3_data *data = dev->data;
	uint64_t sec_hw_time;
	uint64_t usec_hw_time;
	uint64_t sec_unix_time;
	k_spinlock_key_t key;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	usec_hw_time = rtc_ti_lpf3_get_raw_time(data);
	k_spin_unlock(&data->lock, key);

	sec_hw_time = usec_hw_time / USEC_PER_SEC;
	sec_unix_time = sec_hw_time;

	gmtime_r(&sec_unix_time, (struct tm *)timeptr);

	timeptr->tm_nsec = (((int)(usec_hw_time % USEC_PER_SEC)) * 1000);

	/* Unsupported */
	timeptr->tm_isdst = -1;

	return 0;
}

#if CONFIG_RTC_ALARM

static int rtc_ti_lpf3_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						  uint16_t *mask)
{
	if ((mask == NULL) || (id != 0)) {
		return -EINVAL;
	}

	*mask = RTC_TI_LPF3_SUPPORTED_FIELDS;

	return 0;
}

static int rtc_ti_lpf3_alarm_set_time_internal(const struct device *dev, uint16_t id, uint16_t mask,
					       const struct rtc_time *timeptr, bool set_in_isr)
{
	k_spinlock_key_t key;
	struct rtc_ti_lpf3_data *data = dev->data;

	uint64_t usec_hw_time = 0;
	uint64_t sec_target_alarm_time = 0;
	uint64_t sec_hw_time;
	int64_t sec_time_to_alarm;
	uint32_t ch0cc8u;
	uint32_t time8u;

	if (id != 0 || ((mask & RTC_TI_LPF3_SUPPORTED_FIELDS) != mask)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (mask == 0) {
		data->alarm_enabled = false;

		if (!set_in_isr) {
			data->alarm_pending = false;
		}

		/* Reschedule CH0: may remain armed for an active update */
		rtc_ti_lpf3_schedule_ch0_locked(data);
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	usec_hw_time = rtc_ti_lpf3_get_raw_time(data);

	sec_hw_time = usec_hw_time / USEC_PER_SEC;

	data->alarm_setting_time = sec_hw_time;

	data->alarm_mask = mask;

	sec_target_alarm_time = rtc_ti_lpf3_get_masked_alarm_timestamp(sec_hw_time, timeptr, mask);

	sec_time_to_alarm = (int64_t)sec_target_alarm_time - (int64_t)sec_hw_time;

	data->net_compare_sec = sec_time_to_alarm;

	if ((sec_time_to_alarm < 0LL)) {
		data->alarm_enabled = false;

		if (!set_in_isr) {
			data->alarm_pending = false;
		}

		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	data->sec_alarm_time = sec_target_alarm_time;

	/**
	 * If the number of seconds pending for the next alarm is greater than the
	 * maximum value of the compare channel, then, configure the next alarm to
	 * occur at the maximum possible value of the compare channel. Otherwise,
	 * directly write the value of the pending seconds for the alarm to the
	 * compare register + current timestamp.
	 */

	time8u = sys_read32(RTC_BASE + RTC_O_TIME8U);

	if (sec_time_to_alarm > RTC_TI_LPF3_MAX_ALARM_DIFFERENCE_TIME) {
		ch0cc8u = (uint32_t)(RTC_TI_LPF3_SECONDS_TO_8US(
				  RTC_TI_LPF3_MAX_ALARM_DIFFERENCE_TIME)) +
			  time8u;
	} else {
		ch0cc8u = (uint32_t)(RTC_TI_LPF3_SECONDS_TO_8US(sec_time_to_alarm)) + time8u;
	}

	data->alarm_enabled = true;
	data->alarm_time = *timeptr;

	if (!set_in_isr) {
		data->alarm_pending = false;
	}

	data->alarm_compare_value = ch0cc8u;
	data->next_alarm_8u = ch0cc8u;

	/* Schedule CH0 to the earliest of this alarm and any active update */
	rtc_ti_lpf3_schedule_ch0_locked(data);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int rtc_ti_lpf3_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				      const struct rtc_time *timeptr)
{
	return rtc_ti_lpf3_alarm_set_time_internal(dev, id, mask, timeptr, false);
}

static int rtc_ti_lpf3_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				      struct rtc_time *timeptr)
{
	k_spinlock_key_t key;
	struct rtc_ti_lpf3_data *data = dev->data;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	if (id != 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	*mask = data->alarm_mask;
	gmtime_r(&data->sec_alarm_time, (struct tm *)timeptr);

	/* Unsupported */
	timeptr->tm_isdst = -1;
	/* Unsupported */
	timeptr->tm_nsec = 0;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int rtc_ti_lpf3_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct rtc_ti_lpf3_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	ret = 0;

	if (id != 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (data->alarm_pending) {
		data->alarm_pending = false;
		ret = 1;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int rtc_ti_lpf3_alarm_set_callback(const struct device *dev, uint16_t id,
					  rtc_alarm_callback callback, void *user_data)
{
	struct rtc_ti_lpf3_data *data = dev->data;
	k_spinlock_key_t key;

	if (id != 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	/*
	 * Only update the callback pointers. The alarm hardware stays armed
	 * so the caller can still detect the alarm via alarm_is_pending().
	 * To fully stop the alarm, the caller must invoke alarm_set_time()
	 * with mask == 0.
	 */
	data->alarm_callback_data = user_data;
	data->alarm_callback = callback;

	k_spin_unlock(&data->lock, key);

	return 0;
}

#endif /* CONFIG_RTC_ALARM */

#if CONFIG_RTC_UPDATE

static int rtc_ti_lpf3_update_set_callback(const struct device *dev, rtc_update_callback callback,
					   void *user_data)
{
	k_spinlock_key_t key;
	uint32_t time8u;
	uint32_t ch0cc8u;
	int ret = 0;

	struct rtc_ti_lpf3_data *data = dev->data;

	key = k_spin_lock(&data->lock);

	if ((callback == NULL) && (user_data == NULL)) {
		data->update_callback = NULL;
		data->update_callback_data = NULL;
		data->update_enabled = false;
		/* Reschedule CH0: may remain armed for an active alarm */
		rtc_ti_lpf3_schedule_ch0_locked(data);
	} else if (callback != NULL) {
		data->update_callback = callback;
		data->update_callback_data = user_data;
		data->update_enabled = true;

		time8u = sys_read32(RTC_BASE + RTC_O_TIME8U);

		/*
		 * Compute the wall-clock sub-second position in 8us units and
		 * derive the ticks to the next wall-clock second boundary.
		 *
		 * Using (time8u * 8 + time_offset) % 1,000,000 / 8 avoids the
		 * unsigned underflow that the naive two-term subtraction produces
		 * when the offset's fractional part exceeds the hardware timer's
		 * distance to its own second boundary.
		 */
		uint32_t wall_subsec_8u = (uint32_t)(((uint64_t)time8u * 8ULL + data->time_offset) %
						     1000000ULL / 8ULL);
		uint32_t ticks_to_next =
			(wall_subsec_8u == 0U) ? 125000U : (125000U - wall_subsec_8u);

		ch0cc8u = time8u + ticks_to_next;

		data->next_update_8u = ch0cc8u;
		/* Schedule CH0 to the earliest of this update and any active alarm */
		rtc_ti_lpf3_schedule_ch0_locked(data);
	} else {
		ret = -EINVAL;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

#endif /* CONFIG_RTC_UPDATE */

#if CONFIG_RTC_CALIBRATION

static int rtc_ti_lpf3_set_calibration(const struct device *dev, int32_t calibration)
{
	return -ENOTSUP;
}

static int rtc_ti_lpf3_get_calibration(const struct device *dev, int32_t *calibration)
{
	return -ENOTSUP;
}

#endif /* CONFIG_RTC_CALIBRATION */

static void rtc_ti_lpf3_isr(const struct device *dev)
{
#if CONFIG_RTC_ALARM || CONFIG_RTC_UPDATE
	struct rtc_ti_lpf3_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t time8u;
	bool fire_alarm = false;
	bool fire_update = false;

	if (!(sys_read32(RTC_BASE + RTC_O_RIS) & 1)) {
		return;
	}

	sys_write32(RTC_ICLR_EV0_CLR, RTC_BASE + RTC_O_ICLR);

	time8u = sys_read32(RTC_BASE + RTC_O_TIME8U);

	key = k_spin_lock(&data->lock);

#if CONFIG_RTC_ALARM
	if (data->alarm_enabled && ((int32_t)(time8u - data->next_alarm_8u) >= 0)) {
		if (data->net_compare_sec > RTC_TI_LPF3_MAX_ALARM_DIFFERENCE_TIME) {
			/*
			 * Still counting down toward the final alarm time.
			 * Advance by one max-interval and schedule the next
			 * partial compare without firing the alarm callback.
			 */
			data->net_compare_sec -= (int64_t)RTC_TI_LPF3_MAX_ALARM_DIFFERENCE_TIME;

			uint32_t next_partial =
				(data->net_compare_sec > RTC_TI_LPF3_MAX_ALARM_DIFFERENCE_TIME)
					? RTC_TI_LPF3_SECONDS_TO_8US(
						  RTC_TI_LPF3_MAX_ALARM_DIFFERENCE_TIME)
					: RTC_TI_LPF3_SECONDS_TO_8US(data->net_compare_sec);

			data->next_alarm_8u = time8u + next_partial;
			data->alarm_compare_value = data->next_alarm_8u;
		} else {
			/*
			 * Final alarm interval has elapsed. Disable the alarm so
			 * schedule_ch0_locked does not re-arm CH0 on the already-
			 * elapsed compare value.
			 *
			 * The alarm is one-shot: the application must call
			 * alarm_set_time() again if it wants another firing.
			 */
			data->alarm_enabled = false;
			data->alarm_pending = true;
			fire_alarm = true;
		}
	}
#endif /* CONFIG_RTC_ALARM */

#if CONFIG_RTC_UPDATE
	if ((data->update_callback != NULL) && ((int32_t)(time8u - data->next_update_8u) >= 0)) {
		fire_update = true;
		/* Advance to the next 1-second boundary */
		data->next_update_8u += RTC_TI_LPF3_SECONDS_TO_8US(1);
	}
#endif /* CONFIG_RTC_UPDATE */

	/*
	 * Re-arm CH0 for whichever event is next. alarm_enabled is already
	 * false when fire_alarm is set, so schedule_ch0_locked considers
	 * only the update (if any) in that case.
	 */
	rtc_ti_lpf3_schedule_ch0_locked(data);

	k_spin_unlock(&data->lock, key);

#if CONFIG_RTC_ALARM
	if (fire_alarm) {
		if (data->alarm_callback) {
			data->alarm_callback(dev, 0, data->alarm_callback_data);
		}
		/* Re-arm for the next matching occurrence */
		(void)rtc_ti_lpf3_alarm_set_time_internal(dev, 0, data->alarm_mask,
							  &data->alarm_time, true);
	}
#endif /* CONFIG_RTC_ALARM */

#if CONFIG_RTC_UPDATE
	if (fire_update && data->update_callback) {
		data->update_callback(dev, data->update_callback_data);
	}
#endif /* CONFIG_RTC_UPDATE */

#else  /* !(CONFIG_RTC_ALARM || CONFIG_RTC_UPDATE) */
	if (sys_read32(RTC_BASE + RTC_O_RIS) & 1) {
		sys_write32(RTC_ICLR_EV0_CLR, RTC_BASE + RTC_O_ICLR);
	}
#endif /* CONFIG_RTC_ALARM || CONFIG_RTC_UPDATE */
}

static DEVICE_API(rtc, rtc_ti_lpf3_driver_api) = {
	.set_time = rtc_ti_lpf3_set_time,
	.get_time = rtc_ti_lpf3_get_time,
#if CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_ti_lpf3_alarm_get_supported_fields,
	.alarm_set_time = rtc_ti_lpf3_alarm_set_time,
	.alarm_get_time = rtc_ti_lpf3_alarm_get_time,
	.alarm_is_pending = rtc_ti_lpf3_alarm_is_pending,
	.alarm_set_callback = rtc_ti_lpf3_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#if CONFIG_RTC_UPDATE
	.update_set_callback = rtc_ti_lpf3_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */
#if CONFIG_RTC_CALIBRATION
	.set_calibration = rtc_ti_lpf3_set_calibration,
	.get_calibration = rtc_ti_lpf3_get_calibration,
#endif /* CONFIG_RTC_CALIBRATION. */
};

#define RTC_TI_LPF3_DEVICE(id)                                                                     \
	static struct rtc_ti_lpf3_data rtc_ti_lpf3_data_##id;                                      \
                                                                                                   \
	static void rtc_ti_lpf3_irq_config(void)                                                   \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), rtc_ti_lpf3_isr,          \
			    DEVICE_DT_INST_GET(id), 0);                                            \
                                                                                                   \
		sys_write32(EVTSVT_CPUIRQ0SEL_PUBID_AON_RTC_COMB,                                  \
			    EVTSVT_BASE + (EVTSVT_O_CPUIRQ0SEL + (4 * DT_INST_IRQN(id))));         \
                                                                                                   \
		irq_enable(DT_INST_IRQN(id));                                                      \
	}                                                                                          \
                                                                                                   \
	static const struct rtc_ti_lpf3_config rtc_ti_lpf3_config_##id = {                         \
		.irq_config = rtc_ti_lpf3_irq_config,                                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, rtc_ti_lpf3_init, NULL, &rtc_ti_lpf3_data_##id,                  \
			      &rtc_ti_lpf3_config_##id, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,     \
			      &rtc_ti_lpf3_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_TI_LPF3_DEVICE);
