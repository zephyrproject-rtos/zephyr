/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/kernel.h>

#define DT_DRV_COMPAT microchip_dmac_g1_dma

LOG_MODULE_REGISTER(dma_mchp_dmac_g1, CONFIG_DMA_LOG_LEVEL);

#define DMAC_REG ((const struct dma_mchp_dev_config *)(dev)->config)->regs

#define DMAC_BUF_ADDR_ALIGNMENT 4U
#define DMAC_BUF_SIZE_ALIGNMENT 4U
#define DMAC_COPY_ALIGNMENT     4U
#define DMAC_MAX_BLOCK_COUNT    65535U

#define TIMEOUT_VALUE_US 1000
#define DELAY_US         2

enum dma_mchp_ch_state {
	DMA_MCHP_CH_IDLE,
	DMA_MCHP_CH_PENDING,
	DMA_MCHP_CH_SUSPENDED,
	DMA_MCHP_CH_ACTIVE,
};

struct dma_mchp_channel_config {
	dma_callback_t cb;
	void *user_data;
	bool is_err_cb_dis;
	bool is_configured;
};

struct dma_mchp_dmac {
	/* DMA descriptors for channel configurations (must be 16-byte aligned). */
	__aligned(16) dmac_descriptor_registers_t descriptors[DMAC_CH_NUM];
	/* DMA write-back descriptors for tracking completed transfers (16-byte aligned). */
	__aligned(16) dmac_descriptor_registers_t descriptors_wb[DMAC_CH_NUM];
};

struct dma_mchp_dev_config {
	dmac_registers_t *regs;
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	uint8_t num_irq;
	void (*irq_config)(void);
};

struct dma_mchp_dev_data {
	struct dma_context dma_ctx;
	struct dma_mchp_dmac *dmac_desc_data;
	struct dma_mchp_channel_config *dma_channel_config;
};

static enum dma_mchp_ch_state dmac_ch_get_state(dmac_registers_t *dmac_reg, uint32_t channel)
{
	enum dma_mchp_ch_state ch_state;
	uint32_t active_status;
	uint8_t ch_int_flag, ch_status;

	ch_status = dmac_reg->CHANNEL[channel].DMAC_CHSTATUS;
	ch_int_flag = dmac_reg->CHANNEL[channel].DMAC_CHINTFLAG;

	if ((ch_status & DMAC_CHSTATUS_BUSY_Msk) == DMAC_CHSTATUS_BUSY_Msk) {
		active_status = dmac_reg->DMAC_ACTIVE;

		if (((active_status & DMAC_ACTIVE_ABUSY_Msk) == DMAC_ACTIVE_ABUSY_Msk) &&
		    (((active_status & DMAC_ACTIVE_ID_Msk) >> DMAC_ACTIVE_ID_Pos) == channel)) {
			ch_state = DMA_MCHP_CH_ACTIVE;
		} else if ((ch_int_flag & DMAC_CHINTFLAG_SUSP_Msk) == DMAC_CHINTFLAG_SUSP_Msk) {
			ch_state = DMA_MCHP_CH_SUSPENDED;
		} else {
			ch_state = DMA_MCHP_CH_IDLE;
		}
	} else if ((ch_status & DMAC_CHSTATUS_PEND_Msk) == DMAC_CHSTATUS_PEND_Msk) {
		ch_state = DMA_MCHP_CH_PENDING;
	} else {
		ch_state = DMA_MCHP_CH_IDLE;
	}

	return ch_state;
}

static inline void dmac_desc_init(const struct device *dev)
{
	struct dma_mchp_dmac *data =
		((const struct dma_mchp_dev_data *)(dev)->data)->dmac_desc_data;

	DMAC_REG->DMAC_BASEADDR = (uintptr_t)data->descriptors;
	DMAC_REG->DMAC_WRBADDR = (uintptr_t)data->descriptors_wb;
}

static int dmac_desc_block_config(struct dma_block_config *block,
				  dmac_descriptor_registers_t *desc, uint32_t src_data_size)
{
	uint16_t btctrl = 0;

	switch (src_data_size) {
	case 1:
		btctrl |= DMAC_BTCTRL_BEATSIZE_BYTE;
		break;
	case 2:
		btctrl |= DMAC_BTCTRL_BEATSIZE_HWORD;
		break;
	case 4:
		btctrl |= DMAC_BTCTRL_BEATSIZE_WORD;
		break;
	default:
		LOG_ERR("Invalid parameter for DMA source data size");
		return -EINVAL;
	}

	desc->DMAC_BTCNT = (uint16_t)(block->block_size / src_data_size);
	desc->DMAC_DESCADDR = 0;

	switch (block->source_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		desc->DMAC_SRCADDR = block->source_address + block->block_size;
		btctrl |= DMAC_BTCTRL_SRCINC(1);
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		desc->DMAC_SRCADDR = block->source_address;
		break;
	default:
		LOG_ERR("Invalid parameter for DMA source address");
		return -EINVAL;
	}

	switch (block->dest_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		desc->DMAC_DSTADDR = block->dest_address + block->block_size;
		btctrl |= DMAC_BTCTRL_DSTINC(1);
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		desc->DMAC_DSTADDR = block->dest_address;
		break;
	default:
		LOG_ERR("Invalid parameter for DMA destination address");
		return -EINVAL;
	}

	btctrl |= DMAC_BTCTRL_VALID(1);
	desc->DMAC_BTCTRL = btctrl;

	return 0;
}

static int dmac_desc_reload_block(struct dma_mchp_dmac *data, uint32_t channel, uint32_t src,
				  uint32_t dst, size_t size)
{
	dmac_descriptor_registers_t *desc = &data->descriptors[channel];
	dmac_descriptor_registers_t *desc_wb = &data->descriptors_wb[channel];

	if (desc->DMAC_DESCADDR != 0) {
		return -EINVAL;
	}

	switch (((DMAC_BTCTRL_BEATSIZE_Msk & desc->DMAC_BTCTRL) >> DMAC_BTCTRL_BEATSIZE_Pos)) {
	case DMAC_BTCTRL_BEATSIZE_BYTE_Val:
		desc->DMAC_BTCNT = (uint16_t)size;
		break;
	case DMAC_BTCTRL_BEATSIZE_HWORD_Val:
		desc->DMAC_BTCNT = (uint16_t)(size / 2U);
		break;
	case DMAC_BTCTRL_BEATSIZE_WORD_Val:
		desc->DMAC_BTCNT = (uint16_t)(size / 4U);
		break;
	default:
		LOG_ERR("Invalid configuration beat size");
		return -EINVAL;
	}

	if ((DMAC_BTCTRL_SRCINC_Msk & desc->DMAC_BTCTRL) != 0) {
		desc->DMAC_SRCADDR = src + size;
	} else {
		desc->DMAC_SRCADDR = src;
	}

	if ((DMAC_BTCTRL_DSTINC_Msk & desc->DMAC_BTCTRL) != 0) {
		desc->DMAC_DSTADDR = dst + size;
	} else {
		desc->DMAC_DSTADDR = dst;
	}

	desc_wb->DMAC_DSTADDR = desc->DMAC_DSTADDR;
	desc_wb->DMAC_SRCADDR = desc->DMAC_SRCADDR;
	desc_wb->DMAC_BTCNT = desc->DMAC_BTCNT;

	return 0;
}

static int dma_mchp_validate(const struct device *dev, uint32_t channel, struct dma_config *config)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;

	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (dmac_ch_get_state(DMAC_REG, channel) == DMA_MCHP_CH_ACTIVE) {
		LOG_ERR("DMA channel %d is already in use", channel);
		return -EBUSY;
	}

	if (config->source_data_size != config->dest_data_size) {
		LOG_ERR("Source and destination data sizes do not match");
		return -EINVAL;
	}

	if (config->dma_slot >= DMAC_TRIG_NUM) {
		LOG_ERR("Invalid DMA trigger source : %d", config->dma_slot);
		return -EINVAL;
	}

	if (config->channel_priority >= DMAC_LVL_NUM) {
		LOG_ERR("Invalid DMA priority level : %d", config->channel_priority);
		return -EINVAL;
	}

	if (config->source_burst_length != config->dest_burst_length) {
		LOG_ERR("Source and destination burst lengths do not match");
		return -EINVAL;
	}

	if (config->source_burst_length > 16U) {
		LOG_ERR("Burst length exceeds maximum allowed value : %d",
			config->source_burst_length);
		return -EINVAL;
	}

	switch (config->channel_direction) {
	case MEMORY_TO_MEMORY:
	case MEMORY_TO_PERIPHERAL:
	case PERIPHERAL_TO_MEMORY:
		break;
	default:
		LOG_ERR("Invalid DMA channel direction");
		return -EINVAL;
	}

	return 0;
}

static int dma_mchp_setup_channel(const struct device *dev, uint32_t channel,
				  const struct dma_config *config)
{
	uint32_t chctrla = 0;

	if (config->channel_direction == MEMORY_TO_MEMORY) {
		chctrla = DMAC_CHCTRLA_TRIGACT_TRANSACTION | DMAC_CHCTRLA_TRIGSRC(config->dma_slot);
	} else {
		chctrla = DMAC_CHCTRLA_TRIGACT_BURST | DMAC_CHCTRLA_TRIGSRC(config->dma_slot);
	}

	if (config->source_burst_length > 0U) {
		chctrla |= DMAC_CHCTRLA_BURSTLEN(config->source_burst_length - 1U);
	}

	DMAC_REG->CHANNEL[channel].DMAC_CHCTRLA = chctrla;
	DMAC_REG->CHANNEL[channel].DMAC_CHPRILVL = DMAC_CHPRILVL_PRILVL(config->channel_priority);
	DMAC_REG->CHANNEL[channel].DMAC_CHINTENSET = DMAC_CHINTENSET_TCMPL(1);

	if (!config->error_callback_dis) {
		DMAC_REG->CHANNEL[channel].DMAC_CHINTENSET = DMAC_CHINTENSET_TERR(1);
	} else {
		DMAC_REG->CHANNEL[channel].DMAC_CHINTENCLR = DMAC_CHINTENCLR_TERR(1);
	}

	DMAC_REG->CHANNEL[channel].DMAC_CHINTFLAG =
		DMAC_CHINTFLAG_TERR_Msk | DMAC_CHINTFLAG_TCMPL_Msk;

	return 0;
}

static void dma_mchp_isr(const struct device *dev)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;
	uint16_t pend = DMAC_REG->DMAC_INTPEND;
	uint32_t channel = (pend & DMAC_INTPEND_ID_Msk) >> DMAC_INTPEND_ID_Pos;

	DMAC_REG->DMAC_INTPEND = pend;

	if ((pend & (DMAC_INTPEND_TERR_Msk | DMAC_INTPEND_TCMPL_Msk)) == 0) {
		return;
	}

	struct dma_mchp_channel_config *cfg = &dev_data->dma_channel_config[channel];

	if (cfg->cb == NULL) {
		return;
	}

	if (pend & DMAC_INTPEND_TERR_Msk) {
		if (!cfg->is_err_cb_dis) {
			cfg->cb(dev, cfg->user_data, channel, -EIO);
		}
	} else {
		cfg->cb(dev, cfg->user_data, channel, DMA_STATUS_COMPLETE);
	}
}

static int dma_mchp_config(const struct device *dev, uint32_t channel, struct dma_config *config)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;
	struct dma_mchp_channel_config *channel_config;
	dmac_descriptor_registers_t *desc;
	dmac_descriptor_registers_t *desc_wb;
	int ret;

	ret = dma_mchp_validate(dev, channel, config);
	if (ret != 0) {
		return ret;
	}

	ret = dma_mchp_setup_channel(dev, channel, config);
	if (ret != 0) {
		return ret;
	}

	if (config->block_count > 1) {
		LOG_ERR("Multi block transfers not supported");
		return -ENOTSUP;
	}

	desc = &dev_data->dmac_desc_data->descriptors[channel];

	ret = dmac_desc_block_config(config->head_block, desc, config->source_data_size);
	if (ret != 0) {
		return ret;
	}

	desc_wb = &dev_data->dmac_desc_data->descriptors_wb[channel];
	desc_wb->DMAC_SRCADDR = desc->DMAC_SRCADDR;
	desc_wb->DMAC_DSTADDR = desc->DMAC_DSTADDR;
	desc_wb->DMAC_BTCNT = desc->DMAC_BTCNT;

	atomic_set_bit(dev_data->dma_ctx.atomic, channel);

	channel_config = &dev_data->dma_channel_config[channel];
	channel_config->cb = config->dma_callback;
	channel_config->user_data = config->user_data;
	channel_config->is_configured = true;
	channel_config->is_err_cb_dis = config->error_callback_dis;

	return 0;
}

static int dma_mchp_start(const struct device *dev, uint32_t channel)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;

	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (dmac_ch_get_state(DMAC_REG, channel) == DMA_MCHP_CH_ACTIVE) {
		LOG_ERR("DMA channel:%d is currently busy", channel);
		return -EBUSY;
	}

	if (!dev_data->dma_channel_config[channel].is_configured) {
		LOG_ERR("DMA descriptors not configured for channel : %d", channel);
		return -EINVAL;
	}

	DMAC_REG->CHANNEL[channel].DMAC_CHCTRLA |= DMAC_CHCTRLA_ENABLE(1);

	if ((DMAC_REG->CHANNEL[channel].DMAC_CHCTRLA & DMAC_CHCTRLA_TRIGSRC_Msk) == 0) {
		DMAC_REG->DMAC_SWTRIGCTRL = BIT(channel);
	}

	return 0;
}

static int dma_mchp_stop(const struct device *dev, uint32_t channel)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;

	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	DMAC_REG->CHANNEL[channel].DMAC_CHCTRLA &= ~DMAC_CHCTRLA_ENABLE(1);

	return 0;
}

static int dma_mchp_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			   size_t size)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;

	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (dmac_ch_get_state(DMAC_REG, channel) == DMA_MCHP_CH_ACTIVE) {
		LOG_ERR("DMA channel:%d is currently busy", channel);
		return -EBUSY;
	}

	if (!dev_data->dma_channel_config[channel].is_configured) {
		LOG_ERR("DMA descriptors not configured for channel : %d", channel);
		return -EINVAL;
	}

	return dmac_desc_reload_block(dev_data->dmac_desc_data, channel, src, dst, size);
}

static int dma_mchp_suspend(const struct device *dev, uint32_t channel)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;
	uint32_t chctrlb;

	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (dmac_ch_get_state(DMAC_REG, channel) != DMA_MCHP_CH_ACTIVE) {
		LOG_INF("nothing to suspend as dma channel %u is not busy", channel);
		return 0;
	}

	chctrlb = DMAC_REG->CHANNEL[channel].DMAC_CHCTRLB;
	chctrlb &= ~DMAC_CHCTRLB_CMD_Msk;
	chctrlb |= DMAC_CHCTRLB_CMD_SUSPEND;
	DMAC_REG->CHANNEL[channel].DMAC_CHCTRLB = chctrlb;

	return 0;
}

static int dma_mchp_resume(const struct device *dev, uint32_t channel)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;
	uint32_t chctrlb;

	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (dmac_ch_get_state(DMAC_REG, channel) != DMA_MCHP_CH_SUSPENDED) {
		LOG_INF("DMA channel %d is not in suspended state", channel);
		return -EINVAL;
	}

	chctrlb = DMAC_REG->CHANNEL[channel].DMAC_CHCTRLB;
	chctrlb &= ~DMAC_CHCTRLB_CMD_Msk;
	chctrlb |= DMAC_CHCTRLB_CMD_RESUME;
	DMAC_REG->CHANNEL[channel].DMAC_CHCTRLB = chctrlb;

	DMAC_REG->CHANNEL[channel].DMAC_CHINTFLAG = DMAC_CHINTFLAG_SUSP(1);

	return 0;
}

static int dma_mchp_get_status(const struct device *dev, uint32_t channel, struct dma_status *stat)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;
	struct dma_mchp_dmac *data = dev_data->dmac_desc_data;
	uint32_t active;

	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	active = DMAC_REG->DMAC_ACTIVE;

	if (dmac_ch_get_state(DMAC_REG, channel) == DMA_MCHP_CH_ACTIVE) {
		stat->busy = true;
		stat->pending_length = (active & DMAC_ACTIVE_BTCNT_Msk) >> DMAC_ACTIVE_BTCNT_Pos;
	} else {
		stat->busy = false;
		stat->pending_length = data->descriptors_wb[channel].DMAC_BTCNT;
	}

	switch (((DMAC_BTCTRL_BEATSIZE_Msk & data->descriptors[channel].DMAC_BTCTRL) >>
		 DMAC_BTCTRL_BEATSIZE_Pos)) {
	case DMAC_BTCTRL_BEATSIZE_BYTE_Val:
		break;
	case DMAC_BTCTRL_BEATSIZE_HWORD_Val:
		stat->pending_length *= 2U;
		break;
	case DMAC_BTCTRL_BEATSIZE_WORD_Val:
		stat->pending_length *= 4U;
		break;
	default:
		LOG_ERR("Invalid configuration beat size");
		return -EINVAL;
	}

	return 0;
}

static bool dma_mchp_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	ARG_UNUSED(dev);

	if (filter_param == NULL) {
		return true;
	}

	return (channel == *(uint32_t *)filter_param);
}

static int dma_mchp_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	ARG_UNUSED(dev);

	switch ((enum dma_attribute_type)type) {
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*value = DMAC_BUF_ADDR_ALIGNMENT;
		break;
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
		*value = DMAC_BUF_SIZE_ALIGNMENT;
		break;
	case DMA_ATTR_COPY_ALIGNMENT:
		*value = DMAC_COPY_ALIGNMENT;
		break;
	case DMA_ATTR_MAX_BLOCK_COUNT:
		*value = DMAC_MAX_BLOCK_COUNT;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int dma_mchp_init(const struct device *dev)
{
	const struct dma_mchp_dev_config *dev_cfg = dev->config;
	int ret;

	ret = clock_control_on(dev_cfg->clock_dev, dev_cfg->mclk_sys);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to enable MCLK for DMA: %d", ret);
		return ret;
	}

	DMAC_REG->DMAC_CTRL &= ~DMAC_CTRL_DMAENABLE_Msk;
	DMAC_REG->DMAC_CTRL |= DMAC_CTRL_SWRST_Msk;

	if (!WAIT_FOR(((DMAC_REG->DMAC_CTRL & DMAC_CTRL_SWRST_Msk) == 0), TIMEOUT_VALUE_US,
		      k_busy_wait(DELAY_US))) {
		LOG_ERR("DMAC reset timed out");
		return -ETIMEDOUT;
	}

	dmac_desc_init(dev);

	DMAC_REG->DMAC_PRICTRL0 = DMAC_PRICTRL0_LVLPRI0(0) | DMAC_PRICTRL0_LVLPRI1(1) |
				  DMAC_PRICTRL0_LVLPRI2(2) | DMAC_PRICTRL0_LVLPRI3(3);

	DMAC_REG->DMAC_CTRL = DMAC_CTRL_DMAENABLE(1) | DMAC_CTRL_LVLEN(0x0F);

	dev_cfg->irq_config();

	return 0;
}

static DEVICE_API(dma, dma_mchp_api) = {
	.config = dma_mchp_config,
	.start = dma_mchp_start,
	.stop = dma_mchp_stop,
	.reload = dma_mchp_reload,
	.get_status = dma_mchp_get_status,
	.suspend = dma_mchp_suspend,
	.resume = dma_mchp_resume,
	.chan_filter = dma_mchp_chan_filter,
	.get_attribute = dma_mchp_get_attribute,
};

#define DMA_MCHP_IRQ_HANDLER_DECL(n) static void mchp_dma_irq_connect_##n(void)

#define DMA_MCHP_IRQ_CONNECT(idx, n)                                                               \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, idx), (                                                  \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, idx, irq),                                       \
			DT_INST_IRQ_BY_IDX(n, idx, priority),                                      \
			dma_mchp_isr,                                                              \
			DEVICE_DT_INST_GET(n), 0);                                                 \
		irq_enable(DT_INST_IRQ_BY_IDX(n, idx, irq));                                       \
	))

#define DMA_MCHP_IRQ_HANDLER(n)                                                                    \
	static void mchp_dma_irq_connect_##n(void)                                                 \
	{                                                                                          \
		LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)), DMA_MCHP_IRQ_CONNECT, (), n)                  \
	}

#define DMA_MCHP_DATA_DEFN(n)                                                                      \
	static struct dma_mchp_dmac dmac_desc_data_##n;                                            \
	ATOMIC_DEFINE(dma_mchp_atomic##n, DT_INST_PROP(n, dma_channels));                          \
	static struct dma_mchp_channel_config                                                      \
		dma_channel_config_##n[DT_INST_PROP(n, dma_channels)];                             \
	static struct dma_mchp_dev_data dma_mchp_dev_data_##n = {                                  \
		.dma_ctx =                                                                         \
			{                                                                          \
				.magic = DMA_MAGIC,                                                \
				.atomic = dma_mchp_atomic##n,                                      \
				.dma_channels = DT_INST_PROP(n, dma_channels),                     \
			},                                                                         \
		.dmac_desc_data = &dmac_desc_data_##n,                                             \
		.dma_channel_config = dma_channel_config_##n,                                      \
	};

#define DMA_MCHP_CONFIG_DEFN(n)                                                                    \
	static const struct dma_mchp_dev_config dma_mchp_dev_config_##n = {                        \
		.regs = ((dmac_registers_t *)DT_INST_REG_ADDR(n)),                                 \
		.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),             \
		.num_irq = DT_NUM_IRQS(DT_DRV_INST(n)),                                            \
		.irq_config = mchp_dma_irq_connect_##n,                                            \
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock))};

#define DMA_MCHP_DEVICE_INIT(n)                                                                    \
	DMA_MCHP_IRQ_HANDLER_DECL(n);                                                              \
	DMA_MCHP_DATA_DEFN(n);                                                                     \
	DMA_MCHP_CONFIG_DEFN(n);                                                                   \
	DEVICE_DT_INST_DEFINE(n, &dma_mchp_init, NULL, &dma_mchp_dev_data_##n,                     \
			      &dma_mchp_dev_config_##n, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,    \
			      &dma_mchp_api);                                                      \
	DMA_MCHP_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(DMA_MCHP_DEVICE_INIT);
