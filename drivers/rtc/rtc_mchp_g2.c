/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_rtc_g2

#include <string.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "rtc_utils.h"

LOG_MODULE_REGISTER(rtc_mchp_g1, CONFIG_RTC_LOG_LEVEL);

#define RTC_REG_GET_FIELD(value, field) ((RTC_##field##_Msk & value) >> RTC_##field##_Pos)

#ifdef CONFIG_RTC_CALIBRATION
/*
 * Formula adapted from SAMA7D6-Series-Data-Sheet-DS60001851.pdf section 32.6.2
 *
 * Formula if RTC_MR_HIGHPPM is 0
 *   RTC_MR_CORRECTION = (3906 / (20 * ppm)) - 1
 *
 * Formula if RTC_MR_HIGHPPM is 1
 *   RTC_MR_CORRECTION = (3906 / ppm) - 1
 *
 * Since we are working with ppb, we adapt the formula by increasing the
 * terms of the fraction by 1000, turning the ppm into ppb
 *
 * Adapted formula if RTC_MR_HIGHPPM is 0
 *   RTC_MR_CORRECTION = (3906000 / (20 * ppb)) - 1
 *
 * Adapted formula  if RTC_MR_HIGHPPM is 1
 *   RTC_MR_CORRECTION = (3906000 / ppb) - 1
 */
#define RTC_MCHP_LOW_PPM_CORRECTION(ppm)	((uint32_t)((3906000 / (20 * (ppm))) - 1))
#define RTC_MCHP_HIGH_PPM_CORRECTION(ppm)	((uint32_t)((3906000 / (ppm)) - 1))
#define RTC_MCHP_CALCULATE_LOW_PPM(correction)	(3906000 / (((correction) + 1) * 20))
#define RTC_MCHP_CALCULATE_HIGH_PPM(correction)	(3906000 / ((correction) + 1))

/*
 * from SAMA7D6-Series-Data-Sheet-DS60001851.pdf section 32.5.7
 *
 * The RTC clock calibration circuitry allows positive or negative correction in
 * a range of 1.5 ppm to 1950 ppm.
 */
#define RTC_MCHP_CALIBRATE_PPB_MAX		1950000
#define RTC_MCHP_CALIBRATE_PPB_MIN		(-1950000)
#define RTC_MCHP_CALIBRATE_PPB_QUANTA		1500
#define RTC_MCHP_CALIBRATE_PPB_LOW_SCALE	30500
#endif /* CONFIG_RTC_CALIBRATION */

#define RTC_MCHP_TIME_MASK									\
		(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE |			\
		 RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTH |				\
		 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_YEAR |			\
		 RTC_ALARM_TIME_MASK_WEEKDAY)

#ifdef CONFIG_RTC_ALARM
#define RTC_MCHP_ALARM_SUPPORTED_MASK								\
		(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE |			\
		 RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY |			\
		 RTC_ALARM_TIME_MASK_MONTH)
#endif /* CONFIG_RTC_ALARM */

typedef void (*rtc_mchp_irq_init_fn_ptr)(void);

struct rtc_mchp_config {
	rtc_registers_t *regs;
	const struct device *syscwp;
	uint16_t irq_num;
	rtc_mchp_irq_init_fn_ptr irq_init_fn_ptr;
};

struct rtc_mchp_data {
#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	rtc_update_callback update_callback;
	void *update_user_data;
#endif /* CONFIG_RTC_UPDATE */
	struct k_spinlock lock;
	struct k_sem cr_sec_evt_sem;
	struct k_sem cr_upd_ack_sem;
};

static inline void rtc_mchp_disable_wp(const struct device *syscwp)
{
	syscon_write_reg(syscwp, SYSCWP_SYSC_WPMR_REG_OFST,
			 SYSCWP_SYSC_WPMR_WPKEY_PASSWD | SYSCWP_SYSC_WPMR_WPITEN_0 |
			 SYSCWP_SYSC_WPMR_WPEN_0);
}

static inline void rtc_mchp_enable_wp(const struct device *syscwp)
{
	syscon_write_reg(syscwp, SYSCWP_SYSC_WPMR_REG_OFST,
			 SYSCWP_SYSC_WPMR_WPKEY_PASSWD | SYSCWP_SYSC_WPMR_WPITEN_1 |
			 SYSCWP_SYSC_WPMR_WPEN_1);
}

static inline uint32_t rtc_mchp_timr_from_tm(const struct rtc_time *timeptr)
{
	uint32_t timr;

	timr = RTC_TIMR_SEC(bin2bcd(timeptr->tm_sec));
	timr |= RTC_TIMR_MIN(bin2bcd(timeptr->tm_min));
	timr |= RTC_TIMR_HOUR(bin2bcd(timeptr->tm_hour));

	return timr;
}

static inline uint32_t rtc_mchp_calr_from_tm(const struct rtc_time *timeptr)
{
	uint32_t calr;
	uint8_t centuries;
	uint8_t years;

	calr = RTC_CALR_DATE(bin2bcd(timeptr->tm_mday));
	calr |= RTC_CALR_MONTH(bin2bcd(timeptr->tm_mon + 1));
	centuries = (uint8_t)((timeptr->tm_year / 100) + 19);
	years = (uint8_t)((timeptr->tm_year % 100));
	calr |= RTC_CALR_CENT(bin2bcd(centuries));
	calr |= RTC_CALR_YEAR(bin2bcd(years));
	calr |= RTC_CALR_DAY(bin2bcd(timeptr->tm_wday + 1));

	return calr;
}

static int rtc_mchp_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_mchp_data *data = dev->data;
	const struct rtc_mchp_config *config = dev->config;
	rtc_registers_t *regs = config->regs;
	k_spinlock_key_t key;
	int ret = 0;

	if (timeptr == NULL) {
		LOG_ERR("RTC set time failed: time pointer is NULL");

		return -EINVAL;
	} else if (rtc_utils_validate_rtc_time(timeptr, RTC_MCHP_TIME_MASK) == false) {
		LOG_ERR("RTC time parameters are invalid");

		return -EINVAL;
	}

	k_sem_reset(&data->cr_sec_evt_sem);
	k_sem_take(&data->cr_sec_evt_sem, K_MSEC(1100));

	key = k_spin_lock(&data->lock);
	rtc_mchp_disable_wp(config->syscwp);

	/* Request update */
	regs->RTC_CR = (RTC_CR_UPDTIM_Msk | RTC_CR_UPDCAL_Msk);

	if (WAIT_FOR(((regs->RTC_SR & RTC_SR_ACKUPD_Msk) != 0), 5000, k_busy_wait(1))) {
		regs->RTC_SCCR = RTC_SCCR_ACKCLR_Msk;
		regs->RTC_TIMR = rtc_mchp_timr_from_tm(timeptr);
		regs->RTC_CALR = rtc_mchp_calr_from_tm(timeptr);
	} else {
		LOG_ERR("RTC wait ACKUPD timed out");
		printf("%s %d\n", __FILE__, __LINE__);
		ret = -EAGAIN;
	}

	regs->RTC_CR = 0;

	rtc_mchp_enable_wp(config->syscwp);
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int rtc_mchp_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct rtc_mchp_config *config = dev->config;
	rtc_registers_t *regs = config->regs;

	uint32_t timr0;
	uint32_t calr0;
	uint32_t timr1;
	uint32_t calr1;

	/* Validate time and date */
	if (regs->RTC_VER & (RTC_VER_NVTIM_Msk | RTC_VER_NVCAL_Msk)) {
		return -ENODATA;
	}

	/* Read until synchronized (registers updated async) */
	while (1) {
		timr0 = regs->RTC_TIMR;
		calr0 = regs->RTC_CALR;
		timr1 = regs->RTC_TIMR;
		calr1 = regs->RTC_CALR;

		if ((timr0 == timr1) && (calr0 == calr1)) {
			break;
		}
	}

	timeptr->tm_sec = bcd2bin(RTC_REG_GET_FIELD(timr0, TIMR_SEC));
	timeptr->tm_min = bcd2bin(RTC_REG_GET_FIELD(timr0, TIMR_MIN));
	timeptr->tm_hour = bcd2bin(RTC_REG_GET_FIELD(timr0, TIMR_HOUR));
	timeptr->tm_mday = bcd2bin(RTC_REG_GET_FIELD(calr0, CALR_DATE));
	timeptr->tm_mon = bcd2bin(RTC_REG_GET_FIELD(calr0, CALR_MONTH)) - 1;

	timeptr->tm_year = bcd2bin(RTC_REG_GET_FIELD(calr0, CALR_YEAR));
	timeptr->tm_year += ((int)bcd2bin(RTC_REG_GET_FIELD(calr0, CALR_CENT))) * 100;
	timeptr->tm_year -= 1900;

	timeptr->tm_wday = bcd2bin(RTC_REG_GET_FIELD(calr0, CALR_DAY)) - 1;
	timeptr->tm_yday = -1;
	timeptr->tm_isdst = -1;
	timeptr->tm_nsec = 0;

	return 0;
}

static void rtc_mchp_isr(const struct device *dev)
{
	struct rtc_mchp_data *data = dev->data;
	const struct rtc_mchp_config *config = dev->config;
	rtc_registers_t *regs = config->regs;
	uint32_t sr = regs->RTC_SR;

	rtc_mchp_disable_wp(config->syscwp);

	if (sr & RTC_SR_ACKUPD_Msk) {
		regs->RTC_SCCR = RTC_SCCR_ACKCLR_Msk;
		k_sem_give(&data->cr_upd_ack_sem);
	}

#ifdef CONFIG_RTC_ALARM
	if (sr & RTC_SR_ALARM_Msk) {
		regs->RTC_SCCR = RTC_SCCR_ALRCLR_Msk;
		if (data->alarm_callback != NULL) {
			data->alarm_callback(dev, 0, data->alarm_user_data);
		}
	}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
	if (sr & RTC_SCCR_SECCLR_Msk) {
		regs->RTC_SCCR = RTC_SCCR_SECCLR_Msk;
		if (data->update_callback != NULL) {
			data->update_callback(dev, data->update_user_data);
		}

		k_sem_give(&data->cr_sec_evt_sem);
	}
#endif /* CONFIG_RTC_UPDATE */

	rtc_mchp_enable_wp(config->syscwp);
}

#ifdef CONFIG_RTC_ALARM
static inline uint32_t rtc_atmel_timalr_from_tm(const struct rtc_time *timeptr, uint32_t mask)
{
	uint32_t timalr = 0;

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		timalr |= RTC_TIMALR_SECEN_Msk;
		timalr |= RTC_TIMALR_SEC(bin2bcd(timeptr->tm_sec));
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		timalr |= RTC_TIMALR_MINEN_Msk;
		timalr |= RTC_TIMALR_MIN(bin2bcd(timeptr->tm_min));
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		timalr |= RTC_TIMALR_HOUREN_Msk;
		timalr |= RTC_TIMALR_HOUR(bin2bcd(timeptr->tm_hour));
	}

	return timalr;
}

static inline uint32_t rtc_atmel_calalr_from_tm(const struct rtc_time *timeptr, uint32_t mask)
{
	uint32_t calalr = RTC_CALALR_MONTH(1) | RTC_CALALR_DATE(1);

	if (mask & RTC_ALARM_TIME_MASK_MONTH) {
		calalr |= RTC_CALALR_MTHEN_Msk;
		calalr |= RTC_CALALR_MONTH(bin2bcd(timeptr->tm_mon + 1));
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		calalr |= RTC_CALALR_DATEEN_Msk;
		calalr |= RTC_CALALR_DATE(bin2bcd(timeptr->tm_mday));
	}

	return calalr;
}

static inline uint32_t rtc_mchp_alarm_mask_from_timalr(uint32_t timalr)
{
	uint32_t mask = 0;

	if (timalr & RTC_TIMALR_SECEN_Msk) {
		mask |= RTC_ALARM_TIME_MASK_SECOND;
	}

	if (timalr & RTC_TIMALR_MINEN_Msk) {
		mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if (timalr & RTC_TIMALR_HOUREN_Msk) {
		mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	return mask;
}

static inline uint32_t rtc_mchp_alarm_mask_from_calalr(uint32_t calalr)
{
	uint32_t mask = 0;

	if (calalr & RTC_CALALR_MTHEN_Msk) {
		mask |= RTC_ALARM_TIME_MASK_MONTH;
	}

	if (calalr & RTC_CALALR_DATEEN_Msk) {
		mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}

	return mask;
}

static void rtc_mchp_tm_from_timalr_calalr(struct rtc_time *timeptr, uint32_t mask, uint32_t timalr,
					   uint32_t calalr)
{
	memset(timeptr, 0, sizeof(*timeptr));

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		timeptr->tm_sec = bcd2bin(RTC_REG_GET_FIELD(timalr, TIMALR_SEC));
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		timeptr->tm_min = bcd2bin(RTC_REG_GET_FIELD(timalr, TIMALR_MIN));
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		timeptr->tm_hour = bcd2bin(RTC_REG_GET_FIELD(timalr, TIMALR_HOUR));
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		timeptr->tm_mday = bcd2bin(RTC_REG_GET_FIELD(calalr, CALALR_DATE));
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTH) {
		timeptr->tm_mon = bcd2bin(RTC_REG_GET_FIELD(calalr, CALALR_MONTH)) - 1;
	}
}

static int rtc_mchp_alarm_get_supported_fields(const struct device *dev, uint16_t id,
					       uint16_t *mask)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	*mask = RTC_MCHP_ALARM_SUPPORTED_MASK;

	return 0;
}

static int rtc_mchp_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				   const struct rtc_time *timeptr)
{
	struct rtc_mchp_data *data = dev->data;
	const struct rtc_mchp_config *config = dev->config;
	rtc_registers_t *regs = config->regs;
	k_spinlock_key_t key;

	if ((id != 0)) {
		LOG_ERR("RTC Alarm id is out of range");

		return -EINVAL;
	}

	/* Check if the time pointer is provided when the alarm mask is not zero */
	if ((mask != 0) && (timeptr == NULL)) {
		LOG_ERR("No pointer is provided to set RTC alarm");

		return -EINVAL;
	}

	if ((mask & ~RTC_MCHP_ALARM_SUPPORTED_MASK) != 0) {
		LOG_ERR("Invalid RTC alarm mask");

		return -EINVAL;
	}

	if (rtc_utils_validate_rtc_time(timeptr, mask) == false) {
		LOG_ERR("Invalid RTC time");

		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	irq_disable(config->irq_num);
	rtc_mchp_disable_wp(config->syscwp);

	/* Set RTC alarm time */
	regs->RTC_TIMALR = rtc_atmel_timalr_from_tm(timeptr, mask);
	regs->RTC_CALALR = rtc_atmel_calalr_from_tm(timeptr, mask);

	/* Clear alarm pending status */
	regs->RTC_SCCR = RTC_SCCR_ALRCLR_Msk;

	rtc_mchp_enable_wp(config->syscwp);
	irq_enable(config->irq_num);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int rtc_mchp_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				   struct rtc_time *timeptr)
{
	struct rtc_mchp_data *data = dev->data;
	const struct rtc_mchp_config *config = dev->config;
	rtc_registers_t *regs = config->regs;
	k_spinlock_key_t key;

	uint32_t timalr;
	uint32_t calalr;

	if ((id != 0) || (mask == NULL) || (timeptr == NULL)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	timalr = regs->RTC_TIMALR;
	calalr = regs->RTC_CALALR;

	k_spin_unlock(&data->lock, key);

	*mask = rtc_mchp_alarm_mask_from_timalr(timalr);
	*mask |= rtc_mchp_alarm_mask_from_calalr(calalr);

	rtc_mchp_tm_from_timalr_calalr(timeptr, *mask, timalr, calalr);

	return 0;
}

static int rtc_mchp_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct rtc_mchp_data *data = dev->data;
	const struct rtc_mchp_config *config = dev->config;
	rtc_registers_t *regs = config->regs;
	k_spinlock_key_t key;
	int ret = 0;

	if (id != 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if ((regs->RTC_SR & RTC_SR_ALARM_Msk) != 0) {
		regs->RTC_SCCR = RTC_SCCR_ALRCLR_Msk;
		ret = 1;
	}
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int rtc_mchp_alarm_set_callback(const struct device *dev, uint16_t id,
				       rtc_alarm_callback callback, void *user_data)
{
	struct rtc_mchp_data *data = dev->data;
	const struct rtc_mchp_config *config = dev->config;
	rtc_registers_t *regs = config->regs;
	k_spinlock_key_t key;

	if (id != 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	irq_disable(config->irq_num);

	data->alarm_callback = callback;
	data->alarm_user_data = user_data;

	if (data->alarm_callback) {
		regs->RTC_IER = RTC_IER_ALREN_Msk;
	} else {
		regs->RTC_IDR = RTC_IDR_ALRDIS_Msk;
	}

	irq_enable(config->irq_num);
	k_spin_unlock(&data->lock, key);

	return 0;
}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
static int rtc_mchp_update_set_callback(const struct device *dev, rtc_update_callback callback,
					void *user_data)
{
	struct rtc_mchp_data *data = dev->data;
	const struct rtc_mchp_config *config = dev->config;
	rtc_registers_t *regs = config->regs;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	irq_disable(config->irq_num);
	rtc_mchp_disable_wp(config->syscwp);

	data->update_callback = callback;
	data->update_user_data = user_data;

	if (data->update_callback) {
		regs->RTC_IER = RTC_IER_SECEN_Msk;
	} else {
		regs->RTC_IDR = RTC_IDR_SECDIS_Msk;
	}

	rtc_mchp_enable_wp(config->syscwp);
	irq_enable(config->irq_num);

	k_spin_unlock(&data->lock, key);

	return 0;
}
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_CALIBRATION
static int rtc_mchp_set_calibration(const struct device *dev, int32_t calibration)
{
	struct rtc_mchp_data *data = dev->data;
	const struct rtc_mchp_config *config = dev->config;
	rtc_registers_t *regs = config->regs;
	uint32_t slow_clock_calibration;
	k_spinlock_key_t key;
	uint32_t mr_set_bit = 0;
	uint32_t mr;

	if ((calibration < RTC_MCHP_CALIBRATE_PPB_MIN) ||
	    (calibration > RTC_MCHP_CALIBRATE_PPB_MAX)) {
		LOG_ERR("calibration value (%d) out of range", calibration);

		return -EINVAL;
	}

	/* The value written to the register is absolute */
	if (calibration < 0) {
		mr_set_bit |= RTC_MR_NEGPPM_Msk;
		calibration = -calibration;
	}

	if (calibration < RTC_MCHP_CALIBRATE_PPB_QUANTA) {
		slow_clock_calibration = 0;
	} else if (calibration < RTC_MCHP_CALIBRATE_PPB_LOW_SCALE) {
		slow_clock_calibration = RTC_MCHP_LOW_PPM_CORRECTION(calibration);
	} else {
		mr_set_bit |= RTC_MR_HIGHPPM_Msk;
		slow_clock_calibration = RTC_MCHP_HIGH_PPM_CORRECTION(calibration);
	}
	mr_set_bit |= RTC_MR_CORRECTION(slow_clock_calibration);

	key = k_spin_lock(&data->lock);
	rtc_mchp_disable_wp(config->syscwp);

	mr = regs->RTC_MR & ~(RTC_MR_HIGHPPM_Msk | RTC_MR_NEGPPM_Msk | RTC_MR_CORRECTION_Msk);
	regs->RTC_MR = mr | mr_set_bit;

	rtc_mchp_enable_wp(config->syscwp);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int rtc_mchp_get_calibration(const struct device *dev, int32_t *calibration)
{
	const struct rtc_mchp_config *config = dev->config;
	rtc_registers_t *regs = config->regs;
	int32_t correction;
	uint32_t mr;

	if (calibration == NULL) {
		LOG_ERR("Invalid input: calibration pointer is NULL");
		return -EINVAL;
	}

	mr = regs->RTC_MR;
	/* Retrieve the correction value */
	correction = (int32_t)RTC_REG_GET_FIELD(mr, MR_CORRECTION);

	/* Calculate the calibration value based on the correction value */
	if (correction == 0) {
		*calibration = 0;
	} else if (mr & RTC_MR_HIGHPPM_Msk) {
		*calibration = RTC_MCHP_CALCULATE_HIGH_PPM(correction);
	} else {
		*calibration = RTC_MCHP_CALCULATE_LOW_PPM(correction);
	}

	/* Adjust the calibration value based on the sign bit */
	if ((mr & RTC_MR_NEGPPM_Msk) != 0) {
		*calibration *= -1;
	}

	return 0;
}
#endif /* CONFIG_RTC_CALIBRATION */

static DEVICE_API(rtc, rtc_mchp_driver_api) = {
	.set_time = rtc_mchp_set_time,
	.get_time = rtc_mchp_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_mchp_alarm_get_supported_fields,
	.alarm_set_time = rtc_mchp_alarm_set_time,
	.alarm_get_time = rtc_mchp_alarm_get_time,
	.alarm_is_pending = rtc_mchp_alarm_is_pending,
	.alarm_set_callback = rtc_mchp_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	.update_set_callback = rtc_mchp_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = rtc_mchp_set_calibration,
	.get_calibration = rtc_mchp_get_calibration,
#endif /* CONFIG_RTC_CALIBRATION */
};

static int rtc_mchp_init(const struct device *dev)
{
	struct rtc_mchp_data *data = dev->data;
	const struct rtc_mchp_config *config = dev->config;
	rtc_registers_t *regs = config->regs;

	rtc_mchp_disable_wp(config->syscwp);
	regs->RTC_MR &= ~RTC_MR_HRMOD_Msk;
	regs->RTC_CR = 0;
	regs->RTC_IDR = (RTC_IDR_ACKDIS_Msk | RTC_IDR_ALRDIS_Msk | RTC_IDR_SECDIS_Msk |
			 RTC_IDR_TIMDIS_Msk | RTC_IDR_CALDIS_Msk | RTC_IDR_TDERRDIS_Msk);
	rtc_mchp_enable_wp(config->syscwp);

	k_sem_init(&data->cr_sec_evt_sem, 0, 1);
	k_sem_init(&data->cr_upd_ack_sem, 0, 1);
	config->irq_init_fn_ptr();
	irq_enable(config->irq_num);

	return 0;
}

#define RTC_MCHP_DEVICE(n)									\
	static void rtc_mchp_irq_init_##n(void)							\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),				\
			    rtc_mchp_isr, DEVICE_DT_INST_GET(n), 0);				\
	}											\
												\
	static const struct rtc_mchp_config rtc_mchp_config_##n = {				\
		.regs = (rtc_registers_t *)DT_INST_REG_ADDR(n),					\
		.syscwp = DEVICE_DT_GET(DT_INST_PROP(n, protection)),				\
		.irq_num = DT_INST_IRQN(n),							\
		.irq_init_fn_ptr = rtc_mchp_irq_init_##n,					\
	};											\
												\
	static struct rtc_mchp_data rtc_mchp_data_##n;						\
												\
	DEVICE_DT_INST_DEFINE(n, rtc_mchp_init, NULL,						\
			      &rtc_mchp_data_##n,						\
			      &rtc_mchp_config_##n, POST_KERNEL,				\
			      CONFIG_RTC_INIT_PRIORITY,						\
			      &rtc_mchp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_MCHP_DEVICE);
