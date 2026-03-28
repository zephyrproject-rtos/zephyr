/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Andriy Gelman
 * Author: Andriy Gelman  <andriy.gelman@gmail.com>
 */

#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#include <soc.h>
#include <xmc_rtc.h>
#include <xmc_scu.h>

#define DT_DRV_COMPAT infineon_xmc4xxx_rtc

#define RTC_XMC4XXX_DEFAULT_PRESCALER 0x7fff

#define RTC_XMC4XXX_SUPPORTED_ALARM_MASK                                                           \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_YEAR)

struct rtc_xmc4xxx_data {
#if defined(CONFIG_RTC_ALARM)
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
#endif
#if defined(CONFIG_RTC_UPDATE)
	rtc_update_callback update_callback;
	void *update_user_data;
#endif
};

static int rtc_xmc4xxx_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct tm *stdtime;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	XMC_RTC_Stop();

	stdtime = rtc_time_to_tm((struct rtc_time *)timeptr);
	XMC_RTC_SetTimeStdFormat(stdtime);

	XMC_RTC_Start();

	return 0;
}

static int rtc_xmc4xxx_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct tm *stdtime = rtc_time_to_tm(timeptr);

	if (!XMC_RTC_IsRunning()) {
		return -ENODATA;
	}

	if (stdtime == NULL) {
		return -EINVAL;
	}

	XMC_RTC_GetTimeStdFormat(stdtime);
	timeptr->tm_nsec = 0;

	return 0;
}

#if defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE)
static void rtc_xmc4xxx_isr(const struct device *dev)
{
	struct rtc_xmc4xxx_data *dev_data = dev->data;

	uint32_t event = SCU_INTERRUPT->SRRAW;

#if defined(CONFIG_RTC_ALARM)
	if ((event & XMC_SCU_INTERRUPT_EVENT_RTC_ALARM) != 0) {
		if (dev_data->alarm_callback != NULL) {
			dev_data->alarm_callback(dev, 0, dev_data->alarm_user_data);
		}
		XMC_SCU_INTERRUPT_ClearEventStatus(XMC_SCU_INTERRUPT_EVENT_RTC_ALARM);
	}
#endif

#if defined(CONFIG_RTC_UPDATE)
	if ((event & XMC_SCU_INTERRUPT_EVENT_RTC_PERIODIC) != 0) {
		if (dev_data->update_callback != NULL) {
			dev_data->update_callback(dev, dev_data->update_user_data);
		}
		XMC_SCU_INTERRUPT_ClearEventStatus(XMC_SCU_INTERRUPT_EVENT_RTC_PERIODIC);
	}
#endif
}
#endif

#ifdef CONFIG_RTC_ALARM
static int rtc_xmc4xxx_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						  uint16_t *mask)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	*mask = RTC_XMC4XXX_SUPPORTED_ALARM_MASK;
	return 0;
}

static int rtc_xmc4xxx_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				      const struct rtc_time *timeptr)
{
	const struct tm *stdtime = rtc_time_to_tm((struct rtc_time *)timeptr);

	if (id != 0 || (mask > 0 && timeptr == NULL)) {
		return -EINVAL;
	}

	if (mask == 0) {
		XMC_RTC_DisableEvent(XMC_RTC_EVENT_ALARM);
		XMC_SCU_INTERRUPT_ClearEventStatus(XMC_SCU_INTERRUPT_EVENT_RTC_ALARM);
		return 0;
	}

	if (mask != RTC_XMC4XXX_SUPPORTED_ALARM_MASK) {
		return -EINVAL;
	}

	XMC_RTC_SetAlarmStdFormat(stdtime);
	XMC_RTC_EnableEvent(XMC_RTC_EVENT_ALARM);

	return 0;
}

static int rtc_xmc4xxx_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				      struct rtc_time *timeptr)
{
	ARG_UNUSED(dev);
	struct tm *stdtime = rtc_time_to_tm(timeptr);

	if (id != 0 || mask == NULL || timeptr == NULL) {
		return -EINVAL;
	}

	*mask = RTC_XMC4XXX_SUPPORTED_ALARM_MASK;

	XMC_RTC_GetAlarmStdFormat(stdtime);

	return 0;
}

static int rtc_xmc4xxx_alarm_is_pending(const struct device *dev, uint16_t id)
{
	ARG_UNUSED(dev);
	unsigned int key;
	int alarm = 0;
	uint32_t event;

	if (id != 0) {
		return -EINVAL;
	}

	key = irq_lock();
	event = SCU_INTERRUPT->SRRAW;

	if ((event & XMC_SCU_INTERRUPT_EVENT_RTC_ALARM) != 0) {
		alarm = 1;
		XMC_SCU_INTERRUPT_ClearEventStatus(XMC_SCU_INTERRUPT_EVENT_RTC_ALARM);
	}
	irq_unlock(key);

	return alarm;
}

static int rtc_xmc4xxx_alarm_set_callback(const struct device *dev, uint16_t id,
					  rtc_alarm_callback callback, void *user_data)
{
	struct rtc_xmc4xxx_data *dev_data = dev->data;
	unsigned int key;

	if (id != 0) {
		return -EINVAL;
	}

	key = irq_lock();
	dev_data->alarm_callback = callback;
	dev_data->alarm_user_data = user_data;
	irq_unlock(key);

	if (dev_data->alarm_callback) {
		XMC_SCU_INTERRUPT_EnableEvent(XMC_SCU_INTERRUPT_EVENT_RTC_ALARM);
	} else {
		XMC_SCU_INTERRUPT_DisableEvent(XMC_SCU_INTERRUPT_EVENT_RTC_ALARM);
	}

	return 0;
}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
static int rtc_xmc4xxx_update_set_callback(const struct device *dev, rtc_update_callback callback,
					   void *user_data)
{
	struct rtc_xmc4xxx_data *dev_data = dev->data;
	unsigned int key;

	key = irq_lock();
	dev_data->update_callback = callback;
	dev_data->update_user_data = user_data;
	irq_unlock(key);

	if (dev_data->update_callback) {
		XMC_RTC_EnableEvent(XMC_RTC_EVENT_PERIODIC_SECONDS);
		XMC_SCU_INTERRUPT_EnableEvent(XMC_SCU_INTERRUPT_EVENT_RTC_PERIODIC);
	} else {
		XMC_SCU_INTERRUPT_DisableEvent(XMC_SCU_INTERRUPT_EVENT_RTC_PERIODIC);
		XMC_RTC_DisableEvent(XMC_RTC_EVENT_PERIODIC_SECONDS);
	}

	return 0;
}
#endif /* CONFIG_RTC_UPDATE */

static DEVICE_API(rtc, rtc_xmc4xxx_driver_api) = {
	.set_time = rtc_xmc4xxx_set_time,
	.get_time = rtc_xmc4xxx_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_xmc4xxx_alarm_get_supported_fields,
	.alarm_set_time = rtc_xmc4xxx_alarm_set_time,
	.alarm_get_time = rtc_xmc4xxx_alarm_get_time,
	.alarm_is_pending = rtc_xmc4xxx_alarm_is_pending,
	.alarm_set_callback = rtc_xmc4xxx_alarm_set_callback,
#endif
#ifdef CONFIG_RTC_UPDATE
	.update_set_callback = rtc_xmc4xxx_update_set_callback,
#endif
};

#if defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE)
static void rtc_xmc4xxx_irq_config(void)
{
	/* RTC and watchdog share the same interrupt. Shared interrupts must */
	/* be enabled if WDT is enabled and RTC is using alarm or update feature */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), rtc_xmc4xxx_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}
#endif

static int rtc_xmc4xxx_init(const struct device *dev)
{
	if (!XMC_RTC_IsRunning()) {
		if (!XMC_SCU_HIB_IsHibernateDomainEnabled()) {
			XMC_SCU_HIB_EnableHibernateDomain();
		}
		XMC_RTC_SetPrescaler(RTC_XMC4XXX_DEFAULT_PRESCALER);
	}

#if defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE)
	rtc_xmc4xxx_irq_config();
#endif

	return 0;
}


static struct rtc_xmc4xxx_data rtc_xmc4xxx_data_0;

DEVICE_DT_INST_DEFINE(0, rtc_xmc4xxx_init, NULL, &rtc_xmc4xxx_data_0, NULL,
		      POST_KERNEL, CONFIG_RTC_INIT_PRIORITY, &rtc_xmc4xxx_driver_api);
