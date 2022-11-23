/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_adc

#include <assert.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/adc_npcx_threshold.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>
#include <soc.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(adc_npcx, CONFIG_ADC_LOG_LEVEL);

/* ADC speed/delay values during initialization */
#define ADC_REGULAR_DLY_VAL	0x03
#define ADC_REGULAR_ADCCNF2_VAL	0x8B07
#define ADC_REGULAR_GENDLY_VAL	0x0100
#define ADC_REGULAR_MEAST_VAL	0x0001

/* ADC channel number */
#define NPCX_ADC_CH_COUNT DT_INST_PROP(0, channel_count)

/* ADC targeted operating frequency (2MHz) */
#define NPCX_ADC_CLK 2000000

/* ADC internal reference voltage (Unit:mV) */
#define NPCX_ADC_VREF_VOL 2816

/* ADC conversion mode */
#define NPCX_ADC_CHN_CONVERSION_MODE	0
#define NPCX_ADC_SCAN_CONVERSION_MODE	1

#define ADC_NPCX_THRVAL_RESOLUTION	10
#define ADC_NPCX_THRVAL_MAX		BIT_MASK(ADC_NPCX_THRVAL_RESOLUTION)

#define THRCTL(dev, ctl_no) (*((volatile uint16_t *) npcx_thrctl_reg(dev, ctl_no)))

/* Device config */
struct adc_npcx_config {
	/* adc controller base address */
	uintptr_t base;
	/* clock configuration */
	struct npcx_clk_cfg clk_cfg;
	/* amount of thresholds supported */
	const uint8_t threshold_count;
	/* threshold control register offset */
	const uint16_t threshold_reg_offset;
	const struct pinctrl_dev_config *pcfg;
};

struct adc_npcx_threshold_control {
	/*
	 * Selects ADC channel number, for which the measured data is compared
	 * for threshold detection.
	 */
	uint8_t chnsel;
	/*
	 * Sets relation between measured value and assetion threshold value.
	 * in thrval:
	 * 0: Threshold event is generated if Measured data > thrval.
	 * 1: Threshold event is generated if Measured data <= thrval.
	 */
	bool l_h;
	/* Sets the threshold value to which measured data is compared. */
	uint16_t thrval;
	/*
	 * Pointer of work queue item to be notified when threshold assertion
	 * occurs.
	 */
	struct k_work *work;
};

struct adc_npcx_threshold_data {
	/*
	 * While threshold interruption is enabled we need to resume to repetitive
	 * sampling mode after adc_npcx_read is called. This variable records
	 * channels being used in repetitive mode in order to set ADC registers
	 * back to threshold detection when adc_npcx_read is completed.
	 */
	uint16_t repetitive_channels;
	/*
	 * While threshold interruption is enabled, adc_npcx_read must disable
	 * all active threshold running to avoid race condition, this variable
	 * helps restore active threshods after adc_npcs_read has finnished.
	 */
	uint8_t active_thresholds;
	/* This array holds current configuration for each threshold. */
	struct adc_npcx_threshold_control
			control[DT_INST_PROP(0, threshold_count)];
	/*
	 * Pointer of work queue thread to be notified when threshold assertion
	 * occurs.
	 */
	struct k_work_q *work_q;
};

/* Driver data */
struct adc_npcx_data {
	/* Input clock for ADC converter */
	uint32_t input_clk;
	/* mutex of ADC channels */
	struct adc_context ctx;
	/*
	 * Bit-mask indicating the channels to be included in each sampling
	 * of this sequence.
	 */
	uint16_t channels;
	/* ADC Device pointer used in api functions */
	const struct device *adc_dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	/* end pointer of buffer to ensure enough space for storing ADC data. */
	uint16_t *buf_end;
	/* Threshold comparator data pointer */
	struct adc_npcx_threshold_data *threshold_data;
#ifdef CONFIG_PM
	atomic_t current_pm_lock;
#endif
};

/* Driver convenience defines */
#define HAL_INSTANCE(dev) ((struct adc_reg *)((const struct adc_npcx_config *)(dev)->config)->base)

/* ADC local functions */

#ifdef CONFIG_PM
static void adc_npcx_pm_policy_state_lock_get(struct adc_npcx_data *data)
{
	if (atomic_test_and_set_bit(&data->current_pm_lock, 0) == 0) {
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}

static void adc_npcx_pm_policy_state_lock_put(struct adc_npcx_data *data)
{
	if (atomic_test_and_clear_bit(&data->current_pm_lock, 0) == 1) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	}
}
#endif

static inline uint32_t npcx_thrctl_reg(const struct device *dev,
				       uint32_t ctl_no)
{
	const struct adc_npcx_config *config = dev->config;

	return (config->base + config->threshold_reg_offset) + (ctl_no - 1) * 2;
}

static void adc_npcx_isr(const struct device *dev)
{
	const struct adc_npcx_config *config = dev->config;
	struct adc_npcx_data *const data = dev->data;
	struct adc_reg *const inst = HAL_INSTANCE(dev);
	struct adc_npcx_threshold_data *const t_data = data->threshold_data;
	uint16_t status = inst->ADCSTS;
	uint16_t result, channel;

	/* Clear status pending bits first */
	inst->ADCSTS = status;
	LOG_DBG("%s: status is %04X\n", __func__, status);

	/* Is end of conversion cycle event? ie. Scan conversion is done. */
	if (IS_BIT_SET(status, NPCX_ADCSTS_EOCCEV) &&
	    IS_BIT_SET(inst->ADCCNF, NPCX_ADCCNF_INTECCEN)) {
		/* Stop conversion for scan conversion mode */
		inst->ADCCNF |= BIT(NPCX_ADCCNF_STOP);

		/* Get result for each ADC selected channel */
		while (data->channels) {
			channel = find_lsb_set(data->channels) - 1;
			result = GET_FIELD(CHNDAT(config->base, channel),
				NPCX_CHNDAT_CHDAT_FIELD);
			/*
			 * Save ADC result and adc_npcx_validate_buffer_size()
			 * already ensures that the buffer has enough space for
			 * storing result.
			 */
			if (data->buffer < data->buf_end) {
				*data->buffer++ = result;
			}
			data->channels &= ~BIT(channel);
		}
		/* Disable End of cyclic conversion interruption */
		inst->ADCCNF &= ~BIT(NPCX_ADCCNF_INTECCEN);

		if (IS_ENABLED(CONFIG_ADC_CMP_NPCX) &&
		    t_data->active_thresholds) {
			/* Set repetitive channels back */
			inst->ADCCS = t_data->repetitive_channels;
			/* Start conversion */
			inst->ADCCNF |= BIT(NPCX_ADCCNF_START);
		} else {
			/* Disable all channels */
			inst->ADCCS = 0;
			/* Turn off ADC */
			inst->ADCCNF &= ~(BIT(NPCX_ADCCNF_ADCEN));

#ifdef CONFIG_PM
			adc_npcx_pm_policy_state_lock_put(data);
#endif
		}
		/* Inform sampling is done */
		adc_context_on_sampling_done(&data->ctx, data->adc_dev);
	}

	if (!(IS_ENABLED(CONFIG_ADC_CMP_NPCX) && t_data->active_thresholds)) {
		return;
	}
	uint16_t thrcts;

	for (uint8_t i = 0; i < config->threshold_count; i++) {
		if (IS_BIT_SET(inst->THRCTS, i) && IS_BIT_SET(inst->THRCTS,
		    (NPCX_THRCTS_THR1_IEN + i))) {
			/* Avoid clearing other threshold status */
			thrcts = inst->THRCTS &
				 ~GENMASK(config->threshold_count - 1, 0);
			/* Clear threshold status */
			thrcts |= BIT(i);
			inst->THRCTS = thrcts;
			if (t_data->control[i].work) {
				/* Notify work thread */
				k_work_submit_to_queue(t_data->work_q ?
					t_data->work_q : &k_sys_work_q,
					t_data->control[i].work);
			}
		}
	}
}

/*
 * Validate the buffer size with adc channels mask. If it is lower than what
 * we need return -ENOSPC.
 */
static int adc_npcx_validate_buffer_size(const struct device *dev,
					const struct adc_sequence *sequence)
{
	uint8_t channels = 0;
	uint32_t mask;
	size_t needed;

	for (mask = BIT(NPCX_ADC_CH_COUNT - 1); mask != 0; mask >>= 1) {
		if (mask & sequence->channels) {
			channels++;
		}
	}

	needed = channels * sizeof(uint16_t);
	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOSPC;
	}

	return 0;
}

static void adc_npcx_start_scan(const struct device *dev)
{
	struct adc_npcx_data *const data = dev->data;
	struct adc_reg *const inst = HAL_INSTANCE(dev);

#ifdef CONFIG_PM
	adc_npcx_pm_policy_state_lock_get(data);
#endif
	/* Turn on ADC first */
	inst->ADCCNF |= BIT(NPCX_ADCCNF_ADCEN);

	/* Stop conversion for scan conversion mode */
	inst->ADCCNF |= BIT(NPCX_ADCCNF_STOP);

	/* Clear end of cyclic conversion event status flag */
	inst->ADCSTS |= BIT(NPCX_ADCSTS_EOCCEV);

	/* Update selected channels in scan mode by channels mask */
	inst->ADCCS |= data->channels;

	/* Select 'Scan' Conversion mode. */
	SET_FIELD(inst->ADCCNF, NPCX_ADCCNF_ADCMD_FIELD,
			NPCX_ADC_SCAN_CONVERSION_MODE);

	/* Enable end of cyclic conversion event interrupt */
	inst->ADCCNF |= BIT(NPCX_ADCCNF_INTECCEN);

	/* Start conversion */
	inst->ADCCNF |= BIT(NPCX_ADCCNF_START);

	LOG_DBG("Start ADC scan conversion and ADCCNF,ADCCS are (%04X,%04X)\n",
			inst->ADCCNF, inst->ADCCS);
}

static int adc_npcx_start_read(const struct device *dev,
					const struct adc_sequence *sequence)
{
	struct adc_npcx_data *const data = dev->data;
	int error = 0;

	if (!sequence->channels ||
	    (sequence->channels & ~BIT_MASK(NPCX_ADC_CH_COUNT))) {
		LOG_ERR("Invalid ADC channels");
		return -EINVAL;
	}

	/* Fixed 10 bit resolution of npcx ADC */
	if (sequence->resolution != 10) {
		LOG_ERR("Unfixed 10 bit ADC resolution");
		return -ENOTSUP;
	}

	error = adc_npcx_validate_buffer_size(dev, sequence);
	if (error) {
		LOG_ERR("ADC buffer size too small");
		return error;
	}

	/* Save ADC sequence sampling buffer and its end pointer address */
	data->buffer = sequence->buffer;
	data->buf_end = data->buffer + sequence->buffer_size / sizeof(uint16_t);

	/* Start ADC conversion */
	adc_context_start_read(&data->ctx, sequence);
	error = adc_context_wait_for_completion(&data->ctx);

	return error;
}

/* ADC api functions */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_npcx_data *const data =
		CONTAINER_OF(ctx, struct adc_npcx_data, ctx);

	data->repeat_buffer = data->buffer;
	data->channels = ctx->sequence.channels;

	/* Start ADC scan conversion */
	adc_npcx_start_scan(data->adc_dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_npcx_data *const data =
		CONTAINER_OF(ctx, struct adc_npcx_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int adc_npcx_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	uint8_t channel_id = channel_cfg->channel_id;

	if (channel_id >= NPCX_ADC_CH_COUNT) {
		LOG_ERR("Invalid channel %d", channel_id);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Unsupported channel acquisition time");
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Unsupported channel gain %d", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Unsupported channel reference");
		return -ENOTSUP;
	}

	LOG_DBG("ADC channel %d configured", channel_cfg->channel_id);
	return 0;
}

static int adc_npcx_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	struct adc_npcx_data *const data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = adc_npcx_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

#if defined(CONFIG_ADC_ASYNC)
static int adc_npcx_read_async(const struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct adc_npcx_data *const data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = adc_npcx_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif /* CONFIG_ADC_ASYNC */

static void adc_npcx_set_repetitive(const struct device *dev, int chnsel,
				    uint8_t enable)
{
	struct adc_reg *const inst = HAL_INSTANCE(dev);
	struct adc_npcx_data *const data = dev->data;
	struct adc_npcx_threshold_data *const t_data = data->threshold_data;

	/* Stop ADC conversion */
	inst->ADCCNF |= BIT(NPCX_ADCCNF_STOP);

	if (enable) {
#ifdef CONFIG_PM
		adc_npcx_pm_policy_state_lock_get(data);
#endif
		/* Turn on ADC */
		inst->ADCCNF |= BIT(NPCX_ADCCNF_ADCEN);
		/* Set ADC conversion code to SW conversion mode */
		SET_FIELD(inst->ADCCNF, NPCX_ADCCNF_ADCMD_FIELD,
			  NPCX_ADC_SCAN_CONVERSION_MODE);
		/* Update number of channel to be converted */
		inst->ADCCS |= BIT(chnsel);
		/* Set conversion type to repetitive (runs continuously) */
		inst->ADCCNF |= BIT(NPCX_ADCCNF_ADCRPTC);

		t_data->repetitive_channels |= BIT(chnsel);
		/* Start conversion */
		inst->ADCCNF |= BIT(NPCX_ADCCNF_START);
	} else {
		inst->ADCCS &= ~BIT(chnsel);

		t_data->repetitive_channels &= ~BIT(chnsel);
		if (!t_data->repetitive_channels) {
			/* No thesholdd active left, disable repetitive mode */
			inst->ADCCNF &= ~BIT(NPCX_ADCCNF_ADCRPTC);
			/* Turn off ADC */
			inst->ADCCNF &= ~BIT(NPCX_ADCCNF_ADCEN);
#ifdef CONFIG_PM
			adc_npcx_pm_policy_state_lock_put(data);
#endif
		} else {
			/* Start conversion again */
			inst->ADCCNF |= BIT(NPCX_ADCCNF_START);
		}
	}
}

int adc_npcx_threshold_ctrl_set_param(const struct device *dev,
				      const uint8_t th_sel,
				      const struct adc_npcx_threshold_param
				      *param)
{
	const struct adc_npcx_config *config = dev->config;
	struct adc_npcx_data *const data = dev->data;
	struct adc_npcx_threshold_data *const t_data = data->threshold_data;
	struct adc_npcx_threshold_control *const t_ctrl =
					&t_data->control[th_sel];
	int ret = 0;

	if (!IS_ENABLED(CONFIG_ADC_CMP_NPCX)) {
		return -EOPNOTSUPP;
	}

	if (!param || th_sel >= config->threshold_count) {
		return -EINVAL;
	}

	adc_context_lock(&data->ctx, false, NULL);
	switch (param->type) {
	case ADC_NPCX_THRESHOLD_PARAM_CHNSEL:
		if (param->val >= NPCX_ADC_CH_COUNT) {
			ret = -EINVAL;
			break;
		}
		t_ctrl->chnsel = (uint8_t)param->val;
		break;

	case ADC_NPCX_THRESHOLD_PARAM_L_H:
		t_ctrl->l_h = !!param->val;
		break;

	case ADC_NPCX_THRESHOLD_PARAM_THVAL:
		if (param->val == 0 || param->val >= ADC_NPCX_THRVAL_MAX) {
			ret = -EINVAL;
			break;
		}
		t_ctrl->thrval = (uint16_t)param->val;
		break;

	case ADC_NPCX_THRESHOLD_PARAM_WORK:
		if (param->val == 0) {
			ret = -EINVAL;
			break;
		}
		t_ctrl->work = (struct k_work *)param->val;
		break;
	default:
		ret = -EINVAL;
	}
	adc_context_release(&data->ctx, 0);
	return ret;
}

static int adc_npcx_threshold_ctrl_setup(const struct device *dev,
					 const uint8_t th_sel)
{
	struct adc_npcx_data *const data = dev->data;
	struct adc_npcx_threshold_data *const t_data = data->threshold_data;
	const struct adc_npcx_config *config = dev->config;
	struct adc_npcx_threshold_control *const t_ctrl =
					&t_data->control[th_sel];

	if (th_sel >= config->threshold_count) {
		return -EINVAL;
	}

	adc_context_lock(&data->ctx, false, NULL);

	if (t_data->active_thresholds & BIT(th_sel)) {
		/* Unable to setup threshold parameters while active */
		adc_context_release(&data->ctx, 0);
		LOG_ERR("Threshold selected (%d) is active!", th_sel);
		return -EBUSY;
	}

	if (t_ctrl->chnsel >= NPCX_ADC_CH_COUNT ||
	    t_ctrl->thrval >= NPCX_ADC_VREF_VOL ||
	    t_ctrl->thrval == 0 || t_ctrl->work == 0) {
		adc_context_release(&data->ctx, 0);
		LOG_ERR("Threshold selected (%d) is not configured!", th_sel);
		return -EINVAL;
	}

	SET_FIELD(THRCTL(dev, (th_sel + 1)),
		  NPCX_THRCTL_CHNSEL, t_ctrl->chnsel);

	if (t_ctrl->l_h) {
		THRCTL(dev, (th_sel + 1)) |= BIT(NPCX_THRCTL_L_H);
	} else {
		THRCTL(dev, (th_sel + 1)) &= ~BIT(NPCX_THRCTL_L_H);
	}
	/* Set the threshold value. */
	SET_FIELD(THRCTL(dev, (th_sel + 1)), NPCX_THRCTL_THRVAL,
		  t_ctrl->thrval);

	adc_context_release(&data->ctx, 0);
	return 0;
}

static int adc_npcx_threshold_enable_irq(const struct device *dev,
				  const uint8_t th_sel)
{
	struct adc_reg *const inst = HAL_INSTANCE(dev);
	struct adc_npcx_data *const data = dev->data;
	const struct adc_npcx_config *config = dev->config;
	struct adc_npcx_threshold_data *const t_data = data->threshold_data;
	struct adc_npcx_threshold_control *const t_ctrl =
					&t_data->control[th_sel];
	uint16_t thrcts;

	if (th_sel >= config->threshold_count) {
		LOG_ERR("Invalid ADC threshold selection! (%d)", th_sel);
		return -EINVAL;
	}

	adc_context_lock(&data->ctx, false, NULL);
	if (t_ctrl->chnsel >= NPCX_ADC_CH_COUNT ||
	    t_ctrl->thrval >= NPCX_ADC_VREF_VOL ||
	    t_ctrl->thrval == 0 || t_ctrl->work == 0) {
		adc_context_release(&data->ctx, 0);
		LOG_ERR("Threshold selected (%d) is not configured!", th_sel);
		return -EINVAL;
	}

	/* Record new active threshold */
	t_data->active_thresholds |= BIT(th_sel);

	/* avoid clearing other threshold status */
	thrcts = inst->THRCTS & ~GENMASK(config->threshold_count - 1, 0);

	/* Enable threshold detection */
	THRCTL(dev, (th_sel + 1)) |= BIT(NPCX_THRCTL_THEN);

	/* clear threshold status */
	thrcts |= BIT(th_sel);

	/* set enable threshold status */
	thrcts |= BIT(NPCX_THRCTS_THR1_IEN + th_sel);

	inst->THRCTS = thrcts;

	adc_npcx_set_repetitive(dev, t_data->control[th_sel].chnsel, true);

	adc_context_release(&data->ctx, 0);
	return 0;
}

int adc_npcx_threshold_disable_irq(const struct device *dev,
				   const uint8_t th_sel)
{
	struct adc_reg *const inst = HAL_INSTANCE(dev);
	const struct adc_npcx_config *config = dev->config;
	struct adc_npcx_data *const data = dev->data;
	struct adc_npcx_threshold_data *const t_data = data->threshold_data;
	uint16_t thrcts;

	if (!IS_ENABLED(CONFIG_ADC_CMP_NPCX)) {
		return -EOPNOTSUPP;
	}

	if (th_sel >= config->threshold_count) {
		LOG_ERR("Invalid ADC threshold selection! (%d)", th_sel);
		return -EINVAL;
	}

	adc_context_lock(&data->ctx, false, NULL);
	if (!(t_data->active_thresholds & BIT(th_sel))) {
		adc_context_release(&data->ctx, 0);
		LOG_ERR("Threshold selection (%d) is not enabled", th_sel);
		return -ENODEV;
	}
	/* avoid clearing other threshold status */
	thrcts = inst->THRCTS & ~GENMASK(config->threshold_count - 1, 0);

	/* set enable threshold status */
	thrcts &= ~BIT(NPCX_THRCTS_THR1_IEN + th_sel);
	inst->THRCTS = thrcts;

	/* Disable threshold detection */
	THRCTL(dev, (th_sel + 1)) &= ~BIT(NPCX_THRCTL_THEN);

	/* Update active threshold */
	t_data->active_thresholds &= ~BIT(th_sel);

	adc_npcx_set_repetitive(dev, t_data->control[th_sel].chnsel, false);

	adc_context_release(&data->ctx, 0);

	return 0;
}

int adc_npcx_threshold_ctrl_enable(const struct device *dev, uint8_t th_sel,
				   const bool enable)
{
	int ret;

	if (!IS_ENABLED(CONFIG_ADC_CMP_NPCX)) {
		return -EOPNOTSUPP;
	}

	/* Enable/Disable threshold IRQ */
	if (enable) {
		/* Set control threshold registers */
		ret = adc_npcx_threshold_ctrl_setup(dev, th_sel);
		if (ret) {
			return ret;
		}
		ret = adc_npcx_threshold_enable_irq(dev, th_sel);
	} else {
		ret = adc_npcx_threshold_disable_irq(dev, th_sel);
	}
	return ret;
}

int adc_npcx_threshold_mv_to_thrval(uint32_t val_mv, uint32_t *thrval)
{
	if (!IS_ENABLED(CONFIG_ADC_CMP_NPCX)) {
		return -EOPNOTSUPP;
	}

	if (val_mv >= NPCX_ADC_VREF_VOL) {
		return -EINVAL;
	}

	*thrval = (val_mv << ADC_NPCX_THRVAL_RESOLUTION) /
		NPCX_ADC_VREF_VOL;
	return 0;
}

/* ADC driver registration */
static const struct adc_driver_api adc_npcx_driver_api = {
	.channel_setup = adc_npcx_channel_setup,
	.read = adc_npcx_read,
#if defined(CONFIG_ADC_ASYNC)
	.read_async = adc_npcx_read_async,
#endif
	.ref_internal = NPCX_ADC_VREF_VOL,
};

static int adc_npcx_init(const struct device *dev);

PINCTRL_DT_INST_DEFINE(0);
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	"only one 'nuvoton_npcx_adc' compatible node may be present");

static const struct adc_npcx_config adc_npcx_cfg_0 = {
	.base = DT_INST_REG_ADDR(0),
	.clk_cfg = NPCX_DT_CLK_CFG_ITEM(0),
	.threshold_count = DT_INST_PROP(0, threshold_count),
	.threshold_reg_offset = DT_INST_PROP(0, threshold_reg_offset),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static struct adc_npcx_threshold_data threshold_data_0;

static struct adc_npcx_data adc_npcx_data_0 = {
	ADC_CONTEXT_INIT_TIMER(adc_npcx_data_0, ctx),
	ADC_CONTEXT_INIT_LOCK(adc_npcx_data_0, ctx),
	ADC_CONTEXT_INIT_SYNC(adc_npcx_data_0, ctx),
};

#if defined(CONFIG_ADC_CMP_NPCX_WORKQUEUE)
struct k_work_q adc_npcx_work_q;

static K_KERNEL_STACK_DEFINE(adc_npcx_work_q_stack,
			CONFIG_ADC_CMP_NPCX_WORKQUEUE_STACK_SIZE);

static int adc_npcx_init_cmp_work_q(const struct device *dev)
{
	ARG_UNUSED(dev);
	struct k_work_queue_config cfg = {
		.name = "adc_cmp_work",
		.no_yield = false,
	};

	k_work_queue_start(&adc_npcx_work_q,
			   adc_npcx_work_q_stack,
			   K_KERNEL_STACK_SIZEOF(adc_npcx_work_q_stack),
			   CONFIG_ADC_CMP_NPCX_WORKQUEUE_PRIORITY, &cfg);

	threshold_data_0.work_q = &adc_npcx_work_q;
	return 0;
}

SYS_INIT(adc_npcx_init_cmp_work_q, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY);
#endif

DEVICE_DT_INST_DEFINE(0,
		    adc_npcx_init, NULL,
		    &adc_npcx_data_0, &adc_npcx_cfg_0,
		    PRE_KERNEL_1,
		    CONFIG_ADC_INIT_PRIORITY,
		    &adc_npcx_driver_api);

static int adc_npcx_init(const struct device *dev)
{
	const struct adc_npcx_config *const config = dev->config;
	struct adc_npcx_data *const data = dev->data;
	struct adc_reg *const inst = HAL_INSTANCE(dev);
	const struct device *const clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	int prescaler = 0, ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Save ADC device in data */
	data->adc_dev = dev;

	/* Turn on device clock first and get source clock freq. */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t)
							&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on ADC clock fail %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)
			&config->clk_cfg, &data->input_clk);
	if (ret < 0) {
		LOG_ERR("Get ADC clock rate error %d", ret);
		return ret;
	}

	/* Configure the ADC clock */
	prescaler = DIV_ROUND_UP(data->input_clk, NPCX_ADC_CLK);
	if (prescaler > 0x40) {
		prescaler = 0x40;
	}

	/* Set Core Clock Division Factor in order to obtain the ADC clock */
	SET_FIELD(inst->ATCTL, NPCX_ATCTL_SCLKDIV_FIELD, prescaler - 1);

	/* Set regular ADC delay */
	SET_FIELD(inst->ATCTL, NPCX_ATCTL_DLY_FIELD, ADC_REGULAR_DLY_VAL);

	/* Set ADC speed sequentially */
	inst->ADCCNF2 = ADC_REGULAR_ADCCNF2_VAL;
	inst->GENDLY = ADC_REGULAR_GENDLY_VAL;
	inst->MEAST = ADC_REGULAR_MEAST_VAL;

	if (IS_ENABLED(CONFIG_ADC_CMP_NPCX)) {
		data->threshold_data = &threshold_data_0;
	}

	/* Configure ADC interrupt and enable it */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), adc_npcx_isr,
			    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	/* Initialize mutex of ADC channels */
	adc_context_unlock_unconditionally(&data->ctx);

	/* Configure pin-mux for ADC device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("ADC pinctrl setup failed (%d)", ret);
		return ret;
	}

	return 0;
}
