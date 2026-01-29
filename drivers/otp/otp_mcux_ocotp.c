/*
 * Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/logging/log.h>

#include "fsl_ocotp.h"

#define DT_DRV_COMPAT nxp_mcux_ocotp

LOG_MODULE_REGISTER(mcux_ocotp, CONFIG_OTP_LOG_LEVEL);

struct mcux_ocotp_config {
	OCOTP_Type *base;

	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

static inline void mcux_ocotp_lock(const struct device *dev)
{
	struct k_sem *lock = dev->data;

	if (!k_is_pre_kernel()) {
		(void)k_sem_take(lock, K_FOREVER);
	}
}

static inline void mcux_ocotp_unlock(const struct device *dev)
{
	struct k_sem *lock = dev->data;

	if (!k_is_pre_kernel()) {
		k_sem_give(lock);
	}
}

#if defined(CONFIG_OTP_PROGRAM)
static int mcux_ocotp_program(const struct device *dev, off_t offset, const void *buf, size_t len)
{
	const struct mcux_ocotp_config *config = dev->config;

	const uint32_t *buf_u32 = buf;
	uint32_t addr, raw;
	status_t status;

	if (offset < 0 || buf == NULL) {
		return -EINVAL;
	}

	if (!IS_ALIGNED(offset, sizeof(uint32_t)) || !IS_ALIGNED(len, sizeof(uint32_t))) {
		LOG_ERR("Unaligned program not allowed (0x%lx/0x%zx)", offset, len);
		return -EINVAL;
	}

	addr = offset / sizeof(uint32_t);
	len /= sizeof(uint32_t);

	mcux_ocotp_lock(dev);

	while (len > 0) {
		memcpy(&raw, buf_u32, sizeof(uint32_t));

		status = OCOTP_WriteFuseShadowRegister(config->base, addr, raw);
		if (status != kStatus_Success) {
			LOG_ERR("Failed to write OCOTP (%d)", status);
			mcux_ocotp_unlock(dev);
			return -EIO;
		}

		addr++;
		buf_u32++;
		len--;
	}

	mcux_ocotp_unlock(dev);

	return 0;
}
#endif /* CONFIG_OTP_PROGRAM */

static int mcux_ocotp_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct mcux_ocotp_config *config = dev->config;

	uint8_t *buf_u8 = buf;
	uint32_t addr;
	status_t status;
	size_t part, skip;
	uint8_t raw[sizeof(uint32_t)];

	if (offset < 0 || buf == NULL) {
		return -EINVAL;
	}

	/* Aligned start address */
	addr = offset / sizeof(uint32_t);
	/* Skip the first (len mod 4) bytes for unaligned start address */
	skip = offset % sizeof(uint32_t);

	mcux_ocotp_lock(dev);

	while (len > 0) {
		status = OCOTP_ReadFuseShadowRegisterExt(config->base, addr, (uint32_t *)raw, 1);
		if (status != kStatus_Success) {
			LOG_ERR("Failed to read OCOTP (%d)", status);
			mcux_ocotp_unlock(dev);
			return -EIO;
		}

		part = min(len + skip, sizeof(uint32_t));
		part -= skip;

		memcpy(buf_u8, &raw[skip], part);
		buf_u8 += part;

		addr++;
		len -= part;

		/* Skip can only be in the first read */
		skip = 0;
	}

	mcux_ocotp_unlock(dev);

	return 0;
}

static DEVICE_API(otp, mcux_ocotp_api) = {
#if defined(CONFIG_OTP_PROGRAM)
	.program = mcux_ocotp_program,
#endif
	.read = mcux_ocotp_read,
};

static int mcux_ocotp_init(const struct device *dev)
{
	const struct mcux_ocotp_config *config = dev->config;
	uint32_t clock_freq = 0;
	int ret;

	if (config->clock_dev != NULL) {
		if (!device_is_ready(config->clock_dev)) {
			LOG_ERR("Clock not ready");
			return -ENODEV;
		}

		ret = clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq);
		if (ret < 0) {
			LOG_ERR("Clock get rate failed (%d)", ret);
			return ret;
		}
	}

	OCOTP_Init(config->base, clock_freq);

	return 0;
}

#define OTP_MCUX_OCOTP_CLOCK_INIT(inst)                                                            \
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(inst, 0)),                           \
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(inst, 0, name),

#define OTP_MCUX_OCOTP_INIT(inst)                                                                  \
	static K_SEM_DEFINE(mcux_ocotp_##inst##_lock, 1, 1);                                       \
                                                                                                   \
	static const struct mcux_ocotp_config mcux_ocotp_##inst##_config = {                       \
		.base = (OCOTP_Type *)DT_INST_REG_ADDR(inst),                                      \
		IF_ENABLED(DT_INST_CLOCKS_HAS_IDX(inst, 0), (OTP_MCUX_OCOTP_CLOCK_INIT(inst)))     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mcux_ocotp_init, NULL, &mcux_ocotp_##inst##_lock,              \
			      &mcux_ocotp_##inst##_config, PRE_KERNEL_1, CONFIG_OTP_INIT_PRIORITY, \
			      &mcux_ocotp_api);

DT_INST_FOREACH_STATUS_OKAY(OTP_MCUX_OCOTP_INIT)
