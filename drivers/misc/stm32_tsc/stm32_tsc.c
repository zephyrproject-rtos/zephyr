/*
 * Copyright (c) 2024 Arif Balik <arifbalik@outlook.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_tsc

#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/misc/stm32_tsc/stm32_tsc.h>

/* each group only has 4 configurable I/O */
#define GET_GROUP_BITS(val) (uint32_t)(((val) & 0x0f) << ((group->group - 1) * 4))

LOG_MODULE_REGISTER(stm32_tsc, CONFIG_LOG_DEFAULT_LEVEL);

struct stm32_tsc_group_config {
	uint8_t group;
	uint8_t channel_ios;
	uint8_t sampling_io;
	bool use_as_shield;
};

struct stm32_tsc_config {
	const TSC_TypeDef *tsc;
	const struct stm32_pclken *pclken;
	struct reset_dt_spec reset;
	const struct pinctrl_dev_config *pcfg;
	const struct stm32_tsc_group_config *groups;
	uint8_t group_cnt;

	uint32_t pgpsc;
	uint8_t ctph;
	uint8_t ctpl;
	bool spread_spectrum;
	uint8_t sscpsc;
	uint8_t ssd;
	uint16_t max_count;
	bool iodef;
	bool sync_acq;
	bool sync_pol;
	void (*irq_func)(void);
};

struct stm32_tsc_data {
	stm32_tsc_callback_t cb;
	void *user_data;
};

void stm32_tsc_start(const struct device *dev)
{
	const struct stm32_tsc_config *config = dev->config;

	/* clear interrupts */
	sys_set_bits((mem_addr_t)&config->tsc->ICR, TSC_ICR_EOAIC | TSC_ICR_MCEIC);

	/* enable end of acquisition and max count error interrupts */
	sys_set_bits((mem_addr_t)&config->tsc->IER, TSC_IER_EOAIE | TSC_IER_MCEIE);

	/* TODO: When sync acqusition mode is enabled, both this bit and an external input signal
	 * should be set. When the acqusition stops this bit is cleared, so even if a sync signal is
	 * present, the next acqusition will not start until this bit is set again.
	 */
	/* start acquisition */
	sys_set_bit((mem_addr_t)&config->tsc->CR, TSC_CR_START_Pos);
}

void stm32_tsc_register_callback(const struct device *dev, stm32_tsc_callback_t cb, void *user_data)
{
	struct stm32_tsc_data *data = dev->data;

	data->cb = cb;
	data->user_data = user_data;
}

static int stm32_tsc_handle_incoming_data(const struct device *dev)
{
	const struct stm32_tsc_config *config = dev->config;
	struct stm32_tsc_data *data = dev->data;

	if (sys_test_bit((mem_addr_t)&config->tsc->ISR, TSC_ISR_MCEF_Pos)) {
		/* clear max count error flag */
		sys_set_bit((mem_addr_t)&config->tsc->ICR, TSC_ICR_MCEIC_Pos);
		LOG_ERR("%s: max count error", dev->name);
		LOG_HEXDUMP_DBG(config->tsc, sizeof(TSC_TypeDef), "TSC Registers");
		return -EIO;
	}

	if (sys_test_bit((mem_addr_t)&config->tsc->ISR, TSC_ISR_EOAF_Pos)) {
		/* clear end of acquisition flag */
		sys_set_bit((mem_addr_t)&config->tsc->ICR, TSC_ICR_EOAIC_Pos);

		/* read values */
		for (uint8_t i = 0; i < config->group_cnt; i++) {
			const struct stm32_tsc_group_config *group = &config->groups[i];
			uint32_t group_bit = BIT(group->group - 1) << 16;

			if (config->tsc->IOGCSR & group_bit) {
				int32_t count_value = sys_read32(
					(mem_addr_t)&config->tsc->IOGXCR[group->group - 1]);
				data->cb(group->group, count_value, data->user_data);
			}
		}
	}

	return 0;
}

static void stm32_tsc_isr(const struct device *dev)
{
	const struct stm32_tsc_config *config = dev->config;

	/* disable interrupts */
	sys_clear_bits((mem_addr_t)&config->tsc->IER, TSC_IER_EOAIE | TSC_IER_MCEIE);

	stm32_tsc_handle_incoming_data(dev);
}

static int stm32_tsc_init(const struct device *dev)
{
	const struct stm32_tsc_config *config = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	int ret;

	if (!device_is_ready(clk)) {
		LOG_ERR("%s: clock controller device not ready", dev->name);
		return -ENODEV;
	}

	/* reset TSC values to default */
	ret = reset_line_toggle_dt(&config->reset);
	if (ret < 0) {
		LOG_ERR("Failed to reset %s (%d)", dev->name, ret);
		return ret;
	}

	ret = clock_control_on(clk, (clock_control_subsys_t)&config->pclken[0]);
	if (ret < 0) {
		LOG_ERR("Failed to enable clock for %s (%d)", dev->name, ret);
		return ret;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to configure %s pins (%d)", dev->name, ret);
		return ret;
	}

	/* set ctph (bits 31:28) and ctpl (bits 27:24) */
	sys_set_bits((mem_addr_t)&config->tsc->CR, (((config->ctph - 1) << 4) | (config->ctpl - 1))
							   << TSC_CR_CTPL_Pos);

	/* set spread spectrum deviation (bits 23:17) */
	sys_set_bits((mem_addr_t)&config->tsc->CR, config->ssd << TSC_CR_SSD_Pos);

	/* set pulse generator prescaler (bits 14:12) */
	sys_set_bits((mem_addr_t)&config->tsc->CR, config->pgpsc);

	/* set max count value (bits 7:5) */
	sys_set_bits((mem_addr_t)&config->tsc->CR, config->max_count);

	/* set spread spectrum prescaler (bit 15) */
	if (config->sscpsc == 2) {
		sys_set_bit((mem_addr_t)&config->tsc->CR, TSC_CR_SSPSC_Pos);
	}

	/* set sync bit polarity */
	if (config->sync_pol) {
		sys_set_bit((mem_addr_t)&config->tsc->CR, TSC_CR_SYNCPOL_Pos);
	}

	/* set sync acquisition */
	if (config->sync_acq) {
		sys_set_bit((mem_addr_t)&config->tsc->CR, TSC_CR_AM_Pos);
	}

	/* set I/O default mode */
	if (config->iodef) {
		sys_set_bit((mem_addr_t)&config->tsc->CR, TSC_CR_IODEF_Pos);
	}

	/* set spread spectrum */
	if (config->spread_spectrum) {
		sys_set_bit((mem_addr_t)&config->tsc->CR, TSC_CR_SSE_Pos);
	}

	/* group configuration */
	for (int i = 0; i < config->group_cnt; i++) {
		const struct stm32_tsc_group_config *group = &config->groups[i];

		if (group->channel_ios & group->sampling_io) {
			LOG_ERR("%s: group %d has the same channel and sampling I/O", dev->name,
				group->group);
			return -EINVAL;
		}

		/* if use_as_shield is true, the channel I/Os are used as shield, and can only have
		 * values 1,2,4,8
		 */
		if (group->use_as_shield && group->channel_ios != 1 && group->channel_ios != 2 &&
		    group->channel_ios != 4 && group->channel_ios != 8) {
			LOG_ERR("%s: group %d is used as shield, but has invalid channel I/Os. "
				"Can only have one",
				dev->name, group->group);
			return -EINVAL;
		}

		/* clear schmitt trigger hysteresis for enabled I/Os */
		sys_clear_bits((mem_addr_t)&config->tsc->IOHCR,
			       GET_GROUP_BITS(group->channel_ios | group->sampling_io));

		/* set channel I/Os */
		sys_set_bits((mem_addr_t)&config->tsc->IOCCR, GET_GROUP_BITS(group->channel_ios));

		/* set sampling I/O */
		sys_set_bits((mem_addr_t)&config->tsc->IOSCR, GET_GROUP_BITS(group->sampling_io));

		/* enable group */
		if (!group->use_as_shield) {
			sys_set_bit((mem_addr_t)&config->tsc->IOGCSR, group->group - 1);
		}
	}

	/* disable interrupts */
	sys_clear_bits((mem_addr_t)&config->tsc->IER, TSC_IER_EOAIE | TSC_IER_MCEIE);

	/* clear interrupts */
	sys_set_bits((mem_addr_t)&config->tsc->ICR, TSC_ICR_EOAIC | TSC_ICR_MCEIC);

	/* enable peripheral */
	sys_set_bit((mem_addr_t)&config->tsc->CR, TSC_CR_TSCE_Pos);

	config->irq_func();

	return 0;
}

#define STM32_TSC_GROUP_DEFINE(node_id)                                                            \
	{                                                                                          \
		.group = DT_PROP(node_id, group),                                                  \
		.channel_ios = DT_PROP(node_id, channel_ios),                                      \
		.sampling_io = DT_PROP(node_id, sampling_io),                                      \
		.use_as_shield = DT_PROP(node_id, use_as_shield),                                  \
	}

#define STM32_TSC_INIT(index)                                                                      \
	static const struct stm32_pclken pclken_##index[] = STM32_DT_INST_CLOCKS(index);           \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static void stm32_tsc_irq_init_##index(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), stm32_tsc_isr,      \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	};                                                                                         \
                                                                                                   \
	static const struct stm32_tsc_group_config groups_cfg_##index[] = {                        \
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(index, STM32_TSC_GROUP_DEFINE, (,))};        \
                                                                                                   \
	static struct stm32_tsc_data stm32_tsc_data_##index;                                       \
                                                                                                   \
	static const struct stm32_tsc_config stm32_tsc_cfg_##index = {                             \
		.tsc = (TSC_TypeDef *)DT_INST_REG_ADDR(index),                                     \
		.pclken = pclken_##index,                                                          \
		.reset = RESET_DT_SPEC_INST_GET(index),                                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.groups = groups_cfg_##index,                                                      \
		.group_cnt = DT_INST_CHILD_NUM_STATUS_OKAY(index),                                 \
		.pgpsc = DT_INST_STRING_UPPER_TOKEN(index, pulse_generator_prescaler),             \
		.ctph = DT_INST_PROP(index, ctph),                                                 \
		.ctpl = DT_INST_PROP(index, ctpl),                                                 \
		.spread_spectrum = DT_INST_PROP(index, spread_spectrum),                           \
		.sscpsc = DT_INST_PROP(index, spread_spectrum_prescaler),                          \
		.ssd = DT_INST_PROP(index, spread_spectrum_deviation),                             \
		.max_count = DT_INST_STRING_UPPER_TOKEN(index, max_count_value),                   \
		.iodef = DT_INST_PROP(index, iodef_float),                                         \
		.sync_acq = DT_INST_PROP(index, synced_acquisition),                               \
		.sync_pol = DT_INST_PROP(index, syncpol_rising),                                   \
		.irq_func = stm32_tsc_irq_init_##index,                                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, stm32_tsc_init, NULL, &stm32_tsc_data_##index,                \
			      &stm32_tsc_cfg_##index, POST_KERNEL,                                 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_TSC_INIT)
