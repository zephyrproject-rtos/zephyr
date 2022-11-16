/*
 * Copyright (c) 2022, Andrei-Edward Popa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_rtc

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <time.h>
#include <zephyr/kernel.h>

/* pico-sdk includes */
#include <hardware/clocks.h>
#include <hardware/rtc.h>

#define YEAR_OFFSET 1900
#define MONTH_OFFSET 1

LOG_MODULE_REGISTER(RPI_PICO, CONFIG_COUNTER_LOG_LEVEL);

typedef void (*irq_config_func_t)(void);

struct counter_rpi_config {
	struct counter_config_info counter_info;
	const struct reset_dt_spec reset;
	irq_config_func_t irq_config_func;
	rtc_hw_t * const rtc_regs;
};

struct counter_rpi_data {
	struct counter_alarm_cfg alarm_cfg;
	uint32_t ticks;
};

static int rtc_get_tm_time(const struct device *dev, struct tm *tm_time)
{
	const struct counter_rpi_config *config = dev->config;
	rtc_hw_t * const rtc_regs = config->rtc_regs;

	if (!(rtc_regs->ctrl & RTC_CTRL_RTC_ACTIVE_BITS)) {
		return -EINVAL;
	}

	uint32_t rtc_0 = rtc_regs->rtc_0;
	uint32_t rtc_1 = rtc_regs->rtc_1;

	tm_time->tm_wday = (rtc_0 & RTC_RTC_0_DOTW_BITS) >> RTC_RTC_0_DOTW_LSB;
	tm_time->tm_hour = (rtc_0 & RTC_RTC_0_HOUR_BITS) >> RTC_RTC_0_HOUR_LSB;
	tm_time->tm_min = (rtc_0 & RTC_RTC_0_MIN_BITS) >> RTC_RTC_0_MIN_LSB;
	tm_time->tm_sec = (rtc_0 & RTC_RTC_0_SEC_BITS)  >> RTC_RTC_0_SEC_LSB;
	tm_time->tm_year = ((rtc_1 & RTC_RTC_1_YEAR_BITS) >> RTC_RTC_1_YEAR_LSB) - YEAR_OFFSET;
	tm_time->tm_mon = ((rtc_1 & RTC_RTC_1_MONTH_BITS) >> RTC_RTC_1_MONTH_LSB) - MONTH_OFFSET;
	tm_time->tm_mday = (rtc_1 & RTC_RTC_1_DAY_BITS) >> RTC_RTC_1_DAY_LSB;

	LOG_DBG("Desired time is %04d-%02d-%02d %02d:%02d:%02d\n", tm_time->tm_year + YEAR_OFFSET,
		tm_time->tm_mon + MONTH_OFFSET,
		tm_time->tm_mday, tm_time->tm_hour,
		tm_time->tm_min, tm_time->tm_sec);

	return 0;
}

static int rtc_get_unix_time(const struct device *dev, time_t *unix_time)
{
	struct tm tm_time;
	int err;

	err = rtc_get_tm_time(dev, &tm_time);
	if (err) {
		return err;
	}

	*unix_time = timeutil_timegm(&tm_time);

	LOG_DBG("Unix time is %u\n", (uint32_t)(*unix_time));

	return err;
}

static int counter_rpi_start(const struct device *dev);
static int counter_rpi_stop(const struct device *dev);

static int rtc_set_tm_time(const struct device *dev, struct tm *tm_time)
{
	const struct counter_rpi_config *config = dev->config;
	rtc_hw_t * const rtc_regs = config->rtc_regs;

	counter_rpi_stop(dev);

	rtc_regs->setup_0 = ((tm_time->tm_year + YEAR_OFFSET) << RTC_SETUP_0_YEAR_LSB) |
			    ((tm_time->tm_mon + MONTH_OFFSET) << RTC_SETUP_0_MONTH_LSB) |
			    (tm_time->tm_mday << RTC_SETUP_0_DAY_LSB);
	rtc_regs->setup_1 = (tm_time->tm_wday << RTC_SETUP_1_DOTW_LSB) |
			    (tm_time->tm_hour << RTC_SETUP_1_HOUR_LSB) |
			    (tm_time->tm_min << RTC_SETUP_1_MIN_LSB) |
			    (tm_time->tm_sec << RTC_SETUP_1_SEC_LSB);

	rtc_regs->ctrl = RTC_CTRL_LOAD_BITS;

	counter_rpi_start(dev);

	return 0;
}

static int counter_rpi_init(const struct device *dev)
{
	const struct counter_rpi_config *config = dev->config;
	rtc_hw_t * const rtc_regs = config->rtc_regs;
	struct tm start_tm_time;
	time_t start_unix_time;
	uint32_t rtc_freq;

	start_unix_time = 0;
	gmtime_r(&start_unix_time, &start_tm_time);

	rtc_freq = clock_get_hz(clk_rtc);
	if (rtc_freq == 0) {
		return -EINVAL;
	}

	reset_line_toggle(config->reset.dev, config->reset.id);

	rtc_freq -= 1;
	if (rtc_freq > RTC_CLKDIV_M1_BITS) {
		return -EINVAL;
	}

	rtc_regs->clkdiv_m1 = rtc_freq;

	config->irq_config_func();

	rtc_set_tm_time(dev, &start_tm_time);

	return 0;
}

static int counter_rpi_start(const struct device *dev)
{
	const struct counter_rpi_config *config = dev->config;
	rtc_hw_t * const rtc_regs = config->rtc_regs;

	rtc_regs->ctrl |= RTC_CTRL_RTC_ENABLE_BITS;
	while (!(rtc_regs->ctrl & RTC_CTRL_RTC_ACTIVE_BITS)) {
		/* Wait */
	}

	return 0;
}

static int counter_rpi_stop(const struct device *dev)
{
	const struct counter_rpi_config *config = dev->config;
	rtc_hw_t * const rtc_regs = config->rtc_regs;

	rtc_regs->ctrl &= ~RTC_CTRL_RTC_ENABLE_BITS;
	while (rtc_regs->ctrl & RTC_CTRL_RTC_ACTIVE_BITS) {
		/* Wait */
	}

	return 0;
}

static int counter_rpi_get_value(const struct device *dev, uint32_t *ticks)
{
	time_t unix_time;
	int err;

	err = rtc_get_unix_time(dev, &unix_time);
	if (err) {
		return err;
	}

	*ticks = unix_time;

	return 0;
}

static int counter_rpi_set_alarm(const struct device *dev, uint8_t chan_id,
				 const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_rpi_config *config = dev->config;
	struct counter_rpi_data * const data = dev->data;
	rtc_hw_t * const rtc_regs = config->rtc_regs;
	uint32_t seconds_until_alarm;
	time_t current_time;
	struct tm datetime;
	time_t alarm_time;
	int err;

	rtc_regs->irq_setup_0 &= ~RTC_IRQ_SETUP_0_MATCH_ENA_BITS;
	while (rtc_regs->irq_setup_0 & RTC_IRQ_SETUP_0_MATCH_ACTIVE_BITS) {
		/* Wait */
	}

	seconds_until_alarm = alarm_cfg->ticks;

	err = rtc_get_unix_time(dev, &current_time);

	if (err) {
		return err;
	}

	alarm_time = current_time + seconds_until_alarm;

	gmtime_r(&alarm_time, &datetime);

	rtc_regs->irq_setup_0 = ((datetime.tm_year + YEAR_OFFSET) << RTC_IRQ_SETUP_0_YEAR_LSB) |
				((datetime.tm_mon + MONTH_OFFSET) << RTC_IRQ_SETUP_0_MONTH_LSB) |
				(datetime.tm_mday << RTC_IRQ_SETUP_0_DAY_LSB);
	rtc_regs->irq_setup_1 = (datetime.tm_wday << RTC_IRQ_SETUP_1_DOTW_LSB) |
				(datetime.tm_hour << RTC_IRQ_SETUP_1_HOUR_LSB) |
				(datetime.tm_min << RTC_IRQ_SETUP_1_MIN_LSB) |
				(datetime.tm_sec << RTC_IRQ_SETUP_1_SEC_LSB);

	rtc_regs->irq_setup_0 |= RTC_IRQ_SETUP_0_YEAR_ENA_BITS;
	rtc_regs->irq_setup_0 |= RTC_IRQ_SETUP_0_MONTH_ENA_BITS;
	rtc_regs->irq_setup_0 |= RTC_IRQ_SETUP_0_DAY_ENA_BITS;
	rtc_regs->irq_setup_1 |= RTC_IRQ_SETUP_1_DOTW_ENA_BITS;
	rtc_regs->irq_setup_1 |= RTC_IRQ_SETUP_1_HOUR_ENA_BITS;
	rtc_regs->irq_setup_1 |= RTC_IRQ_SETUP_1_MIN_ENA_BITS;
	rtc_regs->irq_setup_1 |= RTC_IRQ_SETUP_1_SEC_ENA_BITS;

	data->alarm_cfg.callback = alarm_cfg->callback;
	data->alarm_cfg.user_data = alarm_cfg->user_data;

	rtc_regs->inte = RTC_INTE_RTC_BITS;

	rtc_regs->irq_setup_0 |= RTC_IRQ_SETUP_0_MATCH_ENA_BITS;
	while (!(rtc_regs->irq_setup_0 & RTC_IRQ_SETUP_0_MATCH_ACTIVE_BITS)) {
		/* Wait */
	}

	return 0;
}

static int counter_rpi_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_rpi_config *config = dev->config;
	rtc_hw_t * const rtc_regs = config->rtc_regs;

	rtc_regs->irq_setup_0 &= ~RTC_IRQ_SETUP_0_MATCH_ENA_BITS;
	while (rtc_regs->irq_setup_0 & RTC_IRQ_SETUP_0_MATCH_ACTIVE_BITS) {
		/* Wait */
	}

	return 0;
}

static int counter_rpi_set_top_value(const struct device *dev,
				     const struct counter_top_cfg *cfg)
{
	return -ENOTSUP;
}

static uint32_t counter_rpi_get_pending_int(const struct device *dev)
{
	const struct counter_rpi_config *config = dev->config;
	rtc_hw_t * const rtc_regs = config->rtc_regs;

	return rtc_regs->ints & RTC_INTS_RTC_BITS;
}

static uint32_t counter_rpi_get_top_value(const struct device *dev)
{
	return UINT32_MAX;
}

static void counter_rpi_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct counter_rpi_data *data = dev->data;
	uint32_t now;

	counter_rpi_cancel_alarm(dev, 0);
	counter_rpi_get_value(dev, &now);

	if (data->alarm_cfg.callback) {
		data->alarm_cfg.callback(dev, 0, now, data->alarm_cfg.user_data);
	}
}

static void counter_rpi_irq_config_func(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    counter_rpi_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

static struct counter_rpi_data counter_data;

static const struct counter_rpi_config counter_config = {
	.counter_info = {
		.max_top_value = UINT32_MAX,
		.freq = 1,
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,
		.channels = 1
	},
	.reset = RESET_DT_SPEC_INST_GET(0),
	.irq_config_func = counter_rpi_irq_config_func,
	.rtc_regs = rtc_hw
};

static const struct counter_driver_api rpi_driver_api = {
	.start = counter_rpi_start,
	.stop = counter_rpi_stop,
	.get_value = counter_rpi_get_value,
	.set_alarm = counter_rpi_set_alarm,
	.cancel_alarm = counter_rpi_cancel_alarm,
	.set_top_value = counter_rpi_set_top_value,
	.get_pending_int = counter_rpi_get_pending_int,
	.get_top_value = counter_rpi_get_top_value,
};

DEVICE_DT_INST_DEFINE(0, &counter_rpi_init,
		      NULL, &counter_data,
		      &counter_config, POST_KERNEL,
		      CONFIG_COUNTER_INIT_PRIORITY,
		      &rpi_driver_api);
