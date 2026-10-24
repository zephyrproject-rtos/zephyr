/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER 1
#include "adc_context.h"

#define DT_DRV_COMPAT microchip_adc_g2

LOG_MODULE_REGISTER(adc_mchp_g2, CONFIG_ADC_LOG_LEVEL);

#define ADC_MCHP_G2_SYNC_TIMEOUT_US 10000

#define ADC_MCHP_G2_RESULT_BITS 12
#define ADC_MCHP_G2_RESULT_MASK BIT_MASK(ADC_MCHP_G2_RESULT_BITS)

/* CORCTRL.SELRES, data sheet 36.7.5. */
#define ADC_MCHP_G2_SELRES_6BIT  0
#define ADC_MCHP_G2_SELRES_8BIT  1
#define ADC_MCHP_G2_SELRES_10BIT 2
#define ADC_MCHP_G2_SELRES_12BIT 3

/* CTRLD.VREFSEL, data sheet 36.5.2.5. */
#define ADC_MCHP_G2_VREFSEL_AVDD  0
#define ADC_MCHP_G2_VREFSEL_VREFH 1

#define ADC_MCHP_G2_RESOLUTION_INVALID 0
#define ADC_MCHP_G2_VREFSEL_INVALID    0xFFU

/* CHNCFG4/5.TRGSRCk, four bits per channel. */
#define ADC_MCHP_G2_TRGSRC_NONE   0
#define ADC_MCHP_G2_TRGSRC_GSWTRG 1
#define ADC_MCHP_G2_TRGSRC_BITS   4

struct adc_mchp_g2_config {
	adc_registers_t *regs;
	fuses_calotp_registers_t *fuses;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
	void (*irq_config)(void);
	uint16_t declared_channels;
	uint8_t num_channels;
	uint8_t ctl_clk_div;
	uint8_t adc_clk_div;
	uint16_t sample_count;
	uint8_t wakeup_exp;
};

struct adc_mchp_g2_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *buffer_repeat;
	uint16_t configured_channels;
	uint16_t pending_channels;
	uint16_t active_channels;
	uint8_t resolution;
	uint8_t vref_sel;
};

static bool adc_mchp_g2_wait(volatile const uint32_t *reg, uint32_t mask, bool want_set,
			      const char *what)
{
	if (WAIT_FOR(((*reg & mask) != 0) == want_set, ADC_MCHP_G2_SYNC_TIMEOUT_US,
		     k_busy_wait(1))) {
		return true;
	}

	LOG_ERR("timeout waiting for %s: register reads 0x%08x, mask 0x%08x", what, *reg, mask);

	return false;
}

/* CHNCFG4/5 are enable-protected: call with the converter disabled. */
static void adc_mchp_g2_program_triggers(adc_registers_t *regs, uint16_t channels)
{
	uint32_t cfg4 = 0;
	uint32_t cfg5 = 0;

	for (uint8_t k = 0; k < 8; k++) {
		if ((channels & BIT(k)) != 0) {
			cfg4 |= ADC_MCHP_G2_TRGSRC_GSWTRG << (k * ADC_MCHP_G2_TRGSRC_BITS);
		}
	}

	for (uint8_t k = 8; k < 16; k++) {
		if ((channels & BIT(k)) != 0) {
			cfg5 |= ADC_MCHP_G2_TRGSRC_GSWTRG
				<< ((k - 8) * ADC_MCHP_G2_TRGSRC_BITS);
		}
	}

	regs->CONFIG[0].ADC_CHNCFG4 = cfg4;
	regs->CONFIG[0].ADC_CHNCFG5 = cfg5;
}

static int adc_mchp_g2_disable(adc_registers_t *regs)
{
	regs->ADC_CTRLA = ADC_CTRLA_ANAEN_Msk;

	if (!adc_mchp_g2_wait(&regs->ADC_SYNCBUSY, ADC_SYNCBUSY_ENABLE_Msk, false,
			      "SYNCBUSY.ENABLE on disable")) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int adc_mchp_g2_enable(adc_registers_t *regs)
{
	regs->ADC_CTRLA = ADC_CTRLA_ANAEN_Msk | ADC_CTRLA_ENABLE_Msk;

	if (!adc_mchp_g2_wait(&regs->ADC_SYNCBUSY, ADC_SYNCBUSY_ENABLE_Msk, false,
			      "SYNCBUSY.ENABLE")) {
		return -ETIMEDOUT;
	}

	if (!adc_mchp_g2_wait(&regs->ADC_CTLINTFLAG, ADC_CTLINTFLAG_VREFRDY_Msk, true,
			      "CTLINTFLAG.VREFRDY")) {
		return -ETIMEDOUT;
	}

	if (!adc_mchp_g2_wait(&regs->ADC_CTLINTFLAG, BIT(ADC_CTLINTFLAG_CRRDY_Pos), true,
			      "CTLINTFLAG.CRRDY0")) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int adc_mchp_g2_set_resolution(const struct device *dev, uint8_t resolution)
{
	const struct adc_mchp_g2_config *cfg = dev->config;
	struct adc_mchp_g2_data *data = dev->data;
	adc_registers_t *regs = cfg->regs;
	uint32_t selres;
	int ret;

	if (data->resolution == resolution) {
		return 0;
	}

	switch (resolution) {
	case 8:
		selres = ADC_MCHP_G2_SELRES_8BIT;
		break;
	case 10:
		selres = ADC_MCHP_G2_SELRES_10BIT;
		break;
	case 12:
		selres = ADC_MCHP_G2_SELRES_12BIT;
		break;
	default:
		LOG_ERR("resolution %u is not one of 8, 10 or 12", resolution);
		return -EINVAL;
	}

	ret = adc_mchp_g2_disable(regs);
	if (ret < 0) {
		return ret;
	}

	data->resolution = ADC_MCHP_G2_RESOLUTION_INVALID;

	regs->CONFIG[0].ADC_CORCTRL = ADC_CORCTRL_SAMC(cfg->sample_count) |
				      ADC_CORCTRL_SELRES(selres) |
				      ADC_CORCTRL_ADCDIV(cfg->adc_clk_div);

	ret = adc_mchp_g2_enable(regs);
	if (ret < 0) {
		return ret;
	}

	data->resolution = resolution;

	return 0;
}

static int adc_mchp_g2_set_vref(const struct device *dev, uint32_t vref_sel)
{
	const struct adc_mchp_g2_config *cfg = dev->config;
	struct adc_mchp_g2_data *data = dev->data;
	adc_registers_t *regs = cfg->regs;
	int ret;

	if (data->vref_sel == vref_sel) {
		return 0;
	}

	ret = adc_mchp_g2_disable(regs);
	if (ret < 0) {
		return ret;
	}

	data->vref_sel = ADC_MCHP_G2_VREFSEL_INVALID;

	regs->ADC_CTRLD = ADC_CTRLD_CTLCKDIV(cfg->ctl_clk_div) | ADC_CTRLD_VREFSEL(vref_sel) |
			  ADC_CTRLD_WKUPEXP(cfg->wakeup_exp) | ADC_CTRLD_CHNEN0_Msk;

	/* CRRDY0 must clear first, or the post-enable wait passes on the stale flag. */
	if (!adc_mchp_g2_wait(&regs->ADC_CTLINTFLAG, BIT(ADC_CTLINTFLAG_CRRDY_Pos), false,
			      "CTLINTFLAG.CRRDY0 clearing")) {
		return -ETIMEDOUT;
	}

	regs->ADC_CTRLD = ADC_CTRLD_CTLCKDIV(cfg->ctl_clk_div) | ADC_CTRLD_VREFSEL(vref_sel) |
			  ADC_CTRLD_WKUPEXP(cfg->wakeup_exp) | ADC_CTRLD_ANLEN0_Msk |
			  ADC_CTRLD_CHNEN0_Msk;

	ret = adc_mchp_g2_enable(regs);
	if (ret < 0) {
		return ret;
	}

	data->vref_sel = vref_sel;

	return 0;
}

static int adc_mchp_g2_channel_setup_locked(const struct device *dev,
					     const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_mchp_g2_config *cfg = dev->config;
	struct adc_mchp_g2_data *data = dev->data;
	uint32_t wanted_vref;
	int ret;

	if (channel_cfg->channel_id >= cfg->num_channels ||
	    (cfg->declared_channels & BIT(channel_cfg->channel_id)) == 0) {
		LOG_ERR("channel %u is not declared in devicetree", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("this converter has no input gain stage");
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("differential inputs are not implemented");
		return -EINVAL;
	}

	switch (channel_cfg->reference) {
	case ADC_REF_VDD_1:
		wanted_vref = ADC_MCHP_G2_VREFSEL_AVDD;
		break;
	case ADC_REF_EXTERNAL0:
		wanted_vref = ADC_MCHP_G2_VREFSEL_VREFH;
		break;
	default:
		LOG_ERR("reference must be ADC_REF_VDD_1 (AVDD) or ADC_REF_EXTERNAL0 (VREFH)");
		return -EINVAL;
	}

	if (wanted_vref != data->vref_sel && data->configured_channels != 0) {
		LOG_ERR("reference does not match the one already programmed");
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT &&
	    channel_cfg->acquisition_time !=
		    ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, cfg->sample_count + 2)) {
		LOG_ERR("acquisition time belongs to the core, set sample-count instead");
		return -EINVAL;
	}

	if (wanted_vref != data->vref_sel) {
		ret = adc_mchp_g2_set_vref(dev, wanted_vref);
		if (ret < 0) {
			return ret;
		}
	}

	data->configured_channels |= BIT(channel_cfg->channel_id);

	return 0;
}

static int adc_mchp_g2_channel_setup(const struct device *dev,
				      const struct adc_channel_cfg *channel_cfg)
{
	struct adc_mchp_g2_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);
	ret = adc_mchp_g2_channel_setup_locked(dev, channel_cfg);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int adc_mchp_g2_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_mchp_g2_data *data = dev->data;
	uint32_t needed;
	int ret;

	if (sequence->channels == 0 || (sequence->channels & ~data->configured_channels) != 0) {
		LOG_ERR("sequence names channels 0x%04x that were never set up",
			sequence->channels & ~data->configured_channels);
		return -EINVAL;
	}

	if (sequence->oversampling != 0) {
		LOG_ERR("hardware oversampling is not implemented");
		return -EINVAL;
	}

	/* sequence->calibrate is ignored: the calibration comes from OTP at init. */

	needed = POPCOUNT(sequence->channels) * sizeof(uint16_t);
	if (sequence->options != NULL) {
		needed *= 1 + sequence->options->extra_samplings;
	}

	if (sequence->buffer_size < needed) {
		LOG_ERR("buffer holds %zu bytes, the sequence needs %u", sequence->buffer_size,
			needed);
		return -ENOMEM;
	}

	ret = adc_mchp_g2_set_resolution(dev, sequence->resolution);
	if (ret < 0) {
		return ret;
	}

	data->active_channels = sequence->channels;
	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_mchp_g2_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_mchp_g2_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);
	ret = adc_mchp_g2_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_mchp_g2_read_async(const struct device *dev, const struct adc_sequence *sequence,
				   struct k_poll_signal *async)
{
	struct adc_mchp_g2_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, true, async);
	ret = adc_mchp_g2_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}
#endif

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_mchp_g2_data *data = CONTAINER_OF(ctx, struct adc_mchp_g2_data, ctx);
	const struct adc_mchp_g2_config *cfg = data->dev->config;
	adc_registers_t *regs = cfg->regs;
	uint32_t chrdy = (uint32_t)cfg->declared_channels << ADC_INTFLAG_CHRDY_Pos;

	data->buffer_repeat = data->buffer;
	data->pending_channels = cfg->declared_channels;

	/* A flag left over from an aborted conversion would end this one early. */
	regs->INT[0].ADC_INTFLAG = chrdy;
	regs->INT[0].ADC_INTENSET = chrdy;

	/* GSWTRG is discarded, not queued, while SYNCBUSY.CTRLB is set. */
	if (!adc_mchp_g2_wait(&regs->ADC_SYNCBUSY, ADC_SYNCBUSY_CTRLB_Msk, false,
			      "SYNCBUSY.CTRLB")) {
		adc_context_complete(ctx, -ETIMEDOUT);
		return;
	}

	regs->ADC_CTRLB = ADC_CTRLB_GSWTRG_Msk;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_mchp_g2_data *data = CONTAINER_OF(ctx, struct adc_mchp_g2_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->buffer_repeat;
	}
}

static void adc_mchp_g2_isr_core0(const struct device *dev)
{
	const struct adc_mchp_g2_config *cfg = dev->config;
	struct adc_mchp_g2_data *data = dev->data;
	adc_registers_t *regs = cfg->regs;
	uint32_t flags = regs->INT[0].ADC_INTFLAG;
	uint16_t ready = (flags & ADC_INTFLAG_CHRDY_Msk) >> ADC_INTFLAG_CHRDY_Pos;

	if (ready == 0) {
		regs->INT[0].ADC_INTFLAG = flags;
		return;
	}

	regs->INT[0].ADC_INTFLAG = (uint32_t)ready << ADC_INTFLAG_CHRDY_Pos;
	data->pending_channels &= ~ready;

	if (data->pending_channels != 0) {
		return;
	}

	regs->INT[0].ADC_INTENCLR = (uint32_t)cfg->declared_channels << ADC_INTFLAG_CHRDY_Pos;

	for (uint8_t k = 0; k < cfg->num_channels; k++) {
		if ((data->active_channels & BIT(k)) == 0) {
			continue;
		}

		regs->ADC_CORCHDATAID = ADC_CORCHDATAID_CORDYID(0) | ADC_CORCHDATAID_CHRDYID(k);

		/* Lower resolutions leave the unresolved low bits zero, so shift down. */
		*data->buffer++ = (regs->ADC_CHRDYDAT & ADC_MCHP_G2_RESULT_MASK) >>
				  (ADC_MCHP_G2_RESULT_BITS - data->resolution);
	}

	adc_context_on_sampling_done(&data->ctx, data->dev);
}

static void adc_mchp_g2_isr_global(const struct device *dev)
{
	const struct adc_mchp_g2_config *cfg = dev->config;
	uint32_t flags = cfg->regs->ADC_CTLINTFLAG;

	LOG_WRN("unexpected controller interrupt, CTLINTFLAG 0x%08x", flags);

	cfg->regs->ADC_CTLINTENCLR = flags;
	cfg->regs->ADC_CTLINTFLAG = flags;
}

static int adc_mchp_g2_init(const struct device *dev)
{
	const struct adc_mchp_g2_config *cfg = dev->config;
	struct adc_mchp_g2_data *data = dev->data;
	adc_registers_t *regs = cfg->regs;
	int ret;

	data->dev = dev;

	ret = clock_control_on(cfg->clock_dev, cfg->mclk_sys);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("cannot enable the APB clock: %d", ret);
		return ret;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->gclk_sys);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("cannot enable the generic clock: %d", ret);
		return ret;
	}

	/* pinctrl-0 is optional, so -ENOENT is not an error. */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		LOG_ERR("cannot apply pinctrl: %d", ret);
		return ret;
	}

	regs->ADC_CTRLA = ADC_CTRLA_SWRST_Msk;
	if (!adc_mchp_g2_wait(&regs->ADC_SYNCBUSY, ADC_SYNCBUSY_SWRST_Msk, false,
			      "SYNCBUSY.SWRST")) {
		return -ETIMEDOUT;
	}

	/* ANAEN before any ANLENn, and CALCTRL only while ANLEN0 is clear. */
	regs->ADC_CTRLA = ADC_CTRLA_ANAEN_Msk;

	regs->CONFIG[0].ADC_CALCTRL = cfg->fuses->FUSES_FCCFG65;

	data->vref_sel = ADC_MCHP_G2_VREFSEL_AVDD;

	regs->ADC_CTRLD = ADC_CTRLD_CTLCKDIV(cfg->ctl_clk_div) |
			  ADC_CTRLD_VREFSEL(data->vref_sel) |
			  ADC_CTRLD_WKUPEXP(cfg->wakeup_exp) | ADC_CTRLD_ANLEN0_Msk |
			  ADC_CTRLD_CHNEN0_Msk;

	regs->CONFIG[0].ADC_CORCTRL = ADC_CORCTRL_SAMC(cfg->sample_count) |
				      ADC_CORCTRL_SELRES(ADC_MCHP_G2_SELRES_12BIT) |
				      ADC_CORCTRL_ADCDIV(cfg->adc_clk_div);

	regs->CONFIG[0].ADC_CHNCFG1 = 0;
	regs->CONFIG[0].ADC_CHNCFG2 = 0;
	regs->CONFIG[0].ADC_CHNCFG3 = 0;
	adc_mchp_g2_program_triggers(regs, cfg->declared_channels);

	data->resolution = 12;

	ret = adc_mchp_g2_enable(regs);
	if (ret < 0) {
		return ret;
	}

	cfg->irq_config();

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, adc_mchp_g2_api) = {
	.channel_setup = adc_mchp_g2_channel_setup,
	.read = adc_mchp_g2_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_mchp_g2_read_async,
#endif
};

#define ADC_MCHP_G2_CHANNEL_BIT(node) | BIT(DT_REG_ADDR(node))

#define ADC_MCHP_G2_DECLARED_CHANNELS(n)                                                         \
	(0 DT_INST_FOREACH_CHILD_STATUS_OKAY(n, ADC_MCHP_G2_CHANNEL_BIT))

#define ADC_MCHP_G2_CHANNEL_ASSERT(node)                                                         \
	BUILD_ASSERT(DT_REG_ADDR(node) < DT_PROP(DT_PARENT(node), num_channels),                  \
		     "channel reg must be less than num-channels");

#define ADC_MCHP_G2_INIT(n)                                                                       \
	BUILD_ASSERT(DT_INST_PROP(n, num_channels) <= 16, "num-channels must be at most 16");    \
	BUILD_ASSERT(DT_INST_PROP(n, ctrl_clock_div) <= 63,                                       \
		     "ctrl-clock-div must be between 0 and 63");                                  \
	BUILD_ASSERT(DT_INST_PROP(n, adc_div_ratio) >= 1 && DT_INST_PROP(n, adc_div_ratio) <= 127,\
		     "adc-div-ratio must be between 1 and 127");                                  \
	BUILD_ASSERT(DT_INST_PROP(n, adc_wkup_clock_count) <= 15,                                 \
		     "adc-wkup-clock-count must be between 0 and 15");                            \
	BUILD_ASSERT(DT_INST_PROP(n, sample_count) <= 1023,                                       \
		     "sample-count must be between 0 and 1023");                                  \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, ADC_MCHP_G2_CHANNEL_ASSERT)                          \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                \
                                                                                                   \
	static void adc_mchp_g2_irq_config_##n(void)                                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, global, irq),                                  \
			    DT_INST_IRQ_BY_NAME(n, global, priority), adc_mchp_g2_isr_global,     \
			    DEVICE_DT_INST_GET(n), 0);                                            \
		irq_enable(DT_INST_IRQ_BY_NAME(n, global, irq));                                  \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, core0, irq),                                   \
			    DT_INST_IRQ_BY_NAME(n, core0, priority), adc_mchp_g2_isr_core0,      \
			    DEVICE_DT_INST_GET(n), 0);                                            \
		irq_enable(DT_INST_IRQ_BY_NAME(n, core0, irq));                                   \
	}                                                                                          \
                                                                                                   \
	static const struct adc_mchp_g2_config adc_mchp_g2_config_##n = {                          \
		.regs = (adc_registers_t *)DT_INST_REG_ADDR(n),                                   \
		.fuses = (fuses_calotp_registers_t *)DT_REG_ADDR(DT_INST_PHANDLE(n, nvm_calib)),  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                        \
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                  \
		.mclk_sys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem),              \
		.gclk_sys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem),              \
		.irq_config = adc_mchp_g2_irq_config_##n,                                         \
		.declared_channels = ADC_MCHP_G2_DECLARED_CHANNELS(n),                            \
		.num_channels = DT_INST_PROP(n, num_channels),                                    \
		.ctl_clk_div = DT_INST_PROP(n, ctrl_clock_div),                                   \
		.adc_clk_div = DT_INST_PROP(n, adc_div_ratio),                                    \
		.sample_count = DT_INST_PROP(n, sample_count),                                    \
		.wakeup_exp = DT_INST_PROP(n, adc_wkup_clock_count),                              \
	};                                                                                         \
                                                                                                   \
	static struct adc_mchp_g2_data adc_mchp_g2_data_##n = {                                    \
		ADC_CONTEXT_INIT_TIMER(adc_mchp_g2_data_##n, ctx),                                \
		ADC_CONTEXT_INIT_LOCK(adc_mchp_g2_data_##n, ctx),                                 \
		ADC_CONTEXT_INIT_SYNC(adc_mchp_g2_data_##n, ctx),                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, adc_mchp_g2_init, NULL,                                          \
			      &adc_mchp_g2_data_##n, &adc_mchp_g2_config_##n, POST_KERNEL,        \
			      CONFIG_ADC_INIT_PRIORITY, &adc_mchp_g2_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_MCHP_G2_INIT)
