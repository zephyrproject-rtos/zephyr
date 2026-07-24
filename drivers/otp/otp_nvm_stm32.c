/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for one-time programmable areas inside STM32 embedded NVM.
 *
 * "OTP for user data" area programming is not supported yet.
 * It shall be implemented as a dedicated, externally visible function
 * in the flash driver(s) called from this driver.
 */

#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "../flash/flash_stm32.h"

#define DT_DRV_COMPAT st_stm32_nvm_otp

struct otp_stm32_nvm_config {
	/* Base address and size of OTP area */
	const uint8_t *base;
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

#if CONFIG_OTP_PROGRAM
#define STM32H7_FLASH_TIMEOUT (2 * DT_PROP(DT_INST(0, st_stm32_nv_flash), max_erase_time))

static int _wait_ready(const struct device *dev)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);
	k_timepoint_t tp = sys_timepoint_calc(K_MSEC(STM32H7_FLASH_TIMEOUT));

	/* Wait for the data buffer to drain and the operation to finish. */
	while (regs->FLASH_STM32_SR & (FLASH_SR_BSY | FLASH_SR_DBNE)) {
		if (sys_timepoint_expired(tp)) {
			return -ETIMEDOUT;
		}
	}

	/* Check for (and clear) errors left by the previous/just-finished op. */
	if (regs->FLASH_STM32_SR & FLASH_STM32_SR_ERRORS) {
		regs->FLASH_STM32_CCR = regs->FLASH_STM32_SR & FLASH_STM32_SR_ERRORS;
		return -EIO;
	}

	return 0;
}

static void _flash_cr_unlock(const struct device *dev)
{
	FLASH_TypeDef *regs = FLASH_STM32_REGS(dev);

	if (!(FLASH_STM32_REGS(dev)->NSCR & FLASH_CR_LOCK)) {
		return;
	}

#ifdef CONFIG_SOC_SERIES_STM32H7X
	regs->KEYR1 = FLASH_KEY1;
	regs->KEYR1 = FLASH_KEY2;
#ifdef DUAL_BANK
	regs->KEYR2 = FLASH_KEY1;
	regs->KEYR2 = FLASH_KEY2;
#endif /* DUAL_BANK */
#elif CONFIG_SOC_SERIES_STM32H5X
	regs->NSKEYR = FLASH_KEY1;
	regs->NSKEYR = FLASH_KEY2;
#else  /* CONFIG_SOC_SERIES_STM32H7X */
	regs->KEYR = FLASH_KEY1;
	regs->KEYR = FLASH_KEY2;
#endif /* CONFIG_SOC_SERIES_STM32H7X */
	barrier_dsync_fence_full();
}

static int otp_stm32_nvm_program(const struct device *dev, off_t offset, const void *buf,
				 size_t len)
{
	const struct otp_stm32_nvm_config *config = dev->config;
	const size_t start = (size_t)offset;
	size_t end;

	/*16-bit alignment and halfwords only */
	if (offset % 2 || len % 2) {
		return -EINVAL;
	}
	if (size_add_overflow(start, len, &end) || end > config->size) {
		return -EINVAL;
	}
	/* Wait until flash is ready */
	int ret = _wait_ready(dev);

	if (ret != 0) {
		return ret;
	}

	_flash_cr_unlock(dev);
	/* Enable programming mode */
	FLASH_STM32_REGS(dev)->NSCR |= FLASH_CR_PG;
	/* Check lock-bit state for the specified memory range */
	if ((BIT_MASK(DIV_ROUND_UP(len, 64)) << start / 64) & FLASH_STM32_REGS(dev)->OTPBLR_CUR) {
		ret = -EACCES;
		goto error;
	}

	size_t cur = start;
	const uint8_t *src = buf;

	while (cur < end) {
		uint16_t v = sys_le16_to_cpu(sys_get_le16(src));

		sys_write16(v, (mem_addr_t)&config->base[cur]);
		/* Wait for write operation to complete */
		ret = _wait_ready(dev);
		if (ret != 0) {
			goto error;
		}

		cur += sizeof(uint16_t);
		src += sizeof(uint16_t);
	}

error:
	FLASH_STM32_REGS(dev)->NSCR &= ~FLASH_CR_PG;
	FLASH_STM32_REGS(dev)->NSCR |= FLASH_CR_LOCK;
	return ret;
}
#endif

static struct flash_stm32_priv flash_data = {
	.regs = (FLASH_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(0)),
};

static DEVICE_API(otp, otp_stm32_nvm_api) = {
	.read = otp_stm32_nvm_read,
#if CONFIG_OTP_PROGRAM
	.program = otp_stm32_nvm_program,
#endif
};

#define OTP_STM32_NVM_INIT_INNER(inst, _cfg)							\
	static const struct otp_stm32_nvm_config _cfg = {					\
		.base = (void *)DT_INST_REG_ADDR(inst),						\
		.size = DT_INST_REG_SIZE(inst),							\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &flash_data, &_cfg,				\
			      POST_KERNEL, CONFIG_OTP_INIT_PRIORITY, &otp_stm32_nvm_api);

#define OTP_STM32_NVM_INIT(inst)								\
	OTP_STM32_NVM_INIT_INNER(inst, CONCAT(otp_stm32_nvm_cfg, inst))

DT_INST_FOREACH_STATUS_OKAY(OTP_STM32_NVM_INIT)
