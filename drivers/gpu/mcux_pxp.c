/*
 * Copyright (c) 2019, Linaro Limited
 * Copyright (c) 2022, Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_pxp

#include <zephyr.h>

#include <drivers/gpu.h>

#include "fsl_pxp.h"

struct mcux_pxp_config {
	PXP_Type *base;
};

struct mcux_pxp_data {
	struct k_sem sem;
};

static int mcux_pxp_start(const struct device *dev)
{
	struct mcux_pxp_config *config = (struct mcux_pxp_config *)dev->config;

	PXP_EnableInterrupts(config->base, kPXP_CompleteInterruptEnable);
	PXP_Start(config->base);

	return 0;
}

static int mcux_pxp_wait_complete(const struct device *dev)
{
	struct mcux_pxp_data *data = (struct mcux_pxp_data *)dev->data;

	k_sem_take(&data->sem, K_FOREVER);

	return 0;
}

static int mcux_pxp_stop(const struct device *dev)
{
	const struct mcux_pxp_config *config = (struct mcux_pxp_config *)dev->config;

	PXP_Deinit(config->base);

	return 0;
}

static const struct gpu_driver_api mcux_pxp_driver_api = {
	.start = mcux_pxp_start,
	.wait_complete = mcux_pxp_wait_complete,
	.stop = mcux_pxp_stop,
};

static void mcux_pxp_isr(struct device *dev)
{
	const struct mcux_pxp_config *config = (struct mcux_pxp_config *)dev->config;
	struct mcux_pxp_data *data = dev->data;
	uint32_t status;

	status = PXP_GetStatusFlags(config->base);
	PXP_ClearStatusFlags(config->base, status);

	k_sem_give(&data->sem);
}

static int mcux_pxp_init(const struct device *dev)
{
	const struct mcux_pxp_config *config = (struct mcux_pxp_config *)dev->config;
	struct mcux_pxp_data *data = (struct mcux_pxp_data *)dev->data;

	k_sem_init(&data->sem, 0, 1);

	PXP_Init(config->base);

	return 0;
}

static const struct mcux_pxp_config mcux_pxp_config_0 = {
	.base = (PXP_Type *)DT_INST_REG_ADDR(0),
};

static struct mcux_pxp_data mcux_pxp_data_0;

static int mcux_pxp_init_0(const struct device *dev)
{

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    mcux_pxp_isr, DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));

	return mcux_pxp_init(dev);
}

DEVICE_DT_INST_DEFINE(0, &mcux_pxp_init_0,
		      device_pm_control_nop, &mcux_pxp_data_0,
		      &mcux_pxp_config_0,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &mcux_pxp_driver_api);
