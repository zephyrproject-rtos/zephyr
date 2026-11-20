/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_fsb

#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(otp_nxp_fsb, CONFIG_OTP_LOG_LEVEL);

struct otp_nxp_fsb_config {
	mm_reg_t base;
	size_t size;
};

static int otp_nxp_fsb_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct otp_nxp_fsb_config *config = dev->config;
	uint8_t *dst = buf;
	size_t word, skip, part;
	uint32_t raw;

	if (buf == NULL || offset < 0) {
		return -EINVAL;
	}

	if ((size_t)offset > config->size || len > config->size - (size_t)offset) {
		LOG_ERR("Read out of range (offset=0x%lx len=0x%zx)", (long)offset, len);
		return -EINVAL;
	}

	word = (size_t)offset / sizeof(uint32_t);
	skip = (size_t)offset % sizeof(uint32_t);

	while (len > 0) {
		raw = sys_read32(config->base + word * sizeof(uint32_t));

		part = MIN(len, sizeof(uint32_t) - skip);
		memcpy(dst, (uint8_t *)&raw + skip, part);

		dst += part;
		len -= part;
		word++;
		skip = 0;
	}

	return 0;
}

static DEVICE_API(otp, otp_nxp_fsb_api) = {
	.read = otp_nxp_fsb_read,
};

#define OTP_NXP_FSB_INIT(inst)                                                                     \
	static const struct otp_nxp_fsb_config otp_nxp_fsb_##inst##_config = {                     \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.size = DT_INST_REG_SIZE(inst),                                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &otp_nxp_fsb_##inst##_config, PRE_KERNEL_1,  \
			      CONFIG_OTP_INIT_PRIORITY, &otp_nxp_fsb_api);

DT_INST_FOREACH_STATUS_OKAY(OTP_NXP_FSB_INIT)
