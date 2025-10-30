/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx93_mediamix

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mediamix, CONFIG_MEDIAMIX_LOG_LEVEL);

#include <fsl_common.h>

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev)  ((const struct mcux_mediamix_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mcux_mediamix_data *)(_dev)->data)

struct mcux_mediamix_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	uint32_t video_pll;
};

struct mcux_mediamix_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
};

static void imx93_mediamix_set_qos_isi(MEDIAMIX_BLK_CTRL_Type *base)
{
	uint32_t reg = 0;

	reg |= MEDIAMIX_BLK_CTRL_ISI1_DEFAULT_QOS_V(0x3) | MEDIAMIX_BLK_CTRL_ISI1_CFG_QOS_V(0x7) |
	       MEDIAMIX_BLK_CTRL_ISI1_DEFAULT_QOS_U(0x3) | MEDIAMIX_BLK_CTRL_ISI1_CFG_QOS_U(0x7) |
	       MEDIAMIX_BLK_CTRL_ISI1_DEFAULT_QOS_Y_R(0x3) |
	       MEDIAMIX_BLK_CTRL_ISI1_CFG_QOS_Y_R(0x7) |
	       MEDIAMIX_BLK_CTRL_ISI1_DEFAULT_QOS_Y_W(0x3) |
	       MEDIAMIX_BLK_CTRL_ISI1_CFG_QOS_Y_W(0x7);
	base->BUS_CONTROL.ISI1 = reg;
}

static const struct mcux_mediamix_config mcux_mediamix_config_0 = {
	DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(0)),
};

static struct mcux_mediamix_config mcux_mediamix_data_0;

static int mcux_mediamix_init_0(const struct device *dev)
{
	MEDIAMIX_BLK_CTRL_Type *base;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);
	base = (MEDIAMIX_BLK_CTRL_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	imx93_mediamix_set_qos_isi(base);

	LOG_INF("%s init succeeded", dev->name);
	return 0;
}

DEVICE_DT_INST_DEFINE(0, mcux_mediamix_init_0, NULL, &mcux_mediamix_data_0, &mcux_mediamix_config_0,
		      POST_KERNEL, CONFIG_MEDIAMIX_BLK_CTRL_INIT_PRIORITY, NULL);
