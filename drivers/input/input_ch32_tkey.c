/*
 * SPDX-FileCopyrightText: Copyright Michael Hope <michaelh@juju.nz>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_ch32_tkey

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <hal_ch32fun.h>

/* Register bit definitions */
#define TKEY_STATR_EOC BIT(1)

#define TKEY_CTLR1_EOCIE    BIT(5)
#define TKEY_CTLR1_TKENABLE BIT(24)
#define TKEY_CTLR1_BUFEN    BIT(26)

#define TKEY_CTLR2_ADON   BIT(0)
#define TKEY_CTLR2_CONT   BIT(1)
#define TKEY_CTLR2_CAL    BIT(2)
#define TKEY_CTLR2_RSTCAL BIT(3)
#define TKEY_CTLR2_ALIGN  BIT(11)

/* Values on or above this count as pressed */
#define TKEY_PRESSED        2000
#define TKEY_CHARGE_TIME    ADC_SampleTime_7Cycles5
#define TKEY_DISCHARGE_TIME 8
#define TKEY_CHARGE_OFFSET  16

/* Per key configuration */
struct input_ch32_tkey_channel_config {
	uint8_t channel_id;
	uint32_t code;
};

/* Per key state */
struct input_ch32_tkey_channel_data {
	bool active;
};

struct input_ch32_tkey_config {
	/* Re-use the ADC definition as ch32fun does not define the TKEY specific names */
	ADC_TypeDef *regs;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	k_timeout_t poll_ticks;
	uint8_t num_channels;
	const struct input_ch32_tkey_channel_config *channels;
	struct input_ch32_tkey_channel_data *channel_data;
};

struct input_ch32_tkey_data {
	struct k_work_delayable scan_work;
	const struct device *dev;
};

static void input_ch32_tkey_poll_channel(const struct device *dev, int channel)
{
	const struct input_ch32_tkey_config *config = dev->config;
	ADC_TypeDef *regs = config->regs;
	const struct input_ch32_tkey_channel_config *ch_cfg = &config->channels[channel];
	struct input_ch32_tkey_channel_data *ch_data = &config->channel_data[channel];
	uint8_t channel_id = ch_cfg->channel_id;
	bool active;

	/* Set the channel and do a single conversion */
	regs->RSQR3 = channel_id & ADC_SQ1;

	if (channel_id < 10) {
		/* First 10 channels are packed into SAMPTR2 */
		uint32_t shift = channel_id * 3;
		regs->SAMPTR2 =
			(regs->SAMPTR2 & ~(ADC_SMP0 << shift)) | (TKEY_CHARGE_TIME << shift);
	} else {
		uint32_t shift = (channel_id - 10) * 3;
		regs->SAMPTR1 =
			(regs->SAMPTR1 & ~(ADC_SMP10 << shift)) | (TKEY_CHARGE_TIME << shift);
	}

	regs->IDATAR1 = TKEY_CHARGE_OFFSET;
	/* Set the discharge time and start the sample */
	regs->RDATAR = TKEY_DISCHARGE_TIME;

	while ((regs->STATR & TKEY_STATR_EOC) == 0) {
	}

	active = regs->RDATAR > TKEY_PRESSED;

	if (active != ch_data->active) {
		ch_data->active = active;
		input_report_key(dev, ch_cfg->code, active ? 1 : 0, true, K_FOREVER);
	}
}

static void input_ch32_tkey_poll(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct input_ch32_tkey_data *data =
		CONTAINER_OF(dwork, struct input_ch32_tkey_data, scan_work);
	const struct device *dev = data->dev;
	const struct input_ch32_tkey_config *config = data->dev->config;

	for (int channel = 0; channel < config->num_channels; channel++) {
		input_ch32_tkey_poll_channel(dev, channel);
	}

	k_work_schedule(&data->scan_work, config->poll_ticks);
}

static int input_ch32_tkey_init(const struct device *dev)
{
	const struct input_ch32_tkey_config *config = dev->config;
	struct input_ch32_tkey_data *data = dev->data;
	ADC_TypeDef *regs = config->regs;
	int err;

	data->dev = dev;

	err = clock_control_on(config->clock_dev, config->clock_subsys);
	if (err < 0) {
		return err;
	}

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	/* Reset and enable the ADC and TKEY function */
	regs->CTLR1 = 0;
	regs->CTLR2 = 0;
	regs->CTLR2 = TKEY_CTLR2_ADON;
	regs->CTLR1 = TKEY_CTLR1_TKENABLE;
	/* Must sample one channel at a time. The number of channels is ADC_L minus 1. */
	regs->RSQR1 = 0 * ADC_L;

	k_work_init_delayable(&data->scan_work, input_ch32_tkey_poll);
	k_work_schedule(&data->scan_work, config->poll_ticks);

	return 0;
}

#define INPUT_CH32_TKEY_CHANNEL_CFG(n)                                                             \
	{                                                                                          \
		.channel_id = DT_PROP(n, channel),                                                 \
		.code = DT_PROP(n, zephyr_code),                                                   \
	},

#define INPUT_CH32_TKEY_INIT(n)                                                                    \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static const struct input_ch32_tkey_channel_config input_ch32_tkey_channels_##n[] = {      \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, INPUT_CH32_TKEY_CHANNEL_CFG)};                \
	static struct input_ch32_tkey_channel_data                                                 \
		input_ch32_tkey_channel_data_##n[MAX(1, DT_INST_CHILD_NUM_STATUS_OKAY(n))];        \
	static const struct input_ch32_tkey_config input_ch32_tkey_config_##n = {                  \
		.regs = (ADC_TypeDef *)DT_INST_REG_ADDR(n),                                        \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, id),                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.poll_ticks = K_MSEC(DT_INST_PROP(n, poll_period_ms)),                             \
		.num_channels = ARRAY_SIZE(input_ch32_tkey_channels_##n),                          \
		.channels = input_ch32_tkey_channels_##n,                                          \
		.channel_data = input_ch32_tkey_channel_data_##n,                                  \
	};                                                                                         \
	static struct input_ch32_tkey_data input_ch32_tkey_data_##n = {};                          \
	DEVICE_DT_INST_DEFINE(n, input_ch32_tkey_init, NULL, &input_ch32_tkey_data_##n,            \
			      &input_ch32_tkey_config_##n, POST_KERNEL,                            \
			      CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INPUT_CH32_TKEY_INIT)
