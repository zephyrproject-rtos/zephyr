/*
 * Copyright (c) 2024 Arif Balik <arifbalik@outlook.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <stdlib.h>
#include <stdbool.h>
#include <autoconf.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

LOG_MODULE_REGISTER(tsc_keys, CONFIG_INPUT_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_tsc

/* each group only has 4 configurable I/O */
#define GET_GROUP_BITS(val, group) (uint32_t)(((val) & 0x0f) << ((group - 1) * 4))

struct stm32_tsc_group_config {
	uint8_t group;
	uint8_t channel_ios;
	uint8_t sampling_io;
	bool use_as_shield;
};

typedef void (*stm32_tsc_group_ready_cb)(uint32_t count_value, void *user_data);

struct stm32_tsc_group_data {
	stm32_tsc_group_ready_cb cb;
	void *user_data;
};

struct stm32_tsc_config {
	const TSC_TypeDef *tsc;
	const struct stm32_pclken *pclken;
	struct reset_dt_spec reset;
	const struct pinctrl_dev_config *pcfg;
	const struct stm32_tsc_group_config *group_config;
	struct stm32_tsc_group_data *group_data;
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

int stm32_tsc_group_register_callback(const struct device *dev, uint8_t group_idx,
				      stm32_tsc_group_ready_cb cb, void *user_data)
{
	const struct stm32_tsc_config *config = dev->config;

	if (group_idx >= config->group_cnt) {
		LOG_ERR("%s: group index %d is out of range", dev->name, group_idx);
		return -EINVAL;
	}

	struct stm32_tsc_group_data *group_data = &config->group_data[group_idx];

	group_data->cb = cb;
	group_data->user_data = user_data;

	return 0;
}

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

static int get_group_index(const struct device *dev, uint8_t group, uint8_t *group_idx)
{
	const struct stm32_tsc_config *config = dev->config;
	const struct stm32_tsc_group_config *groups = config->group_config;

	for (int i = 0; i < config->group_cnt; i++) {
		if (groups[i].group == group) {
			*group_idx = i;
			return 0;
		}
	}

	return -ENODEV;
}

static int stm32_tsc_handle_incoming_data(const struct device *dev)
{
	const struct stm32_tsc_config *config = dev->config;

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
			const struct stm32_tsc_group_config *group = &config->group_config[i];
			uint32_t group_bit = BIT(group->group - 1) << 16;

			if (config->tsc->IOGCSR & group_bit) {
				uint32_t count_value = sys_read32(
					(mem_addr_t)&config->tsc->IOGXCR[group->group - 1]);

				uint8_t group_idx = 0;

				int ret = get_group_index(dev, group->group, &group_idx);

				if (ret < 0) {
					LOG_ERR("%s: group %d not found", dev->name, group->group);
					return ret;
				}

				struct stm32_tsc_group_data *data = &config->group_data[group_idx];

				if (data->cb) {
					data->cb(count_value, data->user_data);
				}
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
	sys_set_bits((mem_addr_t)&config->tsc->CR, config->pgpsc << TSC_CR_PGPSC_Pos);

	/* set max count value (bits 7:5) */
	sys_set_bits((mem_addr_t)&config->tsc->CR, config->max_count << TSC_CR_MCV_Pos);

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
		const struct stm32_tsc_group_config *group = &config->group_config[i];

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
		sys_clear_bits(
			(mem_addr_t)&config->tsc->IOHCR,
			GET_GROUP_BITS(group->channel_ios | group->sampling_io, group->group));

		/* set channel I/Os */
		sys_set_bits((mem_addr_t)&config->tsc->IOCCR,
			     GET_GROUP_BITS(group->channel_ios, group->group));

		/* set sampling I/O */
		sys_set_bits((mem_addr_t)&config->tsc->IOSCR,
			     GET_GROUP_BITS(group->sampling_io, group->group));

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
		.use_as_shield = DT_PROP(node_id, st_use_as_shield),                               \
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
	static const struct stm32_tsc_group_config group_config_cfg_##index[] = {                  \
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(index, STM32_TSC_GROUP_DEFINE, (,))};        \
                                                                                                   \
	static struct stm32_tsc_group_data                                                         \
		group_data_cfg_##index[DT_INST_CHILD_NUM_STATUS_OKAY(index)];                      \
                                                                                                   \
	static const struct stm32_tsc_config stm32_tsc_cfg_##index = {                             \
		.tsc = (TSC_TypeDef *)DT_INST_REG_ADDR(index),                                     \
		.pclken = pclken_##index,                                                          \
		.reset = RESET_DT_SPEC_INST_GET(index),                                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.group_config = group_config_cfg_##index,                                          \
		.group_data = group_data_cfg_##index,                                              \
		.group_cnt = DT_INST_CHILD_NUM_STATUS_OKAY(index),                                 \
		.pgpsc = LOG2CEIL(DT_INST_PROP(index, st_pulse_generator_prescaler)),              \
		.ctph = DT_INST_PROP(index, st_charge_transfer_pulse_high),                        \
		.ctpl = DT_INST_PROP(index, st_charge_transfer_pulse_low),                         \
		.spread_spectrum = DT_INST_PROP(index, st_spread_spectrum),                        \
		.sscpsc = DT_INST_PROP(index, st_spread_spectrum_prescaler),                       \
		.ssd = DT_INST_PROP(index, st_spread_spectrum_deviation),                          \
		.max_count = LOG2CEIL(DT_INST_PROP(index, st_max_count_value) + 1) - 8,            \
		.iodef = DT_INST_PROP(index, st_iodef_float),                                      \
		.sync_acq = DT_INST_PROP(index, st_synced_acquisition),                            \
		.sync_pol = DT_INST_PROP(index, st_syncpol_rising),                                \
		.irq_func = stm32_tsc_irq_init_##index,                                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, stm32_tsc_init, NULL, NULL, &stm32_tsc_cfg_##index,           \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_TSC_INIT)

struct input_tsc_keys_data {
	uint32_t buffer[CONFIG_INPUT_STM32_TSC_KEYS_BUFFER_WORD_SIZE];
	struct ring_buf rb;
	bool expect_release;
	struct k_timer sampling_timer;
};

struct input_tsc_keys_config {
	const struct device *tsc_dev;
	uint32_t sampling_interval_ms;
	int32_t noise_threshold;
	int zephyr_code;
	uint8_t group;
};

static void input_tsc_sampling_timer_callback(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);

	stm32_tsc_start(dev);
}

static void input_tsc_callback_handler(uint32_t count_value, void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	const struct input_tsc_keys_config *config =
		(const struct input_tsc_keys_config *)dev->config;
	struct input_tsc_keys_data *data = (struct input_tsc_keys_data *)dev->data;

	if (ring_buf_item_space_get(&data->rb) == 0) {
		uint32_t oldest_point;
		int32_t slope;

		(void)ring_buf_get(&data->rb, (uint8_t *)&oldest_point, sizeof(oldest_point));

		slope = count_value - oldest_point;
		if (slope < -config->noise_threshold && !data->expect_release) {
			data->expect_release = true;
			input_report_key(dev, config->zephyr_code, 1, false, K_NO_WAIT);
		} else if (slope > config->noise_threshold && data->expect_release) {
			data->expect_release = false;
			input_report_key(dev, config->zephyr_code, 0, false, K_NO_WAIT);
		}
	}

	(void)ring_buf_put(&data->rb, (uint8_t *)&count_value, sizeof(count_value));
}

static int input_tsc_keys_init(const struct device *dev)
{
	const struct input_tsc_keys_config *config = dev->config;
	struct input_tsc_keys_data *data = dev->data;

	if (!device_is_ready(config->tsc_dev)) {
		LOG_ERR("%s: TSC device not ready", config->tsc_dev->name);
		return -ENODEV;
	}

	ring_buf_item_init(&data->rb, CONFIG_INPUT_STM32_TSC_KEYS_BUFFER_WORD_SIZE, data->buffer);

	uint8_t group_index = 0;

	int ret = get_group_index(config->tsc_dev, config->group, &group_index);

	if (ret) {
		LOG_ERR("%s: group %d not found", config->tsc_dev->name, config->group);
		return ret;
	}

	ret = stm32_tsc_group_register_callback(config->tsc_dev, group_index,
						input_tsc_callback_handler, (void *)dev);

	if (ret) {
		LOG_ERR("%s: failed to register callback for group %d", config->tsc_dev->name,
			config->group);
		return ret;
	}

	k_timer_init(&data->sampling_timer, input_tsc_sampling_timer_callback, NULL);
	k_timer_user_data_set(&data->sampling_timer, (void *)config->tsc_dev);
	k_timer_start(&data->sampling_timer, K_NO_WAIT, K_MSEC(config->sampling_interval_ms));

	return 0;
}

#define TSC_KEYS_INIT(inst)                                                                        \
                                                                                                   \
	static struct input_tsc_keys_data tsc_keys_data_##inst;                                    \
                                                                                                   \
	static const struct input_tsc_keys_config tsc_keys_config_##inst = {                       \
		.tsc_dev = DEVICE_DT_GET(DT_GPARENT(inst)),                                        \
		.sampling_interval_ms = DT_PROP(inst, sampling_interval_ms),                       \
		.zephyr_code = DT_PROP(inst, zephyr_code),                                         \
		.noise_threshold = DT_PROP(inst, noise_threshold),                                 \
		.group = DT_PROP(DT_PARENT(inst), group),                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(inst, input_tsc_keys_init, NULL, &tsc_keys_data_##inst,                   \
			 &tsc_keys_config_##inst, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_FOREACH_STATUS_OKAY(tsc_keys, TSC_KEYS_INIT);
