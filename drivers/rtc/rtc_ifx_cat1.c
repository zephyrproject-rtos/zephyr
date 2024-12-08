/*
 * Copyright (c) 2024 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
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

LOG_MODULE_REGISTER(ifx_cat1_rtc, CONFIG_RTC_LOG_LEVEL);

#define DT_DRV_COMPAT infineon_cat1_rtc

#define _IFX_CAT1_RTC_STATE_UNINITIALIZED 0
#define _IFX_CAT1_RTC_STATE_ENABLED       1
#define _IFX_CAT1_RTC_STATE_TIME_SET      2

#define _IFX_CAT1_RTC_INIT_CENTURY 2000
#define _IFX_CAT1_RTC_TM_YEAR_BASE 1900

#if defined(CONFIG_SOC_FAMILY_INFINEON_CAT1B)
#if defined(SRSS_BACKUP_NUM_BREG3) && (SRSS_BACKUP_NUM_BREG3 > 0)
#define _IFX_CAT1_RTC_BREG (BACKUP->BREG_SET3[SRSS_BACKUP_NUM_BREG3 - 1])
#elif defined(SRSS_BACKUP_NUM_BREG2) && (SRSS_BACKUP_NUM_BREG2 > 0)
#define _IFX_CAT1_RTC_BREG (BACKUP->BREG_SET2[SRSS_BACKUP_NUM_BREG2 - 1])
#elif defined(SRSS_BACKUP_NUM_BREG1) && (SRSS_BACKUP_NUM_BREG1 > 0)
#define _IFX_CAT1_RTC_BREG (BACKUP->BREG_SET1[SRSS_BACKUP_NUM_BREG1 - 1])
#elif defined(SRSS_BACKUP_NUM_BREG0) && (SRSS_BACKUP_NUM_BREG0 > 0)
#define _IFX_CAT1_RTC_BREG (BACKUP->BREG_SET0[SRSS_BACKUP_NUM_BREG0 - 1])
#endif
#endif

#define _IFX_CAT1_RTC_BREG_CENTURY_Pos 0UL
#define _IFX_CAT1_RTC_BREG_CENTURY_Msk 0x0000FFFFUL
#define _IFX_CAT1_RTC_BREG_STATE_Pos   16UL
#define _IFX_CAT1_RTC_BREG_STATE_Msk   0xFFFF0000UL

static const uint32_t _IFX_CAT1_RTC_MAX_RETRY = 10;
static const uint32_t _IFX_CAT1_RTC_RETRY_DELAY_MS = 1;

static cy_stc_rtc_dst_t *_ifx_cat1_rtc_dst;

#ifdef CONFIG_PM
static cy_en_syspm_status_t _ifx_cat1_rtc_syspm_callback(cy_stc_syspm_callback_params_t *params,
							 cy_en_syspm_callback_mode_t mode)
{
	return Cy_RTC_DeepSleepCallback(params, mode);
}

static cy_stc_syspm_callback_params_t _ifx_cat1_rtc_pm_cb_params = {NULL, NULL};
static cy_stc_syspm_callback_t _ifx_cat1_rtc_pm_cb = {
	.callback = &_ifx_cat1_rtc_syspm_callback,
	.type = CY_SYSPM_DEEPSLEEP,
	.callbackParams = &_ifx_cat1_rtc_pm_cb_params,
};
#endif /* CONFIG_PM */

#define _IFX_CAT1_RTC_WAIT_ONE_MS() Cy_SysLib_Delay(_IFX_CAT1_RTC_RETRY_DELAY_MS);

/* Internal macro to validate RTC year parameter falls within 21st century */
#define IFX_CAT1_RTC_VALID_CENTURY(year) ((year) >= _IFX_CAT1_RTC_INIT_CENTURY)

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
};

static inline uint16_t _ifx_cat1_rtc_get_state(void)
{
	return _FLD2VAL(_IFX_CAT1_RTC_BREG_STATE, _IFX_CAT1_RTC_BREG);
}

static inline void _ifx_cat1_rtc_set_state(uint16_t init)
{
	_IFX_CAT1_RTC_BREG &= _IFX_CAT1_RTC_BREG_CENTURY_Msk;
	_IFX_CAT1_RTC_BREG |= _VAL2FLD(_IFX_CAT1_RTC_BREG_STATE, init);
}

static inline uint16_t _ifx_cat1_rtc_get_century(void)
{
	return _FLD2VAL(_IFX_CAT1_RTC_BREG_CENTURY, _IFX_CAT1_RTC_BREG);
}

static inline void _ifx_cat1_rtc_set_century(uint16_t century)
{
	_IFX_CAT1_RTC_BREG &= _IFX_CAT1_RTC_BREG_STATE_Msk;
	_IFX_CAT1_RTC_BREG |= _VAL2FLD(_IFX_CAT1_RTC_BREG_CENTURY, century);
}

static void _ifx_cat1_rtc_from_pdl_time(cy_stc_rtc_config_t *pdlTime, const int year,
					struct rtc_time *z_time)
{
	CY_ASSERT(pdlTime != NULL);
	CY_ASSERT(z_time != NULL);

	z_time->tm_sec = (int)pdlTime->sec;
	z_time->tm_min = (int)pdlTime->min;
	z_time->tm_hour = (int)pdlTime->hour;
	z_time->tm_mday = (int)pdlTime->date;
	z_time->tm_year = (int)(year - _IFX_CAT1_RTC_TM_YEAR_BASE);

	/* The subtraction of 1 here is to translate between internal ifx_cat1 code and the Zephyr
	 * driver.
	 */
	z_time->tm_mon = (int)(pdlTime->month - 1u);
	z_time->tm_wday = (int)(pdlTime->dayOfWeek - 1u);

	/* year day not known in pdl RTC structure without conversion */
	z_time->tm_yday = -1;

	/* daylight savings currently marked as unknown */
	z_time->tm_isdst = -1;

	/* nanoseconds not tracked by ifx code. Set to value indicating unknown */
	z_time->tm_nsec = 0;
}

static void _ifx_cat1_rtc_isr_handler(void)
{
	Cy_RTC_Interrupt(_ifx_cat1_rtc_dst, NULL != _ifx_cat1_rtc_dst);
}

void _ifx_cat1_rtc_century_interrupt(void)
{
	/* The century is stored in its own register so when a "century interrupt"
	 * occurs at a rollover. The current century is retrieved and 100 is added
	 * to it and the register is reset to reflect the new century.
	 * i.e. 1999->2000
	 */
	_ifx_cat1_rtc_set_century(_ifx_cat1_rtc_get_century() + 100);
}

static int ifx_cat1_rtc_init(const struct device *dev)
{
	cy_rslt_t rslt = CY_RSLT_SUCCESS;

	Cy_SysClk_ClkBakSetSource(CY_SYSCLK_BAK_IN_CLKLF);

	if (_ifx_cat1_rtc_get_state() == _IFX_CAT1_RTC_STATE_UNINITIALIZED) {
		if (Cy_RTC_IsExternalResetOccurred()) {
			_ifx_cat1_rtc_set_century(_IFX_CAT1_RTC_INIT_CENTURY);
		}

#ifdef CONFIG_PM
		rslt = Cy_SysPm_RegisterCallback(&_ifx_cat1_rtc_pm_cb)
#endif /* CONFIG_PM */

		if (rslt == CY_RSLT_SUCCESS) {
			_ifx_cat1_rtc_set_state(_IFX_CAT1_RTC_STATE_ENABLED);
		} else {
			rslt = -EINVAL;
		}

	} else if (_ifx_cat1_rtc_get_state() == _IFX_CAT1_RTC_STATE_ENABLED ||
		   _ifx_cat1_rtc_get_state() == _IFX_CAT1_RTC_STATE_TIME_SET) {

		if (Cy_RTC_GetInterruptStatus() & CY_RTC_INTR_CENTURY) {
			_ifx_cat1_rtc_century_interrupt();
		}
	}

	Cy_RTC_ClearInterrupt(CY_RTC_INTR_CENTURY);
	Cy_RTC_SetInterruptMask(CY_RTC_INTR_CENTURY);

	_ifx_cat1_rtc_dst = NULL;
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), _ifx_cat1_rtc_isr_handler,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return rslt;
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
	uint32_t year = timeptr->tm_year + _IFX_CAT1_RTC_TM_YEAR_BASE;
	uint32_t year2digit = year % 100;

	cy_rslt_t rslt;
	uint32_t retry = 0;

	if (!CY_RTC_IS_SEC_VALID(sec) || !CY_RTC_IS_MIN_VALID(min) || !CY_RTC_IS_HOUR_VALID(hour) ||
	    !CY_RTC_IS_MONTH_VALID(mon) || !CY_RTC_IS_YEAR_SHORT_VALID(year2digit) ||
	    !IFX_CAT1_RTC_VALID_CENTURY(year)) {

		return -EINVAL;
	}
	do {
		if (retry != 0) {
			_IFX_CAT1_RTC_WAIT_ONE_MS();
		}

		k_spinlock_key_t key = k_spin_lock(&data->lock);

		rslt = Cy_RTC_SetDateAndTimeDirect(sec, min, hour, day, mon, year2digit);
		if (rslt == CY_RSLT_SUCCESS) {
			_ifx_cat1_rtc_set_century((uint16_t)(year) - (uint16_t)(year2digit));
		}

		k_spin_unlock(&data->lock, key);
		++retry;
	} while (rslt == CY_RTC_INVALID_STATE && retry < _IFX_CAT1_RTC_MAX_RETRY);

	retry = 0;
	while (CY_RTC_BUSY == Cy_RTC_GetSyncStatus() && retry < _IFX_CAT1_RTC_MAX_RETRY) {
		_IFX_CAT1_RTC_WAIT_ONE_MS();
		++retry;
	}

	if (rslt == CY_RSLT_SUCCESS) {
		_ifx_cat1_rtc_set_state(_IFX_CAT1_RTC_STATE_TIME_SET);
		return 0;
	} else {
		return -EINVAL;
	}
}

static int ifx_cat1_rtc_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct ifx_cat1_rtc_data *data = dev->data;

	cy_stc_rtc_config_t dateTime = {.hrFormat = CY_RTC_24_HOURS};

	if (_ifx_cat1_rtc_get_state() != _IFX_CAT1_RTC_STATE_TIME_SET) {
		LOG_ERR("Valid time has not been set with rtc_set_time yet");
		return -ENODATA;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	Cy_RTC_GetDateAndTime(&dateTime);
	const int year = (int)(dateTime.year + _ifx_cat1_rtc_get_century());

	k_spin_unlock(&data->lock, key);

	_ifx_cat1_rtc_from_pdl_time(&dateTime, year, timeptr);

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

static DEVICE_API(rtc, ifx_cat1_rtc_driver_api) = {
	.set_time = ifx_cat1_rtc_set_time,
	.get_time = ifx_cat1_rtc_get_time,
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = ifx_cat1_set_calibration,
	.get_calibration = ifx_cat1_get_calibration,
#endif
};

#define INFINEON_CAT1_RTC_INIT(n)                                                                  \
	static struct ifx_cat1_rtc_data ifx_cat1_rtc_data##n;                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ifx_cat1_rtc_init, NULL, &ifx_cat1_rtc_data##n,                   \
			      NULL, PRE_KERNEL_1, CONFIG_RTC_INIT_PRIORITY,        \
			      &ifx_cat1_rtc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_RTC_INIT)
