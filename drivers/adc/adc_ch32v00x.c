/*
 * Copyright (c) 2025 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_adc

#include <hal_ch32fun.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>

struct adc_ch32v00x_config {
	ADC_TypeDef *regs;
	const struct pinctrl_dev_config *pin_cfg;
	const struct device *clock_dev;
	uint8_t clock_id;
};

static int adc_ch32v00x_channel_setup(const struct device *dev,
				      const struct adc_channel_cfg *channel_cfg)
{
	if (channel_cfg->gain != ADC_GAIN_1) {
		return -EINVAL;
	}
	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		return -EINVAL;
	}
	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		return -EINVAL;
	}
	if (channel_cfg->differential) {
		return -EINVAL;
	}
	if (channel_cfg->channel_id >= 10) {
		return -EINVAL;
	}

	return 0;
}

static int adc_ch32v00x_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_ch32v00x_config *config = dev->config;
	ADC_TypeDef *regs = config->regs;
	uint32_t channels = sequence->channels;
	int rsqr = 2;
	int sequence_id = 0;
	int total_channels = 0;
	int i;
	uint16_t *samples = sequence->buffer;

	if (sequence->options != NULL) {
		return -ENOTSUP;
	}
	if (sequence->resolution != 10) {
		return -EINVAL;
	}
	if (sequence->oversampling != 0) {
		return -ENOTSUP;
	}
	if (sequence->channels >= (1 << 10)) {
		return -EINVAL;
	}

	if (sequence->calibrate) {
		regs->CTLR2 |= ADC_RSTCAL;
		while ((regs->CTLR2 & ADC_RSTCAL) != 0) {
		}
		regs->CTLR2 |= ADC_CAL;
		while ((regs->CTLR2 & ADC_CAL) != 0) {
		}
	}

	/*
	 * Build the sample sequence. The channel IDs are packed 5 bits at a time starting in RSQR3
	 * and working down in memory to RSQR1.
	 */
	regs->RSQR1 = 0;
	regs->RSQR2 = 0;
	regs->RSQR3 = 0;

	for (i = 0; channels != 0; i++, channels >>= 1) {
		if ((channels & 1) != 0) {
			total_channels++;
			(&regs->RSQR1)[rsqr] |= i << sequence_id;
			/* Each channel ID is 5 bits wide */
			sequence_id += 5;
			/* Each sequence register can hold 6 x 5 bit channel IDs */
			if (sequence_id >= 30) {
				/* Move on to the next RSQRn register, i.e. RSQR(n-1) */
				sequence_id = 0;
				rsqr--;
			}
		}
	}
	if (total_channels == 0) {
		return 0;
	}
	if (sequence->buffer_size < total_channels * sizeof(*samples)) {
		return -ENOMEM;
	}

	/* Set the number of channels to read. Note that '0' means 'one channel'. */
	regs->RSQR1 |= (total_channels - 1) * ADC_L_0;
	regs->CTLR2 |= ADC_SWSTART;
	for (i = 0; i < total_channels; i++) {
		while ((regs->STATR & ADC_EOC) == 0) {
		}
		*samples++ = regs->RDATAR;
	}

	return 0;
}

static int adc_ch32v00x_init(const struct device *dev)
{
	const struct adc_ch32v00x_config *config = dev->config;
	ADC_TypeDef *regs = config->regs;
	int err;

	clock_control_on(config->clock_dev, (clock_control_subsys_t)(uintptr_t)config->clock_id);

	err = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	/*
	 * The default sampling time is 3 cycles and shows coupling between channels. Use 15 cycles
	 * instead. Arbitrary.
	 */
	regs->SAMPTR2 = ADC_SMP0_1 | ADC_SMP1_1 | ADC_SMP2_1 | ADC_SMP3_1 | ADC_SMP4_1 |
			ADC_SMP5_1 | ADC_SMP6_1 | ADC_SMP7_1 | ADC_SMP8_1 | ADC_SMP9_1;

	regs->CTLR2 = ADC_ADON | ADC_EXTSEL;

	return 0;
}

#define ADC_CH32V00X_DEVICE(n)                                                                     \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static DEVICE_API(adc, adc_ch32v00x_api_##n) = {                                           \
		.channel_setup = adc_ch32v00x_channel_setup,                                       \
		.read = adc_ch32v00x_read,                                                         \
		.ref_internal = DT_INST_PROP(n, vref_mv),                                          \
	};                                                                                         \
                                                                                                   \
	static const struct adc_ch32v00x_config adc_ch32v00x_config_##n = {                        \
		.regs = (ADC_TypeDef *)DT_INST_REG_ADDR(n),                                        \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_id = DT_INST_CLOCKS_CELL(n, id),                                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, adc_ch32v00x_init, NULL, NULL, &adc_ch32v00x_config_##n,          \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &adc_ch32v00x_api_##n);

DT_INST_FOREACH_STATUS_OKAY(ADC_CH32V00X_DEVICE)
