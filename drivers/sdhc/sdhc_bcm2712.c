/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_bcm2712_sdhci

#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/clock.h>

#include "sdhc_standard_core.h"

LOG_MODULE_REGISTER(sdhc_bcm2712, CONFIG_SDHC_LOG_LEVEL);

/* Normal/error status and enables are accessed together as 32-bit words. */
#define SDHC_ERR_TIMEOUT                                                                           \
	((SDHC_ERROR_INT_STAT_CMD_TIMEOUT | SDHC_ERROR_INT_STAT_DATA_TIMEOUT) << 16)
#define SDHC_ERR_CRC      ((SDHC_ERROR_INT_STAT_CMD_CRC | SDHC_ERROR_INT_STAT_DATA_CRC) << 16)
#define SDHC_ERR_AUTO_CMD (SDHC_ERROR_INT_STAT_AUTO_CMD << 16)
#define SDHC_ERR_MASK     (SDHC_ERROR_INT_STAT_MASK << 16)
#define SDHC_STATUS_MASK                                                                           \
	(SDHC_NORMAL_INT_STAT_CMD_COMPLETE | SDHC_NORMAL_INT_STAT_TRANSFER_COMPLETE |              \
	 SDHC_NORMAL_INT_STAT_BUF_WRITE_READY | SDHC_NORMAL_INT_STAT_BUF_READ_READY |              \
	 SDHC_NORMAL_INT_STAT_RETUNING_EVENT | SDHC_NORMAL_INT_STAT_ERROR | SDHC_ERR_MASK)

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
	uint8_t bounce[ROUND_UP(CONFIG_SDHC_BCM2712_DMA_BUFFER_SIZE, SDHC_ADMA2_ALIGNMENT)]
		__aligned(SDHC_ADMA2_ALIGNMENT);
#endif
	void *buffer;
	size_t len;
	size_t cache_len;
	bool bounced;
	bool read;
	bool active;
};

#define CFG_CTRL                 0x00U
#define CFG_CARD_TEST_ENABLE     BIT(31)
#define CFG_CARD_TEST_LEVEL      BIT(30)
#define CFG_PIN_SELECT           0x44U
#define CFG_PIN_MASK             GENMASK(1, 0)
#define CFG_PIN_SD               BIT(1)
#define CFG_CQ_CAPABILITY        0x4cU
#define CFG_TIMER_MULTIPLIER     (3U << 12)
#define CFG_MAX_50MHZ_MODE       0x1acU
#define CFG_MAX_50MHZ_OVERRIDE   BIT(31)
#define CFG_MAX_50MHZ_ENABLE     BIT(0)
#define SDHC_TUNING_TIMEOUT_MS   150
#define SDHC_TUNING_ATTEMPTS     40U
#define SDHC_RECOVERY_TIMEOUT_MS 1000

struct bcm2712_config {
	struct sdhc_standard_host_common_config host;
	uint32_t clock_rate;
	const struct sdhc_adma2_config *adma2;
	bool sdr50;
	bool sdr104;
	bool no_1_8_v;
	bool non_removable;
	const struct pinctrl_dev_config *pinctrl;
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
	/* STAT_EN shadow, updated only by set_status_mask() with irq_lock held. */
	uint32_t status_enable;
	uint32_t adma_error;
	sdhc_interrupt_cb_t card_callback;
	void *card_user_data;
	bool card_irq_enabled;
	bool card_irq_pending;
	bool accepting_irq;
	bool dma_enabled;
	/* Fatal states require sdhc_hw_reset() and card reinitialization. */
	bool failed;
	struct sdhc_adma2_data adma2;
	mm_reg_t cfg_base;
	bool powered;
	bool uhs_enabled;
	bool tuned;
	bool app_cmd;
	uint16_t selected_rca;
	atomic_t retune_needed;
	k_timepoint_t retune_deadline;
	struct sdhc_io io;
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
	if (data->uhs_enabled) {
		/* Linux sdhci_brcmstb_cfginit_2712(): use the delay-line PHY above HS. */
		value = sys_read32(data->cfg_base + CFG_MAX_50MHZ_MODE);
		value &= ~CFG_MAX_50MHZ_ENABLE;
		value |= CFG_MAX_50MHZ_OVERRIDE;
		sys_write32(value, data->cfg_base + CFG_MAX_50MHZ_MODE);
	}

	value = sys_read32(data->cfg_base + CFG_PIN_SELECT);
	sys_write32((value & ~CFG_PIN_MASK) | CFG_PIN_SD, data->cfg_base + CFG_PIN_SELECT);
	/* Card detection is polled through the board's GPIO. */
	value = sys_read32(data->cfg_base + CFG_CTRL);
	value &= ~CFG_CARD_TEST_LEVEL;
	value |= CFG_CARD_TEST_ENABLE;
	sys_write32(value, data->cfg_base + CFG_CTRL);
	sys_write32(CFG_TIMER_MULTIPLIER | MAX(rate / 1000000U, 1U),
		    data->cfg_base + CFG_CQ_CAPABILITY);
	return 0;
}

static int set_regulator_voltage(const struct device *regulator, int32_t voltage)
{
	int32_t current_voltage;
	int ret;

	ret = regulator_set_voltage(regulator, voltage, voltage);
	if (ret != -ENOSYS) {
		return ret;
	}

	ret = regulator_get_voltage(regulator, &current_voltage);
	if (ret != 0) {
		return ret;
	}

	return current_voltage == voltage ? 0 : -ENOTSUP;
}

static int set_io_voltage(const struct device *dev, enum sd_voltage signal_voltage)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *host = dev->data;
	int32_t voltage;
	uint16_t control;
	int ret;

	switch (signal_voltage) {
	case SD_VOL_3_3_V:
		voltage = 3300000;
		break;
	case SD_VOL_1_8_V:
		if (!host->uhs_enabled) {
			return -EINVAL;
		}
		voltage = 1800000;
		break;
	default:
		return -EINVAL;
	}

	if (config->vqmmc == NULL) {
		host->io.signal_voltage = SD_VOL_3_3_V;
		return 0;
	}

	ret = set_regulator_voltage(config->vqmmc, voltage);
	if (ret != 0) {
		host->failed = true;
		return ret;
	}

	control = sdhc_standard_read16(dev, SDHC_REG_HOST_CTRL2);
	control &= ~SDHC_HOST_CTRL2_V18;
	if (signal_voltage == SD_VOL_1_8_V) {
		control |= SDHC_HOST_CTRL2_V18;
	}
	sdhc_standard_write16(dev, SDHC_REG_HOST_CTRL2, control);
	k_msleep(5);

	if ((sdhc_standard_read16(dev, SDHC_REG_HOST_CTRL2) & SDHC_HOST_CTRL2_V18) !=
	    (control & SDHC_HOST_CTRL2_V18)) {
		host->failed = true;
		return -EIO;
	}

	host->io.signal_voltage = signal_voltage;
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
		ret = config->vqmmc != NULL ? set_regulator_voltage(config->vqmmc, 3300000) : 0;

		if (ret != 0) {
			data->failed = true;
			return ret;
		}

		ret = regulator_enable(config->vmmc);
	} else {
		ret = regulator_disable(config->vmmc);
	}

	if (ret == 0) {
		data->powered = on;
		if (!on) {
			data->selected_rca = 0U;
		}
	} else {
		data->failed = true;
	}

	return ret;
}

static int bcm2712_set_timing(const struct device *dev, const struct sdhc_io *io,
			      const struct sdhc_io *previous_io)
{
	struct bcm2712_data *host = dev->data;
	uint32_t max_clock = SD_CLOCK_25MHZ;
	uint16_t mode = 0U;
	uint16_t control;
	uint8_t control1;

	switch (io->timing) {
	case SDHC_TIMING_LEGACY:
	case SDHC_TIMING_SDR12:
		break;
	case SDHC_TIMING_HS:
	case SDHC_TIMING_SDR25:
		if (io->timing == SDHC_TIMING_HS &&
		    (host->host.props.host_caps.high_spd_support == 0U ||
		     io->signal_voltage != SD_VOL_3_3_V)) {
			return -EINVAL;
		}
		/* BCM2712 also uses SDR25 for 3.3 V HS. */
		mode = SDHC_HOST_CTRL2_UHS_SDR25;
		max_clock = SD_CLOCK_50MHZ;
		break;
	case SDHC_TIMING_SDR50:
		if (host->host.props.host_caps.sdr50_support == 0U) {
			return -EINVAL;
		}
		mode = SDHC_HOST_CTRL2_UHS_SDR50;
		max_clock = SD_CLOCK_100MHZ;
		break;
	case SDHC_TIMING_SDR104:
		if (host->host.props.host_caps.sdr104_support == 0U) {
			return -EINVAL;
		}
		mode = SDHC_HOST_CTRL2_UHS_SDR104;
		max_clock = SD_CLOCK_208MHZ;
		break;
	default:
		return -EINVAL;
	}

	if (io->clock > max_clock || (io->timing >= SDHC_TIMING_SDR12 &&
				      (!host->uhs_enabled || io->signal_voltage != SD_VOL_1_8_V))) {
		return -EINVAL;
	}

	control = sdhc_standard_read16(dev, SDHC_REG_HOST_CTRL2);
	control &= ~SDHC_HOST_CTRL2_UHS_MASK;
	control |= mode;

	if (io->clock != previous_io->clock || io->timing != previous_io->timing ||
	    io->signal_voltage != previous_io->signal_voltage ||
	    io->bus_width != previous_io->bus_width || io->power_mode == SDHC_POWER_OFF) {
		control &= ~(SDHC_HOST_CTRL2_EXEC_TUNING | SDHC_HOST_CTRL2_TUNED |
			     SDHC_HOST_CTRL2_PRESET);
		host->tuned = false;
		host->retune_deadline = sys_timepoint_calc(K_FOREVER);
	}
	sdhc_standard_write16(dev, SDHC_REG_HOST_CTRL2, control);

	control1 = sdhc_standard_read8(dev, SDHC_REG_HOST_CTRL1);
	control1 &= ~SDHC_HOST_CTRL1_HIGH_SPEED;
	if (io->timing != SDHC_TIMING_LEGACY && io->timing != SDHC_TIMING_SDR12) {
		control1 |= SDHC_HOST_CTRL1_HIGH_SPEED;
	}
	sdhc_standard_write8(dev, SDHC_REG_HOST_CTRL1, control1);

	return 0;
}

static int bcm2712_card_present(const struct device *dev)
{
	const struct bcm2712_config *config = dev->config;
	int ret;

	if (config->non_removable) {
		return 1;
	}

	ret = gpio_pin_get_dt(&config->cd);

	if (ret < 0) {
		LOG_ERR("Card detection failed: %d", ret);
		return ret;
	}

	return ret > 0 ? 1 : 0;
}

static int bcm2712_dma_map(const struct device *dev, const void *buffer, size_t len,
			   uint64_t *address)
{
	uintptr_t virt = (uintptr_t)buffer;
	uintptr_t phys;
	uint64_t ram_start = DT_REG_ADDR(DT_CHOSEN(zephyr_sram));
	uint64_t ram_size = DT_REG_SIZE(DT_CHOSEN(zephyr_sram));

	ARG_UNUSED(dev);

	/* k_mem_phys_addr() is defined only for permanent kernel RAM mappings. */
#ifdef CONFIG_MMU
	if (virt < CONFIG_KERNEL_VM_BASE || len == 0U ||
	    virt - CONFIG_KERNEL_VM_BASE >= CONFIG_KERNEL_VM_SIZE ||
	    len > CONFIG_KERNEL_VM_SIZE - (virt - CONFIG_KERNEL_VM_BASE)) {
		return -ERANGE;
	}
	phys = k_mem_phys_addr((void *)buffer);
#else
	phys = virt;
#endif

	if (len == 0U || phys < ram_start || phys - ram_start >= ram_size ||
	    len > ram_size - (phys - ram_start)) {
		return -ERANGE;
	}

	/* The SD host accesses this low RAM window at its CPU physical address. */
	*address = phys;
	return 0;
}

static const struct sdhc_adma2_config bcm2712_adma2 = {
	.map = bcm2712_dma_map,
	.dma64 = IS_ENABLED(CONFIG_SDHC_BCM2712_DMA_64BIT),
};

#ifdef CONFIG_SDHC_BCM2712_DMA
static int sync_buffer(void *buffer, size_t len, bool read)
{
	if (!IS_ENABLED(CONFIG_DCACHE)) {
		return 0;
	}

	return read ? sys_cache_data_flush_and_invd_range(buffer, len)
		    : sys_cache_data_flush_range(buffer, len);
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
	size_t stride =
		config->dma64 ? sizeof(struct sdhc_adma2_desc64) : sizeof(struct sdhc_adma2_desc);
	size_t count = DIV_ROUND_UP(len, SDHC_ADMA2_MAX_LENGTH_16BIT);
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
		size_t chunk = MIN(remaining, SDHC_ADMA2_MAX_LENGTH_16BIT);
		uint16_t attr = (uint16_t)(SDHC_ADMA2_ATTR_VALID | SDHC_ADMA2_ATTR_TRAN);

		if (i == count - 1U) {
			attr |= (uint16_t)SDHC_ADMA2_ATTR_END;
		}

		/* A zero 16-bit length encodes a full 64 KiB descriptor. */
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
	LOG_DBG("ADMA2 %s: %zu bytes, %s, %zu descriptors, %u-bit", read ? "read" : "write", len,
		dma->bounced ? "bounce" : "direct", count, config->dma64 ? 64U : 32U);

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

	data->bytes_xfered = (unsigned int)dma->len;
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

static int reset_lines(const struct device *dev, uint8_t mask)
{
	sdhc_standard_write8(dev, SDHC_REG_SW_RESET,
			     sdhc_standard_read8(dev, SDHC_REG_SW_RESET) | mask);

	return WAIT_FOR((sdhc_standard_read8(dev, SDHC_REG_SW_RESET) & mask) == 0U, 100000U,
			k_usleep(100))
		       ? 0
		       : -ETIMEDOUT;
}

static void select_dma_mode(const struct device *dev, bool dma)
{
	const struct bcm2712_config *config = dev->config;
	const struct bcm2712_data *host = dev->data;
	uint8_t control = sdhc_standard_read8(dev, SDHC_REG_HOST_CTRL1);

	control &= ~SDHC_HOST_CTRL1_DMA_MASK;

	if (dma && host->dma_enabled) {
		control |= config->adma2->dma64 ? SDHC_HOST_CTRL1_DMA_ADMA2_64
						: SDHC_HOST_CTRL1_DMA_ADMA2;
	}

	sdhc_standard_write8(dev, SDHC_REG_HOST_CTRL1, control);
}

/* Call with irq_lock held; CARD follows the current registration and pending state. */
static void set_status_mask(const struct device *dev, uint32_t mask)
{
	struct bcm2712_data *data = dev->data;

	mask &= ~SDHC_NORMAL_INT_STAT_CARD;
	if (data->card_irq_enabled && !data->card_irq_pending) {
		mask |= SDHC_NORMAL_INT_STAT_CARD;
	}

	data->status_enable = mask;
	sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT_EN, mask);
}

/* All Signal Enable changes and request boundaries are serialized with the ISR. */
static void set_irq_mask(const struct device *dev, uint32_t mask)
{
	struct bcm2712_data *data = dev->data;
	uint32_t card_enable = 0U;

	data->irq_mask = mask & ~SDHC_NORMAL_INT_STAT_CARD;
	/* Card interrupts remain enabled between requests, unlike completion signals. */
	if (data->card_irq_enabled && !data->card_irq_pending) {
		card_enable = SDHC_NORMAL_INT_STAT_CARD;
	}

	/* Like sdhci_enable_sdio_irq_nolock(), gate the level in both enable registers. */
	if ((data->status_enable & SDHC_NORMAL_INT_STAT_CARD) != card_enable) {
		set_status_mask(dev, data->status_enable);
	}

	sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_SIG_EN, data->irq_mask | card_enable);
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
	if ((sdhc_standard_read32(dev, SDHC_REG_NORMAL_INT_STAT) &
	     SDHC_NORMAL_INT_STAT_RETUNING_EVENT) != 0U) {
		atomic_set(&data->retune_needed, 1);
	}

	sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT, SDHC_STATUS_MASK);
	k_event_clear(&data->irq_event, UINT32_MAX);
	data->irq_status = 0U;
	data->adma_error = 0U;

	if (config->irq_config_func != NULL) {
		data->accepting_irq = true;
		set_irq_mask(dev, SDHC_NORMAL_INT_STAT_CMD_COMPLETE |
					  SDHC_NORMAL_INT_STAT_TRANSFER_COMPLETE |
					  SDHC_NORMAL_INT_STAT_ERROR | SDHC_ERR_MASK |
					  SDHC_NORMAL_INT_STAT_RETUNING_EVENT);
	}

	k_spin_unlock(&data->irq_lock, key);
}

static void __unused bcm2712_isr(const struct device *dev)
{
	struct bcm2712_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->irq_lock);
	sdhc_interrupt_cb_t callback = NULL;
	void *user_data = NULL;
	uint32_t status = sdhc_standard_read32(dev, SDHC_REG_NORMAL_INT_STAT);

	if ((status & SDHC_NORMAL_INT_STAT_CARD) != 0U && data->card_irq_enabled &&
	    !data->card_irq_pending) {
		/* The ISR callback must defer blocking work and rearm after servicing the card. */
		data->card_irq_pending = true;
		set_irq_mask(dev, data->irq_mask);
		callback = data->card_callback;
		user_data = data->card_user_data;
	}

	status &= SDHC_STATUS_MASK;

	if (!data->accepting_irq) {
		if ((status & SDHC_NORMAL_INT_STAT_RETUNING_EVENT) != 0U) {
			atomic_set(&data->retune_needed, 1);
		}

		sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT, status);
		k_spin_unlock(&data->irq_lock, key);

		if (callback != NULL) {
			callback(dev, SDHC_INT_SDIO, user_data);
		}

		return;
	}

	status &= data->irq_mask;

	if (status != 0U) {
		/* The thread services the FIFO before rearming either ready signal. */
		set_irq_mask(dev,
			     data->irq_mask & ~(status & (SDHC_NORMAL_INT_STAT_BUF_READ_READY |
							  SDHC_NORMAL_INT_STAT_BUF_WRITE_READY)));
		data->irq_status |= status;

		if ((status & SDHC_NORMAL_INT_STAT_RETUNING_EVENT) != 0U) {
			atomic_set(&data->retune_needed, 1);
			set_irq_mask(dev, data->irq_mask & ~SDHC_NORMAL_INT_STAT_RETUNING_EVENT);
		}

		if ((status & (SDHC_NORMAL_INT_STAT_ERROR | SDHC_ERR_MASK)) != 0U) {
			data->adma_error = sdhc_standard_read32(dev, SDHC_REG_ADMA_ERR_STAT);
		}

		sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT, status);
		k_event_post(&data->irq_event, status);
	}

	k_spin_unlock(&data->irq_lock, key);
	if (callback != NULL) {
		callback(dev, SDHC_INT_SDIO, user_data);
	}

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
	k_spinlock_key_t key;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	key = k_spin_lock(&data->irq_lock);

	data->card_irq_enabled = false;
	data->card_irq_pending = false;
	data->card_callback = NULL;
	data->card_user_data = NULL;
	data->accepting_irq = false;
	set_irq_mask(dev, 0U);
	(void)sdhc_standard_read32(dev, SDHC_REG_NORMAL_INT_SIG_EN);
	k_spin_unlock(&data->irq_lock, key);

	ret = reset_lines(dev, SDHC_SW_RESET_ALL);
	if (ret != 0) {
		goto out;
	}

	ret = bcm2712_prepare(dev);
	if (ret != 0) {
		goto out;
	}

	key = k_spin_lock(&data->irq_lock);
	/* Reset clears the hardware register even when the cached mask is unchanged. */
	set_status_mask(dev, SDHC_STATUS_MASK);
	set_irq_mask(dev, 0U);
	k_spin_unlock(&data->irq_lock, key);
	sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT, SDHC_STATUS_MASK);
	sdhc_standard_write8(dev, SDHC_REG_TIMEOUT_CTRL, SDHC_TIMEOUT_CTRL_MAX);

#ifdef CONFIG_SDHC_BCM2712_DMA
	ret = bcm2712_adma2_abort(&data->adma2);
#endif

	data->io = (struct sdhc_io){0};
	data->tuned = false;
	data->app_cmd = false;
	data->selected_rca = 0U;
	atomic_clear(&data->retune_needed);
	data->retune_deadline = sys_timepoint_calc(K_FOREVER);
	select_dma_mode(dev, data->dma_enabled);

out:
	data->failed = ret != 0;
	k_mutex_unlock(&data->lock);

	return ret;
}

static int status_error(uint32_t status)
{
	if ((status & (SDHC_NORMAL_INT_STAT_ERROR | SDHC_ERR_MASK)) == 0U) {
		return 0;
	}

	if ((status & SDHC_ERR_TIMEOUT) != 0U) {
		return -ETIMEDOUT;
	}

	return (status & SDHC_ERR_CRC) != 0U ? -EILSEQ : -EIO;
}

static k_timepoint_t timeout_ms_to_timepoint(int32_t timeout_ms)
{
	return sys_timepoint_calc(timeout_ms == SDHC_TIMEOUT_FOREVER ? K_FOREVER
								     : K_MSEC(timeout_ms));
}

static int wait_status(const struct device *dev, uint32_t wanted, k_timepoint_t deadline,
		       uint32_t *observed)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *data = dev->data;
	uint32_t status;
	int ret;

	if (config->irq_config_func != NULL) {
		uint32_t events = k_event_wait_safe(
			&data->irq_event, wanted | SDHC_NORMAL_INT_STAT_ERROR | SDHC_ERR_MASK,
			false, sys_timepoint_timeout(deadline));

		status = read_status(dev);

		if (observed != NULL) {
			*observed = status;
		}

		ret = status_error(status);

		return ret != 0 ? ret : (((events | status) & wanted) != 0U ? 0 : -ETIMEDOUT);
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

		if (sys_timepoint_expired(deadline)) {
			return -ETIMEDOUT;
		}

		k_usleep(100);
	}
}

static int wait_busy(const struct device *dev, k_timepoint_t deadline, uint32_t *observed)
{
	/*
	 * Wait for the controller event, without sampling DAT0. BCM2712 CMD12
	 * can report Transfer Complete alongside Command Complete.
	 */
	return wait_status(dev, SDHC_NORMAL_INT_STAT_TRANSFER_COMPLETE, deadline, observed);
}

static int wait_idle(const struct device *dev, uint32_t mask, k_timepoint_t deadline)
{
	do {
		k_timeout_t timeout = sys_timepoint_timeout(deadline);
		/* Limit each wait to avoid overflowing WAIT_FOR's 32-bit cycle count. */
		uint32_t timeout_us = USEC_PER_SEC;

		if (!K_TIMEOUT_EQ(timeout, K_FOREVER)) {
			timeout_us =
				(uint32_t)MIN(k_ticks_to_us_ceil64(timeout.ticks), USEC_PER_SEC);
		}

		if (WAIT_FOR((sdhc_standard_read32(dev, SDHC_REG_PRESENT_STATE) & mask) == 0U,
			     timeout_us, k_usleep(100))) {
			return 0;
		}
	} while (!sys_timepoint_expired(deadline));

	return -ETIMEDOUT;
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

static bool is_sd_memory_multiblock(const struct sdhc_command *cmd)
{
	return cmd->opcode == SD_READ_MULTIPLE_BLOCK || cmd->opcode == SD_WRITE_MULTIPLE_BLOCK;
}

static bool bcm2712_timing_requires_tuning(const struct bcm2712_data *host)
{
	return host->io.timing == SDHC_TIMING_SDR104 ||
	       (host->io.timing == SDHC_TIMING_SDR50 &&
		host->host.props.host_caps.sdr50_needs_tuning != 0U);
}

/* Called with the request mutex held; SDHCI consumes the tuning block internally. */
static int bcm2712_tune(const struct device *dev)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *host = dev->data;
	struct sdhc_command cmd = {
		.opcode = SD_SEND_TUNING_BLOCK,
		.response_type = SD_RSP_TYPE_R1,
	};
	k_timepoint_t deadline = sys_timepoint_calc(K_MSEC(SDHC_TUNING_TIMEOUT_MS));
	k_spinlock_key_t key;
	uint32_t status_enable;
	uint16_t control;
	int ret = -EIO;

	if (!bcm2712_timing_requires_tuning(host)) {
		return 0;
	}

	if (host->io.clock == 0U || host->io.power_mode != SDHC_POWER_ON) {
		return -EINVAL;
	}

	key = k_spin_lock(&host->irq_lock);
	status_enable = host->status_enable & ~SDHC_NORMAL_INT_STAT_CARD;
	k_spin_unlock(&host->irq_lock, key);

	stop_irq(dev);
	select_dma_mode(dev, false);
	control = sdhc_standard_read16(dev, SDHC_REG_HOST_CTRL2);
	control |= SDHC_HOST_CTRL2_EXEC_TUNING;
	sdhc_standard_write16(dev, SDHC_REG_HOST_CTRL2, control);

	for (uint32_t attempt = 0U; attempt < SDHC_TUNING_ATTEMPTS; attempt++) {
		ret = wait_idle(dev, SDHC_PRESENT_STATE_CMD_INHIBIT, deadline);

		if (ret != 0) {
			break;
		}

		begin_request(dev);
		/* Tuning emits Buffer Read Ready, without ordinary command/data completion. */
		key = k_spin_lock(&host->irq_lock);
		set_status_mask(dev, SDHC_NORMAL_INT_STAT_BUF_READ_READY);

		if (config->irq_config_func != NULL) {
			set_irq_mask(dev, SDHC_NORMAL_INT_STAT_BUF_READ_READY);
		}
		k_spin_unlock(&host->irq_lock, key);

		sdhc_standard_write16(dev, SDHC_REG_BLOCK_SIZE, 64U);
		sdhc_standard_write16(dev, SDHC_REG_BLOCK_COUNT, 0U);
		sdhc_standard_write32(dev, SDHC_REG_ARGUMENT1, 0U);
		sdhc_standard_write32(dev, SDHC_REG_TRANSFER_MODE,
				      ((uint32_t)command_flags(&cmd, true) << 16) |
					      SDHC_TRANSFER_MODE_READ);
		k_timepoint_t read_deadline = sys_timepoint_calc(K_MSEC(50));

		if (sys_timepoint_cmp(read_deadline, deadline) > 0) {
			read_deadline = deadline;
		}

		ret = wait_status(dev, SDHC_NORMAL_INT_STAT_BUF_READ_READY, read_deadline, NULL);
		stop_irq(dev);

		if (ret != 0) {
			break;
		}

		control = sdhc_standard_read16(dev, SDHC_REG_HOST_CTRL2);

		if ((control & SDHC_HOST_CTRL2_EXEC_TUNING) == 0U) {
			ret = (control & SDHC_HOST_CTRL2_TUNED) != 0U ? 0 : -EIO;
			break;
		}

		ret = -ETIMEDOUT;

		if (sys_timepoint_expired(deadline)) {
			break;
		}
	}

	stop_irq(dev);
	key = k_spin_lock(&host->irq_lock);
	set_status_mask(dev, status_enable);
	k_spin_unlock(&host->irq_lock, key);
	sdhc_standard_write32(dev, SDHC_REG_NORMAL_INT_STAT, SDHC_STATUS_MASK);
	select_dma_mode(dev, host->dma_enabled);
	host->tuned = ret == 0;
	atomic_clear(&host->retune_needed);
	host->retune_deadline = sys_timepoint_calc(K_FOREVER);

	if (ret == 0) {
		uint32_t count = host->host.props.host_caps.retune_timer_count;

		/* Mode 1 uses a timer; modes 2/3 use the controller's re-tuning request. */
		if (count != 0U && host->host.props.host_caps.retuning_mode == 0U) {
			host->retune_deadline = sys_timepoint_calc(K_SECONDS(1U << (count - 1U)));
		}
	} else {
		control = sdhc_standard_read16(dev, SDHC_REG_HOST_CTRL2);
		control &= ~(SDHC_HOST_CTRL2_EXEC_TUNING | SDHC_HOST_CTRL2_TUNED);
		sdhc_standard_write16(dev, SDHC_REG_HOST_CTRL2, control);
		int recovery = reset_lines(dev, (SDHC_SW_RESET_CMD | SDHC_SW_RESET_DATA));

		if (recovery != 0) {
			host->failed = true;
			ret = recovery;
		}
	}

	LOG_DBG("Tuning timing %u: result %d, control 0x%04x", host->io.timing, ret,
		sdhc_standard_read16(dev, SDHC_REG_HOST_CTRL2));

	return ret;
}

static int bcm2712_execute_tuning(const struct device *dev)
{
	struct bcm2712_data *host = dev->data;
	int ret;

	k_mutex_lock(&host->lock, K_FOREVER);
	ret = host->failed ? -EIO : bcm2712_tune(dev);
	k_mutex_unlock(&host->lock);

	return ret;
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
			value |= sdhc_standard_read32(dev, SDHC_REG_RESPONSE_0_1 + 4U * (i - 1U)) >>
				 24;
		}

		cmd->response[i] = value;
	}
}

static int wait_buffer_ready(const struct device *dev, bool read, k_timepoint_t deadline)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *data = dev->data;
	uint32_t ready =
		read ? SDHC_NORMAL_INT_STAT_BUF_READ_READY : SDHC_NORMAL_INT_STAT_BUF_WRITE_READY;
	uint32_t enabled = read ? SDHC_PRESENT_STATE_READ_ENABLE : SDHC_PRESENT_STATE_WRITE_ENABLE;
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
		/* Drop the shadow bit with the event; the FIFO may not be ready yet. */
		data->irq_status &= ~ready;

		if (ret != 0 ||
		    (sdhc_standard_read32(dev, SDHC_REG_PRESENT_STATE) & enabled) != 0U) {
			k_spin_unlock(&data->irq_lock, key);

			return ret;
		}

		if (sys_timepoint_expired(deadline)) {
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
	k_timepoint_t deadline = timeout_ms_to_timepoint(data->timeout_ms);
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
					buf[i] = (uint8_t)(word >> (8U * i));
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

	ret = wait_status(dev, SDHC_NORMAL_INT_STAT_TRANSFER_COMPLETE, deadline, NULL);

	if (ret != 0) {
		return ret;
	}

	return wait_idle(dev, SDHC_PRESENT_STATE_DAT_INHIBIT, deadline);
}

static int request_once(const struct device *dev, struct sdhc_command *cmd, struct sdhc_data *data,
			uint32_t flags, bool *submitted)
{
#ifdef CONFIG_SDHC_BCM2712_DMA
	const struct bcm2712_config *config = dev->config;
#endif
	struct bcm2712_data *host = dev->data;
	k_timepoint_t deadline = timeout_ms_to_timepoint(cmd->timeout_ms);
	bool busy = (flags & 3U) == 3U;
	bool dma = false;
	bool read = cmd->opcode == SDIO_RW_EXTENDED
			    ? (cmd->arg & BIT(SDIO_CMD_ARG_RW_SHIFT)) == 0U
			    : cmd->opcode != SD_WRITE_SINGLE_BLOCK &&
				      cmd->opcode != SD_WRITE_MULTIPLE_BLOCK;
	uint32_t transfer = 0U;
	uint32_t inhibit = SDHC_PRESENT_STATE_CMD_INHIBIT;
	uint32_t command_status;
	uint32_t busy_status;
	int ret;

	*submitted = false;

	if ((data != NULL || busy) && cmd->opcode != SD_STOP_TRANSMISSION) {
		inhibit |= SDHC_PRESENT_STATE_DAT_INHIBIT;
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
			if (!dma) {
				LOG_DBG("CMD%u uses PIO: ADMA2 unavailable (%u blocks of %u)",
					cmd->opcode, data->blocks, data->block_size);
			}
		}

#endif

		transfer = SDHC_TRANSFER_MODE_COUNT | (read ? SDHC_TRANSFER_MODE_READ : 0U) |
			   (dma ? SDHC_TRANSFER_MODE_DMA_EN : 0U);

		select_dma_mode(dev, dma);

		if (is_sd_memory_multiblock(cmd)) {
			transfer |= SDHC_TRANSFER_MODE_MULTI;

			/* SDR104 uses predefined-count transfers; the card must support CMD23. */
			if (host->io.timing == SDHC_TIMING_SDR104) {
				sdhc_standard_write32(dev, SDHC_REG_SDMA_SYS_ADDR_ARG2,
						      data->blocks);
				transfer |= SDHC_TRANSFER_MODE_AUTO_CMD23;
			} else {
				transfer |= SDHC_TRANSFER_MODE_AUTO_CMD12;
			}
		} else if (cmd->opcode == SDIO_RW_EXTENDED && data->blocks > 1U) {
			transfer |= SDHC_TRANSFER_MODE_MULTI;
		} else {
			/* Single-block requests need no multi-block transfer flags. */
		}

		sdhc_standard_write16(dev, SDHC_REG_BLOCK_SIZE, data->block_size);
		sdhc_standard_write16(dev, SDHC_REG_BLOCK_COUNT, data->blocks);
	}

	begin_request(dev);
	sdhc_standard_write32(dev, SDHC_REG_ARGUMENT1, cmd->arg);
	/* A single write submits the command with its transfer mode. */
	sdhc_standard_write32(dev, SDHC_REG_TRANSFER_MODE, (flags << 16) | transfer);
	*submitted = true;
	ret = wait_status(dev, SDHC_NORMAL_INT_STAT_CMD_COMPLETE, deadline, &command_status);

	if (ret != 0) {
		return ret;
	}

	read_response(dev, cmd);

	if (data != NULL) {
#ifdef CONFIG_SDHC_BCM2712_DMA
		if (dma) {
			k_timepoint_t data_deadline = timeout_ms_to_timepoint(data->timeout_ms);

			ret = wait_status(dev, SDHC_NORMAL_INT_STAT_TRANSFER_COMPLETE,
					  data_deadline, NULL);

			if (ret == 0) {
				ret = wait_idle(dev, SDHC_PRESENT_STATE_DAT_INHIBIT, data_deadline);
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
	bool app_cmd;
	int recovery;
	int ret;

	if (flags < 0 || cmd->timeout_ms < SDHC_TIMEOUT_FOREVER) {
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
		    data->block_size == 0U || data->block_size > 512U ||
		    data->timeout_ms < SDHC_TIMEOUT_FOREVER) {
			return -EINVAL;
		}
	}

	k_mutex_lock(&host->lock, K_FOREVER);

	if (host->failed) {
		k_mutex_unlock(&host->lock);

		return -EIO;
	}

	app_cmd = host->app_cmd && cmd->opcode != SD_APP_CMD;

	if (data != NULL && bcm2712_timing_requires_tuning(host) &&
	    host->host.props.host_caps.retuning_mode < 2U &&
	    (uint64_t)data->blocks * data->block_size > 4U * 1024U * 1024U) {
		k_mutex_unlock(&host->lock);

		return -EINVAL;
	}

	/* CMD19 must not split CMD55 from its following application command. */
	if (!host->app_cmd && (data != NULL || cmd->opcode == SD_APP_CMD) &&
	    bcm2712_timing_requires_tuning(host)) {
		uint32_t status = sdhc_standard_read32(dev, SDHC_REG_NORMAL_INT_STAT);

		if (!host->tuned || atomic_get(&host->retune_needed) != 0 ||
		    (status & SDHC_NORMAL_INT_STAT_RETUNING_EVENT) != 0U ||
		    sys_timepoint_expired(host->retune_deadline)) {
			ret = bcm2712_tune(dev);

			if (ret != 0) {
				k_mutex_unlock(&host->lock);

				return ret;
			}
		}
	}

	do {
		bool submitted;

		ret = request_once(dev, cmd, data, flags, &submitted);
		stop_irq(dev);

		if (ret == 0) {
			break;
		}

		if (host->io.timing == SDHC_TIMING_SDR104 &&
		    (read_status(dev) & SDHC_ERR_AUTO_CMD) != 0U) {
			LOG_ERR("Auto CMD23 failed: 0x%04x",
				sdhc_standard_read16(dev, SDHC_REG_AUTO_CMD_ERR_STAT));
			/* Reinitialize the card before another predefined-count transfer. */
			host->failed = true;
		}

		LOG_DBG("CMD%u failed: %d, status 0x%08x, ADMA error 0x%08x", cmd->opcode, ret,
			read_status(dev), host->adma_error);
		recovery = reset_lines(dev, (SDHC_SW_RESET_CMD | SDHC_SW_RESET_DATA));
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

		if (submitted && data != NULL && is_sd_memory_multiblock(cmd)) {
			struct sdhc_command stop = {
				.opcode = SD_STOP_TRANSMISSION,
				.response_type = SD_RSP_TYPE_R1b,
				.timeout_ms = SDHC_RECOVERY_TIMEOUT_MS,
			};

			/* Resetting the host does not terminate the card's transfer. */
			recovery = command_flags(&stop, false);
			if (recovery >= 0) {
				recovery = request_once(dev, &stop, NULL, recovery, &submitted);
				stop_irq(dev);
			}

			/*
			 * CMD12 can stop the transfer and still report errors left over
			 * from the aborted data phase. Confirm the card state with CMD13
			 * instead of latching the host on the R1 error flags alone.
			 */
			if (recovery == 0 && (stop.response[0] & SD_R1_ERR_FLAGS) != 0U) {
				LOG_DBG("CMD%u stop response 0x%08x has error flags", cmd->opcode,
					stop.response[0]);
				recovery = -EIO;
			}

			if (recovery != 0 && host->selected_rca != 0U) {
				struct sdhc_command status = {
					.opcode = SD_SEND_STATUS,
					.arg = (uint32_t)host->selected_rca << 16U,
					.response_type = SD_RSP_TYPE_R1,
					.timeout_ms = SDHC_RECOVERY_TIMEOUT_MS,
				};
				int status_flags;

				/* Auto stop may already have returned the card to TRAN. */
				recovery =
					reset_lines(dev, (SDHC_SW_RESET_CMD | SDHC_SW_RESET_DATA));
				if (recovery == 0) {
					status_flags = command_flags(&status, false);
					if (status_flags >= 0) {
						recovery = request_once(dev, &status, NULL,
									status_flags, &submitted);
						stop_irq(dev);
					} else {
						recovery = status_flags;
					}
				}

				if (recovery == 0 &&
				    SD_R1_CURRENT_STATE(status.response[0]) != SDMMC_R1_TRANSFER) {
					recovery = -EIO;
				}
			}

			if (recovery != 0) {
				LOG_ERR("Failed to stop CMD%u: %d, response 0x%08x", cmd->opcode,
					recovery, stop.response[0]);
				host->failed = true;
				ret = recovery;
			}
		}

		/* The SD stack must resend CMD55 before retrying an application command. */
		if (data != NULL || app_cmd || retries == 0U) {
			break;
		}

		retries--;
	} while (true);

	if (ret == 0) {
		if (cmd->opcode == SD_GO_IDLE_STATE) {
			host->selected_rca = 0U;
		} else if (cmd->opcode == SD_SELECT_CARD &&
			   (cmd->response[0] & SD_R1_ERR_FLAGS) == 0U) {
			host->selected_rca = (uint16_t)(cmd->arg >> 16U);
		} else {
			/* Other commands preserve the selected card address. */
		}
	}

	host->app_cmd = (ret == 0) && (cmd->opcode == SD_APP_CMD);
	k_mutex_unlock(&host->lock);

	return ret;
}

static int set_clock(const struct device *dev, uint32_t rate)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *host = dev->data;
	uint16_t control;
	uint32_t divisor = 0U;

	if (rate != 0U) {
		if (rate < config->host.common.min_bus_freq ||
		    rate > config->host.common.max_bus_freq || rate > host->host.props.f_max) {
			return -EINVAL;
		}

		divisor = DIV_ROUND_UP(config->clock_rate, rate);
		/* SDHCI v3 divides by one, or an even divisor from 2 to 2046. */
		divisor = divisor <= 1U ? 0U : DIV_ROUND_UP(divisor, 2U);

		if (divisor > 1023U) {
			return -EINVAL;
		}
	}

	control = sdhc_standard_read16(dev, SDHC_REG_CLOCK_CTRL);
	control &= ~(SDHC_CLOCK_CONTROL_CARD | SDHC_CLOCK_CONTROL_INTERNAL |
		     SDHC_CLOCK_CONTROL_DIV_MASK);
	sdhc_standard_write16(dev, SDHC_REG_CLOCK_CTRL, control);
	sdhc_standard_write8(dev, SDHC_REG_TIMEOUT_CTRL, SDHC_TIMEOUT_CTRL_MAX);

	if (rate == 0U) {
		return 0;
	}

	control |= ((divisor & 0xffU) << 8) | ((divisor & 0x300U) >> 2);
	control |= SDHC_CLOCK_CONTROL_INTERNAL;
	sdhc_standard_write16(dev, SDHC_REG_CLOCK_CTRL, control);
	if (!WAIT_FOR((sdhc_standard_read16(dev, SDHC_REG_CLOCK_CTRL) &
		       SDHC_CLOCK_CONTROL_STABLE) != 0U,
		      100000U, k_usleep(100))) {
		host->failed = true;
		return -ETIMEDOUT;
	}

	sdhc_standard_write16(dev, SDHC_REG_CLOCK_CTRL, control | SDHC_CLOCK_CONTROL_CARD);

	return 0;
}

static int bcm2712_set_io(const struct device *dev, struct sdhc_io *io)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *host = dev->data;
	struct sdhc_io previous_io;
	uint8_t control;
	bool power_on;
	int ret;

	if ((io->bus_width != SDHC_BUS_WIDTH1BIT && io->bus_width != SDHC_BUS_WIDTH4BIT) ||
	    (io->bus_width == SDHC_BUS_WIDTH4BIT && config->host.common.bus_width < 4U) ||
	    io->bus_mode != SDHC_BUSMODE_PUSHPULL || io->enhanced_strobe) {
		return -ENOTSUP;
	}

	k_mutex_lock(&host->lock, K_FOREVER);

	if (host->failed) {
		ret = -EIO;
		goto out;
	}

	previous_io = host->io;
	power_on = io->power_mode == SDHC_POWER_ON && !host->powered;

	ret = set_clock(dev, 0U);
	if (ret != 0) {
		goto out;
	}
	host->io.clock = 0U;

	ret = bcm2712_power(dev, io->power_mode == SDHC_POWER_ON);
	if (ret != 0) {
		goto out;
	}

	if (io->power_mode == SDHC_POWER_ON &&
	    (power_on || io->signal_voltage != host->io.signal_voltage)) {
		ret = set_io_voltage(dev, io->signal_voltage);
		if (ret != 0) {
			goto out;
		}
	}

	control = sdhc_standard_read8(dev, SDHC_REG_HOST_CTRL1);
	control &= ~(SDHC_HOST_CTRL1_WIDTH4 | SDHC_HOST_CTRL1_WIDTH8 | SDHC_HOST_CTRL1_DMA_MASK);

	if (io->bus_width == SDHC_BUS_WIDTH4BIT) {
		control |= SDHC_HOST_CTRL1_WIDTH4;
	}

	sdhc_standard_write8(dev, SDHC_REG_HOST_CTRL1, control);
	sdhc_standard_write8(dev, SDHC_REG_POWER_CTRL,
			     io->power_mode == SDHC_POWER_ON
				     ? SDHC_POWER_CTRL_BUS_POWER | SDHC_POWER_CTRL_VOLTAGE_330
				     : 0U);
	host->io.bus_width = io->bus_width;
	host->io.bus_mode = io->bus_mode;
	host->io.power_mode = io->power_mode;
	host->io.enhanced_strobe = false;

	select_dma_mode(dev, host->dma_enabled);

	ret = bcm2712_set_timing(dev, io, &previous_io);
	if (ret != 0) {
		goto out;
	}
	host->io.timing = io->timing;
	ret = set_clock(dev, io->power_mode == SDHC_POWER_ON ? io->clock : 0U);
	if (ret != 0) {
		goto out;
	}
	host->io.clock = io->power_mode == SDHC_POWER_ON ? io->clock : 0U;

	LOG_DBG("I/O: clock %u, width %u, voltage %s, timing %u, control2 0x%04x", host->io.clock,
		host->io.bus_width, sd_voltage_str(host->io.signal_voltage), host->io.timing,
		sdhc_standard_read16(dev, SDHC_REG_HOST_CTRL2));

out:
	k_mutex_unlock(&host->lock);

	return ret;
}

static int host_card_busy(const struct device *dev)
{
	return (sdhc_standard_read32(dev, SDHC_REG_PRESENT_STATE) & SDHC_PRESENT_STATE_DAT0) == 0U;
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
	bool sdr50;
	bool sdr104;
	int ret;

	if (config->cfg_size < CFG_CQ_CAPABILITY + sizeof(uint32_t) || config->clock_rate == 0U ||
	    config->host.common.min_bus_freq < 400000U ||
	    config->host.common.max_bus_freq > SD_CLOCK_208MHZ ||
	    config->host.common.min_bus_freq > config->host.common.max_bus_freq ||
	    (config->host.common.bus_width != 1U && config->host.common.bus_width != 4U)) {
		return -EINVAL;
	}

	/*
	 * sdhc_standard_host_init() forces K_MEM_DIRECT_MAP. Map here so device_map()
	 * selects direct mapping only when configured, then reuse the property helpers.
	 */
	DEVICE_MMIO_NAMED_MAP(dev, sdhc_base, K_MEM_CACHE_NONE);
	sdhc_common_dt_props_init(props, &config->host.common);
	sdhc_standard_capabilities_init(dev);
	k_mutex_init(&data->lock);
	k_event_init(&data->irq_event);
	sdr50 = config->sdr50 && props->host_caps.sdr50_support != 0U;
	sdr104 = config->sdr104 && props->host_caps.sdr104_support != 0U;
	data->uhs_enabled = config->vqmmc != NULL && (sdr50 || sdr104) && !config->no_1_8_v &&
			    config->host.common.bus_width == 4U &&
			    props->host_caps.vol_180_support != 0U &&
			    props->host_caps.retuning_mode != 3U &&
			    regulator_is_supported_voltage(config->vqmmc, 1800000, 1800000) &&
			    regulator_is_supported_voltage(config->vqmmc, 3300000, 3300000);

	if (data->uhs_enabled && config->cfg_size < CFG_MAX_50MHZ_MODE + sizeof(uint32_t)) {
		return -EINVAL;
	}

	props->f_max = MIN(props->f_max, config->clock_rate);
	props->f_max =
		MIN(props->f_max, data->uhs_enabled ? (sdr104 ? SD_CLOCK_208MHZ : SD_CLOCK_100MHZ)
						    : SD_CLOCK_50MHZ);
	data->dma_enabled =
		IS_ENABLED(CONFIG_SDHC_BCM2712_DMA) && config->adma2 != NULL &&
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
	props->host_caps.vol_180_support = data->uhs_enabled ? 1U : 0U;
	props->host_caps.address_64_bit_support_v4 = 0U;
	props->host_caps.address_64_bit_support_v3 =
		data->dma_enabled && config->adma2->dma64 ? 1U : 0U;
	props->host_caps.sdio_async_interrupt_support = 0U;
	props->host_caps.sdr50_support = data->uhs_enabled && sdr50 ? 1U : 0U;
	props->host_caps.sdr104_support = data->uhs_enabled && sdr104 ? 1U : 0U;
	props->host_caps.ddr50_support = 0U;
	props->host_caps.uhs_2_support = 0U;
	props->host_caps.drv_type_a_support = 0U;
	props->host_caps.drv_type_c_support = 0U;
	props->host_caps.drv_type_d_support = 0U;
	if (!data->uhs_enabled) {
		props->host_caps.retune_timer_count = 0U;
		props->host_caps.sdr50_needs_tuning = 0U;
		props->host_caps.retuning_mode = 0U;
	}
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

static int bcm2712_enable_interrupt(const struct device *dev, sdhc_interrupt_cb_t callback,
				    int sources, void *user_data)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *data = dev->data;
	k_spinlock_key_t key;
	bool enable = (sources & SDHC_INT_SDIO) != 0;

	if ((sources & ~SDHC_INT_SDIO) != 0) {
		return -EINVAL;
	}

	if (enable && callback == NULL) {
		return -EINVAL;
	}

	if (config->irq_config_func == NULL) {
		return -ENOSYS;
	}

	key = k_spin_lock(&data->irq_lock);
	data->card_callback = enable ? callback : NULL;
	data->card_user_data = enable ? user_data : NULL;
	data->card_irq_enabled = enable;
	data->card_irq_pending = false;
	set_irq_mask(dev, data->irq_mask);
	k_spin_unlock(&data->irq_lock, key);

	return 0;
}

static int bcm2712_disable_interrupt(const struct device *dev, int sources)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *data = dev->data;
	k_spinlock_key_t key;

	if ((sources & ~SDHC_INT_SDIO) != 0) {
		return -EINVAL;
	}

	if ((sources & SDHC_INT_SDIO) == 0) {
		return 0;
	}

	if (config->irq_config_func == NULL) {
		return -ENOSYS;
	}

	key = k_spin_lock(&data->irq_lock);
	data->card_irq_enabled = false;
	data->card_irq_pending = false;
	set_irq_mask(dev, data->irq_mask);
	data->card_callback = NULL;
	data->card_user_data = NULL;
	k_spin_unlock(&data->irq_lock, key);

	return 0;
}

static const DEVICE_API(sdhc, bcm2712_api) = {
	.reset = bcm2712_reset,
	.enable_interrupt = bcm2712_enable_interrupt,
	.disable_interrupt = bcm2712_disable_interrupt,
	.request = bcm2712_request,
	.set_io = bcm2712_set_io,
	.get_card_present = bcm2712_card_present,
	.card_busy = host_card_busy,
	.get_host_props = host_props,
	.execute_tuning = bcm2712_execute_tuning,
};

static int bcm2712_init(const struct device *dev)
{
	const struct bcm2712_config *config = dev->config;
	struct bcm2712_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->clock) || !device_is_ready(config->vmmc) ||
	    (config->vqmmc != NULL && !device_is_ready(config->vqmmc)) ||
	    (!config->non_removable && !gpio_is_ready_dt(&config->cd))) {
		return -ENODEV;
	}

	/* Apply explicit pin states for both removable and non-removable hosts. */
	if (config->pinctrl != NULL) {
		ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);

		if (ret != 0) {
			return ret;
		}
	}

	if (!config->non_removable) {
		ret = gpio_pin_configure_dt(&config->cd, GPIO_INPUT);

		if (ret != 0) {
			return ret;
		}
	}

	device_map(&data->cfg_base, config->cfg_addr, config->cfg_size, K_MEM_CACHE_NONE);

	return bcm2712_setup(dev);
}

#define BCM2712_IRQ_CONFIG(n)                                                                      \
	static void bcm2712_irq_config_##n(const struct device *dev)                               \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), bcm2712_isr,                \
			    DEVICE_DT_INST_GET(n), DT_INST_IRQ(n, flags));                         \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

/* clang-format off */
#define BCM2712_DEFINE(n)                                                                      \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0), (PINCTRL_DT_INST_DEFINE(n);), ())     \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, interrupts), (BCM2712_IRQ_CONFIG(n)), ())         \
	static struct bcm2712_data bcm2712_data_##n;                                           \
	static const struct bcm2712_config bcm2712_config_##n = {                              \
		.host = SDHC_STANDARD_COMMON_CONFIG_DT_INST_INIT(n, NULL),                     \
		.irq_config_func = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, interrupts),           \
					     (bcm2712_irq_config_##n), (NULL)),                \
		.sdr50 = DT_INST_PROP(n, sd_uhs_sdr50),                                        \
		.sdr104 = DT_INST_PROP(n, sd_uhs_sdr104),                                      \
		.no_1_8_v = DT_INST_PROP(n, no_1_8_v),                                         \
		.non_removable = DT_INST_PROP(n, non_removable),                               \
		.pinctrl = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0),                    \
				      (PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL)),            \
		.clock_rate = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),             \
		.adma2 = IS_ENABLED(CONFIG_SDHC_BCM2712_DMA) ? &bcm2712_adma2 : NULL,          \
		.cfg_addr = DT_INST_REG_ADDR_BY_NAME(n, cfg),                                  \
		.cfg_size = DT_INST_REG_SIZE_BY_NAME(n, cfg),                                  \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.vmmc = DEVICE_DT_GET(DT_INST_PHANDLE(n, vmmc_supply)),                        \
		.vqmmc = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(n, vqmmc_supply)),              \
		.cd = GPIO_DT_SPEC_INST_GET_OR(n, cd_gpios, {0}),                              \
	};                                                                                     \
	DEVICE_DT_INST_DEFINE(n, bcm2712_init, NULL, &bcm2712_data_##n, &bcm2712_config_##n,   \
			      POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, &bcm2712_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(BCM2712_DEFINE)
