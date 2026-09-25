/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief RTC driver for Infineon CAT1 MCU family.
 */

#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <cy_pdl.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(ifx_cat1_rtc, CONFIG_RTC_LOG_LEVEL);

#define DT_DRV_COMPAT infineon_rtc

#define IFX_CAT1_RTC_STATE_UNINITIALIZED 0
#define IFX_CAT1_RTC_STATE_ENABLED       1
#define IFX_CAT1_RTC_STATE_TIME_SET      2

#define IFX_CAT1_RTC_INIT_CENTURY 2000
#define IFX_CAT1_RTC_TM_YEAR_BASE 1900

#define IFX_CAT1_RTC_ALARMS_COUNT DT_INST_PROP(0, alarms_count)

#ifdef CONFIG_RTC_ALARM
BUILD_ASSERT(IFX_CAT1_RTC_ALARMS_COUNT >= 1 && IFX_CAT1_RTC_ALARMS_COUNT <= 2,
	     "CAT1 RTC hardware provides exactly two alarms");
#endif

/* Bitmask of alarm time fields supported by the hardware.
 *
 * Note: weekday alarms are rejected once a pre-2000 time is set, see
 * ifx_cat1_alarm_set_time().
 */
#define IFX_CAT1_RTC_ALARM_SUPPORTED_FIELDS                                                        \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_WEEKDAY)

#if defined(CONFIG_SOC_FAMILY_INFINEON_CAT1B)
#if defined(SRSS_BACKUP_NUM_BREG3) && (SRSS_BACKUP_NUM_BREG3 > 0)
#define IFX_CAT1_RTC_BREG (BACKUP->BREG_SET3[SRSS_BACKUP_NUM_BREG3 - 1])
#elif defined(SRSS_BACKUP_NUM_BREG2) && (SRSS_BACKUP_NUM_BREG2 > 0)
#define IFX_CAT1_RTC_BREG (BACKUP->BREG_SET2[SRSS_BACKUP_NUM_BREG2 - 1])
#elif defined(SRSS_BACKUP_NUM_BREG1) && (SRSS_BACKUP_NUM_BREG1 > 0)
#define IFX_CAT1_RTC_BREG (BACKUP->BREG_SET1[SRSS_BACKUP_NUM_BREG1 - 1])
#elif defined(SRSS_BACKUP_NUM_BREG0) && (SRSS_BACKUP_NUM_BREG0 > 0)
#define IFX_CAT1_RTC_BREG (BACKUP->BREG_SET0[SRSS_BACKUP_NUM_BREG0 - 1])
#endif
#elif defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#if defined(SRSS_RTC_NUM_BREG3) && (SRSS_RTC_NUM_BREG3 > 0)
#define IFX_CAT1_RTC_BREG (RTC->BREG_SET3[SRSS_RTC_NUM_BREG3 - 1])
#elif defined(SRSS_RTC_NUM_BREG2) && (SRSS_RTC_NUM_BREG2 > 0)
#define IFX_CAT1_RTC_BREG (RTC->BREG_SET2[SRSS_RTC_NUM_BREG2 - 1])
#elif defined(SRSS_RTC_NUM_BREG1) && (SRSS_RTC_NUM_BREG1 > 0)
#define IFX_CAT1_RTC_BREG (RTC->BREG_SET1[SRSS_RTC_NUM_BREG1 - 1])
#elif defined(SRSS_RTC_NUM_BREG0) && (SRSS_RTC_NUM_BREG0 > 0)
#define IFX_CAT1_RTC_BREG (RTC->BREG_SET0[SRSS_RTC_NUM_BREG0 - 1])
#elif defined(SRSS_NUM_HIBDATA) && ((SRSS_NUM_HIBDATA) > 0)
#define IFX_CAT1_RTC_BREG (SRSS->PWR_HIB_DATA[SRSS_NUM_HIBDATA - 1])
#endif
#endif

#define IFX_CAT1_RTC_BREG_CENTURY_Pos 0UL
#define IFX_CAT1_RTC_BREG_CENTURY_Msk 0x0000FFFFUL
#define IFX_CAT1_RTC_BREG_STATE_Pos   16UL
#define IFX_CAT1_RTC_BREG_STATE_Msk   0xFFFF0000UL

static const uint32_t ifx_cat1_rtc_max_retry = 10;
static const uint32_t ifx_cat1_rtc_retry_delay_ms = 1;

#ifdef CONFIG_PM
static cy_en_syspm_status_t ifx_cat1_rtc_syspm_callback(cy_stc_syspm_callback_params_t *params,
							cy_en_syspm_callback_mode_t mode)
{
	return Cy_RTC_DeepSleepCallback(params, mode);
}

static cy_stc_syspm_callback_params_t ifx_cat1_rtc_pm_cb_params = {NULL, NULL};
static cy_stc_syspm_callback_t ifx_cat1_rtc_pm_cb = {
	.callback = &ifx_cat1_rtc_syspm_callback,
	.type = CY_SYSPM_DEEPSLEEP,
	.callbackParams = &ifx_cat1_rtc_pm_cb_params,
};
#endif /* CONFIG_PM */

#define IFX_CAT1_RTC_WAIT_ONE_MS() Cy_SysLib_Delay(ifx_cat1_rtc_retry_delay_ms)

/* Internal macro to validate RTC year parameter */
#define IFX_CAT1_RTC_VALID_CENTURY(year) ((year) >= IFX_CAT1_RTC_TM_YEAR_BASE)

#define MAX_IFX_CAT1_CAL (60)

/* Convert parts per billion to groupings of 128 ticks added or removed from one hour of clock
 * cycles at 32768 Hz.
 *
 * ROUND_DOWN(ppb * 32768Hz * 60min * 60sec / 1000000000, 128) / 128
 * ROUND_DOWN(ppb * 117964800 / 1000000000, 128) / 128
 * ROUND_DOWN(ppb * 9216 / 78125, 128) / 128
 */
#define PPB_TO_WCO_PULSE_SETS(ppb) ((ROUND_DOWN((ppb * 9216 / 78125), 128)) / 128)

/* Convert groupings of 128 ticks added or removed from one hour of clock cycles at
 * 32768 Hz to parts per billion
 *
 * wps * 128 * 1000000000 / 32768Hz * 60min * 60sec
 * wps * 128000000000 / 117964800
 * wps * 78125 / 72
 */
#define WCO_PULSE_SETS_TO_PPB(wps) (wps * 78125 / 72)

struct ifx_cat1_rtc_data {
	struct k_spinlock lock;
#ifdef CONFIG_RTC_ALARM
	struct {
		rtc_alarm_callback cb;
		void *user_data;
		bool pending;
		uint16_t mask;
	} alarm[IFX_CAT1_RTC_ALARMS_COUNT];
#endif
};

static inline uint16_t ifx_cat1_rtc_get_state(void)
{
	return _FLD2VAL(IFX_CAT1_RTC_BREG_STATE, IFX_CAT1_RTC_BREG);
}

static inline void ifx_cat1_rtc_set_state(uint16_t init)
{
	IFX_CAT1_RTC_BREG &= IFX_CAT1_RTC_BREG_CENTURY_Msk;
	IFX_CAT1_RTC_BREG |= _VAL2FLD(IFX_CAT1_RTC_BREG_STATE, init);
}

static inline uint16_t ifx_cat1_rtc_get_century(void)
{
	return _FLD2VAL(IFX_CAT1_RTC_BREG_CENTURY, IFX_CAT1_RTC_BREG);
}

static inline void ifx_cat1_rtc_set_century(uint16_t century)
{
	IFX_CAT1_RTC_BREG &= IFX_CAT1_RTC_BREG_STATE_Msk;
	IFX_CAT1_RTC_BREG |= _VAL2FLD(IFX_CAT1_RTC_BREG_CENTURY, century);
}

static void ifx_cat1_rtc_from_pdl_time(cy_stc_rtc_config_t *pdlTime, const int year,
				       struct rtc_time *z_time)
{
	CY_ASSERT(pdlTime != NULL);
	CY_ASSERT(z_time != NULL);

	z_time->tm_sec = (int)pdlTime->sec;
	z_time->tm_min = (int)pdlTime->min;
	z_time->tm_hour = (int)pdlTime->hour;
	z_time->tm_mday = (int)pdlTime->date;
	z_time->tm_year = (int)(year - IFX_CAT1_RTC_TM_YEAR_BASE);

	/* The subtraction of 1 here is to translate between internal ifx_cat1 code and the Zephyr
	 * driver.
	 */
	z_time->tm_mon = (int)(pdlTime->month - 1u);

	/* pdlTime->dayOfWeek is incorrect for years <2000 - redo calculation */
	z_time->tm_wday =
		(int)(Cy_RTC_ConvertDayOfWeek(pdlTime->date, pdlTime->month, (uint32_t)year) - 1u);

	/* year day not known in pdl RTC structure without conversion */
	z_time->tm_yday = -1;

	/* daylight savings currently marked as unknown */
	z_time->tm_isdst = -1;

	/* nanoseconds not tracked by ifx code. Set to value indicating unknown */
	z_time->tm_nsec = 0;
}

static void ifx_cat1_rtc_century_interrupt(void)
{
	/* The century is stored in its own register so when a "century interrupt"
	 * occurs at a rollover. The current century is retrieved and 100 is added
	 * to it and the register is reset to reflect the new century.
	 * i.e. 1999->2000
	 */
	ifx_cat1_rtc_set_century(ifx_cat1_rtc_get_century() + 100);
}

#ifdef CONFIG_RTC_ALARM
static inline uint32_t ifx_cat1_rtc_alarm_intr_bit(uint16_t id)
{
	return (id == 0) ? CY_RTC_INTR_ALARM1 : CY_RTC_INTR_ALARM2;
}
#endif /* CONFIG_RTC_ALARM */

static void ifx_cat1_rtc_isr_handler(const void *arg)
{
	const struct device *dev = arg;
	struct ifx_cat1_rtc_data *data = dev->data;
	uint32_t status = Cy_RTC_GetInterruptStatusMasked();
#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback cb[IFX_CAT1_RTC_ALARMS_COUNT] = {0};
	void *user_data[IFX_CAT1_RTC_ALARMS_COUNT] = {0};
#endif

	Cy_RTC_ClearInterrupt(status);

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (status & CY_RTC_INTR_CENTURY) {
		ifx_cat1_rtc_century_interrupt();
	}

#ifdef CONFIG_RTC_ALARM
	for (uint16_t id = 0; id < IFX_CAT1_RTC_ALARMS_COUNT; id++) {
		if ((status & ifx_cat1_rtc_alarm_intr_bit(id)) == 0) {
			continue;
		}

		if (data->alarm[id].cb != NULL) {
			cb[id] = data->alarm[id].cb;
			user_data[id] = data->alarm[id].user_data;
		} else {
			data->alarm[id].pending = true;
		}
	}
#endif

	k_spin_unlock(&data->lock, key);

#ifdef CONFIG_RTC_ALARM
	/* Callbacks run outside the lock so they may re-enter the driver. */
	for (uint16_t id = 0; id < IFX_CAT1_RTC_ALARMS_COUNT; id++) {
		if (cb[id] != NULL) {
			cb[id](dev, id, user_data[id]);
		}
	}
#endif
}

static int ifx_cat1_rtc_init(const struct device *dev)
{
	struct ifx_cat1_rtc_data *data = dev->data;
	cy_rslt_t rslt = CY_RSLT_SUCCESS;
	k_spinlock_key_t key;
	uint16_t state;
	int ret = 0;

	Cy_SysClk_ClkBakSetSource(CY_SYSCLK_BAK_IN_CLKLF);

	/* The state and century fields share one backup register, so the whole
	 * read-decide-write sequence has to be atomic.
	 */
	key = k_spin_lock(&data->lock);
	state = ifx_cat1_rtc_get_state();

	if (state == IFX_CAT1_RTC_STATE_UNINITIALIZED) {
		if (Cy_RTC_IsExternalResetOccurred()) {
			ifx_cat1_rtc_set_century(IFX_CAT1_RTC_INIT_CENTURY);
		}

#ifdef CONFIG_PM
		rslt = Cy_SysPm_RegisterCallback(&ifx_cat1_rtc_pm_cb);
#endif /* CONFIG_PM */

		if (rslt == CY_RSLT_SUCCESS) {
			ifx_cat1_rtc_set_state(IFX_CAT1_RTC_STATE_ENABLED);
		} else {
			ret = -EINVAL;
		}

	} else if (state == IFX_CAT1_RTC_STATE_ENABLED || state == IFX_CAT1_RTC_STATE_TIME_SET) {
		if (Cy_RTC_GetInterruptStatus() & CY_RTC_INTR_CENTURY) {
			ifx_cat1_rtc_century_interrupt();
		}
	}

	k_spin_unlock(&data->lock, key);

	Cy_RTC_ClearInterrupt(CY_RTC_INTR_CENTURY | CY_RTC_INTR_ALARM1 | CY_RTC_INTR_ALARM2);
	Cy_RTC_SetInterruptMask(CY_RTC_INTR_CENTURY);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), ifx_cat1_rtc_isr_handler,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return ret;
}

static int ifx_cat1_rtc_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct ifx_cat1_rtc_data *data = dev->data;

	uint32_t sec = timeptr->tm_sec;
	uint32_t min = timeptr->tm_min;
	uint32_t hour = timeptr->tm_hour;
	uint32_t day = timeptr->tm_mday;
	/* The addition of 1 here is to translate between internal ifx_cat1 code and the Zephyr
	 * driver.
	 */
	uint32_t mon = timeptr->tm_mon + 1;
	uint32_t year = timeptr->tm_year + IFX_CAT1_RTC_TM_YEAR_BASE;
	uint32_t year2digit = year % 100;

	cy_rslt_t rslt;
	uint32_t retry = 0;
	k_spinlock_key_t key;

	if (!CY_RTC_IS_SEC_VALID(sec) || !CY_RTC_IS_MIN_VALID(min) || !CY_RTC_IS_HOUR_VALID(hour) ||
	    !CY_RTC_IS_MONTH_VALID(mon) || !CY_RTC_IS_YEAR_SHORT_VALID(year2digit) ||
	    !IFX_CAT1_RTC_VALID_CENTURY(year)) {

		return -EINVAL;
	}
	do {
		if (retry != 0) {
			IFX_CAT1_RTC_WAIT_ONE_MS();
		}

		key = k_spin_lock(&data->lock);

		rslt = Cy_RTC_SetDateAndTimeDirect(sec, min, hour, day, mon, year2digit);
		if (rslt == CY_RSLT_SUCCESS) {
			ifx_cat1_rtc_set_century((uint16_t)(year) - (uint16_t)(year2digit));
		}

		k_spin_unlock(&data->lock, key);
		++retry;
	} while (rslt == CY_RTC_INVALID_STATE && retry < ifx_cat1_rtc_max_retry);

	retry = 0;
	while (CY_RTC_BUSY == Cy_RTC_GetSyncStatus() && retry < ifx_cat1_rtc_max_retry) {
		IFX_CAT1_RTC_WAIT_ONE_MS();
		++retry;
	}

	if (rslt == CY_RSLT_SUCCESS) {
		key = k_spin_lock(&data->lock);
		ifx_cat1_rtc_set_state(IFX_CAT1_RTC_STATE_TIME_SET);
		k_spin_unlock(&data->lock, key);
		return 0;
	} else {
		return -EINVAL;
	}
}

static int ifx_cat1_rtc_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct ifx_cat1_rtc_data *data = dev->data;

	cy_stc_rtc_config_t dateTime = {.hrFormat = CY_RTC_24_HOURS};

	if (ifx_cat1_rtc_get_state() != IFX_CAT1_RTC_STATE_TIME_SET) {
		LOG_ERR("Valid time has not been set with rtc_set_time yet");
		return -ENODATA;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	Cy_RTC_GetDateAndTime(&dateTime);
	const int year = (int)(dateTime.year + ifx_cat1_rtc_get_century());

	k_spin_unlock(&data->lock, key);

	ifx_cat1_rtc_from_pdl_time(&dateTime, year, timeptr);

	return CY_RSLT_SUCCESS;
}

#ifdef CONFIG_RTC_CALIBRATION
static int ifx_cat1_set_calibration(const struct device *dev, int32_t calibration)
{
	cy_rslt_t rslt;

	uint8_t uint_calibration;
	cy_en_rtc_calib_sign_t calibration_sign;

	if (calibration >= 0) {
		calibration_sign = CY_RTC_CALIB_SIGN_POSITIVE;
	} else {
		calibration = abs(calibration);
		calibration_sign = CY_RTC_CALIB_SIGN_NEGATIVE;
	}

	uint_calibration = PPB_TO_WCO_PULSE_SETS(calibration);

	/* Maximum calibration value on cat1b of 60 128 tick groupings */
	if (MAX_IFX_CAT1_CAL < uint_calibration) {
		/* out of supported range */
		return -EINVAL;
	}

	rslt = Cy_RTC_CalibrationControlEnable(uint_calibration, calibration_sign,
					       CY_RTC_CAL_SEL_CAL1);
	if (rslt != CY_RSLT_SUCCESS) {
		return -EINVAL;
	}

	return 0;
}

static int ifx_cat1_get_calibration(const struct device *dev, int32_t *calibration)
{
	ARG_UNUSED(dev);

	uint32_t hw_calibration = _FLD2VAL(BACKUP_CAL_CTL_CALIB_VAL, BACKUP_CAL_CTL);
	cy_en_rtc_calib_sign_t hw_sign =
		(cy_en_rtc_calib_sign_t)(_FLD2VAL(BACKUP_CAL_CTL_CALIB_SIGN, BACKUP_CAL_CTL));

	if (CY_RTC_CALIB_SIGN_POSITIVE == hw_sign) {
		*calibration = WCO_PULSE_SETS_TO_PPB(hw_calibration);
	} else {
		*calibration = WCO_PULSE_SETS_TO_PPB(hw_calibration) * -1;
	}

	return 0;
}
#endif /* CONFIG_RTC_CALIBRATION */

#ifdef CONFIG_RTC_ALARM
static int ifx_cat1_alarm_get_supported_fields(const struct device *dev, uint16_t id,
					       uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id >= IFX_CAT1_RTC_ALARMS_COUNT) {
		return -EINVAL;
	}

	*mask = IFX_CAT1_RTC_ALARM_SUPPORTED_FIELDS;
	return 0;
}

static int ifx_cat1_alarm_validate_fields(uint16_t mask, const struct rtc_time *timeptr)
{
	if ((mask & RTC_ALARM_TIME_MASK_SECOND) && (timeptr->tm_sec < 0 || timeptr->tm_sec > 59)) {
		return -EINVAL;
	}
	if ((mask & RTC_ALARM_TIME_MASK_MINUTE) && (timeptr->tm_min < 0 || timeptr->tm_min > 59)) {
		return -EINVAL;
	}
	if ((mask & RTC_ALARM_TIME_MASK_HOUR) && (timeptr->tm_hour < 0 || timeptr->tm_hour > 23)) {
		return -EINVAL;
	}
	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) &&
	    (timeptr->tm_mday < 1 || timeptr->tm_mday > 31)) {
		return -EINVAL;
	}
	if ((mask & RTC_ALARM_TIME_MASK_MONTH) && (timeptr->tm_mon < 0 || timeptr->tm_mon > 11)) {
		return -EINVAL;
	}
	if ((mask & RTC_ALARM_TIME_MASK_WEEKDAY) &&
	    (timeptr->tm_wday < 0 || timeptr->tm_wday > 6)) {
		return -EINVAL;
	}
	return 0;
}

static void ifx_cat1_alarm_populate_cfg(cy_stc_rtc_alarm_t *alarm_cfg, uint16_t mask,
					const struct rtc_time *timeptr)
{
	alarm_cfg->almEn = CY_RTC_ALARM_ENABLE;

	alarm_cfg->sec = (mask & RTC_ALARM_TIME_MASK_SECOND) ? timeptr->tm_sec : 0;
	alarm_cfg->secEn =
		(mask & RTC_ALARM_TIME_MASK_SECOND) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE;

	alarm_cfg->min = (mask & RTC_ALARM_TIME_MASK_MINUTE) ? timeptr->tm_min : 0;
	alarm_cfg->minEn =
		(mask & RTC_ALARM_TIME_MASK_MINUTE) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE;

	alarm_cfg->hour = (mask & RTC_ALARM_TIME_MASK_HOUR) ? timeptr->tm_hour : 0;
	alarm_cfg->hourEn =
		(mask & RTC_ALARM_TIME_MASK_HOUR) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE;

	alarm_cfg->date = (mask & RTC_ALARM_TIME_MASK_MONTHDAY) ? timeptr->tm_mday : 1;
	alarm_cfg->dateEn =
		(mask & RTC_ALARM_TIME_MASK_MONTHDAY) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE;

	alarm_cfg->month = (mask & RTC_ALARM_TIME_MASK_MONTH) ? timeptr->tm_mon + 1 : 1;
	alarm_cfg->monthEn =
		(mask & RTC_ALARM_TIME_MASK_MONTH) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE;

	alarm_cfg->dayOfWeek = (mask & RTC_ALARM_TIME_MASK_WEEKDAY) ? timeptr->tm_wday + 1 : 1;
	alarm_cfg->dayOfWeekEn =
		(mask & RTC_ALARM_TIME_MASK_WEEKDAY) ? CY_RTC_ALARM_ENABLE : CY_RTC_ALARM_DISABLE;
}

static int ifx_cat1_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				   const struct rtc_time *timeptr)
{
	struct ifx_cat1_rtc_data *data = dev->data;
	cy_en_rtc_alarm_t alarm_idx;
	cy_stc_rtc_alarm_t alarm_cfg = {0};
	cy_en_rtc_status_t rslt;
	uint32_t retry = 0;

	if ((id >= IFX_CAT1_RTC_ALARMS_COUNT) || (mask & ~IFX_CAT1_RTC_ALARM_SUPPORTED_FIELDS)) {
		return -EINVAL;
	}

	alarm_idx = (id == 0) ? CY_RTC_ALARM_1 : CY_RTC_ALARM_2;

	if (mask == 0) {
		/* Disable alarm - set fields to valid defaults for PDL assertions */
		alarm_cfg.almEn = CY_RTC_ALARM_DISABLE;
		alarm_cfg.dayOfWeek = CY_RTC_SUNDAY;
		alarm_cfg.date = 1;
		alarm_cfg.month = 1;
	} else {
		if (timeptr == NULL) {
			return -EINVAL;
		}

		int ret = ifx_cat1_alarm_validate_fields(mask, timeptr);

		if (ret != 0) {
			return ret;
		}

		/* The PDL derives the hardware day-of-week from the date assuming a 20xx
		 * century, so it cannot match once a pre-2000 time has been set.
		 */
		if ((mask & RTC_ALARM_TIME_MASK_WEEKDAY) &&
		    (ifx_cat1_rtc_get_state() == IFX_CAT1_RTC_STATE_TIME_SET) &&
		    (ifx_cat1_rtc_get_century() < IFX_CAT1_RTC_INIT_CENTURY)) {
			return -EINVAL;
		}

		ifx_cat1_alarm_populate_cfg(&alarm_cfg, mask, timeptr);
	}

	do {
		if (retry != 0) {
			IFX_CAT1_RTC_WAIT_ONE_MS();
		}
		rslt = Cy_RTC_SetAlarmDateAndTime(&alarm_cfg, alarm_idx);
		++retry;
	} while (rslt == CY_RTC_INVALID_STATE && retry < ifx_cat1_rtc_max_retry);

	if (rslt == CY_RTC_SUCCESS) {
		uint32_t alarm_bit = ifx_cat1_rtc_alarm_intr_bit(id);
		k_spinlock_key_t key = k_spin_lock(&data->lock);

		data->alarm[id].mask = mask;
		data->alarm[id].pending = false;

		/* Drop any match latched by the previous configuration, otherwise it
		 * fires as soon as the mask bit is set.
		 */
		Cy_RTC_ClearInterrupt(alarm_bit);

		if (mask != 0) {
			Cy_RTC_SetInterruptMask(Cy_RTC_GetInterruptMask() | alarm_bit);
		} else {
			Cy_RTC_SetInterruptMask(Cy_RTC_GetInterruptMask() & ~alarm_bit);
		}

		k_spin_unlock(&data->lock, key);
	}

	return (rslt == CY_RTC_SUCCESS) ? 0 : -EIO;
}

static int ifx_cat1_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				   struct rtc_time *timeptr)
{
	struct ifx_cat1_rtc_data *data = dev->data;
	cy_en_rtc_alarm_t alarm_idx;
	cy_stc_rtc_alarm_t alarm_cfg;

	if (id >= IFX_CAT1_RTC_ALARMS_COUNT) {
		return -EINVAL;
	}

	alarm_idx = (id == 0) ? CY_RTC_ALARM_1 : CY_RTC_ALARM_2;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	Cy_RTC_GetAlarmDateAndTime(&alarm_cfg, alarm_idx);
	*mask = data->alarm[id].mask;

	k_spin_unlock(&data->lock, key);

	memset(timeptr, 0, sizeof(*timeptr));
	timeptr->tm_sec = alarm_cfg.sec;
	timeptr->tm_min = alarm_cfg.min;
	timeptr->tm_hour = alarm_cfg.hour;
	timeptr->tm_mday = alarm_cfg.date;
	timeptr->tm_mon = alarm_cfg.month - 1;
	timeptr->tm_wday = alarm_cfg.dayOfWeek - 1;
	timeptr->tm_yday = -1;
	timeptr->tm_isdst = -1;
	timeptr->tm_nsec = 0;

	return 0;
}

static int ifx_cat1_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct ifx_cat1_rtc_data *data = dev->data;
	int ret;

	if (id >= IFX_CAT1_RTC_ALARMS_COUNT) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ret = data->alarm[id].pending ? 1 : 0;
	data->alarm[id].pending = false;

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ifx_cat1_alarm_set_callback(const struct device *dev, uint16_t id,
				       rtc_alarm_callback callback, void *user_data)
{
	struct ifx_cat1_rtc_data *data = dev->data;

	if (id >= IFX_CAT1_RTC_ALARMS_COUNT) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->alarm[id].cb = callback;
	data->alarm[id].user_data = user_data;

	k_spin_unlock(&data->lock, key);

	return 0;
}
#endif /* CONFIG_RTC_ALARM */

static DEVICE_API(rtc, ifx_cat1_rtc_driver_api) = {
	.set_time = ifx_cat1_rtc_set_time,
	.get_time = ifx_cat1_rtc_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = ifx_cat1_alarm_get_supported_fields,
	.alarm_set_time = ifx_cat1_alarm_set_time,
	.alarm_get_time = ifx_cat1_alarm_get_time,
	.alarm_is_pending = ifx_cat1_alarm_is_pending,
	.alarm_set_callback = ifx_cat1_alarm_set_callback,
#endif
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = ifx_cat1_set_calibration,
	.get_calibration = ifx_cat1_get_calibration,
#endif
};

#define INFINEON_CAT1_RTC_INIT(n)                                                                  \
	static struct ifx_cat1_rtc_data ifx_cat1_rtc_data##n;                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ifx_cat1_rtc_init, NULL, &ifx_cat1_rtc_data##n, NULL,             \
			      PRE_KERNEL_1, CONFIG_RTC_INIT_PRIORITY, &ifx_cat1_rtc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_RTC_INIT)
