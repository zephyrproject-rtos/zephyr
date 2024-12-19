/*
 * Copyright (c) 2019 Intel Corporation.
 * Copyright (c) 2023 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_adc

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_mchp_xec);

#include <zephyr/drivers/adc.h>
#ifdef CONFIG_SOC_SERIES_MEC172X
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#endif
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/irq.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define XEC_ADC_VREF_ANALOG 3300

/* ADC Control Register */
#define XEC_ADC_CTRL_SINGLE_DONE_STATUS		BIT(7)
#define XEC_ADC_CTRL_REPEAT_DONE_STATUS		BIT(6)
#define XER_ADC_CTRL_SOFT_RESET			BIT(4)
#define XEC_ADC_CTRL_POWER_SAVER_DIS		BIT(3)
#define XEC_ADC_CTRL_START_REPEAT		BIT(2)
#define XEC_ADC_CTRL_START_SINGLE		BIT(1)
#define XEC_ADC_CTRL_ACTIVATE			BIT(0)

/* ADC implements two interrupt signals:
 * One-shot(single) conversion of a set of channels
 * Repeat conversion of a set of channels
 * Channel sets for single and repeat may be different.
 */
enum adc_pm_policy_state_flag {
	ADC_PM_POLICY_STATE_SINGLE_FLAG,
	ADC_PM_POLICY_STATE_REPEAT_FLAG,
	ADC_PM_POLICY_STATE_FLAG_COUNT,
};

#define XEC_ADC_MAX_HW_CHAN 16
#define XEC_ADC_CFG_CHANNELS DT_INST_PROP(0, channels)

struct adc_xec_regs {
	uint32_t control_reg;
	uint32_t delay_reg;
	uint32_t status_reg;
	uint32_t single_reg;
	uint32_t repeat_reg;
	uint32_t channel_read_reg[XEC_ADC_CFG_CHANNELS];
	uint32_t unused[10 + (XEC_ADC_MAX_HW_CHAN - XEC_ADC_CFG_CHANNELS)];
	uint32_t config_reg;
	uint32_t vref_channel_reg;
	uint32_t vref_control_reg;
	uint32_t sar_control_reg;
};

struct adc_xec_config {
	struct adc_xec_regs *regs;

	uint8_t girq_single;
	uint8_t girq_single_pos;
	uint8_t girq_repeat;
	uint8_t girq_repeat_pos;
	uint8_t pcr_regidx;
	uint8_t pcr_bitpos;
	const struct pinctrl_dev_config *pcfg;
};

struct adc_xec_data {
	struct adc_context ctx;
	const struct device *adc_dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
#ifdef CONFIG_PM_DEVICE
	ATOMIC_DEFINE(pm_policy_state_flag, ADC_PM_POLICY_STATE_FLAG_COUNT);
#endif
};

#ifdef CONFIG_PM_DEVICE
static void adc_xec_pm_policy_state_lock_get(struct adc_xec_data *data,
					       enum adc_pm_policy_state_flag flag)
{
	if (atomic_test_and_set_bit(data->pm_policy_state_flag, flag) == 0) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}

static void adc_xec_pm_policy_state_lock_put(struct adc_xec_data *data,
					    enum adc_pm_policy_state_flag flag)
{
	if (atomic_test_and_clear_bit(data->pm_policy_state_flag, flag) == 1) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}
#endif

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_xec_data *data = CONTAINER_OF(ctx, struct adc_xec_data, ctx);
	const struct device *adc_dev = data->adc_dev;
	const struct adc_xec_config * const devcfg = adc_dev->config;
	struct adc_xec_regs *regs = devcfg->regs;

	data->repeat_buffer = data->buffer;

#ifdef CONFIG_PM_DEVICE
	adc_xec_pm_policy_state_lock_get(data, ADC_PM_POLICY_STATE_SINGLE_FLAG);
#endif
	regs->single_reg = ctx->sequence.channels;
	regs->control_reg |= XEC_ADC_CTRL_START_SINGLE;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_xec_data *data = CONTAINER_OF(ctx, struct adc_xec_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int adc_xec_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_xec_config *const cfg = dev->config;
	struct adc_xec_regs * const regs = cfg->regs;
	uint32_t areg;

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		return -EINVAL;
	}

	if (channel_cfg->channel_id >= XEC_ADC_CFG_CHANNELS) {
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		return -EINVAL;
	}

	/* Setup VREF */
	areg = regs->vref_channel_reg;
	areg &= ~MCHP_ADC_CH_VREF_SEL_MASK(channel_cfg->channel_id);

	if (channel_cfg->reference == ADC_REF_INTERNAL) {
		areg |= MCHP_ADC_CH_VREF_SEL_PAD(channel_cfg->channel_id);
	} else if (channel_cfg->reference == ADC_REF_EXTERNAL0) {
		areg |= MCHP_ADC_CH_VREF_SEL_GPIO(channel_cfg->channel_id);
	} else {
		return -EINVAL;
	}

	regs->vref_channel_reg = areg;

	/* Differential mode? */
	areg = regs->sar_control_reg;
	areg &= ~BIT(MCHP_ADC_SAR_CTRL_SELDIFF_POS);
	if (channel_cfg->differential != 0) {
		areg |= MCHP_ADC_SAR_CTRL_SELDIFF_EN;
	}
	regs->sar_control_reg = areg;

	return 0;
}

static bool adc_xec_validate_buffer_size(const struct adc_sequence *sequence)
{
	int chan_count = 0;
	size_t buff_need;
	uint32_t chan_mask;

	for (chan_mask = 0x80; chan_mask != 0; chan_mask >>= 1) {
		if (chan_mask & sequence->channels) {
			chan_count++;
		}
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

static int adc_xec_start_read(const struct device *dev,
			      const struct adc_sequence *sequence)
{
	const struct adc_xec_config *const cfg = dev->config;
	struct adc_xec_regs * const regs = cfg->regs;
	struct adc_xec_data * const data = dev->data;
	uint32_t sar_ctrl;

	if (sequence->channels & ~BIT_MASK(XEC_ADC_CFG_CHANNELS)) {
		LOG_ERR("Incorrect channels, bitmask 0x%x", sequence->channels);
		return -EINVAL;
	}

	if (sequence->channels == 0UL) {
		LOG_ERR("No channel selected");
		return -EINVAL;
	}

	if (!adc_xec_validate_buffer_size(sequence)) {
		LOG_ERR("Incorrect buffer size");
		return -ENOMEM;
	}

	/* Setup ADC resolution */
	sar_ctrl = regs->sar_control_reg;
	sar_ctrl &= ~(MCHP_ADC_SAR_CTRL_RES_MASK |
		      (1 << MCHP_ADC_SAR_CTRL_SHIFTD_POS));

	if (sequence->resolution == 12) {
		sar_ctrl |= MCHP_ADC_SAR_CTRL_RES_12_BITS;
	} else if (sequence->resolution == 10) {
		sar_ctrl |= MCHP_ADC_SAR_CTRL_RES_10_BITS;
		sar_ctrl |= MCHP_ADC_SAR_CTRL_SHIFTD_EN;
	} else {
		return -EINVAL;
	}

	regs->sar_control_reg = sar_ctrl;

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_xec_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	struct adc_xec_data * const data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = adc_xec_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_xec_read_async(const struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct adc_xec_data * const data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = adc_xec_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif /* CONFIG_ADC_ASYNC */

static void xec_adc_get_sample(const struct device *dev)
{
	const struct adc_xec_config *const cfg = dev->config;
	struct adc_xec_regs * const regs = cfg->regs;
	struct adc_xec_data * const data = dev->data;
	uint32_t idx;
	uint32_t channels = regs->status_reg;
	uint32_t ch_status = channels;
	uint32_t bit;

	/*
	 * Using the enabled channel bit set, from
	 * lowest channel number to highest, find out
	 * which channel is enabled and copy the ADC
	 * values from hardware registers to the data
	 * buffer.
	 */
	bit = find_lsb_set(channels);
	while (bit != 0) {
		idx = bit - 1;

		*data->buffer = (uint16_t)regs->channel_read_reg[idx];
		data->buffer++;

		channels &= ~BIT(idx);
		bit = find_lsb_set(channels);
	}

	/* Clear the status register */
	regs->status_reg = ch_status;
}

#ifdef CONFIG_SOC_SERIES_MEC172X
static inline void adc_xec_girq_clr(uint8_t girq_idx, uint8_t girq_posn)
{
	mchp_xec_ecia_girq_src_clr(girq_idx, girq_posn);
}

static inline void adc_xec_girq_en(uint8_t girq_idx, uint8_t girq_posn)
{
	mchp_xec_ecia_girq_src_en(girq_idx, girq_posn);
}

static inline void adc_xec_girq_dis(uint8_t girq_idx, uint8_t girq_posn)
{
	mchp_xec_ecia_girq_src_dis(girq_idx, girq_posn);
}
#else

static inline void adc_xec_girq_clr(uint8_t girq_idx, uint8_t girq_posn)
{
	MCHP_GIRQ_SRC(girq_idx) = BIT(girq_posn);
}

static inline void adc_xec_girq_en(uint8_t girq_idx, uint8_t girq_posn)
{
	MCHP_GIRQ_ENSET(girq_idx) = BIT(girq_posn);
}

static inline void adc_xec_girq_dis(uint8_t girq_idx, uint8_t girq_posn)
{
	MCHP_GIRQ_ENCLR(girq_idx) = MCHP_KBC_IBF_GIRQ;
}
#endif

static void adc_xec_single_isr(const struct device *dev)
{
	const struct adc_xec_config *const cfg = dev->config;
	struct adc_xec_regs * const regs = cfg->regs;
	struct adc_xec_data * const data = dev->data;
	uint32_t ctrl;

	/* Clear START_SINGLE bit and clear SINGLE_DONE_STATUS */
	ctrl = regs->control_reg;
	ctrl &= ~XEC_ADC_CTRL_START_SINGLE;
	ctrl |= XEC_ADC_CTRL_SINGLE_DONE_STATUS;
	regs->control_reg = ctrl;

	/* Also clear GIRQ source status bit */
	adc_xec_girq_clr(cfg->girq_single, cfg->girq_single_pos);

	xec_adc_get_sample(dev);

#ifdef CONFIG_PM_DEVICE
	adc_xec_pm_policy_state_lock_put(data, ADC_PM_POLICY_STATE_SINGLE_FLAG);
#endif

	adc_context_on_sampling_done(&data->ctx, dev);

	LOG_DBG("ADC ISR triggered.");
}


#ifdef CONFIG_PM_DEVICE
static int adc_xec_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct adc_xec_config *const devcfg = dev->config;
	struct adc_xec_regs * const adc_regs = devcfg->regs;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_DEFAULT);
		/* ADC activate  */
		adc_regs->control_reg |= XEC_ADC_CTRL_ACTIVATE;
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* ADC deactivate  */
		adc_regs->control_reg &= ~(XEC_ADC_CTRL_ACTIVATE);
		/* If application does not want to turn off ADC pins it will
		 * not define pinctrl-1 for this node.
		 */
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

static DEVICE_API(adc, adc_xec_api) = {
	.channel_setup = adc_xec_channel_setup,
	.read = adc_xec_read,
#if defined(CONFIG_ADC_ASYNC)
	.read_async = adc_xec_read_async,
#endif
	.ref_internal = XEC_ADC_VREF_ANALOG,
};

/* ADC Config Register */
#define XEC_ADC_CFG_CLK_VAL(clk_time)	(		\
	(clk_time << MCHP_ADC_CFG_CLK_LO_TIME_POS) |	\
	(clk_time << MCHP_ADC_CFG_CLK_HI_TIME_POS))

static int adc_xec_init(const struct device *dev)
{
	const struct adc_xec_config *const cfg = dev->config;
	struct adc_xec_regs * const regs = cfg->regs;
	struct adc_xec_data * const data = dev->data;
	int ret;

	data->adc_dev = dev;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("XEC ADC V2 pinctrl setup failed (%d)", ret);
		return ret;
	}

	regs->config_reg = XEC_ADC_CFG_CLK_VAL(DT_INST_PROP(0, clktime));

	regs->control_reg =  XEC_ADC_CTRL_ACTIVATE
		| XEC_ADC_CTRL_POWER_SAVER_DIS
		| XEC_ADC_CTRL_SINGLE_DONE_STATUS
		| XEC_ADC_CTRL_REPEAT_DONE_STATUS;

	adc_xec_girq_dis(cfg->girq_repeat, cfg->girq_repeat_pos);
	adc_xec_girq_clr(cfg->girq_repeat, cfg->girq_repeat_pos);
	adc_xec_girq_dis(cfg->girq_single, cfg->girq_single_pos);
	adc_xec_girq_clr(cfg->girq_single, cfg->girq_single_pos);
	adc_xec_girq_en(cfg->girq_single, cfg->girq_single_pos);

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    adc_xec_single_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

PINCTRL_DT_INST_DEFINE(0);

static struct adc_xec_config adc_xec_dev_cfg_0 = {
	.regs = (struct adc_xec_regs *)(DT_INST_REG_ADDR(0)),
	.girq_single = (uint8_t)(DT_INST_PROP_BY_IDX(0, girqs, 0)),
	.girq_single_pos = (uint8_t)(DT_INST_PROP_BY_IDX(0, girqs, 1)),
	.girq_repeat = (uint8_t)(DT_INST_PROP_BY_IDX(0, girqs, 2)),
	.girq_repeat_pos = (uint8_t)(DT_INST_PROP_BY_IDX(0, girqs, 3)),
	.pcr_regidx = (uint8_t)(DT_INST_PROP_BY_IDX(0, pcrs, 0)),
	.pcr_bitpos = (uint8_t)(DT_INST_PROP_BY_IDX(0, pcrs, 1)),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static struct adc_xec_data adc_xec_dev_data_0 = {
	ADC_CONTEXT_INIT_TIMER(adc_xec_dev_data_0, ctx),
	ADC_CONTEXT_INIT_LOCK(adc_xec_dev_data_0, ctx),
	ADC_CONTEXT_INIT_SYNC(adc_xec_dev_data_0, ctx),
};

PM_DEVICE_DT_INST_DEFINE(0, adc_xec_pm_action);

DEVICE_DT_INST_DEFINE(0, adc_xec_init, PM_DEVICE_DT_INST_GET(0),
		    &adc_xec_dev_data_0, &adc_xec_dev_cfg_0,
		    PRE_KERNEL_1, CONFIG_ADC_INIT_PRIORITY,
		    &adc_xec_api);
