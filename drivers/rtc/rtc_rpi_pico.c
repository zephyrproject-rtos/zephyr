/*
 * Copyright (c) 2022, Andrei-Edward Popa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_rtc

#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>

/* pico-sdk includes */
#include <hardware/clocks.h>
#include <hardware/rtc.h>

LOG_MODULE_REGISTER(RPI_PICO, CONFIG_RTC_LOG_LEVEL);

typedef void (*irq_config_func_t)(void);

struct rtc_rpi_data {

};

struct rtc_rpi_config {
	irq_config_func_t irq_config_func;
};

static int rtc_rpi_set_time(const struct device *dev, const struct rtc_time *timeptr)
{

	return 0;
}

static int rtc_rpi_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	return 0;
}

#ifdef CONFIG_RTC_ALARM
static int  rtc_rpi_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						uint16_t *mask)
{
	return 0;
}

static int rtc_rpi_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				  const struct rtc_time *timeptr)
{
	return 0;
}

static int rtc_rpi_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				  struct rtc_time *timeptr)
{
	return 0;
}

static int rtc_rpi_alarm_is_pending(const struct device *dev, uint16_t id)
{
	return ret;
}

static int rtc_rpi_alarm_set_callback(const struct device *dev, uint16_t id,
				      rtc_alarm_callback callback, void *user_data)
{
	return 0;
}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
static int rtc_rpi_update_set_callback(const struct device *dev,
				       rtc_update_callback callback, void *user_data)
{
	return 0;
}
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_CALIBRATION
static int rtc_rpi_set_calibration(const struct device *dev, int32_t calibration)
{
	return 0;
}

static int rtc_rpi_get_calibration(const struct device *dev, int32_t *calibration)
{
	return 0;
}
#endif /* CONFIG_RTC_CALIBRATION */

int rtc_rpi_init(const struct device *dev)
{
	return 0;
}

static void rtc_rpi_isr(void *arg)
{

}

static void rtc_rpi_irq_config_func(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    rtc_rpi_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

struct rtc_driver_api rpi_driver_api = {
	.set_time = rtc_rpi_set_time,
	.get_time = rtc_rpi_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_rpi_alarm_get_supported_fields,
	.alarm_set_time = rtc_rpi_alarm_set_time,
	.alarm_get_time = rtc_rpi_alarm_get_time,
	.alarm_is_pending = rtc_rpi_alarm_is_pending,
	.alarm_set_callback = rtc_rpi_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	.update_set_callback = rtc_rpi_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = rtc_rpi_set_calibration,
	.get_calibration = rtc_rpi_get_calibration,
#endif /* CONFIG_RTC_CALIBRATION */
};

static struct rtc_rpi_data rpi_data;

static const struct rtc_rpi_config rpi_config = {
	.irq_config_func = rtc_rpi_irq_config_func,
};

DEVICE_DT_INST_DEFINE(0, &rtc_rpi_init,
		      NULL, &rpi_data,
		      &rpi_config, POST_KERNEL,
		      CONFIG_RTC_INIT_PRIORITY,
		      &rpi_driver_api);
