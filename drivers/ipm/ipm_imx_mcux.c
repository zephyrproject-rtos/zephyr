/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT nxp_imx_mcux_mu

#include <errno.h>
#include <string.h>
#include <device.h>
#include <soc.h>
#include <drivers/ipm.h>
#include <fsl_mu.h>

#define 	IMX_IPM_DATA_REGS 1
#define 	RPMSG_MU_CHANNEL (1)

struct mu_mcux_config {
	MU_Type *base;
	void (*irq_config_func)(const struct device *dev);
};

struct mu_mcux_data {
	ipm_callback_t callback;
	void *callback_ctx;
};

#define DEV_CFG(dev) \
	((const struct mu_mcux_config * const)(dev)->config)
#define DEV_DATA(dev) \
	((struct mu_mcux_data * const)(dev)->data)
#define DEV_BASE(dev) \
	((MU_Type *)(DEV_CFG(dev))->base)

static void mu_mcux_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	MU_Type *base = DEV_BASE(dev);
	struct mu_mcux_data *data = DEV_DATA(dev);
	volatile uint32_t value;

	value = MU_ReceiveMsg(base, RPMSG_MU_CHANNEL);

	if (data->callback) {
			//callback_ctx-> input to ISR, value-> output from ISR
		data->callback(dev, data->callback_ctx, RPMSG_MU_CHANNEL, &value);
	}

	/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F
	 * Store immediate overlapping exception return operation
	 * might vector to incorrect interrupt
	 */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
	__DSB();
#endif
}

static int mu_mcux_ipm_send(const struct device *dev, int wait, uint32_t id,
			   const void *data, int size)
{
	MU_Type *base = DEV_BASE(dev);
	uint32_t data32[IMX_IPM_DATA_REGS];
	int i;

	if (id > CONFIG_IPM_IMX_MCUX_MAX_ID_VAL) {
		return -EINVAL;
	}

	if (size > CONFIG_IPM_IMX_MCUX_MAX_DATA_SIZE) {
		return -EMSGSIZE;
	}

	/* Actual message is passing using 32 bits registers */
	memcpy(data32, data, size);

	for (i = 0; i < ARRAY_SIZE(data32); ++i) {
		MU_SendMsg(base, id, data32[i]);
	}

	return 0;
}

static int mu_mcux_ipm_max_data_size_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CONFIG_IPM_IMX_MCUX_MAX_DATA_SIZE;
}

static uint32_t mu_mcux_ipm_max_id_val_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CONFIG_IPM_IMX_MCUX_MAX_ID_VAL;
}

static void mu_mcux_ipm_register_callback(const struct device *dev,
					 ipm_callback_t cb,
					 void *context)
{
	struct mu_mcux_data *data = DEV_DATA(dev);

	data->callback = cb;
	data->callback_ctx = context;
}

static int mu_mcux_ipm_set_enabled(const struct device *dev, int enable)
{
	return 0;
}

static int mu_mcux_init(const struct device *dev)
{
	MU_Type *base = DEV_BASE(dev);
	const struct mu_mcux_config *config = DEV_CFG(dev);

	MU_Init(base);
	config->irq_config_func(dev);

	return 0;
}

static const struct ipm_driver_api mu_mcux_driver_api = {
	.send = mu_mcux_ipm_send,
	.register_callback = mu_mcux_ipm_register_callback,
	.max_data_size_get = mu_mcux_ipm_max_data_size_get,
	.max_id_val_get = mu_mcux_ipm_max_id_val_get,
	.set_enabled = mu_mcux_ipm_set_enabled
};

/* Config MU */

static void mu_mcux_config_func0(const struct device *dev);

static const struct mu_mcux_config mu_mcux_config0 = {
	.base = (MU_Type *)DT_INST_REG_ADDR(0),
	.irq_config_func = mu_mcux_config_func0,
};

static struct mu_mcux_data mu_mcux_data0;

DEVICE_DT_INST_DEFINE(0,
		    &mu_mcux_init,
		    NULL,
		    &mu_mcux_data0, &mu_mcux_config0,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &mu_mcux_driver_api);

static void mu_mcux_config_func0(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    mu_mcux_isr, DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));
}

