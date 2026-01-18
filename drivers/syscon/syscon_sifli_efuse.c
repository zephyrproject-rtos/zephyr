/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_efuse

#include <zephyr/drivers/syscon.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(efuse_sifli, CONFIG_SYSCON_LOG_LEVEL);

/* PMUC register offset for HPSYS_VOUT (48th register * 4 bytes) */
#define PMUC_HPSYS_VOUT_OFFSET 0xC0

/* eFuse controller register offsets from EFUSEC_TypeDef */
#define EFUSEC_CR_OFFSET          offsetof(EFUSEC_TypeDef, CR)
#define EFUSEC_SR_OFFSET          offsetof(EFUSEC_TypeDef, SR)
#define EFUSEC_BANK0_DATA0_OFFSET offsetof(EFUSEC_TypeDef, BANK0_DATA0)

/* eFuse constants */
#define EFUSE_BANK_SIZE     32 /* bytes per bank */
#define EFUSE_BANK_NUM      4
#define EFUSE_TOTAL_SIZE    (EFUSE_BANK_SIZE * EFUSE_BANK_NUM) /* 128 bytes */
#define EFUSE_REGS_PER_BANK 8

/* Read timeout */
#define EFUSE_READ_TIMEOUT_US 10000

struct efuse_sifli_data {
	uint8_t cache[EFUSE_TOTAL_SIZE];
	bool cached;
};

struct efuse_sifli_config {
	uintptr_t base_addr;
	uintptr_t pmuc_addr;
	size_t size;
};

static void efuse_sifli_delay_us(uint32_t us)
{
	k_busy_wait(us);
}

static int efuse_sifli_read_bank(const struct device *dev, uint8_t bank)
{
	const struct efuse_sifli_config *config = dev->config;
	struct efuse_sifli_data *data = dev->data;
	uint32_t timeout = 0;
	uint32_t val;
	uint8_t *cache_ptr;

	if (bank >= EFUSE_BANK_NUM) {
		return -EINVAL;
	}

	/* Set bank select and READ mode (MODE=0), then enable */
	sys_write32((bank << EFUSEC_CR_BANKSEL_Pos), config->base_addr + EFUSEC_CR_OFFSET);

	/* Start read */
	val = sys_read32(config->base_addr + EFUSEC_CR_OFFSET);
	sys_write32(val | EFUSEC_CR_EN, config->base_addr + EFUSEC_CR_OFFSET);

	/* Wait for completion */
	while (!(sys_read32(config->base_addr + EFUSEC_SR_OFFSET) & EFUSEC_SR_DONE)) {
		efuse_sifli_delay_us(1);
		timeout++;
		if (timeout > EFUSE_READ_TIMEOUT_US) {
			LOG_ERR("eFuse read timeout for bank %d", bank);
			return -ETIMEDOUT;
		}
	}

	/* Clear done flag */
	sys_write32(EFUSEC_SR_DONE, config->base_addr + EFUSEC_SR_OFFSET);

	/* Read data from bank registers */
	cache_ptr = &data->cache[bank * EFUSE_BANK_SIZE];

	for (int i = 0; i < EFUSE_REGS_PER_BANK; i++) {
		uint32_t reg_offset =
			EFUSEC_BANK0_DATA0_OFFSET + (bank * EFUSE_REGS_PER_BANK * 4) + (i * 4);
		val = sys_read32(config->base_addr + reg_offset);

		cache_ptr[i * 4 + 0] = val & 0xFF;
		cache_ptr[i * 4 + 1] = (val >> 8) & 0xFF;
		cache_ptr[i * 4 + 2] = (val >> 16) & 0xFF;
		cache_ptr[i * 4 + 3] = (val >> 24) & 0xFF;
	}

	return 0;
}

static int efuse_sifli_cache_all(const struct device *dev)
{
	const struct efuse_sifli_config *config = dev->config;
	struct efuse_sifli_data *data = dev->data;
	uint32_t org_vout;
	uint32_t new_vout;
	int ret = 0;
	unsigned int key;

	if (data->cached) {
		return 0;
	}

	key = irq_lock();

	/* Save original HPSYS_VOUT and adjust voltage for eFuse read */
	org_vout = sys_read32(config->pmuc_addr + PMUC_HPSYS_VOUT_OFFSET) & 0xF;
	new_vout = org_vout + 3;
	if (new_vout > 0xF) {
		new_vout = 0xF;
	}
	if (new_vout < 0xE) {
		new_vout = 0xE;
	}
	sys_write32(new_vout, config->pmuc_addr + PMUC_HPSYS_VOUT_OFFSET);

	/* Delay for voltage stabilization */
	efuse_sifli_delay_us(20);

	/* Read all banks */
	for (int bank = 0; bank < EFUSE_BANK_NUM; bank++) {
		ret = efuse_sifli_read_bank(dev, bank);
		if (ret != 0) {
			LOG_ERR("Failed to read eFuse bank %d: %d", bank, ret);
			break;
		}
	}

	/* Restore original voltage */
	sys_write32(org_vout, config->pmuc_addr + PMUC_HPSYS_VOUT_OFFSET);

	if (ret == 0) {
		data->cached = true;
		LOG_DBG("eFuse cached successfully (%d bytes)", EFUSE_TOTAL_SIZE);
	}

	irq_unlock(key);

	return ret;
}

static int efuse_sifli_read(const struct device *dev, uint16_t reg, uint32_t *val)
{
	struct efuse_sifli_data *data = dev->data;
	const struct efuse_sifli_config *config = dev->config;

	if (!val) {
		return -EINVAL;
	}

	/* Align to 4-byte boundary */
	reg = ROUND_DOWN(reg, 4);

	if (reg >= config->size) {
		return -EINVAL;
	}

	if (!data->cached) {
		int ret = efuse_sifli_cache_all(dev);

		if (ret != 0) {
			return ret;
		}
	}

	*val = *((uint32_t *)&data->cache[reg]);

	return 0;
}

static int efuse_sifli_get_size(const struct device *dev, size_t *size)
{
	const struct efuse_sifli_config *config = dev->config;

	*size = config->size;
	return 0;
}

static int efuse_sifli_get_base(const struct device *dev, uintptr_t *addr)
{
	struct efuse_sifli_data *data = dev->data;

	if (!data->cached) {
		int ret = efuse_sifli_cache_all(dev);

		if (ret != 0) {
			return ret;
		}
	}

	*addr = (uintptr_t)data->cache;
	return 0;
}

static int efuse_sifli_init(const struct device *dev)
{
	int ret;

	LOG_DBG("Initializing SiFli eFuse driver");

	/* Cache all eFuse content at init */
	ret = efuse_sifli_cache_all(dev);
	if (ret != 0) {
		LOG_ERR("Failed to cache eFuse: %d", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(syscon, efuse_sifli_api) = {
	.read = efuse_sifli_read,
	.get_size = efuse_sifli_get_size,
	.get_base = efuse_sifli_get_base,
};

#define EFUSE_SIFLI_INIT(n)                                                                        \
	static struct efuse_sifli_data efuse_sifli_data_##n = {                                    \
		.cached = false,                                                                   \
	};                                                                                         \
                                                                                                   \
	static const struct efuse_sifli_config efuse_sifli_config_##n = {                          \
		.base_addr = DT_INST_REG_ADDR(n),                                                  \
		.pmuc_addr = DT_REG_ADDR(DT_INST_PHANDLE(n, sifli_pmuc)),                          \
		.size = EFUSE_TOTAL_SIZE,                                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, efuse_sifli_init, NULL, &efuse_sifli_data_##n,                    \
			      &efuse_sifli_config_##n, POST_KERNEL, CONFIG_SYSCON_INIT_PRIORITY,   \
			      &efuse_sifli_api);

DT_INST_FOREACH_STATUS_OKAY(EFUSE_SIFLI_INIT)
