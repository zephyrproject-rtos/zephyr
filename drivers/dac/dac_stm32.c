/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 * Copyright (c) 2021 Shlomi Vaknin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_dac

#include <errno.h>

#include <drivers/dac.h>
#include <device.h>
#include <kernel.h>
#include <init.h>
#include <soc.h>
#include <stm32_ll_dac.h>
#include <drivers/dma.h>
#include <dt-bindings/dma/stm32_dma.h>

#define LOG_LEVEL CONFIG_DAC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(dac_stm32);

#include <drivers/clock_control/stm32_clock_control.h>
#include <pinmux/stm32/pinmux_stm32.h>

/* some low-end MCUs have DAC with only one channel */
#ifdef LL_DAC_CHANNEL_2
#define STM32_CHANNEL_COUNT		2
#else
#define STM32_CHANNEL_COUNT		1
#endif

/* first channel always named 1 */
#define STM32_FIRST_CHANNEL		1

#define CHAN(n)		LL_DAC_CHANNEL_##n
static const uint32_t table_channels[] = {
	CHAN(1),
#ifdef LL_DAC_CHANNEL_2
	CHAN(2),
#endif
};

#ifdef CONFIG_DAC_CONTINIOUS_API
struct dac_dma_stream {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	uint8_t priority;
	bool src_addr_increment;
	bool dst_addr_increment;
	int fifo_threshold;
	struct dma_block_config blk_cfg;
};

struct dac_dma_user_data {
	const struct device *dac;
	uint8_t channel;
};
#endif

/* Read-only driver configuration */
struct dac_stm32_cfg {
	/* DAC instance. */
	DAC_TypeDef *base;
	/* Clock configuration. */
	struct stm32_pclken pclken;
	/* pinctrl configurations. */
	const struct soc_gpio_pinctrl *pinctrl;
	/* Number of pinctrl configurations. */
	size_t pinctrl_len;
};

/* Runtime driver data */
struct dac_stm32_data {
	uint8_t channel_count;
	uint8_t resolution;

	uint8_t trigger_source;

#ifdef CONFIG_DAC_CONTINIOUS_API
	struct dac_dma_stream dma_channel1;
	struct dac_dma_stream dma_channel2;

	dac_callback_t callback;
	void *user_data;

	uint8_t dma_buffer[STM32_CHANNEL_COUNT][CONFIG_STM32_DAC_BUFFER_SIZE];

	struct dac_dma_user_data dma_user_data[STM32_CHANNEL_COUNT];

	uint16_t dma_buffer_pos[STM32_CHANNEL_COUNT];
	uint8_t next_dma_buffer_pos[STM32_CHANNEL_COUNT];
	uint16_t dma_buffer_size[STM32_CHANNEL_COUNT];
#endif
};

static const uint32_t stm32_dac_table_triggers[] = {
	LL_DAC_TRIG_SOFTWARE,
	LL_DAC_TRIG_EXT_TIM1_TRGO,
	LL_DAC_TRIG_EXT_TIM2_TRGO,
	LL_DAC_TRIG_EXT_TIM4_TRGO,
	LL_DAC_TRIG_EXT_TIM5_TRGO,
	LL_DAC_TRIG_EXT_TIM6_TRGO,
	LL_DAC_TRIG_EXT_TIM7_TRGO,
	LL_DAC_TRIG_EXT_TIM8_TRGO,
	LL_DAC_TRIG_EXT_TIM15_TRGO,
	LL_DAC_TRIG_EXT_LPTIM1_OUT,
	LL_DAC_TRIG_EXT_LPTIM2_OUT,
	LL_DAC_TRIG_EXT_EXTI_LINE9,
};

static int dac_stm32_write_value(const struct device *dev,
			uint8_t channel, uint32_t value)
{
	struct dac_stm32_data *data = dev->data;
	const struct dac_stm32_cfg *cfg = dev->config;

	if (channel - STM32_FIRST_CHANNEL >= data->channel_count ||
					channel < STM32_FIRST_CHANNEL) {
		LOG_ERR("Channel %d is not valid", channel);
		return -EINVAL;
	}

	if (data->resolution == 8) {
		LL_DAC_ConvertData8RightAligned(cfg->base,
			table_channels[channel - STM32_FIRST_CHANNEL], value);
	} else if (data->resolution == 12) {
		LL_DAC_ConvertData12RightAligned(cfg->base,
			table_channels[channel - STM32_FIRST_CHANNEL], value);
	}

	return 0;
}

#ifdef CONFIG_DAC_CONTINIOUS_API
void dac_stm32_dma_cb(const struct device *dma_dev, void *user_data,
			       uint32_t dma_channel, int status)
{
	struct dac_dma_user_data *dma_user_data = user_data;
	const struct device *dac_dev = dma_user_data->dac;
	uint8_t channel = dma_user_data->channel;
	struct dac_stm32_data *data = dac_dev->data;

	if (status != 0) {
		LOG_ERR("DAC: unknown dma callback error: %d", status);
		return;
	}

	data->next_dma_buffer_pos[channel - STM32_FIRST_CHANNEL] ^= 1;

	data->dma_buffer_pos[channel - STM32_FIRST_CHANNEL] =
			data->next_dma_buffer_pos[channel - STM32_FIRST_CHANNEL] *
			CONFIG_STM32_DAC_BUFFER_SIZE / 2;

	data->dma_buffer_size[channel - STM32_FIRST_CHANNEL] =
			CONFIG_STM32_DAC_BUFFER_SIZE / 2;

	if (data->callback) {
		data->callback(dac_dev, channel, data->user_data);
	}
}

static int dac_stm32_callback_set(const struct device *dev,
				    dac_callback_t callback, void *user_data)
{
	struct dac_stm32_data *data = dev->data;

	data->callback = callback;
	data->user_data = user_data;

	return 0;
}

static int dac_stm32_start_continious(const struct device *dev,
					uint8_t channel)
{
	struct dac_stm32_data *data = dev->data;
    const struct dac_stm32_cfg *config = dev->config;
	DAC_TypeDef *dac = (DAC_TypeDef *)config->base;
    struct dac_dma_stream *dac_stream;
    int ret;

	if (channel - STM32_FIRST_CHANNEL >= data->channel_count ||
					channel < STM32_FIRST_CHANNEL) {
		LOG_ERR("Channel %d is not valid", channel);
		return -EINVAL;
	}

    if (channel == 1) {
		dac_stream = &data->dma_channel1;
        if (dac_stream->dma_dev == NULL) {
            LOG_ERR("DAC ERR: CHANNEL1 DMA device not found!");
            return -ENODEV;
        }
    } else if (channel == 2) {
		dac_stream = &data->dma_channel2;
        if (dac_stream->dma_dev == NULL) {
            LOG_ERR("DAC ERR: CHANNEL2 DMA device not found!");
            return -ENODEV;
        }
    } else {
        LOG_ERR("Channel %d is not valid", channel);
		return -EINVAL;
    }

	dac_stream->blk_cfg.block_size = sizeof(data->dma_buffer[0]);
	dac_stream->blk_cfg.source_address =
				(uint32_t)data->dma_buffer[channel - STM32_FIRST_CHANNEL];

	ret = dma_config(dac_stream->dma_dev, dac_stream->dma_channel,
				&dac_stream->dma_cfg);

	if (ret != 0) {
		LOG_ERR("DAC ERR: DMA config failed!");
		return -EINVAL;
	}

	if (dma_start(dac_stream->dma_dev, dac_stream->dma_channel)) {
		LOG_ERR("DAC ERR: DMA start failed!");
		return -EFAULT;
	}

    LL_DAC_EnableDMAReq(dac, table_channels[channel - STM32_FIRST_CHANNEL]);

	return 0;
}

static int dac_stm32_fill_buffer(const struct device *dev, uint8_t channel,
					uint8_t *data, size_t size)
{
	struct dac_stm32_data *dac_data = dev->data;

	uint8_t *dma_buffer = dac_data->dma_buffer[channel - STM32_FIRST_CHANNEL] +
				dac_data->dma_buffer_pos[channel - STM32_FIRST_CHANNEL];
	uint16_t max_write_size =
				dac_data->dma_buffer_size[channel - STM32_FIRST_CHANNEL];
	uint16_t actual_write_size = MIN(max_write_size, size);

	memcpy(dma_buffer, data, actual_write_size);

	dac_data->dma_buffer_pos[channel - STM32_FIRST_CHANNEL] +=
					actual_write_size;
	dac_data->dma_buffer_size[channel - STM32_FIRST_CHANNEL] -=
					actual_write_size;

	return actual_write_size;
}
#endif

static int dac_stm32_channel_setup(const struct device *dev,
				   const struct dac_channel_cfg *channel_cfg)
{
	struct dac_stm32_data *data = dev->data;
	const struct dac_stm32_cfg *cfg = dev->config;

	if ((channel_cfg->channel_id - STM32_FIRST_CHANNEL >=
			data->channel_count) ||
			(channel_cfg->channel_id < STM32_FIRST_CHANNEL)) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	if ((channel_cfg->resolution == 8) ||
			(channel_cfg->resolution == 12)) {
		data->resolution = channel_cfg->resolution;
	} else {
		LOG_ERR("Resolution not supported");
		return -ENOTSUP;
	}

	/* enable output buffer by default */
	LL_DAC_SetOutputBuffer(cfg->base,
		table_channels[channel_cfg->channel_id - STM32_FIRST_CHANNEL],
		LL_DAC_OUTPUT_BUFFER_ENABLE);

	if (channel_cfg->enable_hw_trigger) {
		LL_DAC_EnableTrigger(cfg->base,
			table_channels[channel_cfg->channel_id - STM32_FIRST_CHANNEL]);

		LL_DAC_SetTriggerSource(cfg->base,
			table_channels[channel_cfg->channel_id - STM32_FIRST_CHANNEL],
			stm32_dac_table_triggers[data->trigger_source]);
	}

	LL_DAC_Enable(cfg->base,
		table_channels[channel_cfg->channel_id - STM32_FIRST_CHANNEL]);

	LOG_DBG("Channel setup succeeded!");

	return 0;
}

#ifdef CONFIG_DAC_CONTINIOUS_API
static int dac_stm32_dma_init(const struct device *dev)
{
    struct dac_stm32_data *data = dev->data;
    const struct dac_stm32_cfg *config = dev->config;
	DAC_TypeDef *dac = (DAC_TypeDef *)config->base;

    if (data->dma_channel1.dma_dev != NULL) {
		if (!device_is_ready(data->dma_channel1.dma_dev)) {
			LOG_ERR("channel1 dma device not ready");
			return -ENODEV;
		}
	}

	data->dma_channel1.blk_cfg.dest_address =
			LL_DAC_DMA_GetRegAddr(dac, LL_DAC_CHANNEL_1,
					LL_DAC_DMA_REG_DATA_12BITS_RIGHT_ALIGNED);
	data->dma_channel1.blk_cfg.source_address = 0; /* source not ready */

	if (data->dma_channel1.src_addr_increment) {
		data->dma_channel1.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_channel1.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	if (data->dma_channel1.dst_addr_increment) {
		data->dma_channel1.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_channel1.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	/* enable circular buffer */
	data->dma_channel1.blk_cfg.source_reload_en  = 1;
	data->dma_channel1.blk_cfg.dest_reload_en = 1;
	data->dma_channel1.blk_cfg.fifo_mode_control =
				data->dma_channel1.fifo_threshold;

	data->dma_channel1.dma_cfg.head_block = &data->dma_channel1.blk_cfg;
	data->dma_channel1.dma_cfg.user_data = (void *)&data->dma_user_data[0];

    if (data->dma_channel2.dma_dev != NULL) {
		if (!device_is_ready(data->dma_channel1.dma_dev)) {
			LOG_ERR("channel2 dma device not ready");
			return -ENODEV;
		}
	}

	data->dma_channel2.blk_cfg.dest_address =
			LL_DAC_DMA_GetRegAddr(dac, LL_DAC_CHANNEL_2,
				LL_DAC_DMA_REG_DATA_12BITS_RIGHT_ALIGNED);
	data->dma_channel2.blk_cfg.source_address = 0; /* source not ready */

	if (data->dma_channel2.src_addr_increment) {
		data->dma_channel2.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_channel2.blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	if (data->dma_channel2.dst_addr_increment) {
		data->dma_channel2.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	} else {
		data->dma_channel2.blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	/* RX disable circular buffer */
	data->dma_channel2.blk_cfg.source_reload_en  = 1;
	data->dma_channel2.blk_cfg.dest_reload_en = 1;
	data->dma_channel2.blk_cfg.fifo_mode_control =
		data->dma_channel2.fifo_threshold;

	data->dma_channel2.dma_cfg.head_block = &data->dma_channel2.blk_cfg;
	data->dma_channel2.dma_cfg.user_data = (void *)&data->dma_user_data[1];

	for (uint8_t i = 0; i < STM32_CHANNEL_COUNT; ++i) {
		data->dma_user_data[i].dac = dev;
		data->dma_user_data[i].channel = i + 1;

		data->dma_buffer_size[i] = CONFIG_STM32_DAC_BUFFER_SIZE / 2;
		data->dma_buffer_pos[i] = CONFIG_STM32_DAC_BUFFER_SIZE / 2;
		data->next_dma_buffer_pos[i] = 1;
	}

    return 0;
}
#endif

static int dac_stm32_init(const struct device *dev)
{
	const struct dac_stm32_cfg *cfg = dev->config;
	int err;

	/* enable clock for subsystem */
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (clock_control_on(clk,
			     (clock_control_subsys_t *) &cfg->pclken) != 0) {
		return -EIO;
	}

	/* Configure dt provided device signals when available */
	err = stm32_dt_pinctrl_configure(cfg->pinctrl,
					 cfg->pinctrl_len,
					 (uint32_t)cfg->base);
	if (err < 0) {
		LOG_ERR("DAC pinctrl setup failed (%d)", err);
		return err;
	}

#ifdef CONFIG_DAC_CONTINIOUS_API
    return dac_stm32_dma_init(dev);
#else
	return 0;
#endif
}

static const struct dac_driver_api api_stm32_driver_api = {
	.channel_setup = dac_stm32_channel_setup,
	.write_value = dac_stm32_write_value,
#ifdef CONFIG_DAC_CONTINIOUS_API
	.callback_set = dac_stm32_callback_set,
	.start_continious = dac_stm32_start_continious,
	.fill_buffer = dac_stm32_fill_buffer
#endif
};

#define DMA_CHANNEL_CONFIG(id, dir)					\
	DT_INST_DMAS_CELL_BY_NAME(id, dir, channel_config)
#define DMA_FEATURES(id, dir)						\
	DT_INST_DMAS_CELL_BY_NAME(id, dir, features)

/* src_dev and dest_dev should be 'MEMORY' or 'PERIPHERAL'. */
#define DAC_DMA_CHANNEL_INIT(index, dir, dir_cap, src_dev, dest_dev)	\
	.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(index, dir)),	\
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(index, dir, channel),	\
	.dma_cfg = {							\
		.dma_slot = DT_INST_DMAS_CELL_BY_NAME(index, dir, slot),\
		.channel_direction = STM32_DMA_CONFIG_DIRECTION(	\
					DMA_CHANNEL_CONFIG(index, dir)),\
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(		\
				DMA_CHANNEL_CONFIG(index, dir)),	\
		.source_data_size = STM32_DMA_CONFIG_##src_dev##_DATA_SIZE(\
					DMA_CHANNEL_CONFIG(index, dir)),\
		.dest_data_size = STM32_DMA_CONFIG_##dest_dev##_DATA_SIZE(\
				DMA_CHANNEL_CONFIG(index, dir)),\
		.source_burst_length = 1, /* SINGLE transfer */		\
		.dest_burst_length = 1,					\
		.block_count = 1,					\
		.dma_callback = dac_stm32_dma_cb,		\
	},								\
	.src_addr_increment = STM32_DMA_CONFIG_##src_dev##_ADDR_INC(	\
				DMA_CHANNEL_CONFIG(index, dir)),	\
	.dst_addr_increment = STM32_DMA_CONFIG_##dest_dev##_ADDR_INC(	\
				DMA_CHANNEL_CONFIG(index, dir)),	\
	.fifo_threshold = STM32_DMA_FEATURES_FIFO_THRESHOLD(		\
				DMA_FEATURES(index, dir)),		\

#ifdef CONFIG_DAC_CONTINIOUS_API
#define DAC_DMA_CHANNEL(index, dir, DIR, src, dest)			\
.dma_##dir = {				\
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, dir),			\
		 (DAC_DMA_CHANNEL_INIT(index, dir, DIR, src, dest)),	\
		 (NULL))				\
	},
#else
#define DAC_DMA_CHANNEL(index, dir, DIR, src, dest)
#endif

#define STM32_DAC_INIT(index)						\
									\
static const struct soc_gpio_pinctrl dac_pins_##index[] =		\
	ST_STM32_DT_INST_PINCTRL(index, 0);				\
									\
static const struct dac_stm32_cfg dac_stm32_cfg_##index = {		\
	.base = (DAC_TypeDef *)DT_INST_REG_ADDR(index),			\
	.pclken = {							\
		.enr = DT_INST_CLOCKS_CELL(index, bits),		\
		.bus = DT_INST_CLOCKS_CELL(index, bus),			\
	},								\
	.pinctrl = dac_pins_##index,					\
	.pinctrl_len = ARRAY_SIZE(dac_pins_##index),			\
};									\
									\
static struct dac_stm32_data dac_stm32_data_##index = {			\
	.channel_count = STM32_CHANNEL_COUNT,				\
    DAC_DMA_CHANNEL(index, channel1, CHANNEL1, MEMORY, PERIPHERAL)		\
    DAC_DMA_CHANNEL(index, channel2, CHANNEL2, MEMORY, PERIPHERAL)		\
	.trigger_source = DT_INST_PROP(index, st_external_trigger),   \
};									\
									\
DEVICE_DT_INST_DEFINE(index, &dac_stm32_init, device_pm_control_nop,	\
		    &dac_stm32_data_##index,				\
		    &dac_stm32_cfg_##index, POST_KERNEL,		\
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,		\
		    &api_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_DAC_INIT)
