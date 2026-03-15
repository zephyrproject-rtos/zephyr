/*
 * Copyright (c) 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/clock.h>
#include <soc.h>

#define DT_DRV_COMPAT nxp_rt7xx_ocotp

LOG_MODULE_REGISTER(nxp_ocotp, CONFIG_OTP_LOG_LEVEL);

#define OCOTP_WR_UNLOCK_KEY  0x3E77U
#define OCOTP_BUSY_TIMEOUT_US 100000U
#define OCOTP_WRITE_POSTAMBLE_US 2U

struct nxp_ocotp_config {
	DEVICE_MMIO_ROM;
	uint32_t size;
};

struct nxp_ocotp_data {
	DEVICE_MMIO_RAM;
	struct k_sem lock;
};

static inline OCOTP_Type *nxp_ocotp_get_base(const struct device *dev)
{
	return (OCOTP_Type *)DEVICE_MMIO_GET(dev);
}

static inline void nxp_ocotp_lock(const struct device *dev)
{
	struct nxp_ocotp_data *data = dev->data;

	if (!k_is_pre_kernel()) {
		(void)k_sem_take(&data->lock, K_FOREVER);
	}
}

static inline void nxp_ocotp_unlock(const struct device *dev)
{
	struct nxp_ocotp_data *data = dev->data;

	if (!k_is_pre_kernel()) {
		k_sem_give(&data->lock);
	}
}

static bool nxp_ocotp_range_is_valid(const struct nxp_ocotp_config *config, off_t offset,
				     size_t len)
{
	size_t start;

	if (offset < 0) {
		return false;
	}

	start = (size_t)offset;

	return start <= config->size && len <= config->size - start;
}

#if defined(CONFIG_OTP_PROGRAM)
static int nxp_ocotp_wait_busy(OCOTP_Type *base)
{
	k_timepoint_t timeout = sys_timepoint_calc(K_USEC(OCOTP_BUSY_TIMEOUT_US));

	while (base->HW_OCOTP_STATUS & OCOTP_HW_OCOTP_STATUS_BUSY_MASK) {
		if (sys_timepoint_expired(timeout)) {
			LOG_ERR("OCOTP timeout waiting for busy clear");
			return -ETIMEDOUT;
		}

		k_busy_wait(1);
	}

	return 0;
}

static int nxp_ocotp_wait_reload_done(OCOTP_Type *base)
{
	k_timepoint_t timeout = sys_timepoint_calc(K_USEC(OCOTP_BUSY_TIMEOUT_US));

	while ((base->HW_OCOTP_STATUS & OCOTP_HW_OCOTP_STATUS_BUSY_MASK) ||
	       (base->CTRL & OCOTP_CTRL_RELOAD_SHADOWS_MASK)) {
		if (sys_timepoint_expired(timeout)) {
			LOG_ERR("OCOTP timeout waiting for shadow reload");
			return -ETIMEDOUT;
		}

		k_busy_wait(1);
	}

	return 0;
}

static int nxp_ocotp_reload_shadow_registers(OCOTP_Type *base)
{
	int ret;

	ret = nxp_ocotp_wait_busy(base);
	if (ret < 0) {
		return ret;
	}

	base->CTRL = OCOTP_CTRL_RELOAD_SHADOWS(1U);

	return nxp_ocotp_wait_reload_done(base);
}
#endif /* CONFIG_OTP_PROGRAM */

static int nxp_ocotp_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct nxp_ocotp_config *config = dev->config;
	OCOTP_Type *base = nxp_ocotp_get_base(dev);
	uint8_t *buf_u8 = buf;
	uint32_t addr;
	size_t part, skip;
	uint32_t raw;

	if (buf == NULL) {
		return -EINVAL;
	}

	if (!nxp_ocotp_range_is_valid(config, offset, len)) {
		LOG_ERR("Read out of range (offset=0x%lx len=0x%zx size=0x%x)",
			offset, len, config->size);
		return -EINVAL;
	}

	/* Compute the word index and byte offset within the first word */
	addr = offset / sizeof(uint32_t);
	skip = offset % sizeof(uint32_t);

	nxp_ocotp_lock(dev);

	while (len > 0) {
		/* Read directly from the shadow register */
		raw = base->OTP_SHADOW[addr];

		part = MIN(len + skip, sizeof(uint32_t));
		part -= skip;

		memcpy(buf_u8, (uint8_t *)&raw + skip, part);
		buf_u8 += part;

		addr++;
		len -= part;

		/* skip is only relevant for the first word */
		skip = 0;
	}

	nxp_ocotp_unlock(dev);

	return 0;
}

#if defined(CONFIG_OTP_PROGRAM)
static int nxp_ocotp_program(const struct device *dev, off_t offset, const void *buf, size_t len)
{
	const struct nxp_ocotp_config *config = dev->config;
	OCOTP_Type *base = nxp_ocotp_get_base(dev);
	const uint8_t *buf_u8 = buf;
	uint32_t addr, data;
	int ret;

	if (buf == NULL) {
		return -EINVAL;
	}

	if (!IS_ALIGNED(offset, sizeof(uint32_t)) || !IS_ALIGNED(len, sizeof(uint32_t))) {
		LOG_ERR("Unaligned program not allowed (0x%lx/0x%zx)", offset, len);
		return -EINVAL;
	}

	if (!nxp_ocotp_range_is_valid(config, offset, len)) {
		LOG_ERR("Program out of range (offset=0x%lx len=0x%zx size=0x%x)",
			offset, len, config->size);
		return -EINVAL;
	}

	addr = offset / sizeof(uint32_t);
	len /= sizeof(uint32_t);

	nxp_ocotp_lock(dev);

	while (len > 0) {
		ret = nxp_ocotp_wait_busy(base);
		if (ret) {
			nxp_ocotp_unlock(dev);
			return ret;
		}

		memcpy(&data, buf_u8, sizeof(data));

		/* Write the fuse address and unlock key to CTRL */
		base->CTRL = OCOTP_CTRL_ADDR(addr) |
			     OCOTP_CTRL_WR_UNLOCK(OCOTP_WR_UNLOCK_KEY);

		/* Write data to trigger the program operation */
		base->HW_OCOTP_WRITE_DATA = data;

		ret = nxp_ocotp_wait_busy(base);
		if (ret) {
			nxp_ocotp_unlock(dev);
			return ret;
		}

		k_busy_wait(OCOTP_WRITE_POSTAMBLE_US);

		if (base->HW_OCOTP_STATUS & OCOTP_HW_OCOTP_STATUS_PROGFAIL_MASK) {
			LOG_ERR("OCOTP program failed at addr %u", addr);
			nxp_ocotp_unlock(dev);
			return -EIO;
		}

		addr++;
		buf_u8 += sizeof(data);
		len--;
	}

	ret = nxp_ocotp_reload_shadow_registers(base);
	if (ret < 0) {
		nxp_ocotp_unlock(dev);
		return ret;
	}

	nxp_ocotp_unlock(dev);

	return 0;
}
#endif /* CONFIG_OTP_PROGRAM */

static DEVICE_API(otp, nxp_ocotp_api) = {
	.read = nxp_ocotp_read,
#if defined(CONFIG_OTP_PROGRAM)
	.program = nxp_ocotp_program,
#endif
};

static int nxp_ocotp_init(const struct device *dev)
{
	struct nxp_ocotp_data *data = dev->data;
	OCOTP_Type *base;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	k_sem_init(&data->lock, 1, 1);

	base = nxp_ocotp_get_base(dev);

	/* Verify that fuses have been loaded into shadow registers */
	if (!(base->HW_OCOTP_STATUS & OCOTP_HW_OCOTP_STATUS_FUSE_LATCHED_MASK)) {
		LOG_ERR("OTP fuses not latched into shadow registers");
		return -EIO;
	}

	return 0;
}

#define NXP_OCOTP_INIT(inst)								\
	static struct nxp_ocotp_data nxp_ocotp_##inst##_data;				\
											\
	static const struct nxp_ocotp_config nxp_ocotp_##inst##_config = {		\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),				\
		.size = DT_INST_REG_SIZE(inst),						\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst, nxp_ocotp_init, NULL, &nxp_ocotp_##inst##_data,	\
			      &nxp_ocotp_##inst##_config, PRE_KERNEL_1,			\
			      CONFIG_OTP_INIT_PRIORITY, &nxp_ocotp_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_OCOTP_INIT)
