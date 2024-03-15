/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_adc

#include <zephyr/kernel.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <NuMicro.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

LOG_MODULE_REGISTER(adc_numaker, CONFIG_ADC_LOG_LEVEL);

/* Device config */
struct adc_numaker_config {
	/* eadc base address */
	EADC_T *eadc_base;
	uint8_t channel_cnt;
	const struct reset_dt_spec reset;
	/* clock configuration */
	uint32_t clk_modidx;
	uint32_t clk_src;
	uint32_t clk_div;
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pincfg;
	void (*irq_config_func)(const struct device *dev);
};

/* Driver context/data */
struct adc_numaker_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *buf_end;
	uint16_t *repeat_buffer;
	bool is_differential;
	uint32_t channels;
};

static int adc_numaker_channel_setup(const struct device *dev,
				     const struct adc_channel_cfg *chan_cfg)
{
	const struct adc_numaker_config *cfg = dev->config;
	struct adc_numaker_data *data = dev->data;

	if (chan_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Not support acquisition time");
		return -ENOTSUP;
	}

	if (chan_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Not support channel gain");
		return -ENOTSUP;
	}

	if (chan_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Not support channel reference");
		return -ENOTSUP;
	}

	if (chan_cfg->channel_id >= cfg->channel_cnt) {
		LOG_ERR("Invalid channel (%u)", chan_cfg->channel_id);
		return -EINVAL;
	}

	data->is_differential = (chan_cfg->differential) ? true : false;

	return 0;
}

static int m_adc_numaker_validate_buffer_size(const struct device *dev,
					      const struct adc_sequence *sequence)
{
	const struct adc_numaker_config *cfg = dev->config;
	uint8_t channel_cnt = 0;
	uint32_t mask;
	size_t needed_size;

	for (mask = BIT(cfg->channel_cnt - 1); mask != 0; mask >>= 1) {
		if (mask & sequence->channels) {
			channel_cnt++;
		}
	}

	needed_size = channel_cnt * sizeof(uint16_t);
	if (sequence->options) {
		needed_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed_size) {
		return -ENOBUFS;
	}

	return 0;
}

static void adc_numaker_isr(const struct device *dev)
{
	const struct adc_numaker_config *cfg = dev->config;
	EADC_T *eadc = cfg->eadc_base;
	struct adc_numaker_data *const data = dev->data;
	uint32_t channel_mask = data->channels;
	uint32_t module_mask = channel_mask;
	uint32_t module_id;
	uint16_t conv_data;
	uint32_t pend_flag;

	/* Clear pending flag first */
	pend_flag = eadc->PENDSTS;
	eadc->PENDSTS = pend_flag;
	LOG_DBG("ADC ISR pend flag: 0x%X\n", pend_flag);
	LOG_DBG("ADC ISR STATUS2[0x%x] STATUS3[0x%x]", eadc->STATUS2, eadc->STATUS3);
	/* Complete the conversion of channels.
	 * Check EAC idle by EADC_STATUS2_BUSY_Msk
	 * Check trigger source coming by EADC_STATUS2_ADOVIF_Msk
	 * Confirm all sample modules are idle by EADC_STATUS2_ADOVIF_Msk
	 */
	if (!(eadc->STATUS2 & EADC_STATUS2_BUSY_Msk) &&
	    ((eadc->STATUS3 & EADC_STATUS3_CURSPL_Msk) == EADC_STATUS3_CURSPL_Msk)) {
		/* Stop the conversion for sample module */
		EADC_STOP_CONV(eadc, module_mask);

		/* Disable sample module A/D ADINT0 interrupt. */
		EADC_DISABLE_INT(eadc, BIT0);

		/* Disable the sample module ADINT0 interrupt source */
		EADC_DISABLE_SAMPLE_MODULE_INT(eadc, 0, module_mask);

		/* Get conversion data of each sample module for selected channel */
		while (module_mask) {
			module_id = find_lsb_set(module_mask) - 1;

			conv_data = EADC_GET_CONV_DATA(eadc, module_id);
			if (data->buffer < data->buf_end) {
				*data->buffer++ = conv_data;
				LOG_DBG("ADC ISR id=%d, data=0x%x", module_id, conv_data);
			}
			module_mask &= ~BIT(module_id);

			/* Disable all channels on each sample module */
			eadc->SCTL[module_id] = 0;
		}

		/* Disable ADC */
		EADC_Close(eadc);

		/* Inform sampling is done */
		adc_context_on_sampling_done(&data->ctx, data->dev);
	}

	/* Clear the A/D ADINT0 interrupt flag */
	EADC_CLR_INT_FLAG(eadc, EADC_STATUS2_ADIF0_Msk);
}

static void m_adc_numaker_start_scan(const struct device *dev)
{
	const struct adc_numaker_config *cfg = dev->config;
	EADC_T *eadc = cfg->eadc_base;
	struct adc_numaker_data *const data = dev->data;
	uint32_t channel_mask = data->channels;
	uint32_t module_mask = channel_mask;
	uint32_t channel_id;
	uint32_t module_id;

	/* Configure the sample module, analog input channel and software trigger source */
	while (channel_mask) {
		channel_id = find_lsb_set(channel_mask) - 1;
		module_id = channel_id;
		channel_mask &= ~BIT(channel_id);
		EADC_ConfigSampleModule(eadc, module_id,
					EADC_SOFTWARE_TRIGGER, channel_id);
	}

	/* Clear the A/D ADINT0 interrupt flag for safe */
	EADC_CLR_INT_FLAG(eadc, EADC_STATUS2_ADIF0_Msk);

	/* Enable sample module A/D ADINT0 interrupt. */
	EADC_ENABLE_INT(eadc, BIT0);

	/* Enable sample module interrupt ADINT0. */
	EADC_ENABLE_SAMPLE_MODULE_INT(eadc, 0, module_mask);

	/* Start conversion */
	EADC_START_CONV(eadc, module_mask);
}

/* Implement ADC API functions of adc_context.h
 * - adc_context_start_sampling()
 * - adc_context_update_buffer_pointer()
 */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_numaker_data *const data =
		CONTAINER_OF(ctx, struct adc_numaker_data, ctx);

	data->repeat_buffer = data->buffer;
	data->channels = ctx->sequence.channels;

	/* Start ADC conversion for sample modules/channels */
	m_adc_numaker_start_scan(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_numaker_data *data =
		CONTAINER_OF(ctx, struct adc_numaker_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int m_adc_numaker_start_read(const struct device *dev,
				  const struct adc_sequence *sequence)
{
	const struct adc_numaker_config *cfg = dev->config;
	struct adc_numaker_data *data = dev->data;
	EADC_T *eadc = cfg->eadc_base;
	int err;

	err = m_adc_numaker_validate_buffer_size(dev, sequence);
	if (err) {
		LOG_ERR("ADC provided buffer is too small");
		return err;
	}

	if (!sequence->resolution) {
		LOG_ERR("ADC resolution is not valid");
		return -EINVAL;
	}
	LOG_DBG("Configure resolution=%d", sequence->resolution);

	/* Enable the A/D converter */
	if (data->is_differential) {
		err = EADC_Open(eadc, EADC_CTL_DIFFEN_DIFFERENTIAL);
	} else {
		err = EADC_Open(eadc, EADC_CTL_DIFFEN_SINGLE_END);
	}

	if (err) {
		LOG_ERR("ADC Open fail (%u)", err);
		return -ENODEV;
	}

	data->buffer = sequence->buffer;
	data->buf_end = data->buffer + sequence->buffer_size / sizeof(uint16_t);

	/* Start ADC conversion */
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_numaker_read(const struct device *dev,
			    const struct adc_sequence *sequence)
{
	struct adc_numaker_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, false, NULL);
	err = m_adc_numaker_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_numaker_read_async(const struct device *dev,
				  const struct adc_sequence *sequence,
				  struct k_poll_signal *async)
{
	struct adc_numaker_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, true, async);
	err = m_adc_numaker_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}
#endif

static const struct adc_driver_api adc_numaker_driver_api = {
	.channel_setup = adc_numaker_channel_setup,
	.read = adc_numaker_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_numaker_read_async,
#endif
};

static int adc_numaker_init(const struct device *dev)
{
	const struct adc_numaker_config *cfg = dev->config;
	struct adc_numaker_data *data = dev->data;
	int err;
	struct numaker_scc_subsys scc_subsys;

	/* Validate this module's reset object */
	if (!device_is_ready(cfg->reset.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}

	data->dev = dev;

	SYS_UnlockReg();

	/* CLK controller */
	memset(&scc_subsys, 0x00, sizeof(scc_subsys));
	scc_subsys.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC;
	scc_subsys.pcc.clk_modidx = cfg->clk_modidx;
	scc_subsys.pcc.clk_src = cfg->clk_src;
	scc_subsys.pcc.clk_div = cfg->clk_div;

	/* Equivalent to CLK_EnableModuleClock() */
	err = clock_control_on(cfg->clk_dev, (clock_control_subsys_t)&scc_subsys);
	if (err != 0) {
		goto done;
	}
	/* Equivalent to CLK_SetModuleClock() */
	err = clock_control_configure(cfg->clk_dev, (clock_control_subsys_t)&scc_subsys, NULL);
	if (err != 0) {
		goto done;
	}

	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		LOG_ERR("Failed to apply pinctrl state");
		goto done;
	}

	/* Reset EADC to default state, same as BSP's SYS_ResetModule(id_rst) */
	reset_line_toggle_dt(&cfg->reset);

	/* Enable NVIC */
	cfg->irq_config_func(dev);

	/* Init mutex of adc_context */
	adc_context_unlock_unconditionally(&data->ctx);

done:
	SYS_LockReg();
	return err;
}

#define ADC_NUMAKER_IRQ_CONFIG_FUNC(n)                                                       \
	static void adc_numaker_irq_config_func_##n(const struct device *dev)                \
	{                                                                                    \
		IRQ_CONNECT(DT_INST_IRQN(n),                                                 \
			    DT_INST_IRQ(n, priority),                                        \
			    adc_numaker_isr,                                                 \
			    DEVICE_DT_INST_GET(n), 0);                                       \
											     \
		irq_enable(DT_INST_IRQN(n));                                                 \
	}

#define ADC_NUMAKER_INIT(inst)						                     \
	PINCTRL_DT_INST_DEFINE(inst);                                                        \
	ADC_NUMAKER_IRQ_CONFIG_FUNC(inst)                                                    \
											     \
	static const struct adc_numaker_config adc_numaker_cfg_##inst = {                    \
		.eadc_base = (EADC_T *)DT_INST_REG_ADDR(inst),                               \
		.channel_cnt = DT_INST_PROP(inst, channels),                                 \
		.reset = RESET_DT_SPEC_INST_GET(inst),                                       \
		.clk_modidx = DT_INST_CLOCKS_CELL(inst, clock_module_index),                 \
		.clk_src = DT_INST_CLOCKS_CELL(inst, clock_source),                          \
		.clk_div = DT_INST_CLOCKS_CELL(inst, clock_divider),                         \
		.clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))),              \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                              \
		.irq_config_func = adc_numaker_irq_config_func_##inst,                       \
	};                                                                                   \
											     \
	static struct adc_numaker_data adc_numaker_data_##inst = {			     \
		ADC_CONTEXT_INIT_TIMER(adc_numaker_data_##inst, ctx),			     \
		ADC_CONTEXT_INIT_LOCK(adc_numaker_data_##inst, ctx),			     \
		ADC_CONTEXT_INIT_SYNC(adc_numaker_data_##inst, ctx),			     \
	};									             \
	DEVICE_DT_INST_DEFINE(inst,                                                          \
			      &adc_numaker_init, NULL,                                       \
			      &adc_numaker_data_##inst, &adc_numaker_cfg_##inst,             \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,			     \
			      &adc_numaker_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_NUMAKER_INIT)
