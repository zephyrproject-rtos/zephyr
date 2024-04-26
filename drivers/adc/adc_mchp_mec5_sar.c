/*
 * Copyright 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mec5_adc_sar

#include <errno.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include <soc.h>
#include <device_mec5.h>
#include <mec_adc_api.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

LOG_MODULE_REGISTER(adc_mchp_mec5_adc_sar, CONFIG_ADC_LOG_LEVEL);

#define ADC_MEC5_CLKTM_DFLT		(MEC_ADC_SAMPLE_CLK_LIT_DFLT)
#define ADC_MEC5_WRMUP_DLY_DFLT		(MEC_ADC_WARM_UP_DLY_CLKS_DFLT)
#define ADC_MEC5_RPTC_START_DLY_DFLT	(MEC_ADC_RPT_CYCLE_START_DLY_DFLT)
#define ADC_MEC5_RPTC_DLY_DFLT		(MEC_ADC_RPT_CYCLE_DLY_DFLT)
#define ADC_MEC5_SAR_CFG_NO_CHG         0xffffffff

#if MEC5_ADC_CHANNELS == 16
/* Hardware supports channels 0 - 15 */
#define ADC_MEC5_SAR_SUPP_CHAN_MAP 0xffffu
#else
/* Hardware supports channels 0 - 7 */
#define ADC_MEC5_SAR_SUPP_CHAN_MAP 0xffu
#endif

struct adc_mec5_sar_devcfg {
	struct adc_regs *regs;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pin_cfg;
	uint32_t sar_config;
	uint16_t warm_up_delay;
	uint16_t rpt_cycle_start_delay;
	uint16_t rpt_cycle_delay;
	uint8_t clktime;
};

struct adc_mec5_sar_data {
	const struct device *dev;
	struct adc_context ctx;
	uint32_t isr_count;
	uint32_t chans_done_status;
	uint16_t *buffer;
	uint16_t *buf_end;
	uint16_t *repeat_buffer;
	uint32_t mask_channels;
	uint8_t num_channels;
#ifdef CONFIG_PM_DEVICE
	ATOMIC_DEFINE(pm_policy_state_flag, ADC_PM_POLICY_STATE_FLAG_COUNT);
#endif
};

/*--------------------------------------------------------------------------*/

static int adc_mec5_sar_init(const struct device *dev)
{
	const struct adc_mec5_sar_devcfg *const devcfg = dev->config;
	struct adc_mec5_sar_data *data = dev->data;
	struct adc_regs *const regs = devcfg->regs;
	uint32_t intr_flags = BIT(MEC_ADC_SINGLE_INTR_POS) | BIT(MEC_ADC_REPEAT_INTR_POS);
	struct mec_adc_config mcfg = {
		.flags = (BIT(MEC_ADC_CFG_SOFT_RESET_POS) | BIT(MEC_ADC_CFG_PWR_SAVE_DIS_POS)
			  | BIT(MEC_ADC_CFG_SAMPLE_TIME_POS) | BIT(MEC_ADC_CFG_WARM_UP_POS)
			  | BIT(MEC_ADC_CFG_RPT_DELAY_POS)),
		.sample_clk_lo_time = devcfg->clktime,
		.sample_clk_hi_time = devcfg->clktime,
		.warm_up_delay = devcfg->warm_up_delay,
		.rpt_start_delay = devcfg->rpt_cycle_start_delay,
		.rpt_cycle_delay = devcfg->rpt_cycle_delay,
	};

	if (devcfg->sar_config != ADC_MEC5_SAR_CFG_NO_CHG) {
		mcfg.sar_config = devcfg->sar_config;
		mcfg.flags |= BIT(MEC_ADC_CFG_SAR_CFG_OVR_POS);
	}

	if (devcfg->pin_cfg) {
		if (pinctrl_apply_state(devcfg->pin_cfg, PINCTRL_STATE_DEFAULT)) {
			return -EIO;
		}
	}

	if (mec_adc_init(regs, &mcfg) != MEC_RET_OK) {
		return -EIO;
	}

	data->dev = dev;

	if (devcfg->irq_config_func) {
		devcfg->irq_config_func(dev);
		mec_adc_girq_status_clr(regs, intr_flags);
		mec_adc_girq_ctrl(regs, intr_flags, 1);
	}

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static void adc_mec5_sar_pm_policy_state_lock_get(struct adc_xec_data *data,
						  enum adc_pm_policy_state_flag flag)
{
	if (atomic_test_and_set_bit(data->pm_policy_state_flag, flag) == 0) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}

static void adc_mec5_sar_pm_policy_state_lock_put(struct adc_xec_data *data,
						  enum adc_pm_policy_state_flag flag)
{
	if (atomic_test_and_clear_bit(data->pm_policy_state_flag, flag) == 1) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}

static int adc_mec5_sar_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct adc_mec5_sar_devcfg *const devcfg = dev->config;
	struct adc_regs *const regs = devcfg->regs;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_DEFAULT);
		mec_adc_activate(regs, 1);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		mec_adc_activate(regs, 0);
		ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_SLEEP);
		if (ret == -ENOENT) { /* pinctrl-1 does not exist.  */
			ret = 0;
		}
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static void adc_mec5_sar_get_sample(const struct device *dev)
{
	const struct adc_mec5_sar_devcfg *const devcfg = dev->config;
	struct adc_mec5_sar_data *data = dev->data;
	struct adc_regs *const regs = devcfg->regs;
	uint32_t channels = data->chans_done_status;
	uint32_t idx = 0;
	uint32_t bit = 0;

	bit = find_lsb_set(channels);
	while (bit != 0) {
		idx = bit - 1;

		*data->buffer = (uint16_t)mec_adc_channel_reading(regs, idx);
		data->buffer++;

		channels &= ~BIT(idx);
		bit = find_lsb_set(channels);
	}
}

static void adc_mec5_sar_single_isr(const struct device *dev)
{
	const struct adc_mec5_sar_devcfg *const devcfg = dev->config;
	struct adc_mec5_sar_data *data = dev->data;
	struct adc_regs *const regs = devcfg->regs;
	uint32_t clr_flags = BIT(MEC_ADC_SINGLE_INTR_POS);

	data->chans_done_status = mec_adc_channels_done(regs);
	mec_adc_status_clear(regs, clr_flags);

	adc_mec5_sar_get_sample(dev);

#ifdef CONFIG_PM_DEVICE
	adc_mec5_sar_pm_policy_state_lock_put(data, ADC_PM_POLICY_STATE_SINGLE_FLAG);
#endif

	adc_context_on_sampling_done(&data->ctx, dev);

	LOG_DBG("ADC ISR triggered.");
}


static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_mec5_sar_data *data = CONTAINER_OF(ctx, struct adc_mec5_sar_data, ctx);
	const struct device *adc_dev = data->dev;
	const struct adc_mec5_sar_devcfg *const devcfg = adc_dev->config;
	struct adc_regs *const regs = devcfg->regs;

	data->repeat_buffer = data->buffer;

#ifdef CONFIG_PM_DEVICE
	adc_mec5_sar_pm_policy_state_lock_get(data, ADC_PM_POLICY_STATE_SINGLE_FLAG);
#endif
	mec_adc_start(regs, ctx->sequence.channels, 0);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_mec5_sar_data *data = CONTAINER_OF(ctx, struct adc_mec5_sar_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

/* If the channel is valid:
 *   select the channel's voltage reference
 *   select single-ended or differential mode
 *   NOTE: Single-ended/Differential affects ALL channels.
 */
static int adc_mec5_sar_channel_setup(const struct device *dev,
				      const struct adc_channel_cfg *chan_cfg)
{
	const struct adc_mec5_sar_devcfg *const devcfg = dev->config;
	struct adc_regs *const regs = devcfg->regs;
	enum mec_adc_chan_vref vref = MEC_ADC_CHAN_VREF_PAD;
	uint8_t diff_enable = 0;

	if (chan_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		return -EINVAL;
	}

	if (chan_cfg->channel_id >= MEC5_ADC_CHANNELS) {
		return -EINVAL;
	}

	if (chan_cfg->gain != ADC_GAIN_1) {
		return -EINVAL;
	}

	/* Setup VREF */
	switch (chan_cfg->reference) {
	case ADC_REF_INTERNAL:
		break;
	case ADC_REF_EXTERNAL0:
		vref = MEC_ADC_CHAN_VREF_GPIO;
		break;
	default:
		return -EINVAL;
	}

	if (mec_adc_chan_vref_select(regs, chan_cfg->channel_id, vref) != MEC_RET_OK) {
		return -EIO;
	}

	if (chan_cfg->differential != 0) {
		diff_enable = 1u;
	}

	if (mec_adc_differential_input_enable(regs, diff_enable) != MEC_RET_OK) {
		return -EIO;
	}

	return 0;
}

static bool adc_mec5_sar_validate_buffer_size(const struct adc_sequence *sequence)
{
	int chan_count = 0, bitpos = 0;
	size_t buff_need = 0;
	uint32_t chan_mask = ADC_MEC5_SAR_SUPP_CHAN_MAP;

	/* returns bit position + 1 of first set bit from the right
	 * if no bits set returns 0
	 */
	bitpos = find_lsb_set(chan_mask);
	while (bitpos) {
		bitpos--;
		if (sequence->channels & BIT(bitpos)) {
			chan_count++;
		}
		chan_mask &= ~BIT(bitpos);
		bitpos = find_lsb_set(chan_mask);
	}

	buff_need = chan_count * sizeof(uint16_t);

	if (sequence->options) {
		buff_need *= 1 + sequence->options->extra_samplings;
	}

	if (buff_need > sequence->buffer_size) {
		return false;
	}

	return true;
}

/* Check adc_sequence has valid channels
 * Configure ADC resolution
 * Start ADC
 * Wait until done
 */
static int adc_mec5_sar_start_read(const struct device *dev,
				   const struct adc_sequence *sequence)
{
	const struct adc_mec5_sar_devcfg *const devcfg = dev->config;
	struct adc_regs *const regs = devcfg->regs;
	struct adc_mec5_sar_data *data = dev->data;

	if (sequence->channels & ~(ADC_MEC5_SAR_SUPP_CHAN_MAP)) {
		LOG_ERR("Incorrect channels, bitmask 0x%x", sequence->channels);
		return -EINVAL;
	}

	if (sequence->channels == 0UL) {
		LOG_ERR("No channel selected");
		return -EINVAL;
	}

	if (!adc_mec5_sar_validate_buffer_size(sequence)) {
		LOG_ERR("Incorrect buffer size");
		return -ENOMEM;
	}

	if (mec_adc_resolution_set(regs, sequence->resolution) != MEC_RET_OK) {
		return -EINVAL;
	}

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_mec5_sar_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_mec5_sar_data *data = dev->data;
	int ret;

	if (!sequence) {
		return -EINVAL;
	}

	adc_context_lock(&data->ctx, false, NULL);
	ret = adc_mec5_sar_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_mec5_sar_read_async(const struct device *dev,
				   const struct adc_sequence *sequence,
				   struct k_poll_signal *async)
{
	struct adc_mec5_sar_data *data = dev->data;
	int ret;

	if (!sequence || !async) {
		return -EINVAL;
	}

	adc_context_lock(&data->ctx, true, async);
	ret = adc_mec5_sar_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}
#endif

#define ADC_MEC5_SAR_DRIVER_API(n)						\
	static const struct adc_driver_api adc_mec5_sar_driver_api_##n = {	\
		.channel_setup = adc_mec5_sar_channel_setup,			\
		.read = adc_mec5_sar_read,					\
		IF_ENABLED(CONFIG_ADC_ASYNC, (.read_async = adc_mec5_sar_read_async,))\
		.ref_internal = MEC_ADC_INTERNAL_VREF_MV,			\
	};

#define ADC_MEC5_SAR_IRQ_CONFIG(n)						\
	static void adc_mec5_sar_irq_cfg_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, single, irq),		\
			DT_INST_IRQ_BY_NAME(n, single, priority),		\
			adc_mec5_sar_single_isr, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQ_BY_NAME(n, single, irq));		\
	};

#define ADC_MEC5_SAR_CLKTIME(i)							\
	DT_INST_PROP_OR(i, clktime, ADC_MEC5_CLKTM_DFLT)
#define ADC_MEC5_SAR_WMUP_DLY(i)						\
	DT_INST_PROP_OR(i, warm_up_delay, ADC_MEC5_WRMUP_DLY_DFLT)
#define ADC_MEC5_SAR_RPT_CYC_START_DLY(i)					\
	DT_INST_PROP_OR(i, repeat_cycle_start_delay, ADC_MEC5_RPTC_START_DLY_DFLT)
#define ADC_MEC5_SAR_RPT_CYC_DLY(i)						\
	DT_INST_PROP_OR(i, repeat_cycle_delay, ADC_MEC5_RPTC_DLY_DFLT)
#define ADC_MEC5_SAR_SAR_CFG(i)							\
	DT_INST_PROP_OR(i, sar_config, ADC_MEC5_SAR_CFG_NO_CHG)

#ifdef CONFIG_PM_DEVICE
#define ADC_MEC5_SAR_PM_DEV_INST_INIT_DEF(i, f)	PM_DEVICE_DT_INST_DEFINE(i, f);
#define ADC_MEC5_SAR_PM_DEV_INST_GET(i)		PM_DEVICE_DT_INST_GET(i)
#else
#define ADC_MEC5_SAR_PM_DEV_INST_INIT_DEF(i, f)
#define ADC_MEC5_SAR_PM_DEV_INST_GET(i) NULL
#endif

#define ADC_MEC5_SAR_INIT_DEVICE(n)						\
	ADC_MEC5_SAR_DRIVER_API(n)						\
	ADC_MEC5_SAR_IRQ_CONFIG(n)						\
	COND_CODE_1(DT_INST_NUM_PINCTRL_STATES(n),				\
				(PINCTRL_DT_INST_DEFINE(n);), (EMPTY))		\
	ADC_MEC5_SAR_PM_DEV_INST_INIT_DEF(n, adc_mec5_sar_pm_action)		\
	static struct adc_mec5_sar_data adc_mec5_sar_data_##n = {		\
		ADC_CONTEXT_INIT_TIMER(adc_mec5_sar_data_##n, ctx),		\
		ADC_CONTEXT_INIT_LOCK(adc_mec5_sar_data_##n, ctx),		\
		ADC_CONTEXT_INIT_SYNC(adc_mec5_sar_data_##n, ctx),		\
	};									\
	static const struct adc_mec5_sar_devcfg adc_mec5_sar_dcfg_##n = {	\
		.regs = (struct adc_regs *)DT_INST_REG_ADDR(n),			\
		.irq_config_func = adc_mec5_sar_irq_cfg_func_##n,		\
		.pin_cfg = COND_CODE_1(DT_INST_NUM_PINCTRL_STATES(n),		\
				(PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL)),	\
		.sar_config = ADC_MEC5_SAR_SAR_CFG(n),				\
		.warm_up_delay = ADC_MEC5_SAR_WMUP_DLY(n),			\
		.rpt_cycle_start_delay = ADC_MEC5_SAR_RPT_CYC_START_DLY(n),	\
		.rpt_cycle_delay = ADC_MEC5_SAR_RPT_CYC_DLY(n),			\
		.clktime = ADC_MEC5_SAR_CLKTIME(n),				\
	};									\
	DEVICE_DT_INST_DEFINE(n,						\
			&adc_mec5_sar_init,					\
			ADC_MEC5_SAR_PM_DEV_INST_GET(n),			\
			&adc_mec5_sar_data_##n,					\
			&adc_mec5_sar_dcfg_##n,					\
			POST_KERNEL,						\
			CONFIG_ADC_INIT_PRIORITY,				\
			&adc_mec5_sar_driver_api_##n);

DT_INST_FOREACH_STATUS_OKAY(ADC_MEC5_SAR_INIT_DEVICE)
