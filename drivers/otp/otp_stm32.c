/*
 * Copyright (c) 2026 James Walmsley <james@fullfat-fs.co.uk>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#if defined(CONFIG_SOC_SERIES_STM32H5X)
#include <zephyr/cache.h>
#endif

#include "../flash/flash_stm32.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#define DT_DRV_COMPAT st_stm32_otp

LOG_MODULE_REGISTER(otp_stm32, LOG_LEVEL_DBG);

struct otp_stm32_config {
	uintptr_t base;
	size_t len;
	const struct device *flash_dev;
};

static K_MUTEX_DEFINE(lock);

static inline void otp_stm32_lock(void)
{
	if (!k_is_pre_kernel()) {
		(void)k_mutex_lock(&lock, K_FOREVER);
	}
}

static inline void otp_stm32_unlock(void)
{
	if (!k_is_pre_kernel()) {
		(void)k_mutex_unlock(&lock);
	}
}

static inline void otp_stm32_flash_lock(const struct device *flash_dev)
{
	if (!k_is_pre_kernel()) {
		flash_stm32_sem_take(flash_dev);
	}
}

static inline void otp_stm32_flash_unlock(const struct device *flash_dev)
{
	if (!k_is_pre_kernel()) {
		flash_stm32_sem_give(flash_dev);
	}
}

static void slow_otp_readout(uint8_t *dst, const uint8_t *src, size_t len)
{
	size_t idx = 0;

	while (idx < len) {
		uintptr_t addr = (uintptr_t)(src + idx);
		uintptr_t aligned = addr & ~(uintptr_t)0x3U;
		uint32_t word = sys_read32((mem_addr_t)aligned);
		size_t offset = addr - aligned;
		size_t chunk = MIN(len - idx, 4U - offset);

		memcpy(dst + idx, ((uint8_t *)&word) + offset, chunk);
		idx += chunk;
	}
}

static int otp_stm32_flash_offset(const struct otp_stm32_config *config, off_t offset,
				  unsigned int *flash_offset)
{
	uintptr_t address = config->base + (uintptr_t)offset;

	if (address < FLASH_STM32_BASE_ADDRESS) {
		return -EINVAL;
	}

	uintptr_t diff = address - FLASH_STM32_BASE_ADDRESS;

	if (diff > UINT_MAX) {
		return -EINVAL;
	}

	*flash_offset = (unsigned int)diff;

	return 0;
}

static int otp_stm32_check_range(const struct otp_stm32_config *config, off_t offset, size_t len)
{
	if (offset < 0) {
		return -EINVAL;
	}

	if (config->len == 0) {
		return -EINVAL;
	}

	if ((size_t)offset > config->len) {
		return -EINVAL;
	}

	if (len > (config->len - (size_t)offset)) {
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_OTP_PROGRAM)
static int otp_stm32_program(const struct device *dev, off_t offset, const void *buf, size_t len)
{
	const struct otp_stm32_config *config = dev->config;
	int ret;
	unsigned int flash_offset;

	LOG_DBG("Writing to otp: %08lx, (%d bytes)", offset, len);

	if (len == 0) {
		return 0;
	}

	if (buf == NULL) {
		return -EINVAL;
	}

	ret = otp_stm32_check_range(config, offset, len);
	if (ret != 0) {
		return ret;
	}

	if (!device_is_ready(config->flash_dev)) {
		LOG_ERR("Flash device not ready");
		return -ENODEV;
	}

	ret = otp_stm32_flash_offset(config, offset, &flash_offset);
	if (ret != 0) {
		return ret;
	}

	LOG_DBG("Writing to otp: %08lx, (%d bytes)", offset, len);

	if (!flash_stm32_valid_write(flash_offset, len)) {
		LOG_ERR("OTP write not aligned to %u bytes", FLASH_STM32_WRITE_BLOCK_SIZE);
		return -EINVAL;
	}

	otp_stm32_lock();
	otp_stm32_flash_lock(config->flash_dev);

	ret = flash_stm32_cr_lock(config->flash_dev, false);
	if (ret == 0) {
		ret = flash_stm32_write_range(config->flash_dev, flash_offset, buf, len);
	}

	int ret2 = flash_stm32_cr_lock(config->flash_dev, true);

	if (ret == 0) {
		ret = ret2;
	}

	otp_stm32_flash_unlock(config->flash_dev);
	otp_stm32_unlock();

	return ret;
}
#endif /* CONFIG_OTP_PROGRAM */

static int otp_stm32_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct otp_stm32_config *config = dev->config;
	const uint8_t *base;
	size_t start;
	size_t end;
	size_t cur;
	int ret;

	if (len == 0) {
		return 0;
	}

	if (buf == NULL) {
		return -EINVAL;
	}

	ret = otp_stm32_check_range(config, offset, len);
	if (ret != 0) {
		return ret;
	}

	if (!device_is_ready(config->flash_dev)) {
		LOG_ERR("Flash device not ready");
		return -ENODEV;
	}

	base = (const uint8_t *)config->base;
	start = (size_t)offset;
	end = start + len;
	cur = start;

#if defined(CONFIG_SOC_SERIES_STM32H5X)
	sys_cache_instr_disable();
#endif

	otp_stm32_lock();
	otp_stm32_flash_lock(config->flash_dev);
	if (IS_ALIGNED(start, sizeof(uint32_t)) && IS_ALIGNED(len, sizeof(uint32_t))) {
		for (; cur < end; cur += sizeof(uint32_t)) {
			uint32_t word = sys_read32((mem_addr_t)(base + cur));

			memcpy((uint8_t *)buf + (cur - start), &word, sizeof(word));
		}

		__ASSERT_NO_MSG(cur == end);
	} else {
		slow_otp_readout((uint8_t *)buf, base + start, len);
	}
	otp_stm32_flash_unlock(config->flash_dev);
	otp_stm32_unlock();

#if defined(CONFIG_SOC_SERIES_STM32H5X)
	sys_cache_instr_enable();
#endif

	return 0;
}

static DEVICE_API(otp, otp_stm32_api) = {
#if defined(CONFIG_OTP_PROGRAM)
	.program = otp_stm32_program,
#endif
	.read = otp_stm32_read,
};

#define OTP_STM32_INIT(n)                                                                          \
	static const struct otp_stm32_config otp_stm32_config_##n = {                              \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.len = DT_INST_REG_SIZE(n),                                                        \
		.flash_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, flash)),                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &otp_stm32_config_##n, POST_KERNEL,            \
			      CONFIG_OTP_INIT_PRIORITY, &otp_stm32_api);

DT_INST_FOREACH_STATUS_OKAY(OTP_STM32_INIT)
