/*
 * Copyright (c) 2024 BayLibre, SAS
 * Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc35xx_lgpt

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/spinlock.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>

#include <ti/devices/cc35xx/inc/hw_gptimer.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>

#define TI_CC35XX_COUNTER_CHANNELS 4
#define TI_CC35XX_CXCFG                                                                            \
	(GPTIMER_C0CFG_EDGE_RISE | GPTIMER_C0CFG_INPUT_EV | GPTIMER_C0CFG_CCACT_TGL_ON_CMP_DIS)
#define TI_CC35XX_CHAN_DISABLE 0
#define TI_CC35XX_CNTR_START   0x01
#define TI_CC35XX_CNTR_STOP    0x00

#define GPTIMER_O_CXCC(index)  (GPTIMER_O_C0CC + ((index) * 4))
#define GPTIMER_O_CXCFG(index) (GPTIMER_O_C0CFG + ((index) * 4))

#define GPTIMER_IMCLR_CXCC_CLR(index) (GPTIMER_IMCLR_C0CC_CLR << (index))
#define GPTIMER_IMSET_CXCC_SET(index) (GPTIMER_IMSET_C0CC_SET << (index))

struct counter_cc35xx_gptimer_config {
	struct counter_config_info counter_info;
	uint32_t base;
	uint32_t prescale;
	uint32_t freq;
};

struct counter_cc35xx_gptimer_data {
	struct counter_alarm_cfg alarm_cfg[TI_CC35XX_COUNTER_CHANNELS];
	struct counter_top_cfg target_cfg;
};

static int counter_cc35xx_gptimer_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_cc35xx_gptimer_config *config = dev->config;

	*ticks = sys_read32(config->base + GPTIMER_O_CNTR);

	return 0;
}

static void counter_cc35xx_gptimer_isr(const struct device *dev)
{
	const struct counter_cc35xx_gptimer_config *config = dev->config;
	struct counter_cc35xx_gptimer_data *data = dev->data;
	uint32_t counter = sys_read32(config->base + GPTIMER_O_CNTR);
	uint32_t reg_mis = sys_read32(config->base + GPTIMER_O_MIS);
	int i;

	sys_write32(reg_mis, config->base + GPTIMER_O_ICLR);

	if ((reg_mis & GPTIMER_MIS_TGT) && data->target_cfg.callback) {
		data->target_cfg.callback(dev, data->target_cfg.user_data);
	}

	for (i = 0; i < TI_CC35XX_COUNTER_CHANNELS; i++) {
		if ((reg_mis & GPTIMER_MIS_C0CC << i) && data->alarm_cfg[i].callback) {
			counter_alarm_callback_t callback = data->alarm_cfg[i].callback;
			void *user_data = data->alarm_cfg[i].user_data;

			data->alarm_cfg[i].flags = 0;
			data->alarm_cfg[i].ticks = 0;
			data->alarm_cfg[i].callback = NULL;
			data->alarm_cfg[i].user_data = NULL;

			callback(dev, i, counter, user_data);
		}
	}
}

static uint32_t counter_cc35xx_gptimer_get_freq(const struct device *dev)
{
	const struct counter_cc35xx_gptimer_config *config = dev->config;

	/* Timer frequency, per SWRU626_TRM, p. 921: [TICKSRC]/([TICKDIV]+1) */
	return (config->freq / (config->prescale + 1));
}

static int counter_cc35xx_gptimer_set_alarm(const struct device *dev, uint8_t chan_id,
					    const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_cc35xx_gptimer_config *config = dev->config;
	struct counter_cc35xx_gptimer_data *data = dev->data;
	uint32_t ticks = alarm_cfg->ticks;
	uint32_t top = sys_read32(config->base + GPTIMER_O_TGTNC);

	if (chan_id >= TI_CC35XX_COUNTER_CHANNELS) {
		return -ENOTSUP;
	}

	if (ticks > top) {
		return -EINVAL;
	}

	/*
	 * Capture compare register always compares against the absolute value
	 * of the counter register.
	 */
	if (!(alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE)) {
		uint64_t absolute_ticks =
			(uint64_t)sys_read32(config->base + GPTIMER_O_CNTR) + ticks;

		if (top != UINT32_MAX) {
			absolute_ticks %= (uint64_t)top + 1U;
		}

		ticks = (uint32_t)absolute_ticks;
	}

	sys_write32(GPTIMER_IMSET_CXCC_SET(chan_id), config->base + GPTIMER_O_IMSET);
	sys_write32(ticks, config->base + GPTIMER_O_CXCC(chan_id));
	sys_write32(TI_CC35XX_CXCFG, config->base + GPTIMER_O_CXCFG(chan_id));

	data->alarm_cfg[chan_id].flags = alarm_cfg->flags;
	data->alarm_cfg[chan_id].ticks = alarm_cfg->ticks;
	data->alarm_cfg[chan_id].callback = alarm_cfg->callback;
	data->alarm_cfg[chan_id].user_data = alarm_cfg->user_data;

	return 0;
}

static int counter_cc35xx_gptimer_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_cc35xx_gptimer_config *config = dev->config;
	struct counter_cc35xx_gptimer_data *data = dev->data;

	if (chan_id >= TI_CC35XX_COUNTER_CHANNELS) {
		return -ENOTSUP;
	}

	sys_write32(GPTIMER_IMCLR_CXCC_CLR(chan_id), config->base + GPTIMER_O_IMCLR);
	sys_write32(TI_CC35XX_CHAN_DISABLE, config->base + GPTIMER_O_CXCC(chan_id));
	sys_write32(TI_CC35XX_CHAN_DISABLE, config->base + GPTIMER_O_CXCFG(chan_id));

	data->alarm_cfg[chan_id].flags = 0;
	data->alarm_cfg[chan_id].ticks = 0;
	data->alarm_cfg[chan_id].callback = NULL;
	data->alarm_cfg[chan_id].user_data = NULL;

	return 0;
}

static uint32_t counter_cc35xx_gptimer_get_top_value(const struct device *dev)
{
	const struct counter_cc35xx_gptimer_config *config = dev->config;

	return sys_read32(config->base + GPTIMER_O_TGTNC);
}

static int counter_cc35xx_gptimer_set_top_value(const struct device *dev,
						const struct counter_top_cfg *cfg)
{
	const struct counter_cc35xx_gptimer_config *config = dev->config;
	struct counter_cc35xx_gptimer_data *data = dev->data;
	uint32_t current = sys_read32(config->base + GPTIMER_O_CNTR);

	for (uint8_t chan_id = 0; chan_id < TI_CC35XX_COUNTER_CHANNELS; chan_id++) {
		if (data->alarm_cfg[chan_id].callback != NULL) {
			return -EBUSY;
		}
	}

	if (cfg->ticks > config->counter_info.max_top_value) {
		return -EINVAL;
	}

	if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) && (cfg->ticks < current)) {
		if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			sys_write32(sys_read32(config->base + GPTIMER_O_CTL) & ~GPTIMER_CTL_MODE_M,
				    config->base + GPTIMER_O_CTL);
		} else {
			return -ETIME;
		}
	}

	if (cfg->callback != NULL) {
		sys_write32(GPTIMER_IMSET_TGT_SET, config->base + GPTIMER_O_IMSET);
	} else {
		sys_write32(GPTIMER_IMCLR_TGT_CLR, config->base + GPTIMER_O_IMCLR);
	}

	sys_write32(cfg->ticks, config->base + GPTIMER_O_TGT);

	data->target_cfg.flags = cfg->flags;
	data->target_cfg.ticks = cfg->ticks;
	data->target_cfg.callback = cfg->callback;
	data->target_cfg.user_data = cfg->user_data;

	return 0;
}

static uint32_t counter_cc35xx_gptimer_get_pending_int(const struct device *dev)
{
	const struct counter_cc35xx_gptimer_config *config = dev->config;

	return !!sys_read32(config->base + GPTIMER_O_MIS);
}

static int counter_cc35xx_gptimer_start(const struct device *dev)
{
	const struct counter_cc35xx_gptimer_config *config = dev->config;
	uint32_t reg = sys_read32(config->base + GPTIMER_O_CTL);

	sys_write32((reg & ~GPTIMER_CTL_MODE_M) | GPTIMER_CTL_MODE_UP_PER,
		    config->base + GPTIMER_O_CTL);

	sys_write32(TI_CC35XX_CNTR_START, config->base + GPTIMER_O_STARTCFG);

	return 0;
}

static int counter_cc35xx_gptimer_stop(const struct device *dev)
{
	const struct counter_cc35xx_gptimer_config *config = dev->config;

	sys_write32(TI_CC35XX_CNTR_STOP, config->base + GPTIMER_O_STARTCFG);

	return 0;
}

static DEVICE_API(counter, cc35xx_counter_api) = {
	.start = counter_cc35xx_gptimer_start,
	.stop = counter_cc35xx_gptimer_stop,
	.get_value = counter_cc35xx_gptimer_get_value,
	.set_alarm = counter_cc35xx_gptimer_set_alarm,
	.cancel_alarm = counter_cc35xx_gptimer_cancel_alarm,
	.get_top_value = counter_cc35xx_gptimer_get_top_value,
	.set_top_value = counter_cc35xx_gptimer_set_top_value,
	.get_pending_int = counter_cc35xx_gptimer_get_pending_int,
	.get_freq = counter_cc35xx_gptimer_get_freq,
};

#define GPTIMER_CLK_PRESCALE(pres) ((pres) << 8)

#define COUNTER_CC35XX_INIT_FUNC(inst)                                                             \
	static int counter_cc35xx_init_##inst(const struct device *dev)                            \
	{                                                                                          \
		const struct counter_cc35xx_gptimer_config *config = dev->config;                  \
		sys_write32(GPTIMER_CLKCFG_ENABLE, config->base + GPTIMER_O_CLKCFG);               \
		sys_write32(GPTIMER_CTL_CMPDIR_BOTH | GPTIMER_CTL_INTP_LATE | GPTIMER_EMU_HALT_EN, \
			    config->base + GPTIMER_O_CTL);                                         \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),                       \
			    counter_cc35xx_gptimer_isr, DEVICE_DT_INST_GET(inst), 0);              \
		irq_enable(DT_INST_IRQN(inst));                                                    \
		sys_write32(config->counter_info.max_top_value, config->base + GPTIMER_O_TGT);     \
		sys_write32(GPTIMER_CLK_PRESCALE(config->prescale),                                \
			    config->base + GPTIMER_O_PRECFG);                                      \
		return 0;                                                                          \
	}

#define COUNTER_CC35XX_INIT(inst)                                                                  \
	COUNTER_CC35XX_INIT_FUNC(inst);                                                            \
	static const struct counter_cc35xx_gptimer_config cc35xx_counter_config_##inst = {         \
		.counter_info =                                                                    \
			{                                                                          \
				.max_top_value = DT_INST_PROP(inst, max_top_value),                \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = TI_CC35XX_COUNTER_CHANNELS,                            \
			},                                                                         \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.prescale = DT_INST_PROP(inst, clk_prescale),                                      \
		.freq = DT_INST_PROP_BY_PHANDLE(inst, clocks, clock_frequency)};                   \
	static struct counter_cc35xx_gptimer_data cc35xx_counter_data_##inst;                      \
	DEVICE_DT_INST_DEFINE(inst, &counter_cc35xx_init_##inst, NULL,                             \
			      &cc35xx_counter_data_##inst, &cc35xx_counter_config_##inst,          \
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY, &cc35xx_counter_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_CC35XX_INIT);
