/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5912_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <zephyr/drivers/pinctrl.h>

#include "reg/reg_adc.h"

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_rts5912, CONFIG_ADC_LOG_LEVEL);

#define RTS5912_ADC_MAX_CHAN        12
#define RTS5912_ADC_POLLING_TIME_MS 1
#define RTS5912_ADC_ENABLE_TIMEOUT  100

struct adc_rts5912_config {
	volatile struct adc_regs *regs;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_CLOCK_CONTROL
	const struct device *clk_dev;
	struct rts5912_sccon_subsys sccon_cfg;
#endif
};

struct adc_rts5912_data {
	struct adc_context ctx;
	const struct device *adc_dev;
	volatile uint16_t *buffer;
	volatile uint16_t *repeat_buffer;
	uint32_t channels;
};

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_rts5912_data *data = CONTAINER_OF(ctx, struct adc_rts5912_data, ctx);
	const struct device *adc_dev = data->adc_dev;
	const struct adc_rts5912_config *const cfg = adc_dev->config;
	volatile struct adc_regs *regs = cfg->regs;

	data->repeat_buffer = data->buffer;

	regs->ctrl |= ADC_CTRL_SGLDNINTEN;
	regs->ctrl |= ADC_CTRL_START;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_rts5912_data *data = CONTAINER_OF(ctx, struct adc_rts5912_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int adc_rts5912_channel_setup(const struct device *dev,
				     const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_rts5912_config *const cfg = dev->config;
	volatile struct adc_regs *regs = cfg->regs;

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Conversion time not supported!");
		return -EINVAL;
	}

	if (channel_cfg->channel_id >= RTS5912_ADC_MAX_CHAN) {
		LOG_ERR("Channel %d not supported!", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("ADC gain not supported!");
		return -EINVAL;
	}

	uint8_t channel_id = channel_cfg->channel_id;

	regs->chctrl |= ((0x01ul << channel_id) | (ADC_CHCTRL_LPFBP << channel_id));
	LOG_DBG("CHCTRL = 0x%08x", regs->chctrl);

	return 0;
}

static bool adc_rts5912_validate_buffer_size(const struct adc_sequence *sequence)
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

static int adc_rts5912_enable(const struct device *dev)
{
	const struct adc_rts5912_config *const cfg = dev->config;
	volatile struct adc_regs *regs = cfg->regs;
	int64_t st = k_uptime_get();

	regs->ctrl |= ADC_CTRL_EN;
	while ((k_uptime_get() - st) < RTS5912_ADC_ENABLE_TIMEOUT) {
		if (regs->sts & ADC_STS_RDY) {
			return 0;
		}
		k_msleep(RTS5912_ADC_POLLING_TIME_MS);
	}

	LOG_ERR("ADC enable timeout");
	regs->ctrl &= ~ADC_CTRL_EN;

	return -EIO;
}

static int adc_rts5912_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_rts5912_data *const data = dev->data;

	if (sequence->channels & ~BIT_MASK(RTS5912_ADC_MAX_CHAN)) {
		LOG_ERR("Incorrect channels, bitmask 0x%x", sequence->channels);
		return -EINVAL;
	}

	if (sequence->channels == 0UL) {
		LOG_ERR("No channel selected");
		return -EINVAL;
	}

	if (!adc_rts5912_validate_buffer_size(sequence)) {
		LOG_ERR("Incorrect buffer size");
		return -ENOMEM;
	}

	data->channels = sequence->channels;
	data->buffer = sequence->buffer;

	if (adc_rts5912_enable(dev) < 0) {
		return -EIO;
	}

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_rts5912_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_rts5912_data *const data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = adc_rts5912_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static void rts5912_adc_get_sample(const struct device *dev)
{
	const struct adc_rts5912_config *const cfg = dev->config;
	volatile struct adc_regs *regs = cfg->regs;
	struct adc_rts5912_data *const data = dev->data;
	uint32_t idx;
	uint32_t channels = data->channels;
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

		*data->buffer = ((uint16_t)regs->chdata[idx] & ADC_CHDATA_RESULT_Msk);
		data->buffer++;

		LOG_DBG("idx=%d, data=%x", idx, regs->chdata[idx]);

		channels &= ~BIT(idx);
		bit = find_lsb_set(channels);
	}
}

static void adc_rts5912_single_isr(const struct device *dev)
{
	const struct adc_rts5912_config *const cfg = dev->config;
	volatile struct adc_regs *regs = cfg->regs;
	struct adc_rts5912_data *const data = dev->data;

	if (regs->sts & ADC_STS_SGLDN) {
		LOG_DBG("single done interrupt triggered.");

		regs->ctrl &= ~(ADC_CTRL_SGLDNINTEN);
		regs->sts &= regs->sts;

		rts5912_adc_get_sample(dev);

		regs->ctrl &= ~ADC_CTRL_EN;
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

static int adc_rts5912_init(const struct device *dev)
{
	const struct adc_rts5912_config *const cfg = dev->config;
	struct adc_rts5912_data *const data = dev->data;
	volatile struct adc_regs *regs = cfg->regs;

	int ret;

	data->adc_dev = dev;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("rts5912 ADC pinctrl setup failed (%d)", ret);
		return ret;
	}

#ifdef CONFIG_CLOCK_CONTROL
	if (!device_is_ready(cfg->clk_dev)) {
		LOG_ERR("clock \"%s\" device not ready", cfg->clk_dev->name);
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clk_dev, (clock_control_subsys_t)&cfg->sccon_cfg);
	if (ret != 0) {
		LOG_ERR("clock power on fail");
		return ret;
	}
#endif

	regs->ctrl = ADC_CTRL_RST;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), adc_rts5912_single_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define DEV_CONFIG_CLK_DEV_INIT(n)                                                                 \
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                          \
	.sccon_cfg = {                                                                             \
		.clk_grp = DT_INST_CLOCKS_CELL(n, clk_grp),                                        \
		.clk_idx = DT_INST_CLOCKS_CELL(n, clk_idx),                                        \
	}

#define ADC_RTS5912_INIT(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static DEVICE_API(adc, adc_rts5912_api_##n) = {                                            \
		.channel_setup = adc_rts5912_channel_setup,                                        \
		.read = adc_rts5912_read,                                                          \
		.ref_internal = DT_INST_PROP(n, vref_mv),                                          \
	};                                                                                         \
                                                                                                   \
	static struct adc_rts5912_config adc_rts5912_dev_cfg_##n = {                               \
		.regs = (struct adc_regs *)(DT_INST_REG_ADDR(n)),                                  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		DEV_CONFIG_CLK_DEV_INIT(n)};                                                       \
                                                                                                   \
	static struct adc_rts5912_data adc_rts5912_dev_data_##n = {                                \
		ADC_CONTEXT_INIT_TIMER(adc_rts5912_dev_data_##n, ctx),                             \
		ADC_CONTEXT_INIT_LOCK(adc_rts5912_dev_data_##n, ctx),                              \
		ADC_CONTEXT_INIT_SYNC(adc_rts5912_dev_data_##n, ctx),                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, adc_rts5912_init, NULL, &adc_rts5912_dev_data_##n,                \
			      &adc_rts5912_dev_cfg_##n, PRE_KERNEL_1, CONFIG_ADC_INIT_PRIORITY,    \
			      &adc_rts5912_api_##n);

DT_INST_FOREACH_STATUS_OKAY(ADC_RTS5912_INIT)
