/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_bcm2712_sdhci

#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/byteorder.h>

#include "sdhc_standard_core.h"

LOG_MODULE_REGISTER(sdhc_bcm2712, CONFIG_SDHC_LOG_LEVEL);

#define SDHC_STATE_CMD_INHIBIT BIT(0)
#define SDHC_STATE_DAT_INHIBIT BIT(1)
#define SDHC_STATE_CARD        BIT(16)
#define SDHC_STATE_WRITE_ENABLE BIT(10)
#define SDHC_STATE_READ_ENABLE  BIT(11)
#define SDHC_STATE_DAT0        BIT(20)

#define SDHC_HOST_WIDTH4    BIT(1)
#define SDHC_HOST_HIGH_SPEED BIT(2)
#define SDHC_HOST_DMA_MASK  GENMASK(4, 3)
#define SDHC_HOST_WIDTH8    BIT(5)
#define SDHC_POWER_ON_330   (0x0fU << 8)

#define SDHC_CLOCK_INTERNAL BIT(0)
#define SDHC_CLOCK_STABLE   BIT(1)
#define SDHC_CLOCK_CARD     BIT(2)
#define SDHC_CLOCK_DIV_MASK (GENMASK(15, 8) | GENMASK(7, 6))
#define SDHC_TIMEOUT_MASK   GENMASK(19, 16)
#define SDHC_TIMEOUT_MAX    (0x0eU << 16)
#define SDHC_RESET_ALL      BIT(24)
#define SDHC_RESET_CMD_DATA (BIT(25) | BIT(26))

#define SDHC_INT_CMD_DONE   BIT(0)
#define SDHC_INT_DATA_DONE  BIT(1)
#define SDHC_INT_WRITE_RDY  BIT(4)
#define SDHC_INT_READ_RDY   BIT(5)
#define SDHC_INT_ERROR      BIT(15)
#define SDHC_ERR_TIMEOUT    (BIT(16) | BIT(20))
#define SDHC_ERR_CRC        (BIT(17) | BIT(21))
#define SDHC_ERR_MASK       GENMASK(31, 16)
#define SDHC_STATUS_MASK (SDHC_INT_CMD_DONE | SDHC_INT_DATA_DONE | SDHC_INT_WRITE_RDY | \
			  SDHC_INT_READ_RDY | SDHC_INT_ERROR | SDHC_ERR_MASK)

#define SDHC_TRANSFER_COUNT BIT(1)
#define SDHC_TRANSFER_STOP  BIT(2)
#define SDHC_TRANSFER_READ  BIT(4)
#define SDHC_TRANSFER_MULTI BIT(5)
#define SDHC_COMMAND_CRC    BIT(3)
#define SDHC_COMMAND_INDEX  BIT(4)
#define SDHC_COMMAND_DATA   BIT(5)
#define SDHC_COMMAND_ABORT  (3U << 6)

#define SDHC_HOST_DMA_ADMA2    BIT(4)
#define SDHC_HOST_DMA_ADMA2_64 GENMASK(4, 3)
#define SDHC_TRANSFER_DMA_EN  BIT(0)

#define SDHC_ADMA2_MAX_LENGTH 65536U
#define SDHC_ADMA2_ATTR_VALID BIT(0)
#define SDHC_ADMA2_ATTR_END   BIT(1)
#define SDHC_ADMA2_ATTR_TRAN  BIT(5)

/* SDHCI v3-compatible descriptors; v4's 16-byte format is not implemented. */
struct sdhc_adma2_desc {
	uint16_t attribute;
	uint16_t length;
	uint32_t address;
} __packed;

struct sdhc_adma2_desc64 {
	uint16_t attribute;
	uint16_t length;
	uint32_t address_low;
	uint32_t address_high;
} __packed;

BUILD_ASSERT(sizeof(struct sdhc_adma2_desc) == 8U);
BUILD_ASSERT(sizeof(struct sdhc_adma2_desc64) == 12U);

struct sdhc_adma2_config {
	/* Map a contiguous CPU buffer to a DMA bus address, or return an error. */
	int (*map)(const struct device *dev, const void *buf, size_t len, uint64_t *addr);
	bool dma64;
};

#ifdef CONFIG_DCACHE_LINE_SIZE
#define SDHC_ADMA2_ALIGNMENT MAX(8U, CONFIG_DCACHE_LINE_SIZE)
#else
#define SDHC_ADMA2_ALIGNMENT 8U
#endif

struct sdhc_adma2_data {
#ifdef CONFIG_SDHC_BCM2712_DMA
	union {
		struct sdhc_adma2_desc dma32[CONFIG_SDHC_BCM2712_DMA_DESC_COUNT];
		struct sdhc_adma2_desc64 dma64[CONFIG_SDHC_BCM2712_DMA_DESC_COUNT];
	} desc __aligned(SDHC_ADMA2_ALIGNMENT);
	uint8_t bounce[4096] __aligned(SDHC_ADMA2_ALIGNMENT);
#endif
	void *buffer;
	size_t len;
	size_t cache_len;
	bool bounced;
	bool read;
	bool active;
};

#define CFG_CTRL             0x00U
#define CFG_CARD_TEST_ENABLE BIT(31)
#define CFG_CARD_TEST_LEVEL  BIT(30)
#define CFG_PIN_SELECT       0x44U
#define CFG_PIN_MASK         GENMASK(1, 0)
#define CFG_PIN_SD           BIT(1)
#define CFG_CQ_CAPABILITY    0x4cU
#define CFG_TIMER_MULTIPLIER (3U << 12)
#define HOST_CTRL2_UHS_SDR25 BIT(0)

struct bcm2712_config {
	struct sdhc_standard_host_common_config host;
	uint32_t clock_rate;
	const struct sdhc_adma2_config *adma2;
	/* R1b quirk: poll DAT0 instead of waiting for Transfer Complete. */
	bool no_busy_irq;
	void (*irq_config_func)(const struct device *dev);
	uintptr_t cfg_addr;
	size_t cfg_size;
	const struct device *clock;
	const struct device *vmmc;
	const struct device *vqmmc;
	struct gpio_dt_spec cd;
};

struct bcm2712_data {
	struct sdhc_standard_host_common_data host;
	struct k_mutex lock;
	struct k_spinlock irq_lock;
	struct k_event irq_event;
	uint32_t irq_status;
	uint32_t irq_mask;
	uint32_t adma_error;
	bool accepting_irq;
	bool dma_enabled;
	bool failed;
	struct sdhc_adma2_data adma2;
	mm_reg_t cfg_base;
	bool powered;
};

static int bcm2712_prepare(const struct device *dev)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *data = dev->data;
	uint32_t rate;
	uint32_t value;
	int ret;

	ret = clock_control_get_rate(config->clock, NULL, &rate);
	if (ret != 0) {
		return ret;
	}
	if (rate != config->clock_rate) {
		return -EINVAL;
	}
	value = sys_read32(data->cfg_base + CFG_PIN_SELECT);
	sys_write32((value & ~CFG_PIN_MASK) | CFG_PIN_SD, data->cfg_base + CFG_PIN_SELECT);
	/* Card detection is polled through the board's GPIO. */
	value = sys_read32(data->cfg_base + CFG_CTRL);
	value &= ~CFG_CARD_TEST_LEVEL;
	value |= CFG_CARD_TEST_ENABLE;
	sys_write32(value, data->cfg_base + CFG_CTRL);
	sys_write32(CFG_TIMER_MULTIPLIER | (rate / 1000000U),
		    data->cfg_base + CFG_CQ_CAPABILITY);
	return 0;
}

static int bcm2712_power(const struct device *dev, bool on)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *data = dev->data;
	int ret;

	if (on == data->powered) {
		return 0;
	}
	if (on) {
		ret = regulator_set_voltage(config->vqmmc, 3300000, 3300000);
		if (ret != 0) {
			return ret;
		}
		ret = regulator_enable(config->vmmc);
	} else {
		ret = regulator_disable(config->vmmc);
	}
	if (ret == 0) {
		data->powered = on;
	}
	return ret;
}

static void bcm2712_set_timing(const struct device *dev, enum sdhc_timing_mode timing)
{
	/*
	 * BCM2712 uses SDR25 for 3.3 V HS, as in Linux's
	 * sdhci_brcmstb_set_uhs_signaling().
	 */
	sdhc_standard_write16(dev, SDHC_REG_HOST_CTRL2,
			      timing == SDHC_TIMING_HS ? HOST_CTRL2_UHS_SDR25 : 0U);
}

static int bcm2712_card_present(const struct device *dev)
{
	const struct bcm2712_config *config = dev->config;

	return gpio_pin_get_dt(&config->cd);
}

static int bcm2712_dma_map(const struct device *dev, const void *buffer, size_t len,
			   uint64_t *address)
{
	uintptr_t virt = (uintptr_t)buffer;
	uintptr_t phys;

	ARG_UNUSED(dev);

	/* k_mem_phys_addr() is defined only for permanent kernel RAM mappings. */
#ifdef CONFIG_MMU
	if (virt < CONFIG_KERNEL_VM_BASE || len == 0U ||
	    virt - CONFIG_KERNEL_VM_BASE >= CONFIG_KERNEL_VM_SIZE ||
	    len > CONFIG_KERNEL_VM_SIZE - (virt - CONFIG_KERNEL_VM_BASE)) {
		return -ERANGE;
	}
#else
	ARG_UNUSED(virt);
#endif

	phys = k_mem_phys_addr((void *)buffer);
	if (len == 0U || phys < CONFIG_SRAM_BASE_ADDRESS ||
	    phys - CONFIG_SRAM_BASE_ADDRESS >= CONFIG_SRAM_SIZE * 1024U ||
	    len > CONFIG_SRAM_SIZE * 1024U - (phys - CONFIG_SRAM_BASE_ADDRESS)) {
		return -ERANGE;
	}

	/* The SD host accesses this low RAM window at its CPU physical address. */
	*address = phys;
	return 0;
}

static const struct sdhc_adma2_config bcm2712_adma2 = {
	.map = bcm2712_dma_map,
	/* Use the v3-compatible 64-bit format selected by Linux sdhci-brcmstb. */
	.dma64 = true,
};

#ifdef CONFIG_SDHC_BCM2712_DMA
static int sync_buffer(void *buffer, size_t len, bool read)
{
	if (!IS_ENABLED(CONFIG_DCACHE)) {
		return 0;
	}

	return read ? sys_cache_data_flush_and_invd_range(buffer, len) :
		      sys_cache_data_flush_range(buffer, len);
}

static int invalidate_buffer(void *buffer, size_t len)
{
	return IS_ENABLED(CONFIG_DCACHE) ? sys_cache_data_invd_range(buffer, len) : 0;
}

static int map_buffer(const struct device *dev, const struct sdhc_adma2_config *config,
		      const void *buffer, size_t len, uint64_t *addr)
{
	int ret = config->map(dev, buffer, len, addr);

	if (ret != 0) {
		return ret;
	}

	if (len == 0U || len - 1U > UINT64_MAX - *addr || (*addr & 3U) != 0U ||
	    (!config->dma64 && (*addr > UINT32_MAX || len - 1U > UINT32_MAX - *addr))) {
		return -ERANGE;
	}

	return 0;
}

static int bcm2712_adma2_prepare(const struct device *dev, const struct sdhc_adma2_config *config,
		       struct sdhc_adma2_data *dma, struct sdhc_data *data, bool read)
{
	size_t len = (size_t)data->blocks * data->block_size;
	size_t line = MAX(4U, sys_cache_data_line_size_get());
	size_t stride = config->dma64 ? sizeof(struct sdhc_adma2_desc64) :
					     sizeof(struct sdhc_adma2_desc);
	size_t count = DIV_ROUND_UP(len, SDHC_ADMA2_MAX_LENGTH);
	void *buffer = data->data;
	uint64_t address;
	uint64_t table;
	int ret;

	if (dma->active) {
		return -EBUSY;
	}

	if (count == 0U || count > CONFIG_SDHC_BCM2712_DMA_DESC_COUNT) {
		return -ENOTSUP;
	}

	dma->bounced = false;
	ret = map_buffer(dev, config, buffer, len, &address);

	if (ret != 0 || !IS_ALIGNED((uintptr_t)buffer, line) || !IS_ALIGNED(len, line)) {
		if (len > sizeof(dma->bounce) || line > SDHC_ADMA2_ALIGNMENT) {
			return -ENOTSUP;
		}

		buffer = dma->bounce;
		dma->bounced = true;
		ret = map_buffer(dev, config, buffer, len, &address);

		if (ret != 0) {
			return -ENOTSUP;
		}
	}

	ret = map_buffer(dev, config, &dma->desc, count * stride, &table);

	if (ret != 0 || !IS_ALIGNED(table, 8U)) {
		return -ENOTSUP;
	}

	if (!read && dma->bounced) {
		memcpy(buffer, data->data, len);
	}

	/* Bounce storage owns the complete cache lines touched by short requests. */
	dma->cache_len = dma->bounced ? ROUND_UP(len, line) : len;
	ret = sync_buffer(buffer, dma->cache_len, read);

	if (ret != 0) {
		return -ENOTSUP;
	}

	for (size_t i = 0U, remaining = len; i < count; i++) {
		size_t chunk = MIN(remaining, SDHC_ADMA2_MAX_LENGTH);
		uint16_t attr = SDHC_ADMA2_ATTR_VALID | SDHC_ADMA2_ATTR_TRAN;

		if (i == count - 1U) {
			attr |= SDHC_ADMA2_ATTR_END;
		}

		if (config->dma64) {
			dma->desc.dma64[i].attribute = sys_cpu_to_le16(attr);
			dma->desc.dma64[i].length = sys_cpu_to_le16((uint16_t)chunk);
			dma->desc.dma64[i].address_low = sys_cpu_to_le32((uint32_t)address);
			dma->desc.dma64[i].address_high =
				sys_cpu_to_le32((uint32_t)(address >> 32));
		} else {
			dma->desc.dma32[i].attribute = sys_cpu_to_le16(attr);
			dma->desc.dma32[i].length = sys_cpu_to_le16((uint16_t)chunk);
			dma->desc.dma32[i].address = sys_cpu_to_le32((uint32_t)address);
		}

		remaining -= chunk;
		address += chunk;
	}

	ret = sync_buffer(&dma->desc, count * stride, false);

	if (ret != 0) {
		return -ENOTSUP;
	}

	barrier_dmem_fence_full();
	/* The standard core performs two 32-bit writes, including the upper address. */
	sdhc_standard_write32(dev, SDHC_REG_ADMA_SYS_ADDR, (uint32_t)table);
	sdhc_standard_write32(dev, SDHC_REG_ADMA_SYS_ADDR + 4U, (uint32_t)(table >> 32));
	dma->buffer = buffer;
	dma->len = len;
	dma->read = read;
	dma->active = true;
	LOG_DBG("ADMA2 %s: %zu bytes, %s, %zu descriptors, %u-bit", read ? "read" : "write",
		len, dma->bounced ? "bounce" : "direct", count, config->dma64 ? 64U : 32U);

	return 0;
}

static int bcm2712_adma2_complete(struct sdhc_adma2_data *dma, struct sdhc_data *data)
{
	int ret;

	barrier_dmem_fence_full();

	if (dma->read) {
		ret = invalidate_buffer(dma->buffer, dma->cache_len);

		if (ret != 0) {
			return ret;
		}

		if (dma->bounced) {
			memcpy(data->data, dma->buffer, dma->len);
		}
	}

	data->bytes_xfered = dma->len;
	dma->active = false;
	dma->bounced = false;

	return 0;
}

static int bcm2712_adma2_abort(struct sdhc_adma2_data *dma)
{
	int ret = 0;

	barrier_dmem_fence_full();

	if (dma->active && dma->read) {
		ret = invalidate_buffer(dma->buffer, dma->cache_len);
	}

	dma->active = false;
	dma->bounced = false;

	return ret;
}
#endif

static int wait_reset(const struct device *dev, uint32_t mask)
{
	int64_t deadline = k_uptime_get() + 100;

	while ((sdhc_standard_read32(dev, SDHC_REG_CLOCK_CTRL) & mask) != 0U) {
		if (k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}

		k_usleep(100);
	}

	return 0;
}

static int reset_lines(const struct device *dev, uint32_t mask)
{
	sdhc_standard_write32(dev, SDHC_REG_CLOCK_CTRL,
			      sdhc_standard_read32(dev, SDHC_REG_CLOCK_CTRL) | mask);

	return wait_reset(dev, mask);
}

static void select_dma_mode(const struct device *dev, bool dma)
{
	const struct bcm2712_config *config = dev->config;
	const struct bcm2712_data *host = dev->data;
	uint32_t control = sdhc_standard_read32(dev, SDHC_REG_HOST_CTRL1);

	control &= ~SDHC_HOST_DMA_MASK;

	if (dma && host->dma_enabled) {
		control |= config->adma2->dma64 ? SDHC_HOST_DMA_ADMA2_64 : SDHC_HOST_DMA_ADMA2;
	}

	sdhc_standard_write32(dev, SDHC_REG_HOST_CTRL1, control);
}

/* All Signal Enable changes and request boundaries are serialized with the ISR. */
static void set_irq_mask(const struct device *dev, uint32_t mask)
{
	struct bcm2712_data *data = dev->data;

	data->irq_mask = mask;
	sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_SIG_EN, mask);
}

static void stop_irq(const struct device *dev)
{
	struct bcm2712_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->irq_lock);

	data->accepting_irq = false;
	set_irq_mask(dev, 0U);
	/* Flush a posted mask write before reset or buffer ownership changes. */
	(void)sdhc_standard_read32(dev, SDHC_REG_NORMAL_INT_SIG_EN);
	k_spin_unlock(&data->irq_lock, key);
}

static void begin_request(const struct device *dev)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->irq_lock);

	data->accepting_irq = false;
	set_irq_mask(dev, 0U);
	(void)sdhc_standard_read32(dev, SDHC_REG_NORMAL_INT_SIG_EN);
	sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT, SDHC_STATUS_MASK);
	k_event_clear(&data->irq_event, UINT32_MAX);
	data->irq_status = 0U;
	data->adma_error = 0U;

	if (config->irq_config_func != NULL) {
		data->accepting_irq = true;
		set_irq_mask(dev, SDHC_INT_CMD_DONE | SDHC_INT_DATA_DONE |
			     SDHC_INT_ERROR | SDHC_ERR_MASK);
	}

	k_spin_unlock(&data->irq_lock, key);
}

static void __unused bcm2712_isr(const struct device *dev)
{
	struct bcm2712_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->irq_lock);
	uint32_t status = sdhc_standard_read32(dev, SDHC_REG_NORMAL_INT_STAT) & SDHC_STATUS_MASK;

	if (!data->accepting_irq) {
		sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT, status);
		k_spin_unlock(&data->irq_lock, key);

		return;
	}

	status &= data->irq_mask;

	if (status != 0U) {
		/* The thread services the FIFO before rearming either ready signal. */
		set_irq_mask(dev, data->irq_mask &
			     ~(status & (SDHC_INT_READ_RDY | SDHC_INT_WRITE_RDY)));
		data->irq_status |= status;

		if ((status & (SDHC_INT_ERROR | SDHC_ERR_MASK)) != 0U) {
			data->adma_error = sdhc_standard_read32(dev, SDHC_REG_ADMA_ERR_STAT);
		}

		sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT, status);
		k_event_post(&data->irq_event, status);
	}

	k_spin_unlock(&data->irq_lock, key);
	LOG_DBG("IRQ status 0x%08x", status);
}

static uint32_t read_status(const struct device *dev)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *data = dev->data;
	uint32_t status;

	if (config->irq_config_func == NULL) {
		return sdhc_standard_read32(dev, SDHC_REG_NORMAL_INT_STAT);
	}

	k_spinlock_key_t key = k_spin_lock(&data->irq_lock);

	status = data->irq_status;
	k_spin_unlock(&data->irq_lock, key);

	return status;
}

static int bcm2712_reset(const struct device *dev)
{
	struct bcm2712_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	stop_irq(dev);
	ret = reset_lines(dev, SDHC_RESET_ALL);

	if (ret == 0) {
		ret = bcm2712_prepare(dev);
	}

	if (ret == 0) {
		sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_SIG_EN, 0U);
		sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT_EN, SDHC_STATUS_MASK);
		sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT, SDHC_STATUS_MASK);
		sdhc_standard_write32(dev, SDHC_REG_CLOCK_CTRL, SDHC_TIMEOUT_MAX);
	}

	if (ret == 0) {
#ifdef CONFIG_SDHC_BCM2712_DMA
		ret = bcm2712_adma2_abort(&data->adma2);
#endif

		data->failed = ret != 0;
		select_dma_mode(dev, data->dma_enabled);
	} else {
		data->failed = true;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int status_error(uint32_t status)
{
	if ((status & (SDHC_INT_ERROR | SDHC_ERR_MASK)) == 0U) {
		return 0;
	}

	if ((status & SDHC_ERR_TIMEOUT) != 0U) {
		return -ETIMEDOUT;
	}

	return (status & SDHC_ERR_CRC) != 0U ? -EILSEQ : -EIO;
}

static int wait_status(const struct device *dev, uint32_t wanted, int64_t deadline,
		       uint32_t *observed)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *data = dev->data;
	uint32_t status;
	int ret;

	if (config->irq_config_func != NULL) {
		int64_t remaining = deadline - k_uptime_get();
		uint32_t events = k_event_wait_safe(&data->irq_event,
			wanted | SDHC_INT_ERROR | SDHC_ERR_MASK, false,
			remaining > 0 ? K_MSEC(remaining) : K_NO_WAIT);

		status = read_status(dev);

		if (observed != NULL) {
			*observed = status;
		}

		ret = status_error(status);

		return ret != 0 ? ret : ((events & wanted) != 0U ? 0 : -ETIMEDOUT);
	}

	while (true) {
		status = sdhc_standard_read32(dev, SDHC_REG_NORMAL_INT_STAT);

		if (observed != NULL) {
			*observed = status;
		}

		ret = status_error(status);

		if (ret != 0) {
			return ret;
		}

		if ((status & wanted) != 0U) {
			/* Status bits are write-one-to-clear, never read-modify-write. */
			sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT, wanted);

			return 0;
		}

		if (k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}

		k_usleep(100);
	}
}

static int wait_busy(const struct device *dev, int64_t deadline, uint32_t *observed)
{
	const struct bcm2712_config *config = dev->config;
	int ret;

	if (!config->no_busy_irq) {
		/*
		 * Wait for the controller event, without sampling DAT0. BCM2712 CMD12
		 * can report Transfer Complete alongside Command Complete.
		 */
		return wait_status(dev, SDHC_INT_DATA_DONE, deadline, observed);
	}

	while (true) {
		*observed = read_status(dev);
		ret = status_error(*observed);

		if (ret != 0) {
			return ret;
		}

		if ((sdhc_standard_read32(dev, SDHC_REG_PRESENT_STATE) & SDHC_STATE_DAT0) != 0U) {
			/* Clear a completion event if the controller supplied one. */
			if (config->irq_config_func != NULL) {
				k_event_clear(&((struct bcm2712_data *)dev->data)->irq_event,
					      SDHC_INT_DATA_DONE);
			} else {
				sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT,
						      SDHC_INT_DATA_DONE);
			}

			return 0;
		}

		if (k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}

		k_usleep(100);
	}
}

static int wait_idle(const struct device *dev, uint32_t mask, int64_t deadline)
{
	while ((sdhc_standard_read32(dev, SDHC_REG_PRESENT_STATE) & mask) != 0U) {
		if (k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}

		k_usleep(100);
	}

	return 0;
}

static int command_flags(const struct sdhc_command *cmd, bool has_data)
{
	uint32_t flags = cmd->opcode << 8;

	if (cmd->opcode > 63U) {
		return -EINVAL;
	}

	switch (cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) {
	case SD_RSP_TYPE_NONE:
		break;
	case SD_RSP_TYPE_R2:
		flags |= 1U | SDHC_COMMAND_CRC;
		break;
	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
		flags |= 2U;
		break;
	case SD_RSP_TYPE_R1b:
	case SD_RSP_TYPE_R5b:
		flags |= 3U | SDHC_COMMAND_CRC | SDHC_COMMAND_INDEX;
		break;
	case SD_RSP_TYPE_R1:
	case SD_RSP_TYPE_R5:
	case SD_RSP_TYPE_R6:
	case SD_RSP_TYPE_R7:
		flags |= 2U | SDHC_COMMAND_CRC | SDHC_COMMAND_INDEX;
		break;
	default:
		return -EINVAL;
	}

	if (has_data) {
		flags |= SDHC_COMMAND_DATA;
	}

	if (cmd->opcode == SD_STOP_TRANSMISSION) {
		flags |= SDHC_COMMAND_ABORT;
	}

	return (int)flags;
}

static void read_response(const struct device *dev, struct sdhc_command *cmd)
{
	uint32_t value;

	memset(cmd->response, 0, sizeof(cmd->response));

	if ((cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) == SD_RSP_TYPE_NONE) {
		return;
	}

	if ((cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) != SD_RSP_TYPE_R2) {
		cmd->response[0] = sdhc_standard_read32(dev, SDHC_REG_RESPONSE_0_1);
		return;
	}

	/* SDHCI strips the CRC byte; Zephyr expects the low word first. */
	for (uint32_t i = 0U; i < ARRAY_SIZE(cmd->response); i++) {
		value = sdhc_standard_read32(dev, SDHC_REG_RESPONSE_0_1 + 4U * i) << 8;

		if (i > 0U) {
			value |= sdhc_standard_read32(dev,
						      SDHC_REG_RESPONSE_0_1 + 4U * (i - 1U)) >> 24;
		}

		cmd->response[i] = value;
	}
}

static int wait_buffer_ready(const struct device *dev, bool read, int64_t deadline)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *data = dev->data;
	uint32_t ready = read ? SDHC_INT_READ_RDY : SDHC_INT_WRITE_RDY;
	uint32_t enabled = read ? SDHC_STATE_READ_ENABLE : SDHC_STATE_WRITE_ENABLE;
	int ret;

	if (config->irq_config_func == NULL) {
		return wait_status(dev, ready, deadline, NULL);
	}

	while (true) {
		k_spinlock_key_t key = k_spin_lock(&data->irq_lock);

		ret = status_error(data->irq_status);
		set_irq_mask(dev, data->irq_mask & ~ready);
		sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT, ready);
		k_event_clear(&data->irq_event, ready);

		if (ret != 0 ||
		    (sdhc_standard_read32(dev, SDHC_REG_PRESENT_STATE) & enabled) != 0U) {
			k_spin_unlock(&data->irq_lock, key);

			return ret;
		}

		if (k_uptime_get() >= deadline) {
			k_spin_unlock(&data->irq_lock, key);

			return -ETIMEDOUT;
		}

		set_irq_mask(dev, data->irq_mask | ready);
		k_spin_unlock(&data->irq_lock, key);
		ret = wait_status(dev, ready, deadline, NULL);

		if (ret != 0) {
			return ret;
		}
	}
}

static int pio_transfer(const struct device *dev, struct sdhc_data *data, bool read)
{
	uint8_t *buf = data->data;
	int64_t deadline = k_uptime_get() + data->timeout_ms;
	uint32_t word;
	size_t count;
	int ret;

	for (uint32_t block = 0U; block < data->blocks; block++) {
		ret = wait_buffer_ready(dev, read, deadline);

		if (ret != 0) {
			return ret;
		}

		for (size_t offset = 0U; offset < data->block_size; offset += 4U) {
			count = MIN(4U, data->block_size - offset);

			if (read) {
				word = sdhc_standard_read32(dev, SDHC_REG_BUF_DATA_PORT);

				for (size_t i = 0U; i < count; i++) {
					buf[i] = word >> (8U * i);
				}
			} else {
				word = 0U;

				for (size_t i = 0U; i < count; i++) {
					word |= (uint32_t)buf[i] << (8U * i);
				}

				sdhc_standard_write32(dev, SDHC_REG_BUF_DATA_PORT, word);
			}

			buf += count;
		}

		data->bytes_xfered += data->block_size;
	}

	return wait_status(dev, SDHC_INT_DATA_DONE, deadline, NULL);
}

static int request_once(const struct device *dev, struct sdhc_command *cmd,
			struct sdhc_data *data, uint32_t flags)
{
#ifdef CONFIG_SDHC_BCM2712_DMA
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *host = dev->data;
#endif
	int64_t deadline = k_uptime_get() + cmd->timeout_ms;
	bool busy = (flags & 3U) == 3U;
	bool dma = false;
	bool read = cmd->opcode != SD_WRITE_SINGLE_BLOCK && cmd->opcode != SD_WRITE_MULTIPLE_BLOCK;
	uint32_t transfer = 0U;
	uint32_t inhibit = SDHC_STATE_CMD_INHIBIT;
	uint32_t command_status;
	uint32_t busy_status;
	int ret;

	if ((data != NULL || busy) && cmd->opcode != SD_STOP_TRANSMISSION) {
		inhibit |= SDHC_STATE_DAT_INHIBIT;
	}

	ret = wait_idle(dev, inhibit, deadline);

	if (ret != 0) {
		return ret;
	}

	LOG_DBG("CMD%u arg 0x%08x, response %u, blocks %u", cmd->opcode, cmd->arg,
		cmd->response_type & SDHC_NATIVE_RESPONSE_MASK, data != NULL ? data->blocks : 0U);

	if (data != NULL) {
		select_dma_mode(dev, false);

#ifdef CONFIG_SDHC_BCM2712_DMA
		if (host->dma_enabled) {
			ret = bcm2712_adma2_prepare(dev, config->adma2, &host->adma2, data, read);

			if (ret != 0 && ret != -ENOTSUP) {
				return ret;
			}

			dma = ret == 0;
		}

#endif

		transfer = SDHC_TRANSFER_COUNT | (read ? SDHC_TRANSFER_READ : 0U) |
			   (dma ? SDHC_TRANSFER_DMA_EN : 0U);

		select_dma_mode(dev, dma);

		if (data->blocks > 1U) {
			transfer |= SDHC_TRANSFER_MULTI | SDHC_TRANSFER_STOP;
		}

		sdhc_standard_write32(dev, SDHC_REG_BLOCK_SIZE,
				      (data->blocks << 16) | data->block_size);
	}

	begin_request(dev);
	sdhc_standard_write32(dev, SDHC_REG_ARGUMENT1, cmd->arg);
	/* A single write submits the command with its transfer mode. */
	sdhc_standard_write32(dev, SDHC_REG_TRANSFER_MODE, (flags << 16) | transfer);
	ret = wait_status(dev, SDHC_INT_CMD_DONE, deadline, &command_status);

	if (ret != 0) {
		return ret;
	}

	read_response(dev, cmd);

	if (data != NULL) {
#ifdef CONFIG_SDHC_BCM2712_DMA
		if (dma) {
			int64_t data_deadline = k_uptime_get() + data->timeout_ms;

			ret = wait_status(dev, SDHC_INT_DATA_DONE, data_deadline, NULL);

			if (ret == 0) {
				ret = wait_idle(dev, SDHC_STATE_DAT_INHIBIT, data_deadline);
			}

			return ret == 0 ? bcm2712_adma2_complete(&host->adma2, data) : ret;
		}

#endif

		return pio_transfer(dev, data, read);
	}

	if (busy) {
		ret = wait_busy(dev, deadline, &busy_status);
		LOG_DBG("CMD%u R1b: command 0x%08x, completion 0x%08x, state 0x%08x, result %d",
			cmd->opcode, command_status, busy_status,
			sdhc_standard_read32(dev, SDHC_REG_PRESENT_STATE), ret);

		return ret;
	}

	return 0;
}

static int bcm2712_request(const struct device *dev, struct sdhc_command *cmd,
			struct sdhc_data *data)
{
	struct bcm2712_data *host = dev->data;
	uint32_t retries = cmd->retries;
	int flags = command_flags(cmd, data != NULL);
	int recovery;
	int ret;

	if (flags < 0 || cmd->timeout_ms < 0) {
		return -EINVAL;
	}

	if (data != NULL) {
		data->bytes_xfered = 0U;

#ifdef CONFIG_SDHC_SCATTER_GATHER_TRANSFER
		if (data->is_sg_data) {
			return -ENOTSUP;
		}
#endif

		if (data->data == NULL || data->blocks == 0U || data->blocks > UINT16_MAX ||
		    data->block_size == 0U || data->block_size > 512U || data->timeout_ms < 0) {
			return -EINVAL;
		}
	}

	k_mutex_lock(&host->lock, K_FOREVER);

	if (host->failed) {
		k_mutex_unlock(&host->lock);

		return -EIO;
	}

	do {
		ret = request_once(dev, cmd, data, flags);
		stop_irq(dev);

		if (ret == 0) {
			break;
		}

		LOG_DBG("CMD%u failed: %d, status 0x%08x, ADMA error 0x%08x",
			cmd->opcode, ret, read_status(dev), host->adma_error);
		recovery = reset_lines(dev, SDHC_RESET_CMD_DATA);
		sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT, SDHC_STATUS_MASK);

		if (recovery != 0) {
			host->failed = true;
			ret = recovery;
			break;
		}

#ifdef CONFIG_SDHC_BCM2712_DMA
		recovery = bcm2712_adma2_abort(&host->adma2);

		if (recovery != 0) {
			ret = recovery;
		}
#endif

		/* A data command may already have changed the card state. */
		if (data != NULL || retries == 0U) {
			break;
		}

		retries--;
	} while (true);
	k_mutex_unlock(&host->lock);

	return ret;
}

static int set_clock(const struct device *dev, uint32_t rate)
{
	const struct bcm2712_config *config = dev->config;
	uint32_t control = sdhc_standard_read32(dev, SDHC_REG_CLOCK_CTRL);
	uint32_t divisor;
	int64_t deadline;

	control &= ~(SDHC_CLOCK_CARD | SDHC_CLOCK_INTERNAL | SDHC_CLOCK_DIV_MASK |
		     SDHC_TIMEOUT_MASK);
	control |= SDHC_TIMEOUT_MAX;
	sdhc_standard_write32(dev, SDHC_REG_CLOCK_CTRL, control);

	if (rate == 0U) {
		return 0;
	}

	divisor = DIV_ROUND_UP(config->clock_rate, rate);
	/* SDHCI v3 divides by one, or an even divisor from 2 to 2046. */
	divisor = divisor <= 1U ? 0U : DIV_ROUND_UP(divisor, 2U);

	if (divisor > 1023U) {
		return -EINVAL;
	}

	control |= ((divisor & 0xffU) << 8) | ((divisor & 0x300U) >> 2);
	control |= SDHC_CLOCK_INTERNAL;
	sdhc_standard_write32(dev, SDHC_REG_CLOCK_CTRL, control);
	deadline = k_uptime_get() + 100;

	while ((sdhc_standard_read32(dev, SDHC_REG_CLOCK_CTRL) & SDHC_CLOCK_STABLE) == 0U) {
		if (k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}

		k_usleep(100);
	}

	sdhc_standard_write32(dev, SDHC_REG_CLOCK_CTRL, control | SDHC_CLOCK_CARD);

	return 0;
}

static int bcm2712_set_io(const struct device *dev, struct sdhc_io *io)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *host = dev->data;
	uint32_t control;
	int ret;

	if ((io->bus_width != SDHC_BUS_WIDTH1BIT && io->bus_width != SDHC_BUS_WIDTH4BIT) ||
	    (io->bus_width == SDHC_BUS_WIDTH4BIT && config->host.common.bus_width < 4U) ||
	    io->signal_voltage != SD_VOL_3_3_V ||
	    (io->timing != SDHC_TIMING_LEGACY && io->timing != SDHC_TIMING_HS) ||
	    (io->timing == SDHC_TIMING_HS && host->host.props.host_caps.high_spd_support == 0U) ||
	    io->bus_mode != SDHC_BUSMODE_PUSHPULL || io->enhanced_strobe ||
	    (io->power_mode != SDHC_POWER_ON && io->power_mode != SDHC_POWER_OFF)) {
		return -ENOTSUP;
	}

	if (io->clock != 0U && (io->clock < config->host.common.min_bus_freq ||
			       io->clock > config->host.common.max_bus_freq ||
			       (io->timing == SDHC_TIMING_LEGACY && io->clock > SD_CLOCK_25MHZ))) {
		return -EINVAL;
	}

	k_mutex_lock(&host->lock, K_FOREVER);

	if (host->failed) {
		k_mutex_unlock(&host->lock);

		return -EIO;
	}

	ret = set_clock(dev, 0U);

	if (ret == 0) {
		ret = bcm2712_power(dev, io->power_mode == SDHC_POWER_ON);
	}

	if (ret == 0) {
		control = sdhc_standard_read32(dev, SDHC_REG_HOST_CTRL1);
		control &= ~(SDHC_HOST_WIDTH4 | SDHC_HOST_WIDTH8 | SDHC_HOST_HIGH_SPEED |
			     SDHC_HOST_DMA_MASK | GENMASK(15, 8));

		if (io->power_mode == SDHC_POWER_ON) {
			control |= SDHC_POWER_ON_330;
		}

		if (io->bus_width == SDHC_BUS_WIDTH4BIT) {
			control |= SDHC_HOST_WIDTH4;
		}

		if (io->timing == SDHC_TIMING_HS) {
			control |= SDHC_HOST_HIGH_SPEED;
		}

		sdhc_standard_write32(dev, SDHC_REG_HOST_CTRL1, control);
		select_dma_mode(dev, host->dma_enabled);
		/* Clear UHS mode, 1.8 V signalling, tuning and preset values. */
		sdhc_standard_write32(dev, SDHC_REG_AUTO_CMD_ERR_STAT, 0U);

		bcm2712_set_timing(dev, io->timing);

		ret = set_clock(dev, io->power_mode == SDHC_POWER_ON ? io->clock : 0U);
	}

	k_mutex_unlock(&host->lock);

	return ret;
}

static int host_card_busy(const struct device *dev)
{
	return (sdhc_standard_read32(dev, SDHC_REG_PRESENT_STATE) & SDHC_STATE_DAT0) == 0U;
}

static int host_props(const struct device *dev, struct sdhc_host_props *props)
{
	const struct bcm2712_data *data = dev->data;

	*props = data->host.props;

	return 0;
}

static int bcm2712_setup(const struct device *dev)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *data = dev->data;
	struct sdhc_host_props *props = &data->host.props;
	int ret;

	if (config->clock_rate == 0U || config->host.common.min_bus_freq < 400000U ||
	    config->host.common.max_bus_freq > SD_CLOCK_50MHZ ||
	    config->host.common.min_bus_freq > config->host.common.max_bus_freq ||
	    (config->host.common.bus_width != 1U && config->host.common.bus_width != 4U)) {
		return -EINVAL;
	}

	/* device_map() enables direct mapping only when the kernel supports it. */
	DEVICE_MMIO_NAMED_MAP(dev, sdhc_base, K_MEM_CACHE_NONE);
	sdhc_common_dt_props_init(props, &config->host.common);
	sdhc_standard_capabilities_init(dev);
	k_mutex_init(&data->lock);
	k_event_init(&data->irq_event);
	data->dma_enabled = IS_ENABLED(CONFIG_SDHC_BCM2712_DMA) && config->adma2 != NULL &&
		config->adma2->map != NULL && props->host_caps.adma_2_support != 0U &&
		(!config->adma2->dma64 || props->host_caps.address_64_bit_support_v3 != 0U);
	ret = bcm2712_reset(dev);

	if (ret != 0) {
		return ret;
	}

	if (props->host_caps.vol_330_support == 0U) {
		return -ENOTSUP;
	}

	/* HS requires both the hardware capability and a board clock above default speed. */
	if (props->host_caps.high_spd_support == 0U ||
	    config->host.common.max_bus_freq <= SD_CLOCK_25MHZ) {
		props->host_caps.high_spd_support = 0U;
		props->f_max = MIN(props->f_max, SD_CLOCK_25MHZ);
	}

	/* Hide unimplemented modes while retaining clock, block and slot properties. */
	props->host_caps.bus_8_bit_support = 0U;
	props->host_caps.adma_2_support = data->dma_enabled ? 1U : 0U;
	props->host_caps.sdma_support = 0U;
	props->host_caps.suspend_res_support = 0U;
	props->host_caps.vol_300_support = 0U;
	props->host_caps.vol_180_support = 0U;
	props->host_caps.address_64_bit_support_v4 = 0U;
	props->host_caps.address_64_bit_support_v3 =
		data->dma_enabled && config->adma2->dma64 ? 1U : 0U;
	props->host_caps.sdio_async_interrupt_support = 0U;
	props->host_caps.sdr50_support = 0U;
	props->host_caps.sdr104_support = 0U;
	props->host_caps.ddr50_support = 0U;
	props->host_caps.uhs_2_support = 0U;
	props->host_caps.drv_type_a_support = 0U;
	props->host_caps.drv_type_c_support = 0U;
	props->host_caps.drv_type_d_support = 0U;
	props->host_caps.retune_timer_count = 0U;
	props->host_caps.sdr50_needs_tuning = 0U;
	props->host_caps.retuning_mode = 0U;
	props->host_caps.adma3_support = 0U;
	props->host_caps.vdd2_180_support = 0U;
	props->hs200_support = false;
	props->hs400_support = false;
	props->hs400_enhanced_strobe_support = false;

	if (config->irq_config_func != NULL) {
		config->irq_config_func(dev);
	}

	return 0;
}

static const DEVICE_API(sdhc, bcm2712_api) = {
	.reset = bcm2712_reset,
	.request = bcm2712_request,
	.set_io = bcm2712_set_io,
	.get_card_present = bcm2712_card_present,
	.card_busy = host_card_busy,
	.get_host_props = host_props,
};

static int bcm2712_init(const struct device *dev)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->clock) || !device_is_ready(config->vmmc) ||
	    !device_is_ready(config->vqmmc) || !gpio_is_ready_dt(&config->cd)) {
		return -ENODEV;
	}
	/* shortcut: retain SD-boot pin setup; add pinctrl for other boot paths. */
	ret = gpio_pin_configure_dt(&config->cd, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}
	device_map(&data->cfg_base, config->cfg_addr, config->cfg_size, K_MEM_CACHE_NONE);
	return bcm2712_setup(dev);
}

#define BCM2712_IRQ_CONFIG(n) \
	static void bcm2712_irq_config_##n(const struct device *dev) \
	{ \
		ARG_UNUSED(dev); \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), bcm2712_isr, \
			    DEVICE_DT_INST_GET(n), DT_INST_IRQ(n, flags)); \
		irq_enable(DT_INST_IRQN(n)); \
	}

#define BCM2712_DEFINE(n) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, interrupts), (BCM2712_IRQ_CONFIG(n)), ()) \
	static struct bcm2712_data bcm2712_data_##n; \
	static const struct bcm2712_config bcm2712_config_##n = { \
		.host = SDHC_STANDARD_COMMON_CONFIG_DT_INST_INIT(n, NULL), \
		.irq_config_func = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, interrupts), \
					     (bcm2712_irq_config_##n), (NULL)), \
		.clock_rate = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency), \
		.adma2 = IS_ENABLED(CONFIG_SDHC_BCM2712_DMA) ? &bcm2712_adma2 : NULL, \
		.cfg_addr = DT_INST_REG_ADDR_BY_NAME(n, cfg), \
		.cfg_size = DT_INST_REG_SIZE_BY_NAME(n, cfg), \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)), \
		.vmmc = DEVICE_DT_GET(DT_INST_PHANDLE(n, vmmc_supply)), \
		.vqmmc = DEVICE_DT_GET(DT_INST_PHANDLE(n, vqmmc_supply)), \
		.cd = GPIO_DT_SPEC_INST_GET(n, cd_gpios), \
	}; \
	DEVICE_DT_INST_DEFINE(n, bcm2712_init, NULL, &bcm2712_data_##n, &bcm2712_config_##n, \
			      POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, &bcm2712_api);

DT_INST_FOREACH_STATUS_OKAY(BCM2712_DEFINE)
