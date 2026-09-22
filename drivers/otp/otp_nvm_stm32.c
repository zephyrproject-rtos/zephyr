/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-FileCopyrightText: Copyright (c) 2026 Metratec GmbH
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for one-time programmable areas inside STM32 embedded NVM.
 *
 * When CONFIG_OTP_PROGRAM is enabled, "OTP for user data" area programming is
 * performed by a dedicated, externally visible function in the flash driver(s)
 * - flash_stm32_otp_program() - which is called from this driver. This keeps
 * the read path free of any dependency on the flash driver.
 */

#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

/* drivers/flash/flash_stm32.h */
#include <flash/flash_stm32.h>

#define DT_DRV_COMPAT st_stm32_nvm_otp

struct otp_stm32_nvm_config {
	/* Base address and size of OTP area */
	uint8_t *base;
	size_t size;
};

static void slow_otp_readout(void *buf, const uint8_t *src, size_t len)
{
	uintptr_t dst = (uintptr_t)buf;
	size_t remaining = len;
	uint16_t tmp;

	if (!IS_ALIGNED(src, sizeof(uint16_t))) {
		/*
		 * First byte is not aligned: read the entire halfword
		 * but write only the byte at higher address (which is
		 * the MSB due to STM32 CPUs being little-endian).
		 */
		tmp = sys_read16((mem_addr_t)(src - 1));
		sys_write8(tmp >> 8u, dst);
		src++; dst++; remaining--;
	}

	__ASSERT_NO_MSG(IS_ALIGNED(src, sizeof(uint16_t)));

	/* Copy bulk using 16-bit reads */
	while (remaining >= 2) {
		UNALIGNED_PUT(sys_read16((mem_addr_t)src), (uint16_t *)dst);
		src += sizeof(uint16_t);
		dst += sizeof(uint16_t);
		remaining -= sizeof(uint16_t);
	}

	__ASSERT_NO_MSG(remaining < 2);

	if (remaining == 1) {
		/*
		 * Ditto as above but for unaligned last byte.
		 * src was incremented at the end of the loop
		 * and already points to the target halfword;
		 * however, we keep the LSB this time since
		 * we want the byte at lower address.
		 */
		tmp = sys_read16((mem_addr_t)src);
		sys_write8(tmp & 0xFFu, dst);
	}
}

static int otp_stm32_nvm_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct otp_stm32_nvm_config *config = dev->config;
	const size_t start = (size_t)offset;
	size_t end;

	if (size_add_overflow(start, len, &end) || end > config->size) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_HAS_STM32_UNCACHED_ACCESS_ONLY_OTP)) {
		sys_cache_instr_disable();
	}

	/*
	 * The OTP/RO area is mapped via the AHB interface which does not
	 * support 8-bit reads on series such as STM32H5 or STM32H7R/S.
	 *
	 * Do NOT use memcpy() - instead, copy using only 16-bit reads
	 * which should have the broadest compatibility.
	 */
	if (likely(IS_ALIGNED(start, sizeof(uint16_t)) &&
		   IS_ALIGNED(len, sizeof(uint16_t)))) {
		/* No unaligned head/tail to handle */
		const uint8_t *base = config->base;
		const char *dst = buf;
		size_t cur = start;

		while (cur < end) {
			uint16_t v = sys_read16((mem_addr_t)&base[cur]);

			UNALIGNED_PUT(v, (uint16_t *)dst);

			cur += sizeof(uint16_t);
			dst += sizeof(uint16_t);
		}

		__ASSERT_NO_MSG(cur == end);
	} else {
		slow_otp_readout(buf, &config->base[start], len);
	}

	if (IS_ENABLED(CONFIG_HAS_STM32_UNCACHED_ACCESS_ONLY_OTP)) {
		sys_cache_instr_enable();
	}

	return 0;
}

#if defined(CONFIG_OTP_STM32_NVM_PROGRAMMING_SUPPORT)
/*
 * Default (weak) OTP programming implementation. Series whose flash driver
 * supports programming OTP-in-NVM override this with a strong definition.
 */
__weak int flash_stm32_otp_program(const struct device *dev, uint8_t *otp_base, off_t offset,
				   const void *data, size_t len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(otp_base);
	ARG_UNUSED(offset);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	return -ENOTSUP;
}

static int otp_stm32_nvm_program(const struct device *dev, off_t offset, const void *buf,
				 size_t len)
{
	const struct otp_stm32_nvm_config *config = dev->config;
	size_t result;

	/* 16-bit alignment, half-words only. */
	if ((offset % sizeof(uint16_t)) != 0 || (len % sizeof(uint16_t)) != 0) {
		return -EINVAL;
	}

	if (size_add_overflow(offset, len, &result) || result > config->size) {
		return -EINVAL;
	}

	/*
	 * The actual programming is done by the flash driver, which serializes
	 * it against regular flash operations on the shared NVM peripheral.
	 */
	const struct device *flash_dev = DEVICE_DT_GET(DT_INST_PARENT(0));
	int res = flash_stm32_otp_program(flash_dev, config->base, offset, buf, len);

	if (res == 0) {
		/* Invalidate D$ lines corresponding to written area */
		const size_t line_size = sys_cache_data_line_size_get();
		uint8_t *invd_start;
		size_t invd_size;

		uint8_t *start = config->base + offset;
		uint8_t *end = start + len;

		invd_start = (uint8_t *)ROUND_DOWN((uintptr_t)start, line_size);
		invd_size = (size_t)ROUND_UP((uintptr_t)end - (uintptr_t)invd_start, line_size);
		sys_cache_data_invd_range(invd_start, invd_size);
	}

	return res;
}
#endif /* CONFIG_OTP_STM32_NVM_PROGRAMMING_SUPPORT */

static DEVICE_API(otp, otp_stm32_nvm_api) = {
	.read = otp_stm32_nvm_read,
#if CONFIG_OTP_STM32_NVM_PROGRAMMING_SUPPORT
	.program = otp_stm32_nvm_program,
#endif
};

#define OTP_STM32_NVM_INIT_INNER(inst, _cfg)							\
	static const struct otp_stm32_nvm_config _cfg = {					\
		.base = (void *)DT_INST_REG_ADDR(inst),						\
		.size = DT_INST_REG_SIZE(inst),							\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &_cfg, PRE_KERNEL_1,			\
			      CONFIG_OTP_INIT_PRIORITY, &otp_stm32_nvm_api);

#define OTP_STM32_NVM_INIT(inst)								\
	OTP_STM32_NVM_INIT_INNER(inst, CONCAT(otp_stm32_nvm_cfg, inst))

DT_INST_FOREACH_STATUS_OKAY(OTP_STM32_NVM_INIT)
