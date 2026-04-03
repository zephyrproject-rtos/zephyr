/*
 * Copyright (c) 2026 SiFli Technologies(Nanjing) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_efuse

#include <stddef.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(otp_sifli_efuse, CONFIG_OTP_LOG_LEVEL);

/* eFuse controller register offsets (derived from CMSIS structure) */
#define EFUSEC_REG_CR offsetof(EFUSEC_TypeDef, CR)
#define EFUSEC_REG_SR offsetof(EFUSEC_TypeDef, SR)

/* PMUC register offsets (derived from CMSIS structure) */
#define PMUC_REG_HPSYS_VOUT offsetof(PMUC_TypeDef, HPSYS_VOUT)

/* Timeout for eFuse read operation */
#define EFUSE_READ_TIMEOUT_US 10000

struct otp_sifli_efuse_config {
	uintptr_t base;
	uintptr_t pmuc_base;
	uint8_t *cache;
	const uint32_t *bank_offsets;
	size_t bank_size;
	size_t bank_num;
};

struct otp_sifli_efuse_data {
	bool cached;
	struct k_mutex lock;
};

static inline uint32_t efuse_read_reg(const struct otp_sifli_efuse_config *config, uint32_t offset)
{
	return sys_read32(config->base + offset);
}

static inline void efuse_write_reg(const struct otp_sifli_efuse_config *config, uint32_t offset,
				   uint32_t value)
{
	sys_write32(value, config->base + offset);
}

static inline uint32_t pmuc_read_reg(const struct otp_sifli_efuse_config *config, uint32_t offset)
{
	return sys_read32(config->pmuc_base + offset);
}

static inline void pmuc_write_reg(const struct otp_sifli_efuse_config *config, uint32_t offset,
				  uint32_t value)
{
	sys_write32(value, config->pmuc_base + offset);
}

/**
 * @brief Read a single bank from eFuse hardware
 *
 * @param config Device configuration
 * @param bank Bank number
 * @param data Output buffer (must be at least bank_size bytes)
 * @return 0 on success, negative errno on failure
 */
static int efuse_read_bank(const struct otp_sifli_efuse_config *config, uint8_t bank, uint8_t *data)
{
	uint32_t org_vout;
	uint32_t new_vout;
	uint32_t timeout = 0;
	uint32_t val;
	uint32_t bank_data_offset;

	if (bank >= config->bank_num) {
		return -EINVAL;
	}

	/* Adjust HPSYS LDO voltage before reading */
	org_vout = pmuc_read_reg(config, PMUC_REG_HPSYS_VOUT);
	new_vout = clamp(org_vout + 3, 0xe, 0xf);
	pmuc_write_reg(config, PMUC_REG_HPSYS_VOUT, new_vout);
	k_busy_wait(20);

	/* Select bank and set READ mode (MODE=0) */
	efuse_write_reg(config, EFUSEC_REG_CR, (bank << EFUSEC_CR_BANKSEL_Pos));

	/* Start read operation */
	efuse_write_reg(config, EFUSEC_REG_CR, (bank << EFUSEC_CR_BANKSEL_Pos) | EFUSEC_CR_EN);

	/* Wait for read completion */
	while ((efuse_read_reg(config, EFUSEC_REG_SR) & EFUSEC_SR_DONE) == 0) {
		k_busy_wait(1);
		timeout++;
		if (timeout > EFUSE_READ_TIMEOUT_US) {
			LOG_ERR("eFuse read timeout for bank %u", bank);
			pmuc_write_reg(config, PMUC_REG_HPSYS_VOUT, org_vout);
			return -ETIMEDOUT;
		}
	}

	/* Clear done flag */
	efuse_write_reg(config, EFUSEC_REG_SR, EFUSEC_SR_DONE);

	/* Get bank data register offset from config */
	bank_data_offset = config->bank_offsets[bank];

	/* Read bank data */
	for (size_t i = 0; i < config->bank_size / sizeof(uint32_t); i++) {
		val = efuse_read_reg(config, bank_data_offset + i * sizeof(uint32_t));
		sys_put_le32(val, &data[i * sizeof(uint32_t)]);
	}

	/* Restore original LDO voltage */
	pmuc_write_reg(config, PMUC_REG_HPSYS_VOUT, org_vout);

	return 0;
}

/**
 * @brief Load all eFuse banks into cache
 */
static int efuse_load_cache(const struct device *dev)
{
	const struct otp_sifli_efuse_config *config = dev->config;
	struct otp_sifli_efuse_data *data = dev->data;
	int ret;

	for (size_t bank = 0; bank < config->bank_num; bank++) {
		ret = efuse_read_bank(config, bank, &config->cache[bank * config->bank_size]);
		if (ret < 0) {
			LOG_ERR("Failed to read eFuse bank %zu: %d", bank, ret);
			return ret;
		}
	}

	data->cached = true;
	LOG_DBG("eFuse cache loaded successfully");

	return 0;
}

static int otp_sifli_efuse_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct otp_sifli_efuse_config *config = dev->config;
	struct otp_sifli_efuse_data *data = dev->data;
	size_t total_size = config->bank_size * config->bank_num;
	int ret = 0;

	if (offset < 0 || (offset + len) > total_size) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Load cache if not already cached */
	if (!data->cached) {
		ret = efuse_load_cache(dev);
		if (ret < 0) {
			k_mutex_unlock(&data->lock);
			return ret;
		}
	}

	/* Copy from cache to output buffer */
	memcpy(buf, &config->cache[offset], len);

	k_mutex_unlock(&data->lock);

	return 0;
}

static int otp_sifli_efuse_init(const struct device *dev)
{
	struct otp_sifli_efuse_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);
	data->cached = false;

	/* Pre-load cache at initialization for better read performance */
	ret = efuse_load_cache(dev);
	if (ret < 0) {
		LOG_WRN("Failed to pre-load eFuse cache: %d", ret);
		/* Non-fatal: cache will be loaded on first read */
	}

	LOG_INF("SiFli eFuse OTP driver initialized");

	return 0;
}

static DEVICE_API(otp, otp_sifli_efuse_api) = {
	.read = otp_sifli_efuse_read,
};

#define OTP_SIFLI_EFUSE_BANK_SIZE(n) DT_INST_PROP(n, sifli_bank_size)
#define OTP_SIFLI_EFUSE_BANK_NUM(n)  DT_INST_PROP_LEN(n, sifli_bank_offsets)

#define OTP_SIFLI_EFUSE_INIT(n)                                                                    \
	static uint8_t otp_sifli_efuse_cache_##n[OTP_SIFLI_EFUSE_BANK_SIZE(n) *                    \
						 OTP_SIFLI_EFUSE_BANK_NUM(n)];                     \
                                                                                                   \
	static const uint32_t otp_sifli_efuse_bank_offsets_##n[] =                                 \
		DT_INST_PROP(n, sifli_bank_offsets);                                               \
                                                                                                   \
	static struct otp_sifli_efuse_data otp_sifli_efuse_data_##n;                               \
                                                                                                   \
	static const struct otp_sifli_efuse_config otp_sifli_efuse_config_##n = {                  \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pmuc_base = DT_REG_ADDR(DT_INST_PHANDLE(n, sifli_pmuc)),                          \
		.cache = otp_sifli_efuse_cache_##n,                                                \
		.bank_offsets = otp_sifli_efuse_bank_offsets_##n,                                  \
		.bank_size = OTP_SIFLI_EFUSE_BANK_SIZE(n),                                         \
		.bank_num = OTP_SIFLI_EFUSE_BANK_NUM(n),                                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, otp_sifli_efuse_init, NULL, &otp_sifli_efuse_data_##n,            \
			      &otp_sifli_efuse_config_##n, POST_KERNEL, CONFIG_OTP_INIT_PRIORITY,  \
			      &otp_sifli_efuse_api);

DT_INST_FOREACH_STATUS_OKAY(OTP_SIFLI_EFUSE_INIT)
