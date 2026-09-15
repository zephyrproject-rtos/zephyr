/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Khai Do
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT allwinner_sunxi_sid

#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/device.h>
#include <zephyr/sys/device_mmio.h>
#include <string.h>
#include <errno.h>

#define SIDC_PRCTL      0x40
#define SIDC_RDKEY      0x60
#define SIDC_OP_LOCK    0xAC

#define SIDC_PRCTL_OFFSET_MASK   GENMASK(24, 16)
#define SIDC_PRCTL_LOCK_MASK     GENMASK(15, 8)
#define SIDC_PRCTL_OP_MASK       GENMASK(1, 0)
#define SIDC_PRCTL_READ          BIT(1)

/* 1 ms timeout limit for polling loop */
#define EFUSE_TIMEOUT_US 1000

struct otp_sunxi_data {
	DEVICE_MMIO_RAM;
};

struct otp_sunxi_config {
	DEVICE_MMIO_ROM;
	size_t size;
};

static int sun8i_efuse_read(uintptr_t base, uint32_t offset, uint32_t *val)
{
	uint32_t reg_val;

	reg_val = sys_read32(base + SIDC_PRCTL);
	reg_val &= ~(SIDC_PRCTL_OFFSET_MASK | SIDC_PRCTL_OP_MASK);
	reg_val |= FIELD_PREP(SIDC_PRCTL_OFFSET_MASK, offset);
	sys_write32(reg_val, base + SIDC_PRCTL);

	/* Lock access using SIDC_OP_LOCK (0xAC) and trigger register-based read (0x2) */
	reg_val &= ~(SIDC_PRCTL_LOCK_MASK | SIDC_PRCTL_OP_MASK);
	reg_val |= FIELD_PREP(SIDC_PRCTL_LOCK_MASK, SIDC_OP_LOCK) | SIDC_PRCTL_READ;
	sys_write32(reg_val, base + SIDC_PRCTL);

	/* Poll until the read bit (0x2) is cleared by hardware */
	if (!WAIT_FOR((sys_read32(base + SIDC_PRCTL) & SIDC_PRCTL_READ) == 0,
		      EFUSE_TIMEOUT_US, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	*val = sys_read32(base + SIDC_RDKEY);

	reg_val &= ~(SIDC_PRCTL_OFFSET_MASK | SIDC_PRCTL_LOCK_MASK | SIDC_PRCTL_OP_MASK);
	sys_write32(reg_val, base + SIDC_PRCTL);

	return 0;
}

static int otp_sunxi_read(const struct device *dev, off_t offset, void *out, size_t len)
{
	const struct otp_sunxi_config *config = dev->config;
	uint8_t *buf = (uint8_t *)out;
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t word_off;
	uint32_t byte_idx;
	uint32_t copy_len;
	uint32_t word;
	size_t end;
	int err;

	if (len == 0) {
		return 0;
	}

	if (out == NULL || offset < 0) {
		return -EINVAL;
	}

	if (size_add_overflow(offset, len, &end) || end > config->size) {
		return -EINVAL;
	}

	word_off = ROUND_DOWN(offset, sizeof(uint32_t));
	byte_idx = offset % sizeof(uint32_t);

	while (len > 0) {
		word = 0;

		err = sun8i_efuse_read(base, word_off, &word);
		if (err < 0) {
			return err;
		}

		copy_len = MIN(len + byte_idx, sizeof(uint32_t)) - byte_idx;

		memcpy(buf, (uint8_t *)&word + byte_idx, copy_len);
		buf += copy_len;

		word_off += sizeof(uint32_t);
		len -= copy_len;
		byte_idx = 0;
	}

	return 0;
}

static DEVICE_API(otp, otp_sunxi_api) = {
	.read = otp_sunxi_read,
};

static int otp_sunxi_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	return 0;
}

#define OTP_SUNXI_INIT(inst) \
	static struct otp_sunxi_data otp_data_##inst; \
	static const struct otp_sunxi_config otp_config_##inst = { \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)), \
		.size = DT_INST_REG_SIZE(inst), \
	}; \
	DEVICE_DT_INST_DEFINE(inst, otp_sunxi_init, NULL, \
			      &otp_data_##inst, &otp_config_##inst, POST_KERNEL, \
			      CONFIG_OTP_INIT_PRIORITY, &otp_sunxi_api);

DT_INST_FOREACH_STATUS_OKAY(OTP_SUNXI_INIT)
