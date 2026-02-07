/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for one-time programmable areas inside STM32 embedded NVM.
 *
 * "OTP for user data" area programming is not supported yet.
 */

#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT st_stm32_nvm_otp

struct otp_stm32_nvm_config {
	/* Base address and size of OTP area */
	const uint8_t *base;
	size_t size;

	/* Indicates OTP area is writeable by user */
	bool user_otp;
};

static int otp_stm32_nvm_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct otp_stm32_nvm_config *config = dev->config;
	size_t start = (size_t)offset;
	size_t end;

	if (size_add_overflow(start, len, &end) || end > config->size) {
		return -EINVAL;
	}

#if defined(CONFIG_SOC_SERIES_STM32H5X)
	sys_cache_instr_disable();
#endif

	memcpy(buf, &config->base[start], len);

#if defined(CONFIG_SOC_SERIES_STM32H5X)
	sys_cache_instr_enable();
#endif

	return 0;
}

static DEVICE_API(otp, otp_stm32_flash_api) = {
	.read = otp_stm32_nvm_read,
};

#define OTP_STM32_FLASH_INIT_INNER(n, _cfg)							\
	static const struct otp_stm32_nvm_config _cfg = {					\
		.base = (void *)DT_INST_REG_ADDR(n),						\
		.size = DT_INST_REG_SIZE(n),							\
		.user_otp = DT_INST_PROP(n, st_user_otp),					\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &_cfg,					\
			      PRE_KERNEL_1, CONFIG_OTP_INIT_PRIORITY, &otp_stm32_flash_api);

#define OTP_STM32_FLASH_INIT(n)									\
	OTP_STM32_FLASH_INIT_INNER(n, CONCAT(otp_stm32_flash_, DT_INST_DEP_ORD(n), _cfg))

DT_INST_FOREACH_STATUS_OKAY(OTP_STM32_FLASH_INIT)
