/*
 * Copyright (c) 2025 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_ocotp

#include <zephyr/drivers/syscon.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(imx_ocotp, CONFIG_SYSCON_LOG_LEVEL);

#include <fsl_ocotp.h>

struct imx_ocotp_config {
	OCOTP_Type *base;
	size_t size;
};

static int imx_ocotp_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct imx_ocotp_config *cfg = dev->config;
	status_t status;

	status = OCOTP_ReadFuseShadowRegisterExt(cfg->base, reg, val, 1);

	return status == kStatus_Success ? 0 : -EIO;
}

static int imx_ocotp_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct imx_ocotp_config *cfg = dev->config;
	status_t status;

	status = OCOTP_WriteFuseShadowRegister(cfg->base, reg, val);

	return status == kStatus_Success ? 0 : -EIO;
}

static int imx_ocotp_get_base(const struct device *dev, uintptr_t *addr)
{
	const struct imx_ocotp_config *cfg = dev->config;

	*addr = (uintptr_t)cfg->base;

	return 0;
}

static int imx_ocotp_get_size(const struct device *dev, size_t *size)
{
	const struct imx_ocotp_config *cfg = dev->config;

	*size = cfg->size;

	return 0;
}

static int imx_ocotp_get_reg_width(const struct device *dev)
{
	ARG_UNUSED(dev);

	return sizeof(uint32_t);
}

static DEVICE_API(syscon, imx_ocotp_driver_api) = {
	.read = imx_ocotp_read_reg,
	.write = imx_ocotp_write_reg,
	.get_base = imx_ocotp_get_base,
	.get_size = imx_ocotp_get_size,
	.get_reg_width = imx_ocotp_get_reg_width,
};

static int imx_ocotp_init(const struct device *dev)
{
	const struct imx_ocotp_config *cfg = dev->config;

	/* FIXME: Better clock control support possible? */
#if defined(CONFIG_SOC_SERIES_IMXRT10XX)
	OCOTP_Init(cfg->base, CLOCK_GetIpgFreq());
#elif defined(CONFIG_SOC_SERIES_IMXRT11XX)
	OCOTP_Init(cfg->base, 0);
#else
#error "Unsupported SoC"
#endif

	return 0;
}

#define SYSCON_INIT(inst)                                                                          \
	static const struct imx_ocotp_config imx_ocotp_config_##inst = {                           \
		.base = (OCOTP_Type *)DT_INST_REG_ADDR(inst),                                      \
		.size = DT_INST_PROP(inst, size),                                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, imx_ocotp_init, NULL, NULL, &imx_ocotp_config_##inst,          \
			      PRE_KERNEL_1, CONFIG_SYSCON_INIT_PRIORITY, &imx_ocotp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SYSCON_INIT);
