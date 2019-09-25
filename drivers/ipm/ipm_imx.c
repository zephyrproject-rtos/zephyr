/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
	void (*irq_config_func)(struct device *dev);
};

struct imx_mu_data {
	ipm_callback_t callback;
	void *callback_ctx;
};

static void imx_mu_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct imx_mu_config *config = dev->config->config_info;
	MU_Type *base = MU(config);
	struct imx_mu_data *data = dev->driver_data;
	u32_t data32[IMX_IPM_DATA_REGS];
	u32_t status_reg;
	s32_t id;
	s32_t i;
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
					data->callback(data->callback_ctx,
						       (u32_t)id,
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

static int imx_mu_ipm_send(struct device *dev, int wait, u32_t id,
			   const void *data, int size)
{
	const struct imx_mu_config *config = dev->config->config_info;
	MU_Type *base = MU(config);
	u32_t data32[IMX_IPM_DATA_REGS];
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

static int imx_mu_ipm_max_data_size_get(struct device *dev)
{
	ARG_UNUSED(dev);

	return CONFIG_IPM_IMX_MAX_DATA_SIZE;
}

static u32_t imx_mu_ipm_max_id_val_get(struct device *dev)
{
	ARG_UNUSED(dev);

	return CONFIG_IPM_IMX_MAX_ID_VAL;
}

static void imx_mu_ipm_register_callback(struct device *dev,
					 ipm_callback_t cb,
					 void *context)
{
	struct imx_mu_data *driver_data = dev->driver_data;

	driver_data->callback = cb;
	driver_data->callback_ctx = context;
}

static int imx_mu_ipm_set_enabled(struct device *dev, int enable)
{
	const struct imx_mu_config *config = dev->config->config_info;
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

static int imx_mu_init(struct device *dev)
{
	const struct imx_mu_config *config = dev->config->config_info;

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

static void imx_mu_config_func_b(struct device *dev);

static const struct imx_mu_config imx_mu_b_config = {
	.base = (MU_Type *)DT_IPM_IMX_MU_B_BASE_ADDRESS,
	.irq_config_func = imx_mu_config_func_b,
};

static struct imx_mu_data imx_mu_b_data;

DEVICE_AND_API_INIT(mu_b, DT_IPM_IMX_MU_B_NAME,
		    &imx_mu_init,
		    &imx_mu_b_data, &imx_mu_b_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &imx_mu_driver_api);

static void imx_mu_config_func_b(struct device *dev)
{
	IRQ_CONNECT(DT_IPM_IMX_MU_B_IRQ,
		    DT_IPM_IMX_MU_B_IRQ_PRI,
		    imx_mu_isr, DEVICE_GET(mu_b), 0);

	irq_enable(DT_IPM_IMX_MU_B_IRQ);
}
