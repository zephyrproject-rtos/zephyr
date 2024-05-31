/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_tpm_timer

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include <fsl_tpm.h>

LOG_MODULE_REGISTER(mcux_tpm, CONFIG_COUNTER_LOG_LEVEL);

#define DEV_CFG(_dev) ((const struct mcux_tpm_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mcux_tpm_data *)(_dev)->data)

struct mcux_tpm_config {
	struct counter_config_info info;

	DEVICE_MMIO_NAMED_ROM(tpm_mmio);

	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;

	tpm_clock_source_t tpm_clock_source;
	tpm_clock_prescale_t prescale;
};

struct mcux_tpm_data {
	DEVICE_MMIO_NAMED_RAM(tpm_mmio);
	counter_alarm_callback_t alarm_callback;
	counter_top_callback_t top_callback;
	uint32_t freq;
	void *alarm_user_data;
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

static int mcux_tpm_set_alarm(const struct device *dev, uint8_t chan_id,
			      const struct counter_alarm_cfg *alarm_cfg)
{
	TPM_Type *base = get_base(dev);
	uint32_t current = TPM_GetCurrentTimerCount(base);
	uint32_t top_value = base->MOD;
	struct mcux_tpm_data *data = dev->data;
	uint32_t ticks = alarm_cfg->ticks;

	if (chan_id != kTPM_Chnl_0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	if (ticks > (top_value))
		return -EINVAL;

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		if (top_value - current >= ticks)
			ticks += current;
		else
			ticks -= top_value - current;
	}

	if (data->alarm_callback)
		return -EBUSY;

	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;

	TPM_SetupOutputCompare(base, kTPM_Chnl_0, kTPM_NoOutputSignal, ticks);
	TPM_EnableInterrupts(base, kTPM_Chnl0InterruptEnable);

	return 0;
}

static int mcux_tpm_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	TPM_Type *base = get_base(dev);
	struct mcux_tpm_data *data = dev->data;

	if (chan_id != kTPM_Chnl_0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	TPM_DisableInterrupts(base, kTPM_Chnl0InterruptEnable);
	data->alarm_callback = NULL;

	return 0;
}

void mcux_tpm_isr(const struct device *dev)
{
	TPM_Type *base = get_base(dev);
	struct mcux_tpm_data *data = dev->data;
	uint32_t current = TPM_GetCurrentTimerCount(base);
	uint32_t status;

	status =  TPM_GetStatusFlags(base) & (kTPM_Chnl0Flag | kTPM_TimeOverflowFlag);
	TPM_ClearStatusFlags(base, status);
	barrier_dsync_fence_full();

	if ((status & kTPM_Chnl0Flag) && data->alarm_callback) {
		TPM_DisableInterrupts(base,
				      kTPM_Chnl0InterruptEnable);
		counter_alarm_callback_t alarm_cb = data->alarm_callback;

		data->alarm_callback = NULL;
		alarm_cb(dev, 0, current, data->alarm_user_data);
	}

	if ((status & kTPM_TimeOverflowFlag) && data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
}

static uint32_t mcux_tpm_get_pending_int(const struct device *dev)
{
	TPM_Type *base = get_base(dev);

	return (TPM_GetStatusFlags(base) & kTPM_Chnl0Flag);
}

static int mcux_tpm_set_top_value(const struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	const struct mcux_tpm_config *config = dev->config;
	TPM_Type *base = get_base(dev);
	struct mcux_tpm_data *data = dev->data;

	if (data->alarm_callback)
		return -EBUSY;

	/* Check if timer already enabled. */
#if defined(FSL_FEATURE_TPM_HAS_SC_CLKS) && FSL_FEATURE_TPM_HAS_SC_CLKS
	if (base->SC & TPM_SC_CLKS_MASK) {
#else
	if (base->SC & TPM_SC_CMOD_MASK) {
#endif
		/* Timer already enabled, check flags before resetting */
		if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET)
			return -ENOTSUP;

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

	return 0;
}

static const struct counter_driver_api mcux_tpm_driver_api = {
	.start = mcux_tpm_start,
	.stop = mcux_tpm_stop,
	.get_value = mcux_tpm_get_value,
	.set_alarm = mcux_tpm_set_alarm,
	.cancel_alarm = mcux_tpm_cancel_alarm,
	.set_top_value = mcux_tpm_set_top_value,
	.get_pending_int = mcux_tpm_get_pending_int,
	.get_top_value = mcux_tpm_get_top_value,
	.get_freq = mcux_tpm_get_freq,
};

#define TO_TPM_PRESCALE_DIVIDE(val) _DO_CONCAT(kTPM_Prescale_Divide_, val)

#define TPM_DEVICE_INIT_MCUX(n)							\
	static struct mcux_tpm_data mcux_tpm_data_ ## n;			\
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
			.channels = 1,						\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,			\
		},								\
	};									\
										\
	static int mcux_tpm_## n ##_init(const struct device *dev);		\
	DEVICE_DT_INST_DEFINE(n,						\
			mcux_tpm_## n ##_init,					\
			NULL,							\
			&mcux_tpm_data_ ## n,					\
			&mcux_tpm_config_ ## n,					\
			POST_KERNEL,						\
			CONFIG_COUNTER_INIT_PRIORITY,				\
			&mcux_tpm_driver_api);					\
										\
	static int mcux_tpm_## n ##_init(const struct device *dev)		\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			DT_INST_IRQ(n, priority),				\
			mcux_tpm_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));					\
		return mcux_tpm_init(dev);					\
	}									\

DT_INST_FOREACH_STATUS_OKAY(TPM_DEVICE_INIT_MCUX)
