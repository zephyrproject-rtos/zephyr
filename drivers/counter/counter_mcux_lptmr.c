/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#if DT_HAS_COMPAT_STATUS_OKAY(nxp_kinetis_lptmr)
#define DT_DRV_COMPAT nxp_kinetis_lptmr
#else
#define DT_DRV_COMPAT nxp_lptmr
#endif

#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <fsl_lptmr.h>

struct mcux_lptmr_config {
	struct counter_config_info info;
	LPTMR_Type *base;
	lptmr_prescaler_clock_select_t clk_source;
	lptmr_prescaler_glitch_value_t prescaler_glitch;
	bool bypass_prescaler_glitch;
	lptmr_timer_mode_t mode;
	lptmr_pin_select_t pin;
	lptmr_pin_polarity_t polarity;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_lptmr_data {
	counter_top_callback_t top_callback;
	void *top_user_data;
};

static int mcux_lptmr_start(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;

	LPTMR_EnableInterrupts(config->base,
			       kLPTMR_TimerInterruptEnable);
	LPTMR_StartTimer(config->base);

	return 0;
}

static int mcux_lptmr_stop(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;

	LPTMR_DisableInterrupts(config->base,
				kLPTMR_TimerInterruptEnable);
	LPTMR_StopTimer(config->base);

	return 0;
}

static int mcux_lptmr_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct mcux_lptmr_config *config = dev->config;

	*ticks = LPTMR_GetCurrentTimerCount(config->base);

	return 0;
}

static int mcux_lptmr_set_top_value(const struct device *dev,
				    const struct counter_top_cfg *cfg)
{
	const struct mcux_lptmr_config *config = dev->config;
	struct mcux_lptmr_data *data = dev->data;

	if (cfg->ticks == 0) {
		return -EINVAL;
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (config->base->CSR & LPTMR_CSR_TEN_MASK) {
		/* Timer already enabled, check flags before resetting */
		if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
			return -ENOTSUP;
		}
		LPTMR_StopTimer(config->base);
		LPTMR_SetTimerPeriod(config->base, cfg->ticks);
		LPTMR_StartTimer(config->base);
	} else {
		LPTMR_SetTimerPeriod(config->base, cfg->ticks);
	}

	return 0;
}

static uint32_t mcux_lptmr_get_pending_int(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;
	uint32_t mask = LPTMR_CSR_TCF_MASK | LPTMR_CSR_TIE_MASK;
	uint32_t flags;

	flags = LPTMR_GetStatusFlags(config->base);

	return ((flags & mask) == mask);
}

static uint32_t mcux_lptmr_get_top_value(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;

	return (config->base->CMR & LPTMR_CMR_COMPARE_MASK) + 1U;
}

static void mcux_lptmr_isr(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;
	struct mcux_lptmr_data *data = dev->data;
	uint32_t flags;

	flags = LPTMR_GetStatusFlags(config->base);
	LPTMR_ClearStatusFlags(config->base, flags);

	if (data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
}

static int mcux_lptmr_init(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;
	lptmr_config_t lptmr_config;

	LPTMR_GetDefaultConfig(&lptmr_config);
	lptmr_config.timerMode = config->mode;
	lptmr_config.enableFreeRunning = false;
	lptmr_config.prescalerClockSource = config->clk_source;
	lptmr_config.bypassPrescaler = config->bypass_prescaler_glitch;
	lptmr_config.value = config->prescaler_glitch;

	if (config->mode == kLPTMR_TimerModePulseCounter) {
		lptmr_config.pinSelect = config->pin;
		lptmr_config.pinPolarity = config->polarity;
	}

	LPTMR_Init(config->base, &lptmr_config);

	LPTMR_SetTimerPeriod(config->base, config->info.max_top_value);

	config->irq_config_func(dev);

	return 0;
}

static const struct counter_driver_api mcux_lptmr_driver_api = {
	.start = mcux_lptmr_start,
	.stop = mcux_lptmr_stop,
	.get_value = mcux_lptmr_get_value,
	.set_top_value = mcux_lptmr_set_top_value,
	.get_pending_int = mcux_lptmr_get_pending_int,
	.get_top_value = mcux_lptmr_get_top_value,
};

#define TO_LPTMR_CLK_SEL(val) _DO_CONCAT(kLPTMR_PrescalerClock_, val)
#define TO_LPTMR_PIN_SEL(val) _DO_CONCAT(kLPTMR_PinSelectInput_, val)

/* Prescaler mapping */
#define LPTMR_PRESCALER_2     kLPTMR_Prescale_Glitch_0
#define LPTMR_PRESCALER_4     kLPTMR_Prescale_Glitch_1
#define LPTMR_PRESCALER_8     kLPTMR_Prescale_Glitch_2
#define LPTMR_PRESCALER_16    kLPTMR_Prescale_Glitch_3
#define LPTMR_PRESCALER_32    kLPTMR_Prescale_Glitch_4
#define LPTMR_PRESCALER_64    kLPTMR_Prescale_Glitch_5
#define LPTMR_PRESCALER_128   kLPTMR_Prescale_Glitch_6
#define LPTMR_PRESCALER_256   kLPTMR_Prescale_Glitch_7
#define LPTMR_PRESCALER_512   kLPTMR_Prescale_Glitch_8
#define LPTMR_PRESCALER_1024  kLPTMR_Prescale_Glitch_9
#define LPTMR_PRESCALER_2048  kLPTMR_Prescale_Glitch_10
#define LPTMR_PRESCALER_4096  kLPTMR_Prescale_Glitch_11
#define LPTMR_PRESCALER_8192  kLPTMR_Prescale_Glitch_12
#define LPTMR_PRESCALER_16384 kLPTMR_Prescale_Glitch_13
#define LPTMR_PRESCALER_32768 kLPTMR_Prescale_Glitch_14
#define LPTMR_PRESCALER_65536 kLPTMR_Prescale_Glitch_15
#define TO_LPTMR_PRESCALER(val) _DO_CONCAT(LPTMR_PRESCALER_, val)

/* Glitch filter mapping */
#define LPTMR_GLITCH_2     kLPTMR_Prescale_Glitch_1
#define LPTMR_GLITCH_4     kLPTMR_Prescale_Glitch_2
#define LPTMR_GLITCH_8     kLPTMR_Prescale_Glitch_3
#define LPTMR_GLITCH_16    kLPTMR_Prescale_Glitch_4
#define LPTMR_GLITCH_32    kLPTMR_Prescale_Glitch_5
#define LPTMR_GLITCH_64    kLPTMR_Prescale_Glitch_6
#define LPTMR_GLITCH_128   kLPTMR_Prescale_Glitch_7
#define LPTMR_GLITCH_256   kLPTMR_Prescale_Glitch_8
#define LPTMR_GLITCH_512   kLPTMR_Prescale_Glitch_9
#define LPTMR_GLITCH_1024  kLPTMR_Prescale_Glitch_10
#define LPTMR_GLITCH_2048  kLPTMR_Prescale_Glitch_11
#define LPTMR_GLITCH_4096  kLPTMR_Prescale_Glitch_12
#define LPTMR_GLITCH_8192  kLPTMR_Prescale_Glitch_13
#define LPTMR_GLITCH_16384 kLPTMR_Prescale_Glitch_14
#define LPTMR_GLITCH_32768 kLPTMR_Prescale_Glitch_15
#define TO_LPTMR_GLITCH(val) _DO_CONCAT(LPTMR_GLITCH_, val)


#define COUNTER_MCUX_SET_INPUT_PIN_CONFIG_VALUES(n)				\
	.mode = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, input_pin),		\
			(kLPTMR_TimerModePulseCounter),				\
			(kLPTMR_TimerModeTimeCounter)),				\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, input_pin),			\
		(.pin = TO_LPTMR_PIN_SEL(DT_INST_PROP(n, input_pin)),),		\
		())								\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, input_pin),			\
		(.polarity = DT_INST_PROP(n, active_low),),			\
		())

/*
 * If prescaler value is set to 1, we treat it as a division of 1 otherwise
 * we set it to one of the enums above
 */
#define COUNTER_MCUX_SET_PRESCALER_GLITCH_VALUE(n)				\
	COND_CODE_1(DT_INST_PROP(n, prescaler),					\
			(1),							\
			(COND_CODE_1(DT_INST_PROP(				\
				n, counter_mode_enable),			\
			(.prescaler_glitch = TO_LPTMR_PRESCALER(		\
				DT_INST_PROP(n, prescaler))),			\
			(.prescaler_glitch = TO_LPTMR_GLITCH(			\
				DT_INST_PROP(n, prescaler)))))),


#define COUNTER_MCUX_LPTMR_DEVICE_INIT(n)					\
	static void mcux_lptmr_irq_config_##n(const struct device *dev)		\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			mcux_lptmr_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static struct mcux_lptmr_data mcux_lptmr_data_##n;			\
	static void mcux_lptmr_irq_config_##n(const struct device *dev);	\
	static struct mcux_lptmr_config mcux_lptmr_config_##n = {		\
		.info = {							\
			.max_top_value = ((DT_INST_PROP(n, resolution) == 32)	\
				? UINT32_MAX : UINT16_MAX),			\
			.freq = DT_INST_PROP(n, clock_frequency) /		\
				DT_INST_PROP(n, prescaler),			\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,			\
			.channels = 0,						\
		},								\
		.base = (LPTMR_Type *)DT_INST_REG_ADDR(n),			\
		.clk_source = TO_LPTMR_CLK_SEL(DT_INST_PROP(n, clk_source)),	\
		.bypass_prescaler_glitch = (bool)DT_INST_PROP(n,		\
				counter_mode_enable),				\
		COUNTER_MCUX_SET_PRESCALER_GLITCH_VALUE(n)			\
		COUNTER_MCUX_SET_INPUT_PIN_CONFIG_VALUES(n)			\
		.irq_config_func = mcux_lptmr_irq_config_##n,			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, &mcux_lptmr_init, NULL,			\
		&mcux_lptmr_data_##n,						\
		&mcux_lptmr_config_##n,						\
		POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,			\
		&mcux_lptmr_driver_api);


DT_INST_FOREACH_STATUS_OKAY(COUNTER_MCUX_LPTMR_DEVICE_INIT)
