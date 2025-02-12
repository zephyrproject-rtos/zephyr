/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_rtc_timer

/*
 * Include esp-idf headers first to avoid
 * redefining BIT() macro
 */
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc.h"
#include <esp_rom_sys.h>
#include <hal/rtc_cntl_ll.h>

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/spinlock.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/esp32_clock_control.h>

#if defined(CONFIG_SOC_SERIES_ESP32C3)
#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>
#else
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp32_counter_rtc, CONFIG_COUNTER_LOG_LEVEL);

#if defined(CONFIG_SOC_SERIES_ESP32C3)
#define ESP32_COUNTER_RTC_ISR_HANDLER isr_handler_t
#else
#define ESP32_COUNTER_RTC_ISR_HANDLER intr_handler_t
#endif

static void counter_esp32_isr(void *arg);

struct counter_esp32_config {
	struct counter_config_info counter_info;
	int irq_source;
	const struct device *clock_dev;
};

struct counter_esp32_data {
	struct counter_alarm_cfg alarm_cfg;
	uint32_t ticks;
	uint32_t clk_src_freq;
};

static int counter_esp32_init(const struct device *dev)
{
	const struct counter_esp32_config *cfg = dev->config;
	struct counter_esp32_data *data = dev->data;


	/* RTC_SLOW_CLK is the default clk source */
	clock_control_get_rate(cfg->clock_dev,
			       (clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_RTC_SLOW,
			       &data->clk_src_freq);

	esp_intr_alloc(cfg->irq_source,
			0,
			(ESP32_COUNTER_RTC_ISR_HANDLER)counter_esp32_isr,
			(void *)dev,
			NULL);

	return 0;
}

static int counter_esp32_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* RTC main timer runs after power-on reset */
	return 0;
}

static int counter_esp32_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	/*
	 * Any reset/sleep mode, except for the power-up
	 * reset, will not stop or reset the RTC timer
	 * ESP32 TRM v4.6 sec. 31.3.11
	 */
	return 0;
}

static int counter_esp32_get_value(const struct device *dev, uint32_t *ticks)
{
	ARG_UNUSED(dev);

	*ticks = (uint32_t) rtc_cntl_ll_get_rtc_time();

	return 0;
}

static int counter_esp32_set_alarm(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);
	struct counter_esp32_data *data = dev->data;
	uint32_t now;
	uint32_t ticks = 0;

#if defined(CONFIG_SOC_SERIES_ESP32) || defined(CONFIG_SOC_SERIES_ESP32C3)
	/* In ESP32/C3 Series the min possible value is 30 us*/
	if (counter_ticks_to_us(dev, alarm_cfg->ticks) < 30) {
		return -EINVAL;
	}
#endif
	data->alarm_cfg.callback = alarm_cfg->callback;
	data->alarm_cfg.user_data = alarm_cfg->user_data;

	counter_esp32_get_value(dev, &now);

	ticks = (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) ? alarm_cfg->ticks
								: now + alarm_cfg->ticks;

	rtc_cntl_ll_set_wakeup_timer(ticks);

	/* RTC main timer set alarm value */
	CLEAR_PERI_REG_MASK(RTC_CNTL_SLP_TIMER1_REG, 0xffffffff);

	/* RTC main timer set alarm enable */
	SET_PERI_REG_MASK(RTC_CNTL_SLP_TIMER1_REG, RTC_CNTL_MAIN_TIMER_ALARM_EN);

	/* RTC main timer interrupt enable */
	SET_PERI_REG_MASK(RTC_CNTL_INT_ENA_REG, RTC_CNTL_MAIN_TIMER_INT_ENA);

	return 0;
}

static int counter_esp32_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);

	/* RTC main timer interrupt disable */
	SET_PERI_REG_MASK(RTC_CNTL_INT_CLR_REG, RTC_CNTL_MAIN_TIMER_INT_CLR);

	/* RTC main timer set alarm disable */
	CLEAR_PERI_REG_MASK(RTC_CNTL_SLP_TIMER1_REG, RTC_CNTL_MAIN_TIMER_ALARM_EN);

	return 0;
}

static int counter_esp32_set_top_value(const struct device *dev,
				       const struct counter_top_cfg *cfg)
{
	const struct counter_esp32_config *config = dev->config;

	if (cfg->ticks != config->counter_info.max_top_value) {
		return -ENOTSUP;
	}

	return 0;
}

static uint32_t counter_esp32_get_pending_int(const struct device *dev)
{
	ARG_UNUSED(dev);

	uint32_t rc = READ_PERI_REG(RTC_CNTL_INT_ST_REG) & RTC_CNTL_MAIN_TIMER_INT_ST;

	return (rc >> RTC_CNTL_MAIN_TIMER_INT_ST_S);
}

/*
 * Espressif's RTC Timer is actually 48-bits in resolution
 * However, the top value returned is limited to UINT32_MAX
 * as per the counter API.
 */
static uint32_t counter_esp32_get_top_value(const struct device *dev)
{
	const struct counter_esp32_config *cfg = dev->config;

	return cfg->counter_info.max_top_value;
}

static uint32_t counter_esp32_get_freq(const struct device *dev)
{
	struct counter_esp32_data *data = dev->data;

	return data->clk_src_freq;
}

static struct counter_esp32_data counter_data;

static const struct counter_esp32_config counter_config = {
	.counter_info = {
		.max_top_value = UINT32_MAX,
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,
		.channels = 1
	},
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.irq_source = DT_INST_IRQN(0),
};

static const struct counter_driver_api rtc_timer_esp32_api = {
	.start = counter_esp32_start,
	.stop = counter_esp32_stop,
	.get_value = counter_esp32_get_value,
	.set_alarm = counter_esp32_set_alarm,
	.cancel_alarm = counter_esp32_cancel_alarm,
	.set_top_value = counter_esp32_set_top_value,
	.get_pending_int = counter_esp32_get_pending_int,
	.get_top_value = counter_esp32_get_top_value,
	.get_freq = counter_esp32_get_freq,
};

static void counter_esp32_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct counter_esp32_data *data = dev->data;
	uint32_t now;

	counter_esp32_cancel_alarm(dev, 0);
	counter_esp32_get_value(dev, &now);

	if (data->alarm_cfg.callback) {
		data->alarm_cfg.callback(dev, 0, now, data->alarm_cfg.user_data);
	}

	/* RTC timer clear interrupt status */
	SET_PERI_REG_MASK(RTC_CNTL_INT_CLR_REG, RTC_CNTL_MAIN_TIMER_INT_CLR);
}

DEVICE_DT_INST_DEFINE(0,
		      &counter_esp32_init,
		      NULL,
		      &counter_data,
		      &counter_config,
		      POST_KERNEL,
		      CONFIG_COUNTER_INIT_PRIORITY,
		      &rtc_timer_esp32_api);
