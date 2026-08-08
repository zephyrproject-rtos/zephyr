/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for the Analog Devices AXI DMAC core.
 * Based on the no-OS reference driver by Analog Devices.
 */

#define DT_DRV_COMPAT adi_axi_dmac

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adi_axi_dmac, CONFIG_DMA_LOG_LEVEL);

#define AXI_DMAC_REG_VERSION         0x0000
#define AXI_DMAC_REG_IRQ_MASK        0x0080
#define AXI_DMAC_REG_IRQ_PENDING     0x0084
#define AXI_DMAC_REG_INTF_DESC       0x0010
#define AXI_DMAC_REG_CTRL            0x0400
#define AXI_DMAC_REG_TRANSFER_SUBMIT 0x0408
#define AXI_DMAC_REG_FLAGS           0x040C
#define AXI_DMAC_REG_DEST_ADDRESS    0x0410
#define AXI_DMAC_REG_SRC_ADDRESS     0x0414
#define AXI_DMAC_REG_X_LENGTH        0x0418
#define AXI_DMAC_REG_Y_LENGTH        0x041C
#define AXI_DMAC_REG_DEST_STRIDE     0x0420
#define AXI_DMAC_REG_SRC_STRIDE      0x0424

#define AXI_DMAC_IRQ_SOT         BIT(0)
#define AXI_DMAC_IRQ_EOT         BIT(1)
#define AXI_DMAC_CTRL_ENABLE     BIT(0)
#define AXI_DMAC_FLAGS_CYCLIC    BIT(0)
#define AXI_DMAC_TRANSFER_SUBMIT BIT(0)
#define AXI_DMAC_QUEUE_FULL      BIT(0)

#define AXI_DMAC_INTF_BPB_DEST_MASK GENMASK(3, 0)
#define AXI_DMAC_INTF_BPB_SRC_MASK  GENMASK(11, 8)

/* This core exposes a single hardwired transfer channel. */
#define AXI_DMAC_CHANNEL 0

enum axi_dmac_direction {
	AXI_DMAC_DIR_INVALID = 0,
	AXI_DMAC_DEV_TO_MEM,
	AXI_DMAC_MEM_TO_DEV,
	AXI_DMAC_MEM_TO_MEM,
};

struct axi_dmac_config {
	DEVICE_MMIO_ROM;
	void (*irq_config)(const struct device *dev);
};

struct axi_dmac_data {
	DEVICE_MMIO_RAM;
	/* Hardware capabilities, auto-probed at init. */
	enum axi_dmac_direction direction;
	uint32_t max_length;
	uint32_t width_src;
	uint32_t width_dest;
	bool hw_cyclic;
	bool has_irq;
	/* Per-transfer configuration (from dma_config). */
	bool cyclic;
	dma_callback_t callback;
	void *user_data;
	/* Burst state machine. */
	volatile bool saw_sot;
	uint32_t remaining_size;
	uint32_t next_src_addr;
	uint32_t next_dest_addr;
	uint32_t pending_bursts;
	/* cyclic mode: saved base address and total size for resubmission */
	uint32_t cyclic_dest_addr;
	uint32_t cyclic_src_addr;
	uint32_t cyclic_size;
};

static inline uint32_t dmac_read(const struct device *dev, uint32_t reg)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + reg);
}

static inline void dmac_write(const struct device *dev, uint32_t reg, uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_GET(dev) + reg);
}

static int dmac_submit_burst(const struct device *dev, struct axi_dmac_data *data)
{
	uint32_t burst_size;

	if (dmac_read(dev, AXI_DMAC_REG_TRANSFER_SUBMIT) & AXI_DMAC_QUEUE_FULL) {
		return -EBUSY;
	}

	if (data->direction == AXI_DMAC_DEV_TO_MEM || data->direction == AXI_DMAC_MEM_TO_MEM) {
		dmac_write(dev, AXI_DMAC_REG_DEST_ADDRESS, data->next_dest_addr);
		dmac_write(dev, AXI_DMAC_REG_DEST_STRIDE, 0);
	}

	if (data->direction == AXI_DMAC_MEM_TO_DEV || data->direction == AXI_DMAC_MEM_TO_MEM) {
		dmac_write(dev, AXI_DMAC_REG_SRC_ADDRESS, data->next_src_addr);
		dmac_write(dev, AXI_DMAC_REG_SRC_STRIDE, 0);
	}

	burst_size = MIN(data->remaining_size, data->max_length + 1) - 1;

	if (data->direction == AXI_DMAC_DEV_TO_MEM || data->direction == AXI_DMAC_MEM_TO_MEM) {
		data->next_dest_addr += burst_size + 1;
	}

	if (data->direction == AXI_DMAC_MEM_TO_DEV || data->direction == AXI_DMAC_MEM_TO_MEM) {
		data->next_src_addr += burst_size + 1;
	}

	data->remaining_size -= burst_size + 1;

	dmac_write(dev, AXI_DMAC_REG_X_LENGTH, burst_size);
	dmac_write(dev, AXI_DMAC_REG_Y_LENGTH, 0);
	dmac_write(dev, AXI_DMAC_REG_TRANSFER_SUBMIT, AXI_DMAC_TRANSFER_SUBMIT);

	data->pending_bursts++;
	return 0;
}

/*
 * Advance the burst state machine in response to a SOT/EOT event. Shared by
 * the ISR and, on cores brought up without an interrupt line, by the polling
 * path in .get_status. Returns the DMA callback status to report, or -1 when
 * the event did not complete (or wrap) a transfer.
 */
static int dmac_service(const struct device *dev, struct axi_dmac_data *data)
{
	uint32_t pending = dmac_read(dev, AXI_DMAC_REG_IRQ_PENDING);
	int event = -1;

	if (!(pending & (AXI_DMAC_IRQ_SOT | AXI_DMAC_IRQ_EOT))) {
		return -1;
	}

	dmac_write(dev, AXI_DMAC_REG_IRQ_PENDING, pending);

	if (pending & AXI_DMAC_IRQ_SOT) {
		data->saw_sot = true;
		if (data->remaining_size > 0) {
			dmac_submit_burst(dev, data);
		}
	}

	if (pending & AXI_DMAC_IRQ_EOT) {
		if (data->pending_bursts > 0) {
			data->pending_bursts--;
		}
		/* Require prior SOT to prevent false completion on stale EOT. */
		if (data->saw_sot && data->remaining_size == 0 && data->pending_bursts == 0) {
			if (data->cyclic) {
				/* Buffer wrapped: rearm and report a block. */
				data->saw_sot = false;
				data->next_src_addr = data->cyclic_src_addr;
				data->next_dest_addr = data->cyclic_dest_addr;
				data->remaining_size = data->cyclic_size;
				/*
				 * Only re-submit when the hardware is not looping
				 * on its own; doing both would queue a second
				 * transfer against the one the core already
				 * restarted. The state above is still restored
				 * either way so the channel keeps reporting busy.
				 */
				if (!data->hw_cyclic) {
					dmac_submit_burst(dev, data);
				}
				event = DMA_STATUS_BLOCK;
			} else {
				event = DMA_STATUS_COMPLETE;
			}
		}
	}

	return event;
}

static void axi_dmac_isr(const struct device *dev)
{
	struct axi_dmac_data *data = dev->data;
	int event = dmac_service(dev, data);

	if (event >= 0 && data->callback != NULL) {
		data->callback(dev, data->user_data, AXI_DMAC_CHANNEL, event);
	}
}

static enum axi_dmac_direction to_hw_dir(uint32_t channel_direction)
{
	switch (channel_direction) {
	case PERIPHERAL_TO_MEMORY:
		return AXI_DMAC_DEV_TO_MEM;
	case MEMORY_TO_PERIPHERAL:
		return AXI_DMAC_MEM_TO_DEV;
	case MEMORY_TO_MEMORY:
		return AXI_DMAC_MEM_TO_MEM;
	default:
		return AXI_DMAC_DIR_INVALID;
	}
}

static enum dma_channel_direction from_hw_dir(enum axi_dmac_direction dir)
{
	switch (dir) {
	case AXI_DMAC_DEV_TO_MEM:
		return PERIPHERAL_TO_MEMORY;
	case AXI_DMAC_MEM_TO_DEV:
		return MEMORY_TO_PERIPHERAL;
	default:
		return MEMORY_TO_MEMORY;
	}
}

static int axi_dmac_configure(const struct device *dev, uint32_t channel, struct dma_config *cfg)
{
	struct axi_dmac_data *data = dev->data;
	struct dma_block_config *block;
	uint32_t dest_addr, src_addr;

	if (channel != AXI_DMAC_CHANNEL) {
		LOG_ERR("invalid channel %u", channel);
		return -EINVAL;
	}

	if (cfg->block_count != 1U || cfg->head_block == NULL) {
		LOG_ERR("exactly one block is required");
		return -EINVAL;
	}

	block = cfg->head_block;
	if (block->block_size == 0U) {
		return -EINVAL;
	}

	/*
	 * The core hardwires its direction in the bitstream; honour the probed
	 * value but reject a request that contradicts it.
	 */
	if (to_hw_dir(cfg->channel_direction) != data->direction) {
		LOG_ERR("requested direction %u != hardware direction %u", cfg->channel_direction,
			data->direction);
		return -EINVAL;
	}

	dest_addr = (uint32_t)block->dest_address;
	src_addr = (uint32_t)block->source_address;

	if (data->width_src && (src_addr % data->width_src)) {
		LOG_ERR("src addr 0x%08x not aligned to %u bytes", src_addr, data->width_src);
		return -EINVAL;
	}

	if (data->width_dest && (dest_addr % data->width_dest)) {
		LOG_ERR("dest addr 0x%08x not aligned to %u bytes", dest_addr, data->width_dest);
		return -EINVAL;
	}

	data->cyclic = cfg->cyclic;
	data->callback = cfg->dma_callback;
	data->user_data = cfg->user_data;
	data->next_src_addr = src_addr;
	data->next_dest_addr = dest_addr;
	data->remaining_size = block->block_size;

	if (cfg->cyclic) {
		data->cyclic_src_addr = src_addr;
		data->cyclic_dest_addr = dest_addr;
		data->cyclic_size = block->block_size;
	}

	return 0;
}

static int axi_dmac_start(const struct device *dev, uint32_t channel)
{
	struct axi_dmac_data *data = dev->data;
	uint32_t ctrl;

	if (channel != AXI_DMAC_CHANNEL) {
		return -EINVAL;
	}

	if (data->remaining_size == 0U) {
		LOG_ERR("start without a configured transfer");
		return -EINVAL;
	}

	data->saw_sot = false;
	data->pending_bursts = 0;

	/*
	 * Hand cyclic mode to the hardware when the core supports it. The software
	 * re-arm in dmac_service() only advances from the ISR or a get_status()
	 * poll, so on a core synthesized without an interrupt line an unattended
	 * cyclic transfer would stop after one buffer -- the hardware flag keeps it
	 * running with no help from the CPU, which is what a continuous DAC
	 * playback needs. dmac_service() still re-arms on top of this: harmless
	 * when the hardware is already looping, and the only mechanism when the
	 * core lacks cyclic support.
	 */
	dmac_write(dev, AXI_DMAC_REG_FLAGS,
		   (data->cyclic && data->hw_cyclic) ? AXI_DMAC_FLAGS_CYCLIC : 0);

	ctrl = dmac_read(dev, AXI_DMAC_REG_CTRL);
	if (!(ctrl & AXI_DMAC_CTRL_ENABLE)) {
		dmac_write(dev, AXI_DMAC_REG_CTRL, 0);
		dmac_write(dev, AXI_DMAC_REG_CTRL, AXI_DMAC_CTRL_ENABLE);
	}

	dmac_write(dev, AXI_DMAC_REG_IRQ_PENDING, AXI_DMAC_IRQ_SOT | AXI_DMAC_IRQ_EOT);
	dmac_write(dev, AXI_DMAC_REG_IRQ_MASK, 0);

	return dmac_submit_burst(dev, data);
}

static int axi_dmac_stop(const struct device *dev, uint32_t channel)
{
	struct axi_dmac_data *data = dev->data;

	if (channel != AXI_DMAC_CHANNEL) {
		return -EINVAL;
	}

	dmac_write(dev, AXI_DMAC_REG_CTRL, 0);
	dmac_write(dev, AXI_DMAC_REG_IRQ_MASK, AXI_DMAC_IRQ_SOT | AXI_DMAC_IRQ_EOT);
	/* Clear cyclic so a subsequent one-shot transfer does not loop forever. */
	dmac_write(dev, AXI_DMAC_REG_FLAGS, 0);

	data->remaining_size = 0;
	data->pending_bursts = 0;
	data->cyclic = false;

	return 0;
}

static int axi_dmac_get_status(const struct device *dev, uint32_t channel,
			       struct dma_status *status)
{
	struct axi_dmac_data *data = dev->data;

	if (channel != AXI_DMAC_CHANNEL) {
		return -EINVAL;
	}

	/*
	 * On a core synthesized without an interrupt line the state machine has
	 * no ISR to pump it, so advance it here whenever status is polled. This
	 * lets a consumer drive a transfer to completion via dma_get_status().
	 */
	if (!data->has_irq) {
		int event = dmac_service(dev, data);

		if (event >= 0 && data->callback != NULL) {
			data->callback(dev, data->user_data, AXI_DMAC_CHANNEL, event);
		}
	}

	status->dir = from_hw_dir(data->direction);
	status->busy = (data->remaining_size > 0) || (data->pending_bursts > 0);
	status->pending_length = data->remaining_size;

	return 0;
}

static DEVICE_API(dma, axi_dmac_driver_api) = {
	.config = axi_dmac_configure,
	.start = axi_dmac_start,
	.stop = axi_dmac_stop,
	.get_status = axi_dmac_get_status,
};

static int axi_dmac_init(const struct device *dev)
{
	const struct axi_dmac_config *config = dev->config;
	struct axi_dmac_data *data = dev->data;
	uint32_t intf_desc;
	uint32_t version;
	uint32_t val;
	bool dest_is_mem, src_is_mem;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	version = dmac_read(dev, AXI_DMAC_REG_VERSION);

	dmac_write(dev, AXI_DMAC_REG_X_LENGTH, 0xFFFFFFFF);
	data->max_length = dmac_read(dev, AXI_DMAC_REG_X_LENGTH);
	if (data->max_length == 0) {
		LOG_ERR("could not detect max transfer length");
		return -EIO;
	}

	dmac_write(dev, AXI_DMAC_REG_DEST_ADDRESS, 0xFFFFFFFF);
	val = dmac_read(dev, AXI_DMAC_REG_DEST_ADDRESS);
	dest_is_mem = (val != 0);

	dmac_write(dev, AXI_DMAC_REG_SRC_ADDRESS, 0xFFFFFFFF);
	val = dmac_read(dev, AXI_DMAC_REG_SRC_ADDRESS);
	src_is_mem = (val != 0);

	if (dest_is_mem && !src_is_mem) {
		data->direction = AXI_DMAC_DEV_TO_MEM;
	} else if (!dest_is_mem && src_is_mem) {
		data->direction = AXI_DMAC_MEM_TO_DEV;
	} else if (dest_is_mem && src_is_mem) {
		data->direction = AXI_DMAC_MEM_TO_MEM;
	} else {
		LOG_ERR("invalid DMA direction (neither port is memory-mapped)");
		return -EIO;
	}

	intf_desc = dmac_read(dev, AXI_DMAC_REG_INTF_DESC);
	data->width_src = 1 << FIELD_GET(AXI_DMAC_INTF_BPB_SRC_MASK, intf_desc);
	data->width_dest = 1 << FIELD_GET(AXI_DMAC_INTF_BPB_DEST_MASK, intf_desc);

	dmac_write(dev, AXI_DMAC_REG_FLAGS, BIT(0));
	val = dmac_read(dev, AXI_DMAC_REG_FLAGS);
	data->hw_cyclic = !!(val & BIT(0));
	dmac_write(dev, AXI_DMAC_REG_FLAGS, 0);

	dmac_write(dev, AXI_DMAC_REG_CTRL, 0);
	dmac_write(dev, AXI_DMAC_REG_IRQ_MASK, AXI_DMAC_IRQ_SOT | AXI_DMAC_IRQ_EOT);

	if (config->irq_config != NULL) {
		config->irq_config(dev);
		data->has_irq = true;
	}

	static const char *const dir_names[] = {
		[AXI_DMAC_DIR_INVALID] = "INVALID",
		[AXI_DMAC_DEV_TO_MEM] = "DEV_TO_MEM",
		[AXI_DMAC_MEM_TO_DEV] = "MEM_TO_DEV",
		[AXI_DMAC_MEM_TO_MEM] = "MEM_TO_MEM",
	};

	LOG_INF("AXI DMAC v%d.%d.%c — %s, max_len=%u, src_w=%u, dest_w=%u, cyclic=%s",
		version >> 16, (version >> 8) & 0xff, version & 0xff, dir_names[data->direction],
		data->max_length + 1, data->width_src, data->width_dest,
		data->hw_cyclic ? "hw" : "sw-only");

	return 0;
}

#define AXI_DMAC_IRQ_CONFIG_FN(n)                                                                  \
	static void axi_dmac_irq_config_##n(const struct device *dev)                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), axi_dmac_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define AXI_DMAC_IRQ_CONFIG_INIT(n) .irq_config = axi_dmac_irq_config_##n,

/* clang-format off */
#define AXI_DMAC_INIT(n)                                                              \
	COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0), (AXI_DMAC_IRQ_CONFIG_FN(n)), ())        \
	static struct axi_dmac_data axi_dmac_data_##n;                                \
	static const struct axi_dmac_config axi_dmac_config_##n = {                   \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                  \
		COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0),                                 \
			    (AXI_DMAC_IRQ_CONFIG_INIT(n)), ())};                       \
	DEVICE_DT_INST_DEFINE(n, axi_dmac_init, NULL, &axi_dmac_data_##n,             \
			      &axi_dmac_config_##n, POST_KERNEL,                      \
			      CONFIG_DMA_INIT_PRIORITY, &axi_dmac_driver_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(AXI_DMAC_INIT)
