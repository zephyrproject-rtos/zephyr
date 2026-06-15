/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Khai Do
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT allwinner_sunxi_sid

#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
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

struct otp_sunxi_config {
	uintptr_t base;
	size_t size;
};

static int sun8i_efuse_read(uintptr_t base, uint32_t offset, uint32_t *val)
{
	uint32_t reg_val;
	uint32_t timeout = EFUSE_TIMEOUT_US;

	reg_val = sys_read32(base + SIDC_PRCTL);
	reg_val &= ~(SIDC_PRCTL_OFFSET_MASK | SIDC_PRCTL_OP_MASK);
	reg_val |= FIELD_PREP(SIDC_PRCTL_OFFSET_MASK, offset);
	sys_write32(reg_val, base + SIDC_PRCTL);

	/* Lock access using SIDC_OP_LOCK (0xAC) and trigger register-based read (0x2) */
	reg_val &= ~(SIDC_PRCTL_LOCK_MASK | SIDC_PRCTL_OP_MASK);
	reg_val |= FIELD_PREP(SIDC_PRCTL_LOCK_MASK, SIDC_OP_LOCK) | SIDC_PRCTL_READ;
	sys_write32(reg_val, base + SIDC_PRCTL);

	/* Poll until the read bit (0x2) is cleared by hardware */
	while (sys_read32(base + SIDC_PRCTL) & SIDC_PRCTL_READ) {
		if (timeout-- == 0) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}

	reg_val &= ~(SIDC_PRCTL_OFFSET_MASK | SIDC_PRCTL_LOCK_MASK | SIDC_PRCTL_OP_MASK);
	sys_write32(reg_val, base + SIDC_PRCTL);

	*val = sys_read32(base + SIDC_RDKEY);

	return 0;
}

static int otp_sunxi_read(const struct device *dev, off_t offset, void *out, size_t len)
{
	const struct otp_sunxi_config *config = dev->config;
	uint8_t *buf = (uint8_t *)out;
	int err;

	if (len == 0) {
		return 0;
	}

	if (out == NULL || offset < 0) {
		return -EINVAL;
	}

	if (offset + len > config->size) {
		return -EINVAL;
	}

	/* eFuse hardware can only be read in 4-byte word alignments */
	uint32_t start_offset = offset;
	uint32_t end_offset = offset + len;

	for (uint32_t off = ROUND_DOWN(start_offset, 4); off < end_offset; off += 4) {
		uint32_t word = 0;

		err = sun8i_efuse_read(config->base, off, &word);
		if (err < 0) {
			return err;
		}
		for (uint32_t i = 0; i < 4; i++) {
			uint32_t byte_pos = off + i;

			if (byte_pos >= start_offset && byte_pos < end_offset) {
				buf[byte_pos - start_offset] = (uint8_t)(word >> (i * 8));
			}
		}
	}

	return 0;
}

static DEVICE_API(otp, otp_sunxi_api) = {
	.read = otp_sunxi_read,
};

#define OTP_SUNXI_INIT(inst) \
	static const struct otp_sunxi_config otp_config_##inst = { \
		.base = DT_INST_REG_ADDR(inst), \
		.size = DT_INST_PROP(inst, size), \
	}; \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, \
			      &otp_config_##inst, POST_KERNEL, \
			      CONFIG_OTP_INIT_PRIORITY, &otp_sunxi_api);

DT_INST_FOREACH_STATUS_OKAY(OTP_SUNXI_INIT)
