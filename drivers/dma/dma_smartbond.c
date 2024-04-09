/*
 * Copyright (c) 2023 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/irq.h>
#include <DA1469xAB.h>
#include <da1469x_config.h>
#include <system_DA1469x.h>
#include <da1469x_otp.h>
#include <zephyr/drivers/dma/dma_smartbond.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dma_smartbond, CONFIG_DMA_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_smartbond_dma

#define SMARTBOND_IRQN      DT_INST_IRQN(0)
#define SMARTBOND_IRQ_PRIO  DT_INST_IRQ(0, priority)

#define DMA_CHANNELS_COUNT   DT_PROP(DT_NODELABEL(dma), dma_channels)
#define DMA_BLOCK_COUNT     DT_PROP(DT_NODELABEL(dma), block_count)
#define DMA_SECURE_CHANNEL  7

#define DMA_CTRL_REG_SET_FIELD(_field, _var, _val) \
	(_var) = \
	(((_var) & ~DMA_DMA0_CTRL_REG_ ## _field ## _Msk) | \
	(((_val) << DMA_DMA0_CTRL_REG_ ## _field ## _Pos) & DMA_DMA0_CTRL_REG_ ## _field ## _Msk))

#define DMA_CTRL_REG_GET_FIELD(_field, _var) \
	(((_var) & DMA_DMA0_CTRL_REG_ ## _field ## _Msk) >> DMA_DMA0_CTRL_REG_ ## _field ## _Pos)

#define DMA_CHN2REG(_idx)   (&((struct channel_regs *)DMA)[(_idx)])

#define DMA_MUX_SHIFT(_idx)   (((_idx) >> 1) * 4)

#define DMA_REQ_MUX_REG_SET(_idx, _val) \
	DMA->DMA_REQ_MUX_REG = \
		(DMA->DMA_REQ_MUX_REG & ~(0xf << DMA_MUX_SHIFT((_idx)))) | \
		(((_val) & 0xf) << DMA_MUX_SHIFT((_idx)))

#define DMA_REQ_MUX_REG_GET(_idx) \
	((DMA->DMA_REQ_MUX_REG >> DMA_MUX_SHIFT((_idx))) & 0xf)

#define CRYPTO_KEYS_BUF_ADDR   0x30040100
#define CRYPTO_KEYS_BUF_SIZE   0x100
#define IS_AES_KEYS_BUF_RANGE(_a)  ((uint32_t)(_a) >= (uint32_t)(CRYPTO_KEYS_BUF_ADDR)) && \
	((uint32_t)(_a) < (uint32_t)(CRYPTO_KEYS_BUF_ADDR + CRYPTO_KEYS_BUF_SIZE))

/*
 * DMA channel priority level. The smaller the value the lower the priority granted to a channel
 * when two or more channels request the bus at the same time. For channels of same priority an
 * inherent mechanism is applied in which the lower the channel number the higher the priority.
 */
enum dma_smartbond_channel_prio {
	DMA_SMARTBOND_CHANNEL_PRIO_0 = 0x0,  /* Lowest channel priority */
	DMA_SMARTBOND_CHANNEL_PRIO_1,
	DMA_SMARTBOND_CHANNEL_PRIO_2,
	DMA_SMARTBOND_CHANNEL_PRIO_3,
	DMA_SMARTBOND_CHANNEL_PRIO_4,
	DMA_SMARTBOND_CHANNEL_PRIO_5,
	DMA_SMARTBOND_CHANNEL_PRIO_6,
	DMA_SMARTBOND_CHANNEL_PRIO_7,         /* Highest channel priority */
	DMA_SMARTBOND_CHANNEL_PRIO_MAX
};

enum dma_smartbond_channel {
	DMA_SMARTBOND_CHANNEL_0 = 0x0,
	DMA_SMARTBOND_CHANNEL_1,
	DMA_SMARTBOND_CHANNEL_2,
	DMA_SMARTBOND_CHANNEL_3,
	DMA_SMARTBOND_CHANNEL_4,
	DMA_SMARTBOND_CHANNEL_5,
	DMA_SMARTBOND_CHANNEL_6,
	DMA_SMARTBOND_CHANNEL_7,
	DMA_SMARTBOND_CHANNEL_MAX
};

enum dma_smartbond_burst_len {
	DMA_SMARTBOND_BURST_LEN_1B  = 0x1, /* Burst mode is disabled */
	DMA_SMARTBOND_BURST_LEN_4B  = 0x4, /* Perform bursts of 4 beats (INCR4) */
	DMA_SMARTBOND_BURST_LEN_8B  = 0x8  /* Perform bursts of 8 beats (INCR8) */
};

/*
 * DMA bus width indicating how many bytes are retrived/written per transfer.
 * Note that the bus width is the same for the source and destination.
 */
enum dma_smartbond_bus_width {
	DMA_SMARTBOND_BUS_WIDTH_1B = 0x1,
	DMA_SMARTBOND_BUS_WIDTH_2B = 0x2,
	DMA_SMARTBOND_BUS_WIDTH_4B = 0x4
};

enum dreq_mode {
	DREQ_MODE_SW = 0x0,
	DREQ_MODE_HW
};

enum burst_mode {
	BURST_MODE_0B = 0x0,
	BURST_MODE_4B = 0x1,
	BURST_MODE_8B = 0x2
};

enum bus_width {
	BUS_WIDTH_1B = 0x0,
	BUS_WIDTH_2B = 0x1,
	BUS_WIDTH_4B = 0x2
};

enum addr_adj {
	ADDR_ADJ_NO_CHANGE = 0x0,
	ADDR_ADJ_INCR
};

enum copy_mode {
	COPY_MODE_BLOCK = 0x0,
	COPY_MODE_INIT
};

enum req_sense {
	REQ_SENSE_LEVEL = 0x0,
	REQ_SENSE_EDGE
};

struct channel_regs {
	__IO uint32_t DMA_A_START;
	__IO uint32_t DMA_B_START;
	__IO uint32_t DMA_INT_REG;
	__IO uint32_t DMA_LEN_REG;
	__IO uint32_t DMA_CTRL_REG;

	__I uint32_t DMA_IDX_REG;
	__I uint32_t RESERVED[2];
};

struct dma_channel_data {
	dma_callback_t cb;
	void *user_data;
	enum dma_smartbond_bus_width bus_width;
	enum dma_smartbond_burst_len burst_len;
	enum dma_channel_direction dir;
	bool is_dma_configured;
};

struct dma_smartbond_data {
	/* Should be the first member of the driver data */
	struct dma_context dma_ctx;

	ATOMIC_DEFINE(channels_atomic, DMA_CHANNELS_COUNT);

	/* User callbacks and data to be stored per channel */
	struct dma_channel_data channel_data[DMA_CHANNELS_COUNT];
};

/* True if there is any DMA activity on any channel, false otheriwise. */
static bool dma_smartbond_is_dma_active(void)
{
	int idx;
	struct channel_regs *regs;

	for (idx = 0; idx < DMA_CHANNELS_COUNT; idx++) {
		regs = DMA_CHN2REG(idx);

		if (DMA_CTRL_REG_GET_FIELD(DMA_ON, regs->DMA_CTRL_REG)) {
			return true;
		}
	}

	return false;
}

static void dma_smartbond_set_channel_status(uint32_t channel, bool status)
{
	unsigned int key;
	struct channel_regs *regs = DMA_CHN2REG(channel);

	key = irq_lock();

	if (status) {
		/* Make sure the status register for the requested channel is cleared. */
		DMA->DMA_CLEAR_INT_REG |= BIT(channel);
		/* Enable interrupts for the requested channel. */
		DMA->DMA_INT_MASK_REG |= BIT(channel);

		/* Check if this is the first attempt to enable DMA interrupts. */
		if (!irq_is_enabled(SMARTBOND_IRQN)) {
			irq_enable(SMARTBOND_IRQN);
		}

		DMA_CTRL_REG_SET_FIELD(DMA_ON, regs->DMA_CTRL_REG, 0x1);
	} else {
		DMA_CTRL_REG_SET_FIELD(DMA_ON, regs->DMA_CTRL_REG, 0x0);

		/*
		 * It might happen that DMA is already in progress. Make sure the current
		 * on-going transfer is complete (cannot be interrupted).
		 */
		while (DMA_CTRL_REG_GET_FIELD(DMA_ON, regs->DMA_CTRL_REG)) {
		}

		/* Disable interrupts for the requested channel */
		DMA->DMA_INT_MASK_REG &= ~(BIT(channel));
		/* Clear the status register; the requested channel should be considered obsolete */
		DMA->DMA_CLEAR_INT_REG |= BIT(channel);

		/* DMA interrupts should be disabled only if all channels are disabled. */
		if (!dma_smartbond_is_dma_active()) {
			irq_disable(SMARTBOND_IRQN);
		}
	}

	irq_unlock(key);
}

static bool dma_channel_dst_addr_check_and_adjust(uint32_t channel, uint32_t *dst)
{
	uint32_t phy_address;
	uint32_t secure_boot_reg;
	bool is_aes_keys_protected, is_qspic_keys_protected;

	phy_address = black_orca_phy_addr(*dst);

	secure_boot_reg = CRG_TOP->SECURE_BOOT_REG;
	is_aes_keys_protected =
		(secure_boot_reg & CRG_TOP_SECURE_BOOT_REG_PROT_AES_KEY_READ_Msk);
	is_qspic_keys_protected =
		(secure_boot_reg & CRG_TOP_SECURE_BOOT_REG_PROT_QSPI_KEY_READ_Msk);

	/*
	 * If the destination address reflects the AES key buffer area and secure keys are protected
	 * then only the secure channel #7 can be used to transfer data to AES key buffer.
	 */
	if ((IS_AES_KEYS_BUF_RANGE(phy_address) &&
		(is_aes_keys_protected || is_qspic_keys_protected) &&
		(channel != DMA_SECURE_CHANNEL))) {
		LOG_ERR("Keys are protected. Only secure channel #7 can be employed.");
		return false;
	}

	if (IS_QSPIF_ADDRESS(phy_address) || IS_QSPIF_CACHED_ADDRESS(phy_address) ||
				IS_OTP_ADDRESS(phy_address) || IS_OTP_P_ADDRESS(phy_address)) {
		LOG_ERR("Invalid destination location.");
		return false;
	}

	*dst = phy_address;

	return true;
}

static bool dma_channel_src_addr_check_and_adjust(uint32_t channel, uint32_t *src)
{
	uint32_t phy_address;
	uint32_t secure_boot_reg;
	bool is_aes_keys_protected, is_qspic_keys_protected;

	/* DMA can only access physical addresses, not remapped. */
	phy_address = black_orca_phy_addr(*src);

	if (IS_QSPIF_CACHED_ADDRESS(phy_address)) {
		/*
		 * To achiebe max. perfomance, peripherals should not access the Flash memory
		 * through the instruction cache controller (avoid cache misses).
		 */
		phy_address += (MCU_QSPIF_M_BASE - MCU_QSPIF_M_CACHED_BASE);
	} else if (IS_OTP_ADDRESS(phy_address)) {
		/* Peripherals should access OTP through its peripheral address space. */
		phy_address += (MCU_OTP_M_P_BASE - MCU_OTP_M_BASE);
	}

	secure_boot_reg = CRG_TOP->SECURE_BOOT_REG;
	is_aes_keys_protected =
		(secure_boot_reg & CRG_TOP_SECURE_BOOT_REG_PROT_AES_KEY_READ_Msk);
	is_qspic_keys_protected =
		(secure_boot_reg & CRG_TOP_SECURE_BOOT_REG_PROT_QSPI_KEY_READ_Msk);

	/*
	 * If the source address reflects protected area in OTP then only the
	 * secure channel #7 can be used to fetch secure keys data.
	 */
	if (((IS_ADDRESS_USER_DATA_KEYS_SEGMENT(phy_address) && is_aes_keys_protected) ||
	     (IS_ADDRESS_QSPI_FW_KEYS_SEGMENT(phy_address) && is_qspic_keys_protected)) &&
							(channel != DMA_SECURE_CHANNEL)) {
		LOG_ERR("Keys are protected. Only secure channel #7 can be employed.");
		return false;
	}

	*src = phy_address;

	return true;
}

static bool dma_channel_update_dreq_mode(enum dma_channel_direction direction,
									uint32_t *dma_ctrl_reg)
{
	switch (direction) {
	case MEMORY_TO_HOST:
	case HOST_TO_MEMORY:
	case MEMORY_TO_MEMORY:
		/* DMA channel starts immediately */
		DMA_CTRL_REG_SET_FIELD(DREQ_MODE, *dma_ctrl_reg, DREQ_MODE_SW);
		break;
	case PERIPHERAL_TO_MEMORY:
	case PERIPHERAL_TO_PERIPHERAL:
		/* DMA channels starts by peripheral DMA req */
		DMA_CTRL_REG_SET_FIELD(DREQ_MODE, *dma_ctrl_reg, DREQ_MODE_HW);
		break;
	default:
		return false;
	};

	return true;
}

static bool dma_channel_update_src_addr_adj(enum dma_addr_adj addr_adj, uint32_t *dma_ctrl_reg)
{
	switch (addr_adj) {
	case DMA_ADDR_ADJ_NO_CHANGE:
		DMA_CTRL_REG_SET_FIELD(AINC, *dma_ctrl_reg, ADDR_ADJ_NO_CHANGE);
		break;
	case DMA_ADDR_ADJ_INCREMENT:
		DMA_CTRL_REG_SET_FIELD(AINC, *dma_ctrl_reg, ADDR_ADJ_INCR);
		break;
	default:
		return false;
	}

	return true;
}

static bool dma_channel_update_dst_addr_adj(enum dma_addr_adj addr_adj, uint32_t *dma_ctrl_reg)
{
	switch (addr_adj) {
	case DMA_ADDR_ADJ_NO_CHANGE:
		DMA_CTRL_REG_SET_FIELD(BINC, *dma_ctrl_reg, ADDR_ADJ_NO_CHANGE);
		break;
	case DMA_ADDR_ADJ_INCREMENT:
		DMA_CTRL_REG_SET_FIELD(BINC, *dma_ctrl_reg, ADDR_ADJ_INCR);
		break;
	default:
		return false;
	}

	return true;
}

static bool dma_channel_update_bus_width(uint16_t bw, uint32_t *dma_ctrl_reg)
{
	switch (bw) {
	case DMA_SMARTBOND_BUS_WIDTH_1B:
		DMA_CTRL_REG_SET_FIELD(BW, *dma_ctrl_reg, BUS_WIDTH_1B);
		break;
	case DMA_SMARTBOND_BUS_WIDTH_2B:
		DMA_CTRL_REG_SET_FIELD(BW, *dma_ctrl_reg, BUS_WIDTH_2B);
		break;
	case DMA_SMARTBOND_BUS_WIDTH_4B:
		DMA_CTRL_REG_SET_FIELD(BW, *dma_ctrl_reg, BUS_WIDTH_4B);
		break;
	default:
		return false;
	}

	return true;
}

static bool dma_channel_update_burst_mode(uint16_t burst, uint32_t *dma_ctrl_reg)
{
	switch (burst) {
	case DMA_SMARTBOND_BURST_LEN_1B:
		DMA_CTRL_REG_SET_FIELD(BURST_MODE, *dma_ctrl_reg, BURST_MODE_0B);
		break;
	case DMA_SMARTBOND_BURST_LEN_4B:
		DMA_CTRL_REG_SET_FIELD(BURST_MODE, *dma_ctrl_reg, BURST_MODE_4B);
		break;
	case DMA_SMARTBOND_BURST_LEN_8B:
		DMA_CTRL_REG_SET_FIELD(BURST_MODE, *dma_ctrl_reg, BURST_MODE_8B);
		break;
	default:
		return false;
	}

	return true;
}

static void dma_channel_update_req_sense(enum dma_smartbond_trig_mux trig_mux,
						uint32_t channel, uint32_t *dma_ctrl_reg)
{
	switch (trig_mux) {
	case DMA_SMARTBOND_TRIG_MUX_UART:
	case DMA_SMARTBOND_TRIG_MUX_UART2:
	case DMA_SMARTBOND_TRIG_MUX_UART3:
	case DMA_SMARTBOND_TRIG_MUX_I2C:
	case DMA_SMARTBOND_TRIG_MUX_I2C2:
	case DMA_SMARTBOND_TRIG_MUX_USB:
		/* Odd channel numbers should reflect TX path */
		if (channel & BIT(0)) {
			DMA_CTRL_REG_SET_FIELD(REQ_SENSE, *dma_ctrl_reg, REQ_SENSE_EDGE);
			break;
		}
	default:
		DMA_CTRL_REG_SET_FIELD(REQ_SENSE, *dma_ctrl_reg, REQ_SENSE_LEVEL);
	}
}

static void dma_set_mux_request(enum dma_smartbond_trig_mux trig_mux, uint32_t channel)
{
	unsigned int key;

	key = irq_lock();
	DMA_REQ_MUX_REG_SET(channel, trig_mux);

	/*
	 * Having same trigger for different channels can cause unpredictable results.
	 * The audio triggers (src and pcm) are an exception, as they use 2 pairs each
	 * for DMA access.
	 * The lesser significant selector has higher priority and will control
	 * the DMA acknowledge signal driven to the selected peripheral. Make sure
	 * the current selector does not match with selectors of
	 * higher priorities (dma channels of lower indexing). It's OK if a
	 * channel of higher indexing defines the same peripheral request source
	 * (should be ignored as it has lower priority).
	 */
	if (trig_mux != DMA_SMARTBOND_TRIG_MUX_NONE) {
		switch (channel) {
		case DMA_SMARTBOND_CHANNEL_7:
		case DMA_SMARTBOND_CHANNEL_6:
			if (DMA_REQ_MUX_REG_GET(DMA_SMARTBOND_CHANNEL_5) == trig_mux) {
				DMA_REQ_MUX_REG_SET(DMA_SMARTBOND_CHANNEL_5,
								DMA_SMARTBOND_TRIG_MUX_NONE);
			}
			/* fall-through */
		case DMA_SMARTBOND_CHANNEL_5:
		case DMA_SMARTBOND_CHANNEL_4:
			if (DMA_REQ_MUX_REG_GET(DMA_SMARTBOND_CHANNEL_3) == trig_mux) {
				DMA_REQ_MUX_REG_SET(DMA_SMARTBOND_CHANNEL_3,
								DMA_SMARTBOND_TRIG_MUX_NONE);
			}
			/* fall-through */
		case DMA_SMARTBOND_CHANNEL_3:
		case DMA_SMARTBOND_CHANNEL_2:
			if (DMA_REQ_MUX_REG_GET(DMA_SMARTBOND_CHANNEL_1) == trig_mux) {
				DMA_REQ_MUX_REG_SET(DMA_SMARTBOND_CHANNEL_1,
								DMA_SMARTBOND_TRIG_MUX_NONE);
			}
		case DMA_SMARTBOND_CHANNEL_1:
		case DMA_SMARTBOND_CHANNEL_0:
			break;
		}
	}

	irq_unlock(key);
}

static int dma_smartbond_config(const struct device *dev, uint32_t channel, struct dma_config *cfg)
{
	struct dma_smartbond_data *data = dev->data;
	struct channel_regs *regs;
	uint32_t dma_ctrl_reg;
	uint32_t src_dst_address;

	if (channel >= DMA_CHANNELS_COUNT) {
		LOG_ERR("Inavlid DMA channel index");
		return -EINVAL;
	}
	regs = DMA_CHN2REG(channel);

	dma_ctrl_reg = regs->DMA_CTRL_REG;

	if (DMA_CTRL_REG_GET_FIELD(DMA_ON, dma_ctrl_reg)) {
		LOG_ERR("Requested channel is enabled. It should first be disabled");
		return -EIO;
	}

	if (cfg == NULL || cfg->head_block == NULL) {
		LOG_ERR("Missing configuration structure");
		return -EINVAL;
	}

	/* Error handling is not supported; just warn user. */
	if (!cfg->error_callback_dis) {
		LOG_WRN("Error handling is not supported");
	}

	if (!cfg->complete_callback_en) {
		data->channel_data[channel].cb = cfg->dma_callback;
		data->channel_data[channel].user_data = cfg->user_data;
	} else {
		LOG_WRN("User callback can only be called at completion only and not per block.");

		/* Nulify pointers to indicate notifications are disabled. */
		data->channel_data[channel].cb = NULL;
		data->channel_data[channel].user_data = NULL;
	}

	data->channel_data[channel].dir = cfg->channel_direction;

	if (cfg->block_count > DMA_BLOCK_COUNT) {
		LOG_WRN("A single block is supported. The rest blocks will be discarded");
	}

	if (cfg->channel_priority >= DMA_SMARTBOND_CHANNEL_PRIO_MAX) {
		cfg->channel_priority = DMA_SMARTBOND_CHANNEL_PRIO_7;
		LOG_WRN("Channel priority exceeded max. Setting to highest valid level");
	}

	DMA_CTRL_REG_SET_FIELD(DMA_PRIO, dma_ctrl_reg, cfg->channel_priority);

	if (((cfg->source_burst_length != cfg->dest_burst_length) ||
		!dma_channel_update_burst_mode(cfg->source_burst_length, &dma_ctrl_reg))) {
		LOG_ERR("Invalid burst mode or source and destination mode mismatch");
		return -EINVAL;
	}

	data->channel_data[channel].burst_len = cfg->source_burst_length;

	if (cfg->source_data_size != cfg->dest_data_size ||
		!dma_channel_update_bus_width(cfg->source_data_size, &dma_ctrl_reg)) {
		LOG_ERR("Invalid bus width or source and destination bus width mismatch");
		return -EINVAL;
	}

	data->channel_data[channel].bus_width = cfg->source_data_size;

	if (cfg->source_chaining_en || cfg->dest_chaining_en ||
		cfg->head_block->source_gather_en || cfg->head_block->dest_scatter_en ||
		cfg->head_block->source_reload_en || cfg->head_block->dest_reload_en) {
		LOG_WRN("Chainning, scattering, gathering or reloading is not supported");
	}

	if (!dma_channel_update_src_addr_adj(cfg->head_block->source_addr_adj,
								&dma_ctrl_reg)) {
		LOG_ERR("Invalid source address adjustment");
		return -EINVAL;
	}

	if (!dma_channel_update_dst_addr_adj(cfg->head_block->dest_addr_adj, &dma_ctrl_reg)) {
		LOG_ERR("Invalid destination address adjustment");
		return -EINVAL;
	}

	if (!dma_channel_update_dreq_mode(cfg->channel_direction, &dma_ctrl_reg)) {
		LOG_ERR("Inavlid channel direction");
		return -EINVAL;
	}

	/* Cyclic is valid only when DREQ_MODE is set */
	if (cfg->cyclic && DMA_CTRL_REG_GET_FIELD(DREQ_MODE, dma_ctrl_reg) != DREQ_MODE_HW) {
		LOG_ERR("Circular mode is only supported for non memory-memory transfers");
		return -EINVAL;
	}

	DMA_CTRL_REG_SET_FIELD(CIRCULAR, dma_ctrl_reg, cfg->cyclic);

	if (DMA_CTRL_REG_GET_FIELD(DREQ_MODE, dma_ctrl_reg) == DREQ_MODE_SW &&
		DMA_CTRL_REG_GET_FIELD(AINC, dma_ctrl_reg) == ADDR_ADJ_NO_CHANGE &&
		DMA_CTRL_REG_GET_FIELD(BINC, dma_ctrl_reg) == ADDR_ADJ_INCR) {
		/*
		 * Valid for memory initialization to a specific value. This process
		 * cannot be interrupted by other DMA channels.
		 */
		DMA_CTRL_REG_SET_FIELD(DMA_INIT, dma_ctrl_reg, COPY_MODE_INIT);
	} else {
		DMA_CTRL_REG_SET_FIELD(DMA_INIT, dma_ctrl_reg, COPY_MODE_BLOCK);
	}

	dma_channel_update_req_sense(cfg->dma_slot, channel, &dma_ctrl_reg);

	regs->DMA_CTRL_REG = dma_ctrl_reg;

	/* Requested address might be changed */
	src_dst_address = cfg->head_block->source_address;
	if (!dma_channel_src_addr_check_and_adjust(channel, &src_dst_address)) {
		return -EINVAL;
	}

	if (src_dst_address % cfg->source_data_size) {
		LOG_ERR("Source address is not bus width aligned");
		return -EINVAL;
	}

	regs->DMA_A_START = src_dst_address;

	src_dst_address = cfg->head_block->dest_address;
	if (!dma_channel_dst_addr_check_and_adjust(channel, &src_dst_address)) {
		return -EINVAL;
	}

	if (src_dst_address % cfg->dest_data_size) {
		LOG_ERR("Destination address is not bus width aligned");
		return -EINVAL;
	}

	regs->DMA_B_START = src_dst_address;

	if (cfg->head_block->block_size % (cfg->source_data_size * cfg->source_burst_length)) {
		LOG_ERR("Requested data size is not multiple of bus width");
		return -EINVAL;
	}

	regs->DMA_LEN_REG = (cfg->head_block->block_size / cfg->source_data_size) - 1;

	/* Interrupt will be raised once all transfers are complete. */
	regs->DMA_INT_REG = (cfg->head_block->block_size / cfg->source_data_size) - 1;

	if ((cfg->source_handshake != cfg->dest_handshake) ||
		(cfg->source_handshake != 0)/*HW*/) {
		LOG_ERR("Source/destination handshakes mismatch or invalid");
		return -EINVAL;
	}

	dma_set_mux_request(cfg->dma_slot, channel);

	/* Designate that channel has been configured */
	data->channel_data[channel].is_dma_configured = true;

	return 0;
}


static int dma_smartbond_reload(const struct device *dev, uint32_t channel, uint32_t src,
									uint32_t dst, size_t size)
{
	struct dma_smartbond_data *data = dev->data;
	struct channel_regs *regs;

	if (channel >= DMA_CHANNELS_COUNT) {
		LOG_ERR("Inavlid DMA channel index");
		return -EINVAL;
	}
	regs = DMA_CHN2REG(channel);

	if (!data->channel_data[channel].is_dma_configured) {
		LOG_ERR("Requested DMA channel should first be configured");
		return -EINVAL;
	}

	if (size == 0) {
		LOG_ERR("Min. transfer size is one");
		return -EINVAL;
	}

	if (DMA_CTRL_REG_GET_FIELD(DMA_ON, regs->DMA_CTRL_REG)) {
		LOG_ERR("Channel is busy, settings cannot be changed mid-transfer");
		return -EBUSY;
	}

	if (src % data->channel_data[channel].bus_width) {
		LOG_ERR("Source address is not bus width aligned");
		return -EINVAL;
	}

	if (!dma_channel_src_addr_check_and_adjust(channel, &src)) {
		return -EINVAL;
	}

	regs->DMA_A_START = src;

	if (dst % data->channel_data[channel].bus_width) {
		LOG_ERR("Destination address is not bus width aligned");
		return -EINVAL;
	}

	if (!dma_channel_dst_addr_check_and_adjust(channel, &dst)) {
		return -EINVAL;
	}

	regs->DMA_B_START = dst;

	if (size % (data->channel_data[channel].burst_len *
							data->channel_data[channel].bus_width)) {
		LOG_ERR("Requested data size is not multiple of bus width");
		return -EINVAL;
	}

	regs->DMA_LEN_REG = (size / data->channel_data[channel].bus_width) - 1;

	/* Interrupt will be raised once all transfers are complete. */
	regs->DMA_INT_REG = (size / data->channel_data[channel].bus_width) - 1;

	return 0;
}

static int dma_smartbond_start(const struct device *dev, uint32_t channel)
{
	struct channel_regs *regs;
	struct dma_smartbond_data *data = dev->data;

	if (channel >= DMA_CHANNELS_COUNT) {
		LOG_ERR("Inavlid DMA channel index");
		return -EINVAL;
	}
	regs = DMA_CHN2REG(channel);

	if (!data->channel_data[channel].is_dma_configured) {
		LOG_ERR("Requested DMA channel should first be configured");
		return -EINVAL;
	}

	/* Should return succss if the requested channel is already started. */
	if (DMA_CTRL_REG_GET_FIELD(DMA_ON, regs->DMA_CTRL_REG)) {
		return 0;
	}

	dma_smartbond_set_channel_status(channel, true);

	return 0;
}

static int dma_smartbond_stop(const struct device *dev, uint32_t channel)
{
	struct channel_regs *regs;

	if (channel >= DMA_CHANNELS_COUNT) {
		LOG_ERR("Inavlid DMA channel index");
		return -EINVAL;
	}
	regs = DMA_CHN2REG(channel);

	/*
	 * In normal mode DMA_ON is cleared automatically. However we need to clear
	 * the corresponding register mask and disable NVIC if there is no other
	 * channel in use.
	 */
	dma_smartbond_set_channel_status(channel, false);

	return 0;
}

static int dma_smartbond_suspend(const struct device *dev, uint32_t channel)
{
	if (channel >= DMA_CHANNELS_COUNT) {
		LOG_ERR("Inavlid DMA channel index");
		return -EINVAL;
	}

	/*
	 * Freezing the DMA engine is valid for memory-to-memory operations.
	 * Valid memory locations are SYSRAM and/or PSRAM.
	 */
	LOG_WRN("DMA is freezed globally");

	/*
	 * Freezing the DMA engine can be done universally and not per channel!.
	 * An attempt to disable the channel would result in resetting the IDX
	 * register next time the channel was re-enabled.
	 */
	GPREG->SET_FREEZE_REG = GPREG_SET_FREEZE_REG_FRZ_DMA_Msk;

	return 0;
}

static int dma_smartbond_resume(const struct device *dev, uint32_t channel)
{
	if (channel >= DMA_CHANNELS_COUNT) {
		LOG_ERR("Inavlid DMA channel index");
		return -EINVAL;
	}

	LOG_WRN("DMA is unfreezed globally");

	/* Unfreezing the DMA engine can be done unviversally and not per channel! */
	GPREG->RESET_FREEZE_REG = GPREG_RESET_FREEZE_REG_FRZ_DMA_Msk;

	return 0;
}

static int dma_smartbond_get_status(const struct device *dev, uint32_t channel,
							struct dma_status *stat)
{
	struct channel_regs *regs;
	int key;
	struct dma_smartbond_data *data = dev->data;
	uint8_t bus_width;
	uint32_t dma_ctrl_reg, dma_idx_reg, dma_len_reg;

	if (channel >= DMA_CHANNELS_COUNT) {
		LOG_ERR("Inavlid DMA channel index");
		return -EINVAL;
	}

	if (stat == NULL) {
		LOG_ERR("User should provide a valid pointer to store the status info requested");
	}

	if (!data->channel_data[channel].is_dma_configured) {
		LOG_ERR("Requested DMA channel should first be configured");
		return -EINVAL;
	}

	regs = DMA_CHN2REG(channel);

	/*
	 * The DMA is running in parallel with CPU and so it might happen that an on-going transfer
	 * might be completed the moment user parses the status results. Disable interrupts globally
	 * so there is no chance for a new transfer to be initiated from within ISR and so changing
	 * the channel registers values.
	 */
	key = irq_lock();

	dma_ctrl_reg = regs->DMA_CTRL_REG;
	dma_idx_reg = regs->DMA_IDX_REG;
	dma_len_reg = regs->DMA_LEN_REG;

	/* Calculate how many byes each transfer consists of. */
	bus_width = DMA_CTRL_REG_GET_FIELD(BW, dma_ctrl_reg);
	if (bus_width == BUS_WIDTH_1B) {
		bus_width = 1;
	} else {
		bus_width <<= 1;
	}

	/* Convert transfers to bytes. */
	stat->total_copied = dma_idx_reg * bus_width;
	stat->pending_length = ((dma_len_reg + 1) - dma_idx_reg) * bus_width;
	stat->busy = DMA_CTRL_REG_GET_FIELD(DMA_ON, dma_ctrl_reg);
	stat->dir = data->channel_data[channel].dir;

	/* DMA does not support circular buffer functionality */
	stat->free = 0;
	stat->read_position = 0;
	stat->write_position = 0;

	irq_unlock(key);

	return 0;
}

static int dma_smartbond_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	if (value == NULL) {
		LOG_ERR("User should provide a valid pointer to attribute value");
		return -EINVAL;
	}

	switch (type) {
	/*
	 * Source and destination addresses should be multiple of a channel's bus width.
	 * This info could be provided at runtime given that attributes of a specific
	 * channel could be requested.
	 */
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
	case DMA_ATTR_COPY_ALIGNMENT:
	/*
	 * Buffer size should be multiple of a channel's bus width multiplied by burst length.
	 * This info could be provided at runtime given that attributes of a specific channel
	 * could be requested.
	 */
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
		return -ENOSYS;
	case DMA_ATTR_MAX_BLOCK_COUNT:
		*value = DMA_BLOCK_COUNT;
		return 0;
	default:
		return -EINVAL;
	}
}

static bool dma_smartbond_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	uint32_t requested_channel;

	if (channel >= DMA_CHANNELS_COUNT) {
		LOG_ERR("Inavlid DMA channel index");
		return -EINVAL;
	}

	/* If user does not provide any channel request explicitly, return true. */
	if (filter_param == NULL) {
		return true;
	}

	requested_channel = *(uint32_t *)filter_param;

	if (channel == requested_channel) {
		return true;
	}

	return false;
}

static struct dma_driver_api dma_smartbond_driver_api = {
	.config = dma_smartbond_config,
	.reload = dma_smartbond_reload,
	.start = dma_smartbond_start,
	.stop = dma_smartbond_stop,
	.suspend = dma_smartbond_suspend,
	.resume = dma_smartbond_resume,
	.get_status = dma_smartbond_get_status,
	.get_attribute = dma_smartbond_get_attribute,
	.chan_filter = dma_smartbond_chan_filter
};

static void smartbond_dma_isr(const void *arg)
{
	uint16_t dma_int_status_reg;
	int i;
	struct channel_regs *regs;
	struct dma_smartbond_data *data = ((const struct device *)arg)->data;

	/*
	 * A single interrupt line is generated for all channels and so each channel
	 * should be parsed separately.
	 */
	for (i = 0, dma_int_status_reg = DMA->DMA_INT_STATUS_REG;
		 i < DMA_CHANNELS_COUNT && dma_int_status_reg != 0; ++i, dma_int_status_reg >>= 1) {
		/* Check if the selected channel has raised the interrupt line */
		if (dma_int_status_reg & BIT(0)) {

			regs = DMA_CHN2REG(i);
			/*
			 * Should be valid if callbacks are explicitly enabled by users.
			 * Interrupt should be triggered only when the total size of
			 * bytes has been transferred. Bus errors cannot raise interrupts.
			 */
			if (data->channel_data[i].cb) {
				data->channel_data[i].cb((const struct device *)arg,
				data->channel_data[i].user_data, i, DMA_STATUS_COMPLETE);
			}
			/* Channel line should be cleared otherwise ISR will keep firing! */
			DMA->DMA_CLEAR_INT_REG = BIT(i);
		}
	}
}

static int dma_smartbond_init(const struct device *dev)
{
#ifdef CONFIG_DMA_64BIT
	LOG_ERR("64-bit addressing mode is not supported\n");
	return -ENOSYS;
#endif

	int idx;
	struct dma_smartbond_data *data;

	data = dev->data;
	data->dma_ctx.magic = DMA_MAGIC;
	data->dma_ctx.dma_channels = DMA_CHANNELS_COUNT;
	data->dma_ctx.atomic = data->channels_atomic;

	/* Make sure that all channels are disabled. */
	for (idx = 0; idx < DMA_CHANNELS_COUNT; idx++) {
		dma_smartbond_set_channel_status(idx, false);
		data->channel_data[idx].is_dma_configured = false;
	}

	IRQ_CONNECT(SMARTBOND_IRQN, SMARTBOND_IRQ_PRIO, smartbond_dma_isr,
								DEVICE_DT_INST_GET(0), 0);

	return 0;
}

#define SMARTBOND_DMA_INIT(inst) \
	BUILD_ASSERT((inst) == 0, "multiple instances are not supported"); \
	\
	static struct dma_smartbond_data dma_smartbond_data_ ## inst; \
	\
	DEVICE_DT_INST_DEFINE(0, dma_smartbond_init, NULL, \
		&dma_smartbond_data_ ## inst, NULL,	\
		POST_KERNEL, \
		CONFIG_DMA_INIT_PRIORITY, \
		&dma_smartbond_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SMARTBOND_DMA_INIT)
