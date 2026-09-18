/*
 * Copyright 2023-2024, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_tpm_timer

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#ifdef CONFIG_COUNTER_CAPTURE
#include <zephyr/drivers/pinctrl.h>
#endif /* CONFIG_COUNTER_CAPTURE */
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>

#include <fsl_tpm.h>

LOG_MODULE_REGISTER(mcux_tpm, CONFIG_COUNTER_LOG_LEVEL);

#ifdef CONFIG_COUNTER_CAPTURE
#define TPM_CAPTURE_VALID_FLAGS (COUNTER_CAPTURE_BOTH_EDGES | COUNTER_CAPTURE_SINGLE_SHOT)
#endif /* CONFIG_COUNTER_CAPTURE */

struct mcux_tpm_channel_data {
	counter_alarm_callback_t alarm_callback;
	void *alarm_user_data;
#ifdef CONFIG_COUNTER_CAPTURE
	counter_capture_cb_t capture_callback;
	void *capture_user_data;
	tpm_input_capture_edge_t capture_edge;
	counter_capture_flags_t capture_flags;
	bool capture_single_shot;
#endif /* CONFIG_COUNTER_CAPTURE */
};

#define DEV_CFG(_dev) ((const struct mcux_tpm_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mcux_tpm_data *)(_dev)->data)

struct mcux_tpm_config {
	struct counter_config_info info;

	DEVICE_MMIO_NAMED_ROM(tpm_mmio);

	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;

	tpm_clock_source_t tpm_clock_source;
	tpm_clock_prescale_t prescale;
#ifdef CONFIG_COUNTER_CAPTURE
	const struct pinctrl_dev_config *pincfg;
#endif /* CONFIG_COUNTER_CAPTURE */
	void (*irq_config_func)(void);
};

struct mcux_tpm_data {
	DEVICE_MMIO_NAMED_RAM(tpm_mmio);
	counter_top_callback_t top_callback;
	uint32_t freq;
	struct mcux_tpm_channel_data channels[TPM_CONTROLS_COUNT];
	void *top_user_data;
};

static TPM_Type *get_base(const struct device *dev)
{
	return (TPM_Type *)DEVICE_MMIO_NAMED_GET(dev, tpm_mmio);
}

static int mcux_tpm_start(const struct device *dev)
{
	const struct mcux_tpm_config *config = dev->config;
	TPM_Type *base = get_base(dev);

	TPM_StartTimer(base, config->tpm_clock_source);

	return 0;
}

static int mcux_tpm_stop(const struct device *dev)
{
	TPM_Type *base = get_base(dev);

	TPM_StopTimer(base);

	return 0;
}

static int mcux_tpm_get_value(const struct device *dev, uint32_t *ticks)
{
	TPM_Type *base = get_base(dev);

	*ticks = TPM_GetCurrentTimerCount(base);

	return 0;
}

static int mcux_tpm_reset(const struct device *dev)
{
	TPM_Type *base = get_base(dev);

	/* Writing any value to CNT resets the counter to its initial value. */
	base->CNT = 0;

	return 0;
}

static int mcux_tpm_set_alarm(const struct device *dev, uint8_t chan_id,
			      const struct counter_alarm_cfg *alarm_cfg)
{
	TPM_Type *base = get_base(dev);
	uint32_t current = TPM_GetCurrentTimerCount(base);
	uint32_t top_value = base->MOD;
	struct mcux_tpm_data *data = dev->data;
	uint32_t ticks = alarm_cfg->ticks;

	if (chan_id >= DEV_CFG(dev)->info.channels) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	if (data->channels[chan_id].alarm_callback != NULL) {
		LOG_ERR("channel already in use");
		return -EBUSY;
	}

#ifdef CONFIG_COUNTER_CAPTURE
	if (data->channels[chan_id].capture_callback != NULL) {
		LOG_ERR("channel already configured for capture");
		return -EBUSY;
	}
#endif /* CONFIG_COUNTER_CAPTURE */

	if (ticks > (top_value)) {
		return -EINVAL;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		if (top_value - current >= ticks) {
			ticks += current;
		} else {
			ticks -= top_value - current;
		}
	}

	data->channels[chan_id].alarm_callback = alarm_cfg->callback;
	data->channels[chan_id].alarm_user_data = alarm_cfg->user_data;

	TPM_SetupOutputCompare(base, chan_id, kTPM_NoOutputSignal, ticks);
	TPM_EnableInterrupts(base, BIT(chan_id));

	return 0;
}

static int mcux_tpm_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	TPM_Type *base = get_base(dev);
	struct mcux_tpm_data *data = dev->data;

	if (chan_id >= DEV_CFG(dev)->info.channels) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	TPM_DisableInterrupts(base, BIT(chan_id));
	data->channels[chan_id].alarm_callback = NULL;
	data->channels[chan_id].alarm_user_data = NULL;

	return 0;
}

#ifdef CONFIG_COUNTER_CAPTURE
static bool mcux_tpm_capture_is_enabled(TPM_Type *base, uint8_t chan_id)
{
	uint32_t cnsc = base->CONTROLS[chan_id].CnSC;

	/*
	 * Input capture is selected when MSnB:MSnA is 00 and ELSnB:ELSnA is not 00,
	 * so the channel mode bits are the only state needed to tell whether capture
	 * is currently armed in hardware.
	 */
	return ((cnsc & (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK)) == 0U) &&
	       ((cnsc & (TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)) != 0U);
}

static int mcux_tpm_capture_edge(counter_capture_flags_t flags, tpm_input_capture_edge_t *edge)
{
	if ((flags & ~TPM_CAPTURE_VALID_FLAGS) != 0U) {
		return -EINVAL;
	}

	if ((flags & COUNTER_CAPTURE_BOTH_EDGES) == COUNTER_CAPTURE_BOTH_EDGES) {
		*edge = kTPM_RiseAndFallEdge;
	} else if ((flags & COUNTER_CAPTURE_FALLING_EDGE) != 0U) {
		*edge = kTPM_FallingEdge;
	} else if ((flags & COUNTER_CAPTURE_RISING_EDGE) != 0U) {
		*edge = kTPM_RisingEdge;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int mcux_tpm_capture_configure(const struct device *dev, uint8_t chan_id,
				      counter_capture_flags_t flags, counter_capture_cb_t cb,
				      void *user_data)
{
	struct mcux_tpm_data *data = dev->data;
	tpm_input_capture_edge_t edge;
	int ret;

	if (chan_id >= DEV_CFG(dev)->info.channels) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	if (cb == NULL) {
		return -EINVAL;
	}

	if (data->channels[chan_id].alarm_callback != NULL) {
		LOG_ERR("channel %u already configured for alarm", chan_id);
		return -EBUSY;
	}

	if (mcux_tpm_capture_is_enabled(get_base(dev), chan_id)) {
		LOG_ERR("capture channel %u is enabled", chan_id);
		return -EBUSY;
	}

	ret = mcux_tpm_capture_edge(flags, &edge);
	if (ret != 0) {
		return ret;
	}

	data->channels[chan_id].capture_callback = cb;
	data->channels[chan_id].capture_user_data = user_data;
	data->channels[chan_id].capture_edge = edge;
	data->channels[chan_id].capture_flags = flags & COUNTER_CAPTURE_BOTH_EDGES;
	data->channels[chan_id].capture_single_shot = (flags & COUNTER_CAPTURE_SINGLE_SHOT) != 0U;

	return 0;
}

static int mcux_tpm_enable_capture(const struct device *dev, uint8_t chan_id)
{
	TPM_Type *base = get_base(dev);
	struct mcux_tpm_data *data = dev->data;
	struct mcux_tpm_channel_data *channel;

	if (chan_id >= DEV_CFG(dev)->info.channels) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	channel = &data->channels[chan_id];
	if (channel->alarm_callback != NULL) {
		LOG_ERR("channel %u already configured for alarm", chan_id);
		return -EBUSY;
	}

	if (channel->capture_callback == NULL) {
		LOG_ERR("capture callback not configured for channel %u", chan_id);
		return -EINVAL;
	}

	if (mcux_tpm_capture_is_enabled(base, chan_id)) {
		return -EBUSY;
	}

	TPM_ClearStatusFlags(base, BIT(chan_id));
	TPM_SetupInputCapture(base, (tpm_chnl_t)chan_id, channel->capture_edge);
	TPM_EnableInterrupts(base, BIT(chan_id));

	return 0;
}

static int mcux_tpm_disable_capture(const struct device *dev, uint8_t chan_id)
{
	TPM_Type *base = get_base(dev);
	struct mcux_tpm_data *data = dev->data;

	if (chan_id >= DEV_CFG(dev)->info.channels) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	TPM_DisableInterrupts(base, BIT(chan_id));
	(void)TPM_DisableChannel(base, (tpm_chnl_t)chan_id);
	TPM_ClearStatusFlags(base, BIT(chan_id));

	data->channels[chan_id].capture_callback = NULL;
	data->channels[chan_id].capture_user_data = NULL;
	data->channels[chan_id].capture_flags = 0U;
	data->channels[chan_id].capture_single_shot = false;

	return 0;
}
#endif /* CONFIG_COUNTER_CAPTURE */

void mcux_tpm_isr(const struct device *dev)
{
	TPM_Type *base = get_base(dev);
	struct mcux_tpm_data *data = dev->data;
	uint32_t current = TPM_GetCurrentTimerCount(base);
	uint32_t status;
#ifdef CONFIG_COUNTER_CAPTURE
	uint32_t capture_ticks[TPM_CONTROLS_COUNT] = {0};
#endif /* CONFIG_COUNTER_CAPTURE */

	status = TPM_GetStatusFlags(base);

#ifdef CONFIG_COUNTER_CAPTURE
	/*
	 * Sample CnV before CHF is cleared. Every selected edge latches the counter
	 * into CnV, and the reference manual only guarantees that a CHF interrupt is
	 * not lost across the clearing sequence, not that CnV still holds the value
	 * this interrupt was raised for.
	 */
	for (uint8_t chan = 0; chan < DEV_CFG(dev)->info.channels; chan++) {
		if ((status & BIT(chan)) != 0U &&
		    (data->channels[chan].capture_callback != NULL)) {
			capture_ticks[chan] = TPM_GetChannelValue(base, (tpm_chnl_t)chan);
		}
	}
#endif /* CONFIG_COUNTER_CAPTURE */

	TPM_ClearStatusFlags(base, status);
	barrier_dsync_fence_full();

	for (uint8_t chan = 0; chan < DEV_CFG(dev)->info.channels; chan++) {
		if ((status & BIT(chan)) != 0 && (data->channels[chan].alarm_callback != NULL)) {
			counter_alarm_callback_t alarm_callback =
				data->channels[chan].alarm_callback;
			void *alarm_user_data = data->channels[chan].alarm_user_data;

			data->channels[chan].alarm_callback = NULL;
			data->channels[chan].alarm_user_data = NULL;
			alarm_callback(dev, chan, current, alarm_user_data);
		}
	}

#ifdef CONFIG_COUNTER_CAPTURE
	for (uint8_t chan = 0; chan < DEV_CFG(dev)->info.channels; chan++) {
		counter_capture_cb_t capture_callback;
		counter_capture_flags_t capture_flags;
		void *capture_user_data;

		if ((status & BIT(chan)) == 0U) {
			continue;
		}

		capture_callback = data->channels[chan].capture_callback;
		if (capture_callback == NULL) {
			continue;
		}

		capture_user_data = data->channels[chan].capture_user_data;
		capture_flags = data->channels[chan].capture_flags;

		if (data->channels[chan].capture_single_shot) {
			capture_flags |= COUNTER_CAPTURE_SINGLE_SHOT;
			(void)mcux_tpm_disable_capture(dev, chan);
		} else {
			capture_flags |= COUNTER_CAPTURE_CONTINUOUS;
		}

		capture_callback(dev, chan, capture_flags, capture_ticks[chan], capture_user_data);
	}
#endif /* CONFIG_COUNTER_CAPTURE */

	if ((status & kTPM_TimeOverflowFlag) && data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
}

static uint32_t mcux_tpm_get_pending_int(const struct device *dev)
{
	TPM_Type *base = get_base(dev);

	return TPM_GetStatusFlags(base) ? 1 : 0;
}

static int mcux_tpm_set_top_value(const struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	const struct mcux_tpm_config *config = dev->config;
	TPM_Type *base = get_base(dev);
	struct mcux_tpm_data *data = dev->data;

	for (uint8_t chan = 0; chan < config->info.channels; chan++) {
		if (data->channels[chan].alarm_callback) {
			return -EBUSY;
		}
	}

	/* Check if timer already enabled. */
#if defined(FSL_FEATURE_TPM_HAS_SC_CLKS) && FSL_FEATURE_TPM_HAS_SC_CLKS
	if (base->SC & TPM_SC_CLKS_MASK) {
#else
	if (base->SC & TPM_SC_CMOD_MASK) {
#endif
		/* Timer already enabled, check flags before resetting */
		if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
			return -ENOTSUP;
		}

		TPM_StopTimer(base);
		base->CNT = 0;
		TPM_SetTimerPeriod(base, cfg->ticks);
		TPM_StartTimer(base, config->tpm_clock_source);
	} else {
		base->CNT = 0;
		TPM_SetTimerPeriod(base, cfg->ticks);
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	TPM_EnableInterrupts(base, kTPM_TimeOverflowInterruptEnable);

	return 0;
}

static uint32_t mcux_tpm_get_top_value(const struct device *dev)
{
	TPM_Type *base = get_base(dev);

	return base->MOD;
}

static uint32_t mcux_tpm_get_freq(const struct device *dev)
{
	struct mcux_tpm_data *data = dev->data;

	return data->freq;
}

static int mcux_tpm_init(const struct device *dev)
{
	const struct mcux_tpm_config *config = dev->config;
	struct mcux_tpm_data *data = dev->data;
	tpm_config_t tpmConfig;
	uint32_t input_clock_freq;
	TPM_Type *base;

	DEVICE_MMIO_NAMED_MAP(dev, tpm_mmio, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

#ifdef CONFIG_COUNTER_CAPTURE
	if (config->pincfg != NULL) {
		int pinctrl_err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);

		if (pinctrl_err != 0) {
			return pinctrl_err;
		}
	}
#endif /* CONFIG_COUNTER_CAPTURE */

	for (uint8_t chan = 0; chan < DEV_CFG(dev)->info.channels; chan++) {
		data->channels[chan].alarm_callback = NULL;
		data->channels[chan].alarm_user_data = NULL;
#ifdef CONFIG_COUNTER_CAPTURE
		data->channels[chan].capture_callback = NULL;
		data->channels[chan].capture_user_data = NULL;
		data->channels[chan].capture_flags = 0U;
		data->channels[chan].capture_single_shot = false;
#endif /* CONFIG_COUNTER_CAPTURE */
	}

	int err = clock_control_configure(config->clock_dev, config->clock_subsys, NULL);

	if (err) {
		/* Check if error is due to lack of support */
		if (err != -ENOSYS) {
			/* Real error occurred */
			LOG_ERR("Failed to configure clock: %d", err);
			return err;
		}
	}

	if (clock_control_on(config->clock_dev, config->clock_subsys)) {
		LOG_ERR("Could not turn on clock");
		return -EINVAL;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &input_clock_freq)) {
		LOG_ERR("Could not get clock frequency");
		return -EINVAL;
	}

	data->freq = input_clock_freq / (1U << config->prescale);

	TPM_GetDefaultConfig(&tpmConfig);
	tpmConfig.prescale = config->prescale;
	base = get_base(dev);
	TPM_Init(base, &tpmConfig);

	/* Set the modulo to max value. */
	base->MOD = TPM_MAX_COUNTER_VALUE(base);

	config->irq_config_func();

	return 0;
}

static DEVICE_API(counter, mcux_tpm_driver_api) = {
	.start = mcux_tpm_start,
	.stop = mcux_tpm_stop,
	.get_value = mcux_tpm_get_value,
	.reset = mcux_tpm_reset,
	.set_alarm = mcux_tpm_set_alarm,
	.cancel_alarm = mcux_tpm_cancel_alarm,
	.set_top_value = mcux_tpm_set_top_value,
	.get_pending_int = mcux_tpm_get_pending_int,
	.get_top_value = mcux_tpm_get_top_value,
	.get_freq = mcux_tpm_get_freq,
#ifdef CONFIG_COUNTER_CAPTURE
	.capture_configure = mcux_tpm_capture_configure,
	.enable_capture = mcux_tpm_enable_capture,
	.disable_capture = mcux_tpm_disable_capture,
#endif /* CONFIG_COUNTER_CAPTURE */
};

#define TO_TPM_PRESCALE_DIVIDE(val) _DO_CONCAT(kTPM_Prescale_Divide_, val)

#ifdef CONFIG_COUNTER_CAPTURE
#define TPM_PINCTRL_DEFINE(n)							\
	IF_ENABLED(DT_INST_PINCTRL_HAS_NAME(n, default),			\
		   (PINCTRL_DT_INST_DEFINE(n);))
#define TPM_PINCTRL_INIT(n)							\
	.pincfg = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0),		\
			      (PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL)),
#else
#define TPM_PINCTRL_DEFINE(n)
#define TPM_PINCTRL_INIT(n)
#endif /* CONFIG_COUNTER_CAPTURE */

#define TPM_DEVICE_INIT_MCUX(n)							\
	TPM_PINCTRL_DEFINE(n)							\
	static struct mcux_tpm_data mcux_tpm_data_ ## n;			\
	static void mcux_tpm_irq_config_ ## n(void);				\
										\
	static const struct mcux_tpm_config mcux_tpm_config_ ## n = {		\
		DEVICE_MMIO_NAMED_ROM_INIT(tpm_mmio, DT_DRV_INST(n)),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.clock_subsys =							\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
		.tpm_clock_source = kTPM_SystemClock,				\
		.prescale = TO_TPM_PRESCALE_DIVIDE(DT_INST_PROP(n, prescaler)),	\
		.info = {							\
			.max_top_value = TPM_MAX_COUNTER_VALUE(TPM(n)),		\
			.freq = 0,						\
			.channels = FSL_FEATURE_TPM_CHANNEL_COUNTn(		\
					(TPM_Type *)DT_INST_REG_ADDR(n)),	\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,			\
		},								\
		TPM_PINCTRL_INIT(n)						\
		.irq_config_func = mcux_tpm_irq_config_ ## n,			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			mcux_tpm_init,						\
			NULL,							\
			&mcux_tpm_data_ ## n,					\
			&mcux_tpm_config_ ## n,					\
			POST_KERNEL,						\
			CONFIG_COUNTER_INIT_PRIORITY,				\
			&mcux_tpm_driver_api);					\
										\
	static void mcux_tpm_irq_config_ ## n(void)				\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			DT_INST_IRQ(n, priority),				\
			mcux_tpm_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));					\
	}									\

DT_INST_FOREACH_STATUS_OKAY(TPM_DEVICE_INIT_MCUX)
