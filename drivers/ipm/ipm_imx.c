/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_mu

#include <errno.h>
#include <string.h>
#include <device.h>
#include <soc.h>
#include <drivers/ipm.h>
#include <mu_imx.h>

#define MU(config) ((MU_Type *)config->base)

#if ((CONFIG_IPM_IMX_MAX_DATA_SIZE % 4) != 0)
#error CONFIG_IPM_IMX_MAX_DATA_SIZE is invalid
#endif

#define IMX_IPM_DATA_REGS (CONFIG_IPM_IMX_MAX_DATA_SIZE / 4)

struct imx_mu_config {
	MU_Type *base;
	void (*irq_config_func)(const struct device *dev);
};

struct imx_mu_data {
	ipm_callback_t callback;
	void *user_data;
};

static void imx_mu_isr(const struct device *dev)
{
	const struct imx_mu_config *config = dev->config;
	MU_Type *base = MU(config);
	struct imx_mu_data *data = dev->data;
	uint32_t data32[IMX_IPM_DATA_REGS];
	uint32_t status_reg;
	int32_t id;
	int32_t i;
	bool all_registers_full;

	status_reg = base->SR >>= MU_SR_RFn_SHIFT;

	for (id = CONFIG_IPM_IMX_MAX_ID_VAL; id >= 0; id--) {
		if (status_reg & 0x1U) {
			/*
			 * Check if all receive registers are full. If not,
			 * it is violation of the protocol (status register
			 * are set earlier than all receive registers).
			 * Do not read any of the registers in such situation.
			 */
			all_registers_full = true;
			for (i = 0; i < IMX_IPM_DATA_REGS; i++) {
				if (!MU_IsRxFull(base,
						(id * IMX_IPM_DATA_REGS) + i)) {
					all_registers_full = false;
					break;
				}
			}
			if (all_registers_full) {
				for (i = 0; i < IMX_IPM_DATA_REGS; i++) {
					MU_ReceiveMsg(base,
						(id * IMX_IPM_DATA_REGS) + i,
						&data32[i]);
				}

				if (data->callback) {
					data->callback(dev, data->user_data,
						       (uint32_t)id,
						       &data32[0]);
				}
			}
		}
		status_reg >>= IMX_IPM_DATA_REGS;
	}

	/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F
	 * Store immediate overlapping exception return operation
	 * might vector to incorrect interrupt
	 */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
	__DSB();
#endif
}

static int imx_mu_ipm_send(const struct device *dev, int wait, uint32_t id,
			   const void *data, int size)
{
	const struct imx_mu_config *config = dev->config;
	MU_Type *base = MU(config);
	uint32_t data32[IMX_IPM_DATA_REGS];
	mu_status_t status;
	int i;

	if (id > CONFIG_IPM_IMX_MAX_ID_VAL) {
		return -EINVAL;
	}

	if (size > CONFIG_IPM_IMX_MAX_DATA_SIZE) {
		return -EMSGSIZE;
	}

	/* Actual message is passing using 32 bits registers */
	memcpy(data32, data, size);

	for (i = 0; i < IMX_IPM_DATA_REGS; i++) {
		status = MU_TrySendMsg(base, id * IMX_IPM_DATA_REGS + i,
				       data32[i]);
		if (status == kStatus_MU_TxNotEmpty) {
			return -EBUSY;
		}
	}

	if (wait) {
		while (!MU_IsTxEmpty(base,
			(id * IMX_IPM_DATA_REGS) + IMX_IPM_DATA_REGS - 1)) {
		}
	}

	return 0;
}

static int imx_mu_ipm_max_data_size_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CONFIG_IPM_IMX_MAX_DATA_SIZE;
}

static uint32_t imx_mu_ipm_max_id_val_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return CONFIG_IPM_IMX_MAX_ID_VAL;
}

static void imx_mu_ipm_register_callback(const struct device *dev,
					 ipm_callback_t cb,
					 void *user_data)
{
	struct imx_mu_data *driver_data = dev->data;

	driver_data->callback = cb;
	driver_data->user_data = user_data;
}

static int imx_mu_ipm_set_enabled(const struct device *dev, int enable)
{
	const struct imx_mu_config *config = dev->config;
	MU_Type *base = MU(config);

#if CONFIG_IPM_IMX_MAX_DATA_SIZE_4
	if (enable) {
		MU_EnableRxFullInt(base, 0U);
		MU_EnableRxFullInt(base, 1U);
		MU_EnableRxFullInt(base, 2U);
		MU_EnableRxFullInt(base, 3U);
	} else {
		MU_DisableRxFullInt(base, 0U);
		MU_DisableRxFullInt(base, 1U);
		MU_DisableRxFullInt(base, 2U);
		MU_DisableRxFullInt(base, 3U);
	}
#elif CONFIG_IPM_IMX_MAX_DATA_SIZE_8
	if (enable) {
		MU_EnableRxFullInt(base, 1U);
		MU_EnableRxFullInt(base, 3U);
	} else {
		MU_DisableRxFullInt(base, 1U);
		MU_DisableRxFullInt(base, 3U);
	}
#elif CONFIG_IPM_IMX_MAX_DATA_SIZE_16
	if (enable) {
		MU_EnableRxFullInt(base, 3U);
	} else {
		MU_DisableRxFullInt(base, 3U);
	}
#else
#error "CONFIG_IPM_IMX_MAX_DATA_SIZE_n is not set"
#endif

	return 0;
}

static int imx_mu_init(const struct device *dev)
{
	const struct imx_mu_config *config = dev->config;

	MU_Init(MU(config));
	config->irq_config_func(dev);

	return 0;
}

static const struct ipm_driver_api imx_mu_driver_api = {
	.send = imx_mu_ipm_send,
	.register_callback = imx_mu_ipm_register_callback,
	.max_data_size_get = imx_mu_ipm_max_data_size_get,
	.max_id_val_get = imx_mu_ipm_max_id_val_get,
	.set_enabled = imx_mu_ipm_set_enabled
};

/* Config MU */

static void imx_mu_config_func_b(const struct device *dev);

static const struct imx_mu_config imx_mu_b_config = {
	.base = (MU_Type *)DT_INST_REG_ADDR(0),
	.irq_config_func = imx_mu_config_func_b,
};

static struct imx_mu_data imx_mu_b_data;

DEVICE_DT_INST_DEFINE(0,
		    &imx_mu_init,
		    device_pm_control_nop,
		    &imx_mu_b_data, &imx_mu_b_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &imx_mu_driver_api);

static void imx_mu_config_func_b(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    imx_mu_isr, DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));
}
