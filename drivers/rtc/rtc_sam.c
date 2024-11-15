/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_rtc

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>

#include <string.h>
#include <soc.h>
#include "rtc_utils.h"

#define RTC_SAM_REG_GET_FIELD(value, field) \
	((RTC_##field##_Msk & value) >> RTC_##field##_Pos)

#define RTC_SAM_WPMR_DISABLE 0x52544300
#define RTC_SAM_WPMR_ENABLE 0x52544301

#define RTC_SAM_CALIBRATE_PPB_MAX (1950000)
#define RTC_SAM_CALIBRATE_PPB_MIN (-1950000)
#define RTC_SAM_CALIBRATE_PPB_QUANTA (1500)
#define RTC_SAM_CALIBRATE_PPB_LOW_SCALE (30500)

#define RTC_SAM_TIME_MASK                                                                          \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_YEAR |     \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

typedef void (*rtc_sam_irq_init_fn_ptr)(void);

struct rtc_sam_config {
	Rtc *regs;
	uint16_t irq_num;
	rtc_sam_irq_init_fn_ptr irq_init_fn_ptr;
};

struct rtc_sam_data {
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

static void rtc_sam_disable_wp(void)
{
	REG_RTC_WPMR = RTC_SAM_WPMR_DISABLE;
}

static void rtc_sam_enable_wp(void)
{
	REG_RTC_WPMR = RTC_SAM_WPMR_ENABLE;
}

static uint32_t rtc_sam_timr_from_tm(const struct rtc_time *timeptr)
{
	uint32_t timr;

	timr = RTC_TIMR_SEC(bin2bcd(timeptr->tm_sec));
	timr |= RTC_TIMR_MIN(bin2bcd(timeptr->tm_min));
	timr |= RTC_TIMR_HOUR(bin2bcd(timeptr->tm_hour));

	return timr;
}

static uint32_t rtc_sam_calr_from_tm(const struct rtc_time *timeptr)
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

static int rtc_sam_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_sam_data *data = dev->data;
	const struct rtc_sam_config *config = dev->config;
	Rtc *regs = config->regs;

	if (rtc_utils_validate_rtc_time(timeptr, RTC_SAM_TIME_MASK) == false) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	k_sem_reset(&data->cr_sec_evt_sem);
	k_sem_take(&data->cr_sec_evt_sem, K_MSEC(1100));
	k_sem_reset(&data->cr_upd_ack_sem);

	/* Enable update acknowledge interrupt */
	regs->RTC_IER = RTC_IER_ACKEN;

	rtc_sam_disable_wp();

	/* Request update */
	regs->RTC_CR = (RTC_CR_UPDTIM | RTC_CR_UPDCAL);

	/* Await update acknowledge */
	if (k_sem_take(&data->cr_upd_ack_sem, K_MSEC(1100)) < 0) {
		regs->RTC_CR = 0;

		rtc_sam_enable_wp();

		/* Disable update acknowledge interrupt */
		regs->RTC_IDR = RTC_IDR_ACKDIS;

		k_spin_unlock(&data->lock, key);
		return -EAGAIN;
	}

	regs->RTC_TIMR = rtc_sam_timr_from_tm(timeptr);
	regs->RTC_CALR = rtc_sam_calr_from_tm(timeptr);
	regs->RTC_CR = 0;
	rtc_sam_enable_wp();
	regs->RTC_IDR = RTC_IDR_ACKDIS;
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int rtc_sam_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct rtc_sam_config *config = dev->config;
	Rtc *regs = config->regs;

	uint32_t timr0;
	uint32_t calr0;
	uint32_t timr1;
	uint32_t calr1;

	/* Validate time and date */
	if (regs->RTC_VER & (RTC_VER_NVTIM | RTC_VER_NVCAL)) {
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

	timeptr->tm_sec = bcd2bin(RTC_SAM_REG_GET_FIELD(timr0, TIMR_SEC));
	timeptr->tm_min = bcd2bin(RTC_SAM_REG_GET_FIELD(timr0, TIMR_MIN));
	timeptr->tm_hour = bcd2bin(RTC_SAM_REG_GET_FIELD(timr0, TIMR_HOUR));
	timeptr->tm_mday = bcd2bin(RTC_SAM_REG_GET_FIELD(calr0, CALR_DATE));
	timeptr->tm_mon = bcd2bin(RTC_SAM_REG_GET_FIELD(calr0, CALR_MONTH)) - 1;

	timeptr->tm_year = bcd2bin(RTC_SAM_REG_GET_FIELD(calr0, CALR_YEAR));
	timeptr->tm_year += ((int)bcd2bin(RTC_SAM_REG_GET_FIELD(calr0, CALR_CENT))) * 100;
	timeptr->tm_year -= 1900;

	timeptr->tm_wday = bcd2bin(RTC_SAM_REG_GET_FIELD(calr0, CALR_DAY)) - 1;
	timeptr->tm_yday = -1;
	timeptr->tm_isdst = -1;
	timeptr->tm_nsec = 0;
	return 0;
}

static void rtc_sam_isr(const struct device *dev)
{
	struct rtc_sam_data *data = dev->data;
	const struct rtc_sam_config *config = dev->config;
	Rtc *regs = config->regs;

	uint32_t sr = regs->RTC_SR;

	if (sr & RTC_SR_ACKUPD) {
		regs->RTC_SCCR = RTC_SCCR_ACKCLR;
		k_sem_give(&data->cr_upd_ack_sem);
	}

#ifdef CONFIG_RTC_ALARM
	if (sr & RTC_SR_ALARM) {
		regs->RTC_SCCR = RTC_SCCR_ALRCLR;
		if (data->alarm_callback != NULL) {
			data->alarm_callback(dev, 0, data->alarm_user_data);
		}
	}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
	if (sr & RTC_SR_SEC) {
		regs->RTC_SCCR = RTC_SCCR_SECCLR;
		if (data->update_callback != NULL) {
			data->update_callback(dev, data->update_user_data);
		}

		k_sem_give(&data->cr_sec_evt_sem);
	}
#endif /* CONFIG_RTC_UPDATE */
}

#ifdef CONFIG_RTC_ALARM
static uint16_t rtc_sam_alarm_get_supported_mask(void)
{
	return (RTC_ALARM_TIME_MASK_SECOND
	      | RTC_ALARM_TIME_MASK_MINUTE
	      | RTC_ALARM_TIME_MASK_HOUR
	      | RTC_ALARM_TIME_MASK_MONTHDAY
	      | RTC_ALARM_TIME_MASK_MONTH);
}

static uint32_t rtc_atmel_timalr_from_tm(const struct rtc_time *timeptr, uint32_t mask)
{
	uint32_t timalr = 0;

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		timalr |= RTC_TIMALR_SECEN;
		timalr |= RTC_TIMALR_SEC(bin2bcd(timeptr->tm_sec));
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		timalr |= RTC_TIMALR_MINEN;
		timalr |= RTC_TIMALR_MIN(bin2bcd(timeptr->tm_min));
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		timalr |= RTC_TIMALR_HOUREN;
		timalr |= RTC_TIMALR_HOUR(bin2bcd(timeptr->tm_hour));
	}

	return timalr;
}

static uint32_t rtc_atmel_calalr_from_tm(const struct rtc_time *timeptr, uint32_t mask)
{
	uint32_t calalr = RTC_CALALR_MONTH(1) | RTC_CALALR_DATE(1);

	if (mask & RTC_ALARM_TIME_MASK_MONTH) {
		calalr |= RTC_CALALR_MTHEN;
		calalr |= RTC_CALALR_MONTH(bin2bcd(timeptr->tm_mon + 1));
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		calalr |= RTC_CALALR_DATEEN;
		calalr |= RTC_CALALR_DATE(bin2bcd(timeptr->tm_mday));
	}

	return calalr;
}

static uint32_t rtc_sam_alarm_mask_from_timalr(uint32_t timalr)
{
	uint32_t mask = 0;

	if (timalr & RTC_TIMALR_SECEN) {
		mask |= RTC_ALARM_TIME_MASK_SECOND;
	}

	if (timalr & RTC_TIMALR_MINEN) {
		mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if (timalr & RTC_TIMALR_HOUREN) {
		mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	return mask;
}

static uint32_t rtc_sam_alarm_mask_from_calalr(uint32_t calalr)
{
	uint32_t mask = 0;

	if (calalr & RTC_CALALR_MTHEN) {
		mask |= RTC_ALARM_TIME_MASK_MONTH;
	}

	if (calalr & RTC_CALALR_DATEEN) {
		mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}

	return mask;
}

static void rtc_sam_tm_from_timalr_calalr(struct rtc_time *timeptr, uint32_t mask,
						 uint32_t timalr, uint32_t calalr)
{
	memset(timeptr, 0x00, sizeof(*timeptr));

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		timeptr->tm_sec = bcd2bin(RTC_SAM_REG_GET_FIELD(timalr, TIMALR_SEC));
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		timeptr->tm_min = bcd2bin(RTC_SAM_REG_GET_FIELD(timalr, TIMALR_MIN));
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		timeptr->tm_hour = bcd2bin(RTC_SAM_REG_GET_FIELD(timalr, TIMALR_HOUR));
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		timeptr->tm_mday = bcd2bin(RTC_SAM_REG_GET_FIELD(calalr, CALALR_DATE));
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTH) {
		timeptr->tm_mon = bcd2bin(RTC_SAM_REG_GET_FIELD(calalr, CALALR_MONTH)) - 1;
	}
}

static int rtc_sam_alarm_get_supported_fields(const struct device *dev, uint16_t id,
					      uint16_t *mask)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	*mask = rtc_sam_alarm_get_supported_mask();
	return 0;
}

static int rtc_sam_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				  const struct rtc_time *timeptr)
{
	struct rtc_sam_data *data = dev->data;
	const struct rtc_sam_config *config = dev->config;
	Rtc *regs = config->regs;

	uint32_t timalr;
	uint32_t calalr;
	uint32_t mask_supported;

	mask_supported = rtc_sam_alarm_get_supported_mask();

	if ((id != 0)) {
		return -EINVAL;
	}

	if ((mask > 0) && (timeptr == NULL)) {
		return -EINVAL;
	}

	if (mask & ~mask_supported) {
		return -EINVAL;
	}

	if (rtc_utils_validate_rtc_time(timeptr, mask) == false) {
		return -EINVAL;
	}

	timalr = rtc_atmel_timalr_from_tm(timeptr, mask);
	calalr = rtc_atmel_calalr_from_tm(timeptr, mask);

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	irq_disable(config->irq_num);
	rtc_sam_disable_wp();

	/* Set RTC alarm time */
	regs->RTC_TIMALR = timalr;
	regs->RTC_CALALR = calalr;

	rtc_sam_enable_wp();

	/* Clear alarm pending status */
	regs->RTC_SCCR = RTC_SCCR_ALRCLR;

	irq_enable(config->irq_num);
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int rtc_sam_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				  struct rtc_time *timeptr)
{
	struct rtc_sam_data *data = dev->data;
	const struct rtc_sam_config *config = dev->config;
	Rtc *regs = config->regs;

	uint32_t timalr;
	uint32_t calalr;

	if ((id != 0) || (mask == NULL) || (timeptr == NULL)) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	timalr = regs->RTC_TIMALR;
	calalr = regs->RTC_CALALR;

	k_spin_unlock(&data->lock, key);

	*mask = rtc_sam_alarm_mask_from_timalr(timalr);
	*mask |= rtc_sam_alarm_mask_from_calalr(calalr);

	rtc_sam_tm_from_timalr_calalr(timeptr, *mask, timalr, calalr);
	return 0;
}

static int rtc_sam_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct rtc_sam_data *data = dev->data;
	const struct rtc_sam_config *config = dev->config;
	Rtc *regs = config->regs;

	if (id != 0) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if ((regs->RTC_SR & RTC_SR_ALARM) == 0) {
		k_spin_unlock(&data->lock, key);

		return 0;
	}

	regs->RTC_SCCR = RTC_SCCR_ALRCLR;
	k_spin_unlock(&data->lock, key);
	return 1;
}

static int rtc_sam_alarm_set_callback(const struct device *dev, uint16_t id,
				      rtc_alarm_callback callback, void *user_data)
{
	struct rtc_sam_data *data = dev->data;
	const struct rtc_sam_config *config = dev->config;
	Rtc *regs = config->regs;

	if (id != 0) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	irq_disable(config->irq_num);
	data->alarm_callback = callback;
	data->alarm_user_data = user_data;

	if (data->alarm_callback) {
		regs->RTC_IER = RTC_IER_ALREN;
	} else {
		regs->RTC_IDR = RTC_IDR_ALRDIS;
	}

	irq_enable(config->irq_num);
	k_spin_unlock(&data->lock, key);
	return 0;
}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
static int rtc_sam_update_set_callback(const struct device *dev, rtc_update_callback callback,
				       void *user_data)
{
	struct rtc_sam_data *data = dev->data;
	const struct rtc_sam_config *config = dev->config;
	Rtc *regs = config->regs;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	irq_disable(config->irq_num);

	data->update_callback = callback;
	data->update_user_data = user_data;

	if (data->update_callback) {
		regs->RTC_IER = RTC_IER_SECEN;
	} else {
		regs->RTC_IDR = RTC_IDR_SECDIS;
	}

	irq_enable(config->irq_num);

	k_spin_unlock(&data->lock, key);

	return 0;
}
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_CALIBRATION
static int rtc_sam_set_calibration(const struct device *dev, int32_t calibration)
{
	struct rtc_sam_data *data = dev->data;
	const struct rtc_sam_config *config = dev->config;
	Rtc *regs = config->regs;
	bool negative_calibration;
	bool high_calibration;
	uint32_t slow_clock_calibration;
	uint32_t mr;

	if ((calibration < RTC_SAM_CALIBRATE_PPB_MIN) ||
	    (calibration > RTC_SAM_CALIBRATE_PPB_MAX)) {
		return -EINVAL;
	}

	/* The value written to the register is absolute */
	if (calibration < 0) {
		negative_calibration = true;
		calibration = -calibration;
	} else {
		negative_calibration = false;
	}

	/*
	 * Formula adapted from
	 * Atmel-11157-32-bit-Cortex-M4-Microcontroller-SAM4E16-SAM4E8_Datasheet.pdf
	 * section 15.6.2
	 *
	 * Formula if RTC_MR_HIGHPPM is 0
	 *
	 *   RTC_MR_CORRECTION = (3906 / (20 * ppm)) - 1
	 *
	 * Formula if RTC_MR_HIGHPPM is 1
	 *
	 *   RTC_MR_CORRECTION = (3906 / ppm) - 1
	 *
	 * Since we are working with ppb, we adapt the formula by increasing the
	 * terms of the fraction by 1000, turning the ppm into ppb
	 *
	 * Adapted formula if RTC_MR_HIGHPPM is 0
	 *
	 *   RTC_MR_CORRECTION = (3906000 / (20 * ppb)) - 1
	 *
	 * Adapted formula  if RTC_MR_HIGHPPM is 1
	 *
	 *   RTC_MR_CORRECTION = (3906000 / ppb) - 1
	 */
	if (calibration < RTC_SAM_CALIBRATE_PPB_QUANTA) {
		high_calibration = false;
		slow_clock_calibration = 0;
	} else if (calibration < RTC_SAM_CALIBRATE_PPB_LOW_SCALE) {
		high_calibration = false;
		slow_clock_calibration = (uint32_t)((3906000 / (20 * calibration)) - 1);
	} else {
		high_calibration = true;
		slow_clock_calibration = (uint32_t)((3906000 / calibration) - 1);
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	rtc_sam_disable_wp();

	mr = regs->RTC_MR;

	if (negative_calibration == true) {
		mr |= RTC_MR_NEGPPM;
	} else {
		mr &= ~RTC_MR_NEGPPM;
	}

	mr &= ~RTC_MR_CORRECTION_Msk;
	mr |= RTC_MR_CORRECTION(slow_clock_calibration);

	if (high_calibration == true) {
		mr |= RTC_MR_HIGHPPM;
	} else {
		mr &= ~RTC_MR_HIGHPPM;
	}

	regs->RTC_MR = mr;

	rtc_sam_enable_wp();

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int rtc_sam_get_calibration(const struct device *dev, int32_t *calibration)
{
	const struct rtc_sam_config *config = dev->config;
	Rtc *regs = config->regs;

	uint32_t mr;
	int32_t correction;

	if (calibration == NULL) {
		return -EINVAL;
	}

	mr = regs->RTC_MR;

	correction = (int32_t)RTC_SAM_REG_GET_FIELD(mr, MR_CORRECTION);

	/* Formula documented in rtc_sam_set_calibration() */
	if (correction == 0) {
		*calibration = 0;
	} else if (mr & RTC_MR_HIGHPPM) {
		*calibration = 3906000 / (correction + 1);
	} else {
		*calibration = 3906000 / ((correction + 1) * 20);
	}

	if (mr & RTC_MR_NEGPPM) {
		*calibration = -*calibration;
	}

	return 0;
}
#endif /* CONFIG_RTC_CALIBRATION */

static const struct rtc_driver_api rtc_sam_driver_api = {
	.set_time = rtc_sam_set_time,
	.get_time = rtc_sam_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_sam_alarm_get_supported_fields,
	.alarm_set_time = rtc_sam_alarm_set_time,
	.alarm_get_time = rtc_sam_alarm_get_time,
	.alarm_is_pending = rtc_sam_alarm_is_pending,
	.alarm_set_callback = rtc_sam_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	.update_set_callback = rtc_sam_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = rtc_sam_set_calibration,
	.get_calibration = rtc_sam_get_calibration,
#endif /* CONFIG_RTC_CALIBRATION */
};

static int rtc_sam_init(const struct device *dev)
{
	struct rtc_sam_data *data = dev->data;
	const struct rtc_sam_config *config = dev->config;
	Rtc *regs = config->regs;

	rtc_sam_disable_wp();
	regs->RTC_MR &= ~(RTC_MR_HRMOD | RTC_MR_PERSIAN);
	regs->RTC_CR = 0;
	rtc_sam_enable_wp();
	regs->RTC_IDR = (RTC_IDR_ACKDIS
			       | RTC_IDR_ALRDIS
			       | RTC_IDR_SECDIS
			       | RTC_IDR_TIMDIS
			       | RTC_IDR_CALDIS
			       | RTC_IDR_TDERRDIS);

	k_sem_init(&data->cr_sec_evt_sem, 0, 1);
	k_sem_init(&data->cr_upd_ack_sem, 0, 1);
	config->irq_init_fn_ptr();
	irq_enable(config->irq_num);
	return 0;
}

#define RTC_SAM_DEVICE(id)						\
	static void rtc_sam_irq_init_##id(void)				\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),	\
			    rtc_sam_isr, DEVICE_DT_INST_GET(id), 0);	\
	}								\
									\
	static const struct rtc_sam_config rtc_sam_config_##id = {	\
		.regs = (Rtc *)DT_INST_REG_ADDR(id),			\
		.irq_num = DT_INST_IRQN(id),				\
		.irq_init_fn_ptr = rtc_sam_irq_init_##id,		\
	};								\
									\
	static struct rtc_sam_data rtc_sam_data_##id;			\
									\
	DEVICE_DT_INST_DEFINE(id, rtc_sam_init, NULL,			\
			      &rtc_sam_data_##id,			\
			      &rtc_sam_config_##id, POST_KERNEL,	\
			      CONFIG_RTC_INIT_PRIORITY,			\
			      &rtc_sam_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_SAM_DEVICE);
