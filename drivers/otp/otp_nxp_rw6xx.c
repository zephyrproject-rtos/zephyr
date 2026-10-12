/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_ocotp.h>

#ifdef CONFIG_OTP_PROGRAM
#include <fsl_romapi_otp.h>
#endif

#define DT_DRV_COMPAT nxp_rw6xx_ocotp

LOG_MODULE_REGISTER(nxp_rw6xx_ocotp, CONFIG_OTP_LOG_LEVEL);

struct nxp_rw6xx_ocotp_config {
	size_t size;
};

static K_MUTEX_DEFINE(nxp_rw6xx_ocotp_mutex);

static inline void nxp_rw6xx_ocotp_lock(void)
{
	if (!k_is_pre_kernel()) {
		(void)k_mutex_lock(&nxp_rw6xx_ocotp_mutex, K_FOREVER);
	}
}

static inline void nxp_rw6xx_ocotp_unlock(void)
{
	if (!k_is_pre_kernel()) {
		(void)k_mutex_unlock(&nxp_rw6xx_ocotp_mutex);
	}
}

static bool nxp_rw6xx_ocotp_range_valid(const struct device *dev, off_t offset, size_t len)
{
	const struct nxp_rw6xx_ocotp_config *cfg = dev->config;

	if (offset < 0) {
		return false;
	}

	size_t start = (size_t)offset;

	return start <= cfg->size && len <= cfg->size - start;
}

static int nxp_rw6xx_ocotp_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	uint8_t *buf_u8 = buf;
	uint32_t addr, skip;
	int ret = 0;

	if (buf == NULL) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	if (!nxp_rw6xx_ocotp_range_valid(dev, offset, len)) {
		LOG_ERR("Read out of range (offset=0x%lx len=0x%zx)", (long)offset, len);
		return -EINVAL;
	}

	addr = (uint32_t)offset / sizeof(uint32_t);
	skip = (uint32_t)offset % sizeof(uint32_t);

	nxp_rw6xx_ocotp_lock();

	while (len > 0) {
		uint32_t raw;
		size_t part;
		status_t status;

		status = OCOTP_OtpFuseRead(addr, &raw);
		if (status != kStatus_Success) {
			LOG_ERR("Fuse read failed at word %u (status %d)", addr, (int)status);
			ret = -EIO;
			break;
		}

		part = MIN(len + skip, sizeof(uint32_t)) - skip;
		memcpy(buf_u8, (uint8_t *)&raw + skip, part);

		buf_u8 += part;
		len -= part;
		addr++;
		skip = 0;
	}

	nxp_rw6xx_ocotp_unlock();

	return ret;
}

#ifdef CONFIG_OTP_PROGRAM
static int nxp_rw6xx_ocotp_program(const struct device *dev, off_t offset, const void *buf,
				    size_t len)
{
	const uint8_t *buf_u8 = buf;
	uint32_t addr;
	int ret = 0;

	if (buf == NULL) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	if (!IS_ALIGNED(offset, sizeof(uint32_t)) || !IS_ALIGNED(len, sizeof(uint32_t))) {
		LOG_ERR("Unaligned program not allowed (0x%lx/0x%zx)", (long)offset, len);
		return -EINVAL;
	}

	if (!nxp_rw6xx_ocotp_range_valid(dev, offset, len)) {
		LOG_ERR("Program out of range (offset=0x%lx len=0x%zx)", (long)offset, len);
		return -EINVAL;
	}

	addr = (uint32_t)offset / sizeof(uint32_t);

	nxp_rw6xx_ocotp_lock();

	while (len > 0) {
		uint32_t data;
		status_t status;

		memcpy(&data, buf_u8, sizeof(uint32_t));

		status = otp_fuse_program(addr, data);
		if (status != kStatus_Success) {
			LOG_ERR("Fuse program failed at word %u (status %d)", addr, (int)status);
			ret = -EIO;
			break;
		}

		buf_u8 += sizeof(uint32_t);
		len -= sizeof(uint32_t);
		addr++;
	}

	/*
	 * Reads go straight to the fuse array, but other consumers (and hardware
	 * blocks that latch their configuration at boot) see the shadow
	 * registers, which still hold the pre-program values until reloaded.
	 */
	if (ret == 0) {
		status_t status = otp_shadow_register_reload();

		if (status != kStatus_Success) {
			LOG_ERR("Shadow register reload failed (status %d)", (int)status);
			ret = -EIO;
		}
	}

	nxp_rw6xx_ocotp_unlock();

	return ret;
}
#endif /* CONFIG_OTP_PROGRAM */

static DEVICE_API(otp, nxp_rw6xx_ocotp_api) = {
	.read = nxp_rw6xx_ocotp_read,
#ifdef CONFIG_OTP_PROGRAM
	.program = nxp_rw6xx_ocotp_program,
#endif
};

static int nxp_rw6xx_ocotp_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	OCOTP_OtpInit();

	return 0;
}

/*
 * The SDK OCOTP HAL targets the single OCOTP peripheral through a fixed base
 * pointer, so a second instance could not be addressed even if one existed.
 */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "Only a single nxp,rw6xx-ocotp instance is supported");

static const struct nxp_rw6xx_ocotp_config nxp_rw6xx_ocotp_dev_config = {
	.size = DT_INST_REG_SIZE(0),
};

DEVICE_DT_INST_DEFINE(0, nxp_rw6xx_ocotp_init, NULL, NULL,
		      &nxp_rw6xx_ocotp_dev_config, PRE_KERNEL_1,
		      CONFIG_OTP_INIT_PRIORITY, &nxp_rw6xx_ocotp_api);
