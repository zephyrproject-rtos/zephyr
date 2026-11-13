/*
 * Copyright (c) 2026 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_ast2700_otp

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(otp_ast27x0, CONFIG_OTP_LOG_LEVEL);

#define AST27X0_OTP_KEY    0x000U
#define AST27X0_OTP_CMD    0x004U
#define AST27X0_OTP_STATUS 0x018U
#define AST27X0_OTP_ADDR   0x01cU
#define AST27X0_OTP_RDATA  0x020U

#define AST27X0_OTP_UNLOCK_KEY 0x349fe38aU
#define AST27X0_OTP_CMD_READ   0x23b1e361U
#define AST27X0_OTP_BUSY       BIT(0)
#define AST27X0_OTP_ERROR      GENMASK(7, 4)

#define AST27X0_OTP_SIZE       16384U
#define AST27X0_OTP_TIMEOUT_US 10000U

struct ast27x0_otp_config {
	DEVICE_MMIO_ROM;
	uint8_t word_size;
};

struct ast27x0_otp_data {
	DEVICE_MMIO_RAM;
	struct k_mutex lock;
};

static int ast27x0_otp_wait_complete(const struct device *dev, uint32_t word_addr)
{
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t status;
	bool done;

	done = WAIT_FOR(((status = sys_read32(base + AST27X0_OTP_STATUS)) & AST27X0_OTP_BUSY) == 0U,
			AST27X0_OTP_TIMEOUT_US, k_busy_wait(1U));
	if (!done) {
		LOG_ERR("Read timeout at word 0x%04x: status 0x%08x", word_addr, status);
		return -ETIMEDOUT;
	}

	if (FIELD_GET(AST27X0_OTP_ERROR, status) == 0U) {
		return 0;
	}

	LOG_ERR("Read failed at word 0x%04x: status 0x%08x", word_addr, status);

	switch (FIELD_GET(AST27X0_OTP_ERROR, status)) {
	case 3U:
	case 4U:
		return -EACCES;
	case 1U:
	case 2U:
	default:
		return -EIO;
	}
}

static int ast27x0_otp_read_word(const struct device *dev, uint32_t word_addr, uint32_t *value)
{
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	int ret;

	sys_write32(word_addr, base + AST27X0_OTP_ADDR);
	sys_write32(AST27X0_OTP_CMD_READ, base + AST27X0_OTP_CMD);
	ret = ast27x0_otp_wait_complete(dev, word_addr);
	if (ret == 0) {
		*value = sys_read32(base + AST27X0_OTP_RDATA);
	}

	return ret;
}

static int ast27x0_otp_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct ast27x0_otp_config *config = dev->config;
	struct ast27x0_otp_data *data = dev->data;
	size_t word_size = config->word_size;
	uint8_t *out = buf;
	int ret = 0;
	int unlock_ret;

	if (offset < 0) {
		return -EINVAL;
	}

	if (len == 0U) {
		return 0;
	}

	if (offset > (off_t)AST27X0_OTP_SIZE || len > AST27X0_OTP_SIZE - (size_t)offset ||
	    buf == NULL) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	while (len > 0U) {
		uint32_t word_addr = (uint32_t)offset / word_size;
		size_t byte_in_word = (size_t)offset % word_size;
		size_t count = MIN(word_size - byte_in_word, len);
		uint8_t bytes[sizeof(uint32_t)];
		uint32_t value;

		ret = ast27x0_otp_read_word(dev, word_addr, &value);
		if (ret != 0) {
			break;
		}

		sys_put_le32(value, bytes);
		memcpy(out, &bytes[byte_in_word], count);
		out += count;
		offset += count;
		len -= count;
	}

	unlock_ret = k_mutex_unlock(&data->lock);

	return ret != 0 ? ret : unlock_ret;
}

static int ast27x0_otp_init(const struct device *dev)
{
	struct ast27x0_otp_data *data = dev->data;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	sys_write32(AST27X0_OTP_UNLOCK_KEY, DEVICE_MMIO_GET(dev) + AST27X0_OTP_KEY);

	return k_mutex_init(&data->lock);
}

static DEVICE_API(otp, ast27x0_otp_api) = {
	.read = ast27x0_otp_read,
};

#define AST27X0_OTP_DEFINE(inst)                                                                   \
	static const struct ast27x0_otp_config ast27x0_otp_config_##inst = {                       \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                                           \
		.word_size = DT_INST_PROP(inst, aspeed_word_size),                                 \
	};                                                                                        \
	static struct ast27x0_otp_data ast27x0_otp_data_##inst;                                    \
	DEVICE_DT_INST_DEFINE(inst, ast27x0_otp_init, NULL, &ast27x0_otp_data_##inst,              \
			      &ast27x0_otp_config_##inst, POST_KERNEL, CONFIG_OTP_INIT_PRIORITY,   \
			      &ast27x0_otp_api);

DT_INST_FOREACH_STATUS_OKAY(AST27X0_OTP_DEFINE)
