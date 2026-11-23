/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_sam_flexcom

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mfd_flexcomm, CONFIG_MFD_LOG_LEVEL);

struct sam_flexcomm_config {
	flexcom_registers_t *reg;
	const int mode;
};

static int sam_flexcomm_init(const struct device *dev)
{
	const struct sam_flexcomm_config *config = dev->config;

	config->reg->FLEX_MR = (config->reg->FLEX_MR & ~FLEX_MR_OPMODE_Msk) |
			       FLEX_MR_OPMODE(config->mode);

	LOG_DBG("%s set Operating Mode to %d", dev->name, config->mode);

	return 0;
}

#define SAM_FLEXCOMM_INIT(n)							\
										\
	static const struct sam_flexcomm_config sam_flexcomm_config_##n = {	\
		.reg = (flexcom_registers_t *)DT_INST_REG_ADDR(n),		\
		.mode = DT_INST_PROP(n, mchp_flexcom_mode),			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      &sam_flexcomm_init,				\
			      NULL,						\
			      NULL,						\
			      &sam_flexcomm_config_##n,				\
			      PRE_KERNEL_1,					\
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,		\
			      NULL);						\

DT_INST_FOREACH_STATUS_OKAY(SAM_FLEXCOMM_INIT)
