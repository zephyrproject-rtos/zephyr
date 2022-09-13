/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_rtc_1khz

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>

#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

#include <fsl_rtc.h>

#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(mcux_rtc_1khz, CONFIG_COUNTER_LOG_LEVEL);

#define RTC_1KHZ_INIT_PRIORITY 51
#if CONFIG_COUNTER_INIT_PRIORITY >= RTC_1KHZ_INIT_PRIORITY
	#error "rtc init priority config error"
#endif

#define RTC_WAKEUP_FREQ	(1000)

/**
 * @brief NXP 1KHz high-resolution/wake-up RTC timer
 * 
 * Currently only outputs low frequency clocks for flexio.
 * Count and interrupts funciton may be added in the future.
 */

struct mcux_lpc_rtc_data {
	counter_alarm_callback_t alarm_callback;
	counter_top_callback_t top_callback;
	void *alarm_user_data;
	void *top_user_data;
};

struct mcux_lpc_rtc_config {
	struct counter_config_info info;
	RTC_Type *base;

#if defined(CONFIG_RTC_MCUX_OSC32K)
	const struct device *clock_dev;				/* used for enable 32K RTC clock */
	clock_control_subsys_t clock_subsys;		/* fixed value: MCUX_OSC32K_CLK */
#endif

#if defined(CONFIG_RTC_FLEXIO_OUTPUT_ENABLE)
	uint16_t output_freq;						/* the output low frequency of flexio */
#endif

#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif
};

static int mcux_lpc_rtc_start(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);

	RTC_EnableWakeupTimer(config->base, true);

	/* main rtc timer should be running, otherwise wakeup timer is inactive */
	if (!(config->base->CTRL & RTC_CTRL_RTC_EN_MASK)) {
		RTC_StartTimer(config->base);
	}

#if defined(CONFIG_RTC_MCUX_OSC32K)
	/* Enables the 32 kHz output of the RTC oscillator. 
	 * Otherwise there is no clock to the wakeup timer
	 */
	clock_control_on(config->clock_dev, config->clock_subsys);
#endif

	return 0;
}

static int mcux_lpc_rtc_stop(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);

	RTC_EnableWakeupTimer(config->base, false);

	/* disable the 32 kHz output of the RTC oscillator. There is no clock to the wakeup timer */
	clock_control_off(config->clock_dev, config->clock_subsys);

	return 0;
}

static int mcux_lpc_rtc_init(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);

	/* RTC has been initialized in 1Hz main RTC timer */

	/* Support output 32KHZ_CLKOUT / CLKOUT / LOW_FREQ_CLKOUT / LOW_FREQ_CLKOUT_N */
#ifdef CONFIG_PINCTRL
	int err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}
#endif

#if defined(CONFIG_RTC_MCUX_OSC32K)
	/* Enables the 32 kHz output of the RTC oscillator.
	 * Otherwise there is no clock to the wakeup timer
	 */
	clock_control_on(config->clock_dev, config->clock_subsys);
#endif

#if defined(CONFIG_RTC_FLEXIO_OUTPUT_ENABLE)
	/* If it is flexio low frequency output, config low frequency clock divide */
	uint8_t divide = (config->info.freq % config->output_freq) ? ((config->info.freq / config->output_freq) + 1) :
					(config->info.freq / config->output_freq);

	clock_control_configure(config->clock_dev, config->clock_subsys, &divide);
#endif

	/* When warm reset, enable bit is not cleared. So disable 1kHz timer first! */
	mcux_lpc_rtc_stop(dev);

	return 0;
}

static const struct counter_driver_api mcux_rtc_driver_api = {
	.start = mcux_lpc_rtc_start,
	.stop = mcux_lpc_rtc_stop,
	.get_value = NULL,
	.set_alarm = NULL,
	.cancel_alarm = NULL,
	.set_top_value = NULL,
	.get_pending_int = NULL,
	.get_top_value = NULL,
};


/* low freq output pin config */
#ifdef CONFIG_PINCTRL
#define RTC_PINCTRL_DEFINE(id) PINCTRL_DT_INST_DEFINE(id);
#define RTC_PINCTRL_INIT(id) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),
#else
#define RTC_PINCTRL_DEFINE(id)
#define RTC_PINCTRL_INIT(id)
#endif


#if defined(CONFIG_RTC_MCUX_OSC32K)
	#define RTC_CLOCK_CONFIG(id) 											\
			.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),			\
			.clock_subsys =													\
					(clock_control_subsys_t)DT_INST_CLOCKS_CELL(id, name),
#else
	#define RTC_CLOCK_CONFIG(id) 
#endif


#if defined(CONFIG_RTC_FLEXIO_OUTPUT_ENABLE)
	#define RTC_FLEXIO_LOW_FREQ_CONFIG		\
			.output_freq = CONFIG_RTC_FLEXIO_OUTPUT_FREQ,
#else
	#define RTC_FLEXIO_LOW_FREQ_CONFIG
#endif

static struct mcux_lpc_rtc_data mcux_lpc_rtc_data;

#define COUNTER_LPC_RTC_DEVICE(id)										\
	RTC_PINCTRL_DEFINE(id)												\
	static const struct mcux_lpc_rtc_config mcux_lpc_rtc_config##id = { \
		.base = (RTC_Type *)DT_REG_ADDR(DT_PARENT(DT_DRV_INST(id))),						\
		.info = {														\
			.max_top_value = UINT16_MAX,								\
			.freq = RTC_WAKEUP_FREQ,									\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,						\
			.channels = 1,												\
		},																\
		RTC_CLOCK_CONFIG(id)											\
		RTC_FLEXIO_LOW_FREQ_CONFIG										\
		RTC_PINCTRL_INIT(id)											\
	};																	\
	DEVICE_DT_INST_DEFINE(id, mcux_lpc_rtc_init,						\
			NULL, &mcux_lpc_rtc_data, &mcux_lpc_rtc_config##id,			\
			POST_KERNEL, RTC_1KHZ_INIT_PRIORITY,						\
			&mcux_rtc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_LPC_RTC_DEVICE)
