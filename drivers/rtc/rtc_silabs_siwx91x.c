/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Silicon Laboratories Inc.
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include "rtc_utils.h"
#include "sl_si91x_calendar.h"

#define DT_DRV_COMPAT silabs_siwx91x_rtc

LOG_MODULE_REGISTER(siwx91x_rtc, CONFIG_RTC_LOG_LEVEL);

#define TM_YEAR_REF          1900
#define SIWX91X_RTC_YEAR_MAX 2399
#define SIWX91X_RTC_YEAR_MIN 2000
#define SIWX91X_RTC_ALARM_MASK                                                                     \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_YEAR)

struct siwx91x_rtc_config {
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
#ifdef CONFIG_RTC_ALARM
	uint32_t irq;
	void (*irq_config)(void);
#endif
#ifdef CONFIG_WUC
	struct wuc_dt_spec wuc;
#endif
	bool wakeup_source;
};

struct siwx91x_rtc_data {
	struct k_spinlock lock;
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
	uint16_t alarm_mask;
};

static bool siwx91x_rtc_is_wakeup_source(const struct device *dev)
{
	const struct siwx91x_rtc_config *config = dev->config;

#ifdef CONFIG_WUC
	return config->wakeup_source && (config->wuc.dev != NULL);
#else
	ARG_UNUSED(config);
	return false;
#endif
}

static bool siwx91x_rtc_wake_via_wuc(const struct device *dev)
{
	return siwx91x_rtc_is_wakeup_source(dev) && pm_device_wakeup_is_enabled(dev);
}

static void rtc_time_to_siwx91x_time_set(const struct rtc_time *tm,
					 sl_calendar_datetime_config_t *cldr)
{
	int full_year = tm->tm_year + TM_YEAR_REF;

	cldr->Year = (full_year % 100);
	cldr->Century = (full_year >= 2000) ? (full_year - 2000) / 100 : 0;
	cldr->Month = tm->tm_mon + 1;
	cldr->Day = tm->tm_mday;
	cldr->DayOfWeek = (RTC_DAY_OF_WEEK_T)tm->tm_wday;
	cldr->Hour = tm->tm_hour;
	cldr->Minute = tm->tm_min;
	cldr->Second = tm->tm_sec;
	cldr->MilliSeconds = tm->tm_nsec / NSEC_PER_MSEC;
}

static void siwx91x_time_to_rtc_time_set(const sl_calendar_datetime_config_t *cldr,
					 struct rtc_time *tm)
{
	int full_year = 2000 + (cldr->Century * 100) + cldr->Year;

	tm->tm_year = full_year - TM_YEAR_REF;
	tm->tm_mon = cldr->Month - 1;
	tm->tm_mday = cldr->Day;
	tm->tm_wday = (int)cldr->DayOfWeek;
	tm->tm_hour = cldr->Hour;
	tm->tm_min = cldr->Minute;
	tm->tm_sec = cldr->Second;
	tm->tm_nsec = cldr->MilliSeconds * NSEC_PER_MSEC;
}

static int siwx91x_rtc_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	sl_calendar_datetime_config_t siwx91x_time = {};
	struct siwx91x_rtc_data *data = dev->data;
	int year;
	int ret;

	year = timeptr->tm_year + TM_YEAR_REF;
	if (year < SIWX91X_RTC_YEAR_MIN || year > SIWX91X_RTC_YEAR_MAX) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	LOG_DBG("Set RTC time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
		"min = %d, sec = %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	rtc_time_to_siwx91x_time_set(timeptr, &siwx91x_time);

	ret = sl_si91x_calendar_set_date_time(&siwx91x_time);
	if (ret) {
		LOG_WRN("Set Timer returned an error - %d!", ret);
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int siwx91x_rtc_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	sl_calendar_datetime_config_t siwx91x_time = {};
	struct siwx91x_rtc_data *data = dev->data;
	int ret;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ret = sl_si91x_calendar_get_date_time(&siwx91x_time);
	if (ret != 0) {
		LOG_WRN("Get Timer returned an error - %d!", ret);
		goto unlock;
	}

	siwx91x_time_to_rtc_time_set(&siwx91x_time, timeptr);

	LOG_DBG("get time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
		"min = %d, sec = %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

#ifdef CONFIG_RTC_ALARM
static int siwx91x_alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	ARG_UNUSED(dev);

	if ((id != 0U) || (mask == NULL)) {
		return -EINVAL;
	}

	*mask = SIWX91X_RTC_ALARM_MASK;
	return 0;
}

static int siwx91x_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				  const struct rtc_time *timeptr)
{
	sl_calendar_datetime_config_t alarm = {};
	const struct siwx91x_rtc_config *config = dev->config;
	struct siwx91x_rtc_data *data = dev->data;
	sl_status_t status;
	k_spinlock_key_t key;

	if (id != 0U) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (mask == 0U) {
		RSI_RTC_AlamEnable(RTC, false);
		RSI_RTC_IntrMask(RTC_ALARM_INTR);
		data->alarm_mask = 0U;
		k_spin_unlock(&data->lock, key);
		irq_disable(config->irq);
#ifdef CONFIG_WUC
		if (siwx91x_rtc_is_wakeup_source(dev)) {
			(void)wuc_disable_wakeup_source_dt(&config->wuc);
		}
#endif
		return 0;
	}

	/*
	 * Calendar hardware only supports a full date/time comparator. It does
	 * not support the recurring/partial-match semantics of a partial mask.
	 * tm_nsec is accepted and truncated to milliseconds by
	 * rtc_time_to_siwx91x_time_set().
	 */
	if ((mask != SIWX91X_RTC_ALARM_MASK) || (timeptr == NULL) ||
	    !rtc_utils_validate_rtc_time(timeptr, mask)) {
		k_spin_unlock(&data->lock, key);
		return -ENOTSUP;
	}

	rtc_time_to_siwx91x_time_set(timeptr, &alarm);
	status = sl_si91x_calendar_set_alarm(&alarm);
	if (status == SL_STATUS_OK) {
		RSI_RTC_AlamEnable(RTC, true);
		RSI_RTC_IntrUnMask(RTC_ALARM_INTR);
		data->alarm_mask = mask;
	}
	k_spin_unlock(&data->lock, key);

	if (status != SL_STATUS_OK) {
		return -EIO;
	}

	if (data->alarm_callback != NULL) {
		irq_enable(config->irq);
	}

#ifdef CONFIG_WUC
	if (siwx91x_rtc_wake_via_wuc(dev)) {
		int ret = wuc_enable_wakeup_source_dt(&config->wuc);

		if (ret != 0) {
			(void)siwx91x_alarm_set_time(dev, id, 0U, NULL);
			return ret;
		}
	}
#endif

	return 0;
}

static int siwx91x_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				  struct rtc_time *timeptr)
{
	sl_calendar_datetime_config_t alarm = {};
	struct siwx91x_rtc_data *data = dev->data;
	sl_status_t status;
	k_spinlock_key_t key;

	if ((id != 0U) || (mask == NULL) || (timeptr == NULL)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	status = sl_si91x_calendar_get_alarm(&alarm);
	if (status == SL_STATUS_OK) {
		siwx91x_time_to_rtc_time_set(&alarm, timeptr);
		*mask = data->alarm_mask;
	}
	k_spin_unlock(&data->lock, key);

	return status == SL_STATUS_OK ? 0 : -EIO;
}

static int siwx91x_alarm_is_pending(const struct device *dev, uint16_t id)
{
	const struct siwx91x_rtc_config *config = dev->config;
	bool pending;

	if (id != 0U) {
		return -EINVAL;
	}

	pending = sl_si91x_calendar_is_alarm_trigger_enabled();

#ifdef CONFIG_WUC
	if (!pending && siwx91x_rtc_is_wakeup_source(dev)) {
		int ret = wuc_check_wakeup_source_triggered_dt(&config->wuc);

		if (ret < 0) {
			return ret;
		}
		pending = ret > 0;
	}
#endif

	if (pending) {
		sl_si91x_calendar_clear_alarm_trigger();
#ifdef CONFIG_WUC
		if (siwx91x_rtc_is_wakeup_source(dev)) {
			(void)wuc_clear_wakeup_source_triggered_dt(&config->wuc);
		}
#endif
	}

	return pending ? 1 : 0;
}

static int siwx91x_alarm_set_callback(const struct device *dev, uint16_t id,
				      rtc_alarm_callback callback, void *user_data)
{
	const struct siwx91x_rtc_config *config = dev->config;
	struct siwx91x_rtc_data *data = dev->data;
	k_spinlock_key_t key;

	if (id != 0U) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	data->alarm_callback = callback;
	data->alarm_user_data = user_data;
	k_spin_unlock(&data->lock, key);

	if (callback != NULL) {
		irq_enable(config->irq);
	} else {
		irq_disable(config->irq);
	}

	return 0;
}

static void siwx91x_rtc_alarm_isr(const void *arg)
{
	const struct device *dev = arg;
	struct siwx91x_rtc_data *data = dev->data;
	rtc_alarm_callback callback;
	void *user_data;
	k_spinlock_key_t key;

	if (!sl_si91x_calendar_is_alarm_trigger_enabled()) {
		return;
	}

	sl_si91x_calendar_clear_alarm_trigger();

	key = k_spin_lock(&data->lock);
	callback = data->alarm_callback;
	user_data = data->alarm_user_data;
	k_spin_unlock(&data->lock, key);

	if (callback != NULL) {
		callback(dev, 0U, user_data);
	}
}
#endif /* CONFIG_RTC_ALARM */

static int siwx91x_rtc_pm_action(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);

	return 0;
}

static int siwx91x_rtc_init(const struct device *dev)
{
	const struct siwx91x_rtc_config *config = dev->config;
	int ret;

	/* Not needed because rtc clock is always on and depends on uulp_lf_ref clock */
	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret && ret != -EALREADY) {
		return ret;
	}

	/* No clock selection is managed by calendar API, but IPMU is*/
	sl_si91x_calendar_init();

#ifdef CONFIG_RTC_ALARM
	config->irq_config();
#endif

	return pm_device_driver_init(dev, siwx91x_rtc_pm_action);
}

static DEVICE_API(rtc, siwx91x_rtc_driver_api) = {
	.set_time = siwx91x_rtc_set_time,
	.get_time = siwx91x_rtc_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = siwx91x_alarm_get_supported_fields,
	.alarm_set_time = siwx91x_alarm_set_time,
	.alarm_get_time = siwx91x_alarm_get_time,
	.alarm_is_pending = siwx91x_alarm_is_pending,
	.alarm_set_callback = siwx91x_alarm_set_callback,
#endif
};

#ifdef CONFIG_WUC
#define SIWX91X_RTC_WUC_INIT(inst) .wuc = WUC_DT_SPEC_INST_GET_OR(inst, {0}),
#else
#define SIWX91X_RTC_WUC_INIT(inst)
#endif

#define SIWX91X_RTC_INIT(inst)                                                                     \
	static void siwx91x_rtc_irq_config_##inst(void)                                            \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),                       \
			    siwx91x_rtc_alarm_isr, DEVICE_DT_INST_GET(inst), 0);                   \
		irq_disable(DT_INST_IRQN(inst));                                                   \
	}                                                                                          \
                                                                                                   \
	static const struct siwx91x_rtc_config siwx91x_rtc_config_##inst = {                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, clkid),          \
		.irq = DT_INST_IRQN(inst),                                                         \
		.irq_config = siwx91x_rtc_irq_config_##inst,                                       \
		SIWX91X_RTC_WUC_INIT(inst).wakeup_source =                                         \
			DT_INST_PROP_OR(inst, wakeup_source, false),                               \
	};                                                                                         \
                                                                                                   \
	static struct siwx91x_rtc_data siwx91x_rtc_data##inst;                                     \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, siwx91x_rtc_pm_action);                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &siwx91x_rtc_init, PM_DEVICE_DT_INST_GET(inst),                \
			      &siwx91x_rtc_data##inst, &siwx91x_rtc_config_##inst, POST_KERNEL,    \
			      CONFIG_RTC_INIT_PRIORITY, &siwx91x_rtc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_RTC_INIT)
