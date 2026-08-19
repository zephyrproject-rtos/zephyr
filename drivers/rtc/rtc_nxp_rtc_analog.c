/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_rtc_analog

#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <fsl_rtc.h>

#include "rtc_utils.h"

LOG_MODULE_REGISTER(nxp_rtc_analog, CONFIG_RTC_LOG_LEVEL);

#define NXP_RTC_ANALOG_NUM_ALARMS 3

/* Maximum retries for read collision recovery */
#define RTC_READ_RETRY_MAX 3

/*
 * Year offset between Zephyr rtc_time (years since 1900)
 * and YEAR register (year after 2000).
 */
#define NXP_RTC_ANALOG_YEAR_OFFSET 1900

#define NXP_RTC_ANALOG_YEAR_MIN 2000
#define NXP_RTC_ANALOG_YEAR_MAX 2099

#define NXP_RTC_ANALOG_REQUIRED_TIME_MASK                                                          \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_YEAR |     \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

struct nxp_rtc_analog_config {
	RTC_Type *base;
	void (*irq_config_func)(const struct device *dev);
};

#ifdef CONFIG_RTC_ALARM
static const uint32_t nxp_rtc_alarm_irq_enable[] = {
	kRTC_Alarm0InterruptEnable,
	kRTC_Alarm1InterruptEnable,
	kRTC_Alarm2InterruptEnable,
};

static const uint32_t nxp_rtc_alarm_irq_flag[] = {
	kRTC_Alarm0InterruptFlag,
	kRTC_Alarm1InterruptFlag,
	kRTC_Alarm2InterruptFlag,
};

struct nxp_rtc_analog_alarm {
	rtc_alarm_callback callback;
	void *user_data;
	uint16_t mask;
	struct rtc_time time;
	bool pending;
};
#endif /* CONFIG_RTC_ALARM */

struct nxp_rtc_analog_data {
#ifdef CONFIG_RTC_ALARM
	struct nxp_rtc_analog_alarm alarms[NXP_RTC_ANALOG_NUM_ALARMS];
#endif /* CONFIG_RTC_ALARM */
};

static int nxp_rtc_analog_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct nxp_rtc_analog_config *config = dev->config;
	rtc_datetime_t hal_dt;
	status_t status;
	int year;

	if (!timeptr || !rtc_utils_validate_rtc_time(timeptr, NXP_RTC_ANALOG_REQUIRED_TIME_MASK)) {
		LOG_ERR("Invalid time parameter");
		return -EINVAL;
	}

	year = timeptr->tm_year + NXP_RTC_ANALOG_YEAR_OFFSET;

	if (year < NXP_RTC_ANALOG_YEAR_MIN || year > NXP_RTC_ANALOG_YEAR_MAX) {
		LOG_ERR("Year %d out of range [%d, %d]", year, NXP_RTC_ANALOG_YEAR_MIN,
			NXP_RTC_ANALOG_YEAR_MAX);
		return -EINVAL;
	}

	hal_dt.year = (uint16_t)year;
	hal_dt.month = (uint8_t)(timeptr->tm_mon + 1);
	hal_dt.day = (uint8_t)timeptr->tm_mday;
	hal_dt.hour = (uint8_t)timeptr->tm_hour;
	hal_dt.minute = (uint8_t)timeptr->tm_min;
	hal_dt.second = (uint8_t)timeptr->tm_sec;
	hal_dt.hundredthOfSecond = (uint8_t)(timeptr->tm_nsec / 10000000);
	hal_dt.dayOfWeek = (uint8_t)timeptr->tm_wday;

	uint32_t key = irq_lock();

	status = RTC_SetDatetime(config->base, &hal_dt);

	irq_unlock(key);

	if (status != kStatus_Success) {
		LOG_ERR("Failed to set datetime");
		return -EIO;
	}

	/* Two RTC counts (20ms) must separate between a write and a read */
	k_msleep(20);

	return 0;
}

static int nxp_rtc_analog_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct nxp_rtc_analog_config *config = dev->config;
	rtc_datetime_t hal_dt;
	status_t status;

	if (!timeptr) {
		return -EINVAL;
	}

	/* Retry on read collision (kStatus_Fail) */
	for (int i = 0; i < RTC_READ_RETRY_MAX; i++) {
		status = RTC_GetDatetime(config->base, &hal_dt);
		if (status != kStatus_Fail) {
			break;
		}
	}

	if (status != kStatus_Success) {
		LOG_ERR("Failed to get datetime after %d retries", RTC_READ_RETRY_MAX);
		return -EIO;
	}

	timeptr->tm_year = (int)hal_dt.year - NXP_RTC_ANALOG_YEAR_OFFSET;
	timeptr->tm_mon = (int)hal_dt.month - 1;
	timeptr->tm_mday = (int)hal_dt.day;
	timeptr->tm_hour = (int)hal_dt.hour;
	timeptr->tm_min = (int)hal_dt.minute;
	timeptr->tm_sec = (int)hal_dt.second;
	timeptr->tm_nsec = (int)hal_dt.hundredthOfSecond * 10000000;
	timeptr->tm_wday = (int)hal_dt.dayOfWeek;
	timeptr->tm_yday = -1;
	timeptr->tm_isdst = -1;

	return 0;
}

#ifdef CONFIG_RTC_ALARM

static uint8_t convert_to_hal_alarm_mask(uint16_t mask)
{
	uint8_t hal_mask = (uint8_t)kRTC_AlarmMaskIgnoreAll;

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		hal_mask &= ~(uint8_t)kRTC_AlarmMaskIgnoreSecond;
	}
	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		hal_mask &= ~(uint8_t)kRTC_AlarmMaskIgnoreMinute;
	}
	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		hal_mask &= ~(uint8_t)kRTC_AlarmMaskIgnoreHour;
	}
	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		hal_mask &= ~(uint8_t)kRTC_AlarmMaskIgnoreDay;
	}
	if (mask & RTC_ALARM_TIME_MASK_MONTH) {
		hal_mask &= ~(uint8_t)kRTC_AlarmMaskIgnoreMonth;
	}
	if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		hal_mask &= ~(uint8_t)kRTC_AlarmMaskIgnoreDayOfWeek;
	}

	return hal_mask;
}

static int nxp_rtc_analog_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						     uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id >= NXP_RTC_ANALOG_NUM_ALARMS) {
		LOG_ERR("Invalid alarm id %u", id);
		return -EINVAL;
	}

	*mask = (RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE |
		 RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY |
		 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_WEEKDAY |
		 RTC_ALARM_TIME_MASK_NSEC);

	return 0;
}

static int nxp_rtc_analog_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
					 const struct rtc_time *timeptr)
{
	const struct nxp_rtc_analog_config *config = dev->config;
	struct nxp_rtc_analog_data *data = dev->data;
	rtc_bcd_alarm_config_t hal_alarm = {0};
	status_t status;

	if (id >= NXP_RTC_ANALOG_NUM_ALARMS) {
		LOG_ERR("Invalid alarm id %u", id);
		return -EINVAL;
	}

	if (mask == 0) {
		/* Disable this alarm */
		RTC_DisableAlarm(config->base, (rtc_alarm_id_t)id);
		RTC_DisableInterrupts(config->base, nxp_rtc_alarm_irq_enable[id]);
		data->alarms[id].mask = 0;
		data->alarms[id].pending = false;
		return 0;
	}

	if (!timeptr || !rtc_utils_validate_rtc_time(timeptr, mask)) {
		LOG_ERR("Invalid alarm time parameter");
		return -EINVAL;
	}

	/* BCD alarm does not support year matching */
	if (mask & RTC_ALARM_TIME_MASK_YEAR) {
		LOG_ERR("Year matching not supported for BCD alarm");
		return -EINVAL;
	}

	hal_alarm.second = (uint8_t)timeptr->tm_sec;
	hal_alarm.minute = (uint8_t)timeptr->tm_min;
	hal_alarm.hour = (uint8_t)timeptr->tm_hour;
	hal_alarm.day = (uint8_t)timeptr->tm_mday;
	hal_alarm.month = (uint8_t)(timeptr->tm_mon + 1);
	hal_alarm.dayOfWeek = (uint8_t)timeptr->tm_wday;
	hal_alarm.hundredthOfSecond =
		(mask & RTC_ALARM_TIME_MASK_NSEC) ? (uint8_t)(timeptr->tm_nsec / 10000000) : 0U;
	hal_alarm.mask = convert_to_hal_alarm_mask(mask);
	hal_alarm.mode = kRTC_AlarmModeRepeat;
	hal_alarm.enable = true;

	uint32_t key = irq_lock();

	status = RTC_ConfigureBCDAlarm(config->base, (rtc_alarm_id_t)id, &hal_alarm);
	if (status != kStatus_Success) {
		LOG_ERR("Failed to configure alarm %u", id);
		irq_unlock(key);
		return -EIO;
	}

	/* Enable the alarm interrupt */
	RTC_EnableInterrupts(config->base, nxp_rtc_alarm_irq_enable[id]);

	/* Store alarm settings for readback */
	data->alarms[id].mask = mask;
	data->alarms[id].time = *timeptr;
	data->alarms[id].pending = false;

	irq_unlock(key);

	return 0;
}

static int nxp_rtc_analog_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
					 struct rtc_time *timeptr)
{
	struct nxp_rtc_analog_data *data = dev->data;

	if (id >= NXP_RTC_ANALOG_NUM_ALARMS || !mask || !timeptr) {
		LOG_ERR("Invalid alarm id %u or NULL parameter", id);
		return -EINVAL;
	}

	*mask = data->alarms[id].mask;
	if (*mask == 0) {
		memset(timeptr, 0, sizeof(*timeptr));
	} else {
		*timeptr = data->alarms[id].time;
	}

	return 0;
}

static int nxp_rtc_analog_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct nxp_rtc_analog_data *data = dev->data;
	int ret;

	if (id >= NXP_RTC_ANALOG_NUM_ALARMS) {
		LOG_ERR("Invalid alarm id %u", id);
		return -EINVAL;
	}

	uint32_t key = irq_lock();

	ret = data->alarms[id].pending ? 1 : 0;
	data->alarms[id].pending = false;

	irq_unlock(key);

	return ret;
}

static int nxp_rtc_analog_alarm_set_callback(const struct device *dev, uint16_t id,
					     rtc_alarm_callback callback, void *user_data)
{
	struct nxp_rtc_analog_data *data = dev->data;

	if (id >= NXP_RTC_ANALOG_NUM_ALARMS) {
		LOG_ERR("Invalid alarm id %u", id);
		return -EINVAL;
	}

	uint32_t key = irq_lock();

	data->alarms[id].callback = callback;
	data->alarms[id].user_data = user_data;

	irq_unlock(key);

	return 0;
}

static void nxp_rtc_analog_alarm_isr(const struct device *dev, uint16_t id)
{
	const struct nxp_rtc_analog_config *config = dev->config;
	struct nxp_rtc_analog_data *data = dev->data;
	uint32_t flag = nxp_rtc_alarm_irq_flag[id];

	if (RTC_GetInterruptFlags(config->base) & flag) {
		RTC_ClearInterruptFlags(config->base, flag);

		if (data->alarms[id].callback) {
			data->alarms[id].callback(dev, id, data->alarms[id].user_data);
			data->alarms[id].pending = false;
		} else {
			data->alarms[id].pending = true;
		}
	}
}

static void nxp_rtc_analog_isr_alarm0(const struct device *dev)
{
	nxp_rtc_analog_alarm_isr(dev, 0);
}

static void nxp_rtc_analog_isr_alarm1(const struct device *dev)
{
	nxp_rtc_analog_alarm_isr(dev, 1);
}

static void nxp_rtc_analog_isr_alarm2(const struct device *dev)
{
	nxp_rtc_analog_alarm_isr(dev, 2);
}

#endif /* CONFIG_RTC_ALARM */

static int nxp_rtc_analog_init(const struct device *dev)
{
	const struct nxp_rtc_analog_config *config = dev->config;
	RTC_Type *base = config->base;
#ifdef CONFIG_RTC_ALARM
	struct nxp_rtc_analog_data *data = dev->data;
#endif

	/*
	 * Only call RTC_Init() when the RTC is not already running (EN=0),
	 * i.e. on first power-up. RTC_Init() performs a software reset that
	 * would clear any battery-backed time, so we skip it when the RTC
	 * is already enabled (EN=1) to preserve the current time.
	 */
	if (!(base->CONFIG & RTC_CONFIG_EN_MASK)) {
		rtc_config_t rtc_cfg;

		RTC_GetDefaultConfig(&rtc_cfg);
		rtc_cfg.operatingMode = kRTC_ModeTimeDate;
		rtc_cfg.enableXtal32 = true;
		rtc_cfg.enable2kHzOutputSMM = true;
		rtc_cfg.alarmInitialEnable[0] = false;
		rtc_cfg.alarmInitialEnable[1] = false;
		rtc_cfg.alarmInitialEnable[2] = false;
		rtc_cfg.enableWatchdog = false;

		if (RTC_Init(base, &rtc_cfg) != kStatus_Success) {
			LOG_ERR("RTC initialization failed");
			return -EIO;
		}

		if (RTC_StartTimer(base) != kStatus_Success) {
			LOG_ERR("Failed to start RTC timer");
			return -EIO;
		}

	} else {

		/* Ensure time/date mode (not free-running) */
		if (base->CONFIG & RTC_CONFIG_FREE_RUNNING_MASK) {
			LOG_WRN("RTC in free-running mode, switching to time/date mode");
			unsigned int key = irq_lock();

			RTC_StopTimer(base);
			base->CONFIG &= ~RTC_CONFIG_FREE_RUNNING_MASK;
			RTC_StartTimer(base);

			irq_unlock(key);
		}

		/* Disable alarms for clean state */
		RTC_DisableAlarm(base, kRTC_Alarm_0);
		RTC_DisableAlarm(base, kRTC_Alarm_1);
		RTC_DisableAlarm(base, kRTC_Alarm_2);
		RTC_DisableInterrupts(base, (uint32_t)(kRTC_Alarm0InterruptEnable |
						       kRTC_Alarm1InterruptEnable |
						       kRTC_Alarm2InterruptEnable));
		RTC_ClearInterruptFlags(base, (uint32_t)(kRTC_Alarm0InterruptFlag |
							 kRTC_Alarm1InterruptFlag |
							 kRTC_Alarm2InterruptFlag));
	}

#ifdef CONFIG_RTC_ALARM
	for (int i = 0; i < NXP_RTC_ANALOG_NUM_ALARMS; i++) {
		data->alarms[i].callback = NULL;
		data->alarms[i].user_data = NULL;
		data->alarms[i].mask = 0;
		data->alarms[i].pending = false;
	}
#endif /* CONFIG_RTC_ALARM */

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(rtc, nxp_rtc_analog_driver_api) = {
	.set_time = nxp_rtc_analog_set_time,
	.get_time = nxp_rtc_analog_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = nxp_rtc_analog_alarm_get_supported_fields,
	.alarm_set_time = nxp_rtc_analog_alarm_set_time,
	.alarm_get_time = nxp_rtc_analog_alarm_get_time,
	.alarm_is_pending = nxp_rtc_analog_alarm_is_pending,
	.alarm_set_callback = nxp_rtc_analog_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
};

#define NXP_RTC_ANALOG_IRQ_CONNECT(n, idx, isr_func)                                               \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, idx, irq), DT_INST_IRQ_BY_IDX(n, idx, priority), \
			    isr_func, DEVICE_DT_INST_GET(n), 0);                                   \
		irq_enable(DT_INST_IRQ_BY_IDX(n, idx, irq));                                       \
	} while (false)

#define NXP_RTC_ANALOG_IRQ_CONNECT_IF(n, idx, isr_func)                                            \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, idx),                          \
		   (NXP_RTC_ANALOG_IRQ_CONNECT(n, idx, isr_func);))

#define NXP_RTC_ANALOG_CONFIG_FUNC(n)                                                              \
	static void nxp_rtc_analog_config_func_##n(const struct device *dev)                       \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IF_ENABLED(CONFIG_RTC_ALARM, (                                \
			NXP_RTC_ANALOG_IRQ_CONNECT_IF(n, 0,                  \
				nxp_rtc_analog_isr_alarm0)                \
			NXP_RTC_ANALOG_IRQ_CONNECT_IF(n, 1,                  \
				nxp_rtc_analog_isr_alarm1)                \
			NXP_RTC_ANALOG_IRQ_CONNECT_IF(n, 2,                  \
				nxp_rtc_analog_isr_alarm2)                \
		))                    \
	}

#define NXP_RTC_ANALOG_DEVICE_INIT(n)                                                              \
	NXP_RTC_ANALOG_CONFIG_FUNC(n)                                                              \
                                                                                                   \
	static const struct nxp_rtc_analog_config nxp_rtc_analog_config_##n = {                    \
		.base = (RTC_Type *)DT_INST_REG_ADDR(n),                                           \
		.irq_config_func = nxp_rtc_analog_config_func_##n,                                 \
	};                                                                                         \
                                                                                                   \
	static struct nxp_rtc_analog_data nxp_rtc_analog_data_##n;                                 \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, nxp_rtc_analog_init, NULL, &nxp_rtc_analog_data_##n,              \
			      &nxp_rtc_analog_config_##n, PRE_KERNEL_1, CONFIG_RTC_INIT_PRIORITY,  \
			      &nxp_rtc_analog_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_RTC_ANALOG_DEVICE_INIT)
