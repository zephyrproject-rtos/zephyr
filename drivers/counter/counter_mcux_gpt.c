/*
 * Copyright (c) 2019, Linaro Limited.
 * Copyright 2024-2025 NXP
 * Copyright 2026 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_gpt

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <fsl_gpt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>

#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

LOG_MODULE_REGISTER(mcux_gpt, CONFIG_COUNTER_LOG_LEVEL);

#ifdef CONFIG_COUNTER_CAPTURE
#define IMX_GPT_N_CAPTURE_CHANNELS 2
#else
#define IMX_GPT_N_CAPTURE_CHANNELS 1
#endif

#define IMX_GPT_N_CHANNELS IMX_GPT_N_CAPTURE_CHANNELS

#define DEV_CFG(_dev) ((const struct mcux_gpt_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mcux_gpt_data *)(_dev)->data)

struct mcux_gpt_config {
	/* info must be first element */
	struct counter_config_info info;

	DEVICE_MMIO_NAMED_ROM(gpt_mmio);

	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	bool enable_free_run;
	gpt_clock_source_t clock_source;
	uint8_t osc_divider;
#ifdef CONFIG_COUNTER_64BITS_FREQ
	uint64_t high_freq;
	uint64_t low_freq;
	uint64_t osc_freq;
#else
	uint32_t high_freq;
	uint32_t low_freq;
	uint32_t osc_freq;
#endif
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif
	void (*irq_config_func)(void);
};

struct mcux_gpt_data {
	DEVICE_MMIO_NAMED_RAM(gpt_mmio);
	counter_alarm_callback_t alarm_callback;
	counter_top_callback_t top_callback;
	void *alarm_user_data;
	void *top_user_data;
#ifdef CONFIG_COUNTER_CAPTURE
	counter_capture_cb_t capture_callback[IMX_GPT_N_CAPTURE_CHANNELS];
	void *capture_user_data[IMX_GPT_N_CAPTURE_CHANNELS];
	gpt_input_operation_mode_t capture_mode[IMX_GPT_N_CAPTURE_CHANNELS];
	bool capture_single_shot[IMX_GPT_N_CAPTURE_CHANNELS];
#endif
};

#ifdef CONFIG_COUNTER_CAPTURE
static const gpt_input_capture_channel_t capture_ch[IMX_GPT_N_CAPTURE_CHANNELS] = {
	kGPT_InputCapture_Channel1, /* chan_id=0 → CAPTURE1 */
	kGPT_InputCapture_Channel2, /* chan_id=1 → CAPTURE2 */
};
static const uint32_t capture_irq[IMX_GPT_N_CAPTURE_CHANNELS] = {
	kGPT_InputCapture1InterruptEnable,
	kGPT_InputCapture2InterruptEnable,
};
static const uint32_t capture_flag[IMX_GPT_N_CAPTURE_CHANNELS] = {
	kGPT_InputCapture1Flag,
	kGPT_InputCapture2Flag,
};
#endif /* CONFIG_COUNTER_CAPTURE */

static GPT_Type *get_base(const struct device *dev)
{
	return (GPT_Type *)DEVICE_MMIO_NAMED_GET(dev, gpt_mmio);
}

static int mcux_gpt_start(const struct device *dev)
{
	GPT_Type *base = get_base(dev);

	GPT_StartTimer(base);

	return 0;
}

static int mcux_gpt_stop(const struct device *dev)
{
	GPT_Type *base = get_base(dev);

	GPT_StopTimer(base);

	return 0;
}

static int mcux_gpt_get_value(const struct device *dev, uint32_t *ticks)
{
	GPT_Type *base = get_base(dev);

	*ticks = GPT_GetCurrentTimerCount(base);
	return 0;
}

static int mcux_gpt_set_alarm(const struct device *dev, uint8_t chan_id,
			      const struct counter_alarm_cfg *alarm_cfg)
{
	GPT_Type *base = get_base(dev);
	struct mcux_gpt_data *data = dev->data;

	uint32_t current = GPT_GetCurrentTimerCount(base);
	uint32_t ticks = alarm_cfg->ticks;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		ticks += current;
	}

	if (data->alarm_callback) {
		return -EBUSY;
	}

	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;

	GPT_SetOutputCompareValue(base, kGPT_OutputCompare_Channel1,
				  ticks);
	GPT_EnableInterrupts(base, kGPT_OutputCompare1InterruptEnable);

	return 0;
}

static int mcux_gpt_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	GPT_Type *base = get_base(dev);
	struct mcux_gpt_data *data = dev->data;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	GPT_DisableInterrupts(base, kGPT_OutputCompare1InterruptEnable);
	data->alarm_callback = NULL;

	return 0;
}

void mcux_gpt_isr(const struct device *dev)
{
	GPT_Type *base = get_base(dev);
	struct mcux_gpt_data *data = dev->data;
	uint32_t current = GPT_GetCurrentTimerCount(base);
	uint32_t status;

	status =  GPT_GetStatusFlags(base, kGPT_OutputCompare1Flag |
#ifdef CONFIG_COUNTER_CAPTURE
				     kGPT_InputCapture1Flag | kGPT_InputCapture2Flag |
#endif
				     kGPT_RollOverFlag);
	GPT_ClearStatusFlags(base, status);
	barrier_dsync_fence_full();

	if ((status & kGPT_OutputCompare1Flag) && data->alarm_callback) {
		GPT_DisableInterrupts(base,
				      kGPT_OutputCompare1InterruptEnable);
		counter_alarm_callback_t alarm_cb = data->alarm_callback;
		data->alarm_callback = NULL;
		alarm_cb(dev, 0, current, data->alarm_user_data);
	}

	if ((status & kGPT_RollOverFlag) && data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}

#ifdef CONFIG_COUNTER_CAPTURE
	for (uint8_t i = 0; i < ARRAY_SIZE(capture_ch); i++) {
		counter_capture_flags_t cflags;
		counter_capture_cb_t cb;
		bool single_shot;
		uint32_t ticks;

		if ((status & capture_flag[i]) != capture_flag[i]) {
			continue;
		}

		/* Read ICR before potentially disabling (ICR holds until next capture) */
		ticks = GPT_GetInputCaptureValue(base, capture_ch[i]);
		cb = data->capture_callback[i];
		single_shot = data->capture_single_shot[i];
		cflags = single_shot ? COUNTER_CAPTURE_SINGLE_SHOT : COUNTER_CAPTURE_CONTINUOUS;

		if (single_shot) {
			GPT_DisableInterrupts(base, capture_irq[i]);
			GPT_SetInputOperationMode(base, capture_ch[i],
						  kGPT_InputOperation_Disabled);
			data->capture_callback[i] = NULL;
		}

		if (cb != NULL) {
			cb(dev, i, cflags, ticks, data->capture_user_data[i]);
		}
	}
#endif /* CONFIG_COUNTER_CAPTURE */
}

static uint32_t mcux_gpt_get_pending_int(const struct device *dev)
{
	GPT_Type *base = get_base(dev);
	uint32_t flags = kGPT_OutputCompare1Flag;

#ifdef CONFIG_COUNTER_CAPTURE
	flags |= kGPT_InputCapture1Flag | kGPT_InputCapture2Flag;
#endif

	return GPT_GetStatusFlags(base, flags);
}

static int mcux_gpt_set_top_value(const struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	const struct mcux_gpt_config *config = dev->config;
	GPT_Type *base = get_base(dev);
	struct mcux_gpt_data *data = dev->data;

	if (cfg->ticks != config->info.max_top_value) {
		LOG_ERR("Wrap can only be set to 0x%x",
			config->info.max_top_value);
		return -ENOTSUP;
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	GPT_EnableInterrupts(base, kGPT_RollOverFlagInterruptEnable);

	return 0;
}

static uint32_t mcux_gpt_get_top_value(const struct device *dev)
{
	const struct mcux_gpt_config *config = dev->config;

	return config->info.max_top_value;
}

#ifdef CONFIG_COUNTER_CAPTURE
static int mcux_gpt_capture_configure(const struct device *dev, uint8_t chan_id,
				      counter_capture_flags_t flags, counter_capture_cb_t cb,
				      void *user_data)
{
	GPT_Type *base = get_base(dev);
	struct mcux_gpt_data *data = dev->data;
	gpt_input_operation_mode_t mode;

	if (chan_id >= ARRAY_SIZE(capture_ch)) {
		return -EINVAL;
	}
	if (cb == NULL) {
		return -EINVAL;
	}
	if (GPT_GetInputOperationMode(base, capture_ch[chan_id]) != kGPT_InputOperation_Disabled) {
		LOG_ERR("Invalid operation mode");
		return -EBUSY;
	}

	if ((flags & COUNTER_CAPTURE_BOTH_EDGES) == COUNTER_CAPTURE_BOTH_EDGES) {
		mode = kGPT_InputOperation_BothEdge;
	} else if ((flags & COUNTER_CAPTURE_FALLING_EDGE) == COUNTER_CAPTURE_FALLING_EDGE) {
		mode = kGPT_InputOperation_FallEdge;
	} else if ((flags & COUNTER_CAPTURE_RISING_EDGE) == COUNTER_CAPTURE_RISING_EDGE) {
		mode = kGPT_InputOperation_RiseEdge;
	} else {
		return -EINVAL;
	}

	data->capture_callback[chan_id] = cb;
	data->capture_user_data[chan_id] = user_data;
	data->capture_mode[chan_id] = mode;
	data->capture_single_shot[chan_id] = (flags & COUNTER_CAPTURE_SINGLE_SHOT) != 0;

	return 0;
}

static int mcux_gpt_enable_capture(const struct device *dev, uint8_t chan_id)
{
	GPT_Type *base = get_base(dev);
	struct mcux_gpt_data *data = dev->data;

	if (chan_id >= ARRAY_SIZE(capture_ch)) {
		return -EINVAL;
	}
	if (data->capture_callback[chan_id] == NULL) {
		return -EINVAL;
	}

	GPT_SetInputOperationMode(base, capture_ch[chan_id], data->capture_mode[chan_id]);
	GPT_EnableInterrupts(base, capture_irq[chan_id]);

	return 0;
}

static int mcux_gpt_disable_capture(const struct device *dev, uint8_t chan_id)
{
	GPT_Type *base = get_base(dev);
	struct mcux_gpt_data *data = dev->data;

	if (chan_id >= ARRAY_SIZE(capture_ch)) {
		return -EINVAL;
	}

	GPT_DisableInterrupts(base, capture_irq[chan_id]);
	GPT_SetInputOperationMode(base, capture_ch[chan_id], kGPT_InputOperation_Disabled);
	data->capture_callback[chan_id] = NULL;

	return 0;
}
#endif /* CONFIG_COUNTER_CAPTURE */

static int mcux_gpt_reset(const struct device *dev)
{
	GPT_Type *base = get_base(dev);
	bool was_free_run = (base->CR & GPT_CR_FRR_MASK) != 0;
	uint32_t saved_ir;
	bool of1_before;

	if (was_free_run) {
		/* Snapshot the OF1 state. */
		of1_before = (GPT_GetStatusFlags(base, kGPT_OutputCompare1Flag) != 0U);
		/* Save and disable all GPT interrupts to guard against
		 * a spurious OCR1 match during the Restart mode window.
		 */
		saved_ir = base->IR;
		base->IR = 0U;
		/* Switch to Restart mode (FRR=0) to enable the OCR1
		 * write-reset mechanism.
		 */
		base->CR &= ~GPT_CR_FRR_MASK;
	}

	/* The GPT hardware resets CNT to 0 on any write to OCR1
	 * when operating in Restart mode.
	 */
	base->OCR[0] = base->OCR[0];

	if (was_free_run) {
		/* Restore Free-Run mode. */
		base->CR |= GPT_CR_FRR_MASK;

		/* If OF1 was not set before but is set now, clear it
		 * so it does not fire as a false alarm when IR is restored.
		 */
		if (!of1_before &&
		    (GPT_GetStatusFlags(base, kGPT_OutputCompare1Flag) != 0U)) {
			GPT_ClearStatusFlags(base, kGPT_OutputCompare1Flag);
		}

		/* Restore interrupts. */
		base->IR = saved_ir;
	}

	return 0;
}

static int mcux_gpt_init(const struct device *dev)
{
	const struct mcux_gpt_config *config = dev->config;
	gpt_config_t gptConfig;
	uint32_t clock_freq;
	GPT_Type *base;

	DEVICE_MMIO_NAMED_MAP(dev, gpt_mmio, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

#ifdef CONFIG_PINCTRL
	{
		int ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);

		if (ret < 0 && ret != -ENOENT) {
			return ret;
		}
	}
#endif

	GPT_GetDefaultConfig(&gptConfig);
	gptConfig.enableFreeRun = config->enable_free_run;
	gptConfig.clockSource = config->clock_source;

	switch (config->clock_source) {
	case kGPT_ClockSource_Off:
		gptConfig.divider = 1U;
		break;
	case kGPT_ClockSource_Ext:
		/* External clock: frequency unknown to CCM; prescaler fixed at 1 */
		gptConfig.divider = 1U;
		break;
	case kGPT_ClockSource_HighFreq:
	case kGPT_ClockSource_LowFreq: {
		uint32_t ref_freq = (config->clock_source == kGPT_ClockSource_HighFreq)
					    ? config->high_freq
					    : config->low_freq;

		if (ref_freq == 0 || config->info.freq == 0 ||
		    (ref_freq % config->info.freq != 0)) {
			LOG_ERR("Cannot adjust GPT freq to %u", config->info.freq);
			return -EINVAL;
		}

		gptConfig.divider = ref_freq / config->info.freq;
		break;
	}
	case kGPT_ClockSource_Osc:
		uint32_t osc_in;

		if (config->osc_freq == 0) {
			LOG_ERR("kGPT_ClockSource_Osc is selected but osc_freq is 0");
			return -EINVAL;
		}
		osc_in = config->osc_freq / config->osc_divider;

		if (config->info.freq == 0 || (osc_in % config->info.freq != 0)) {
			LOG_ERR("Cannot adjust GPT freq to %u with OSC clock",
				config->info.freq);
			return -EINVAL;
		}
		gptConfig.divider = osc_in / config->info.freq;

		break;
	default:
		/* Internal clock sources (periph): query CCM */
		if (!device_is_ready(config->clock_dev)) {
			LOG_ERR("clock control device not ready");
			return -ENODEV;
		}
		if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
					   &clock_freq)) {
			return -EINVAL;
		}
		if (config->info.freq == 0 || (clock_freq % config->info.freq != 0)) {
			LOG_ERR("Cannot adjust GPT freq to %u", config->info.freq);
			return -EINVAL;
		}
		gptConfig.divider = clock_freq / config->info.freq;
		break;
	}

	base = get_base(dev);
	GPT_Init(base, &gptConfig);

	/* Set oscillator prescaler AFTER GPT_Init (SWR resets PRESCALER24M) */
	if (config->clock_source == kGPT_ClockSource_Osc) {
		GPT_SetOscClockDivider(base, config->osc_divider);
	}

	config->irq_config_func();

	return 0;
}

static DEVICE_API(counter, mcux_gpt_driver_api) = {
	.start = mcux_gpt_start,
	.stop = mcux_gpt_stop,
	.get_value = mcux_gpt_get_value,
	.set_alarm = mcux_gpt_set_alarm,
	.cancel_alarm = mcux_gpt_cancel_alarm,
	.set_top_value = mcux_gpt_set_top_value,
	.get_pending_int = mcux_gpt_get_pending_int,
	.get_top_value = mcux_gpt_get_top_value,
	.reset = mcux_gpt_reset,
#ifdef CONFIG_COUNTER_CAPTURE
	.capture_configure = mcux_gpt_capture_configure,
	.enable_capture = mcux_gpt_enable_capture,
	.disable_capture = mcux_gpt_disable_capture,
#endif
};

#ifdef CONFIG_PINCTRL
#define MCUX_GPT_PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n)
#define MCUX_GPT_PINCTRL_INIT(n)   .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#else
#define MCUX_GPT_PINCTRL_DEFINE(n)
#define MCUX_GPT_PINCTRL_INIT(n)
#endif

#define GPT_DEVICE_INIT_MCUX(n)                                                                    \
	MCUX_GPT_PINCTRL_DEFINE(n);                                                                \
	static struct mcux_gpt_data mcux_gpt_data_##n;                                             \
	static void mcux_gpt_irq_config_##n(void);                                                 \
                                                                                                   \
	static const struct mcux_gpt_config mcux_gpt_config_##n = {                                \
		DEVICE_MMIO_NAMED_ROM_INIT(gpt_mmio, DT_DRV_INST(n)),                              \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.enable_free_run = (DT_INST_ENUM_IDX_OR(n, run_mode, 0) == 1),                     \
		.clock_source = (gpt_clock_source_t)DT_INST_ENUM_IDX_OR(n, clock_source, 1),       \
		.high_freq = DT_INST_PROP_OR(n, high_freq, 0),                                     \
		.low_freq = DT_INST_PROP_OR(n, low_freq, 0),                                       \
		.osc_freq = DT_INST_PROP_OR(n, osc_freq, 0),                                       \
		.osc_divider = DT_INST_PROP_OR(n, osc_divider, 1),                                 \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = UINT32_MAX,                                       \
				.freq = DT_INST_PROP_OR(n, gptfreq, 0),                            \
				.channels = IMX_GPT_N_CHANNELS,                                    \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
			},                                                                         \
		.irq_config_func = mcux_gpt_irq_config_##n,                                        \
		MCUX_GPT_PINCTRL_INIT(n)};                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mcux_gpt_init, NULL, &mcux_gpt_data_##n, &mcux_gpt_config_##n,    \
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY, &mcux_gpt_driver_api);    \
                                                                                                   \
	static void mcux_gpt_irq_config_##n(void)                                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mcux_gpt_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

DT_INST_FOREACH_STATUS_OKAY(GPT_DEVICE_INIT_MCUX)
