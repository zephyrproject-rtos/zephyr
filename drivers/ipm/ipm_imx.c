/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/irq.h>
#if defined(CONFIG_IPM_IMX_REV2)
#define DT_DRV_COMPAT nxp_imx_mu_rev2
#include "fsl_mu.h"
#else
#define DT_DRV_COMPAT nxp_imx_mu
#include <mu_imx.h>
#endif

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

#if defined(CONFIG_IPM_IMX_REV2)
/*!
 * @brief Check RX full status.
 *
 * This function checks the specific receive register full status.
 *
 * @param base Register base address for the module.
 * @param index RX register index to check.
 * @retval true RX register is full.
 * @retval false RX register is not full.
 */
static inline bool MU_IsRxFull(MU_Type *base, uint32_t index)
{
	switch (index) {
	case 0:
		return (bool)(MU_GetStatusFlags(base) & kMU_Rx0FullFlag);
	case 1:
		return (bool)(MU_GetStatusFlags(base) & kMU_Rx1FullFlag);
	case 2:
		return (bool)(MU_GetStatusFlags(base) & kMU_Rx2FullFlag);
	case 3:
		return (bool)(MU_GetStatusFlags(base) & kMU_Rx3FullFlag);
	default:
		/* This shouldn't happen */
		assert(false);
		return false;
	}
}

/*!
 * @brief Check TX empty status.
 *
 * This function checks the specific transmit register empty status.
 *
 * @param base Register base address for the module.
 * @param index TX register index to check.
 * @retval true TX register is empty.
 * @retval false TX register is not empty.
 */
static inline bool MU_IsTxEmpty(MU_Type *base, uint32_t index)
{
	switch (index) {
	case 0:
		return (bool)(MU_GetStatusFlags(base) & kMU_Tx0EmptyFlag);
	case 1:
		return (bool)(MU_GetStatusFlags(base) & kMU_Tx1EmptyFlag);
	case 2:
		return (bool)(MU_GetStatusFlags(base) & kMU_Tx2EmptyFlag);
	case 3:
		return (bool)(MU_GetStatusFlags(base) & kMU_Tx3EmptyFlag);
	default:
		/* This shouldn't happen */
		assert(false);
		return false;
	}
}
#endif

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
#if defined(CONFIG_IPM_IMX_REV2)
					data32[i] = MU_ReceiveMsg(base,
						(id * IMX_IPM_DATA_REGS) + i);
#else
					MU_ReceiveMsg(base,
						(id * IMX_IPM_DATA_REGS) + i,
						&data32[i]);
#endif
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
	 * might vector to incorrect interrupt. For Cortex-M7, if
	 * core speed much faster than peripheral register write
	 * speed, the peripheral interrupt flags may be still set
	 * after exiting ISR, this results to the same error similar
	 * with errata 838869.
	 */
#if (defined __CORTEX_M) && ((__CORTEX_M == 4U) || (__CORTEX_M == 7U))
	__DSB();
#endif
}

static int imx_mu_ipm_send(const struct device *dev, int wait, uint32_t id,
			   const void *data, int size)
{
	const struct imx_mu_config *config = dev->config;
	MU_Type *base = MU(config);
	uint32_t data32[IMX_IPM_DATA_REGS];
#if !IS_ENABLED(CONFIG_IPM_IMX_REV2)
	mu_status_t status;
#endif
	int i;

	if (id > CONFIG_IPM_IMX_MAX_ID_VAL) {
		return -EINVAL;
	}

	if (size > CONFIG_IPM_IMX_MAX_DATA_SIZE) {
		return -EMSGSIZE;
	}

	/* Actual message is passing using 32 bits registers */
	memcpy(data32, data, size);

#if defined(CONFIG_IPM_IMX_REV2)
	if (wait) {
		for (i = 0; i < IMX_IPM_DATA_REGS; i++) {
			MU_SendMsgNonBlocking(base, id * IMX_IPM_DATA_REGS + i,
								  data32[i]);
		}
		while (!MU_IsTxEmpty(base,
			(id * IMX_IPM_DATA_REGS) + IMX_IPM_DATA_REGS - 1)) {
		}
	} else {
		for (i = 0; i < IMX_IPM_DATA_REGS; i++) {
			if (MU_IsTxEmpty(base, id * IMX_IPM_DATA_REGS + i)) {
				MU_SendMsg(base, id * IMX_IPM_DATA_REGS + i,
						   data32[i]);
			} else {
				return -EBUSY;
			}
		}
	}

#else
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
#endif

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
#if defined(CONFIG_IPM_IMX_REV2)
#if CONFIG_IPM_IMX_MAX_DATA_SIZE_4
	if (enable) {
		MU_EnableInterrupts(base, kMU_Rx0FullInterruptEnable);
		MU_EnableInterrupts(base, kMU_Rx1FullInterruptEnable);
		MU_EnableInterrupts(base, kMU_Rx2FullInterruptEnable);
		MU_EnableInterrupts(base, kMU_Rx3FullInterruptEnable);
	} else {
		MU_DisableInterrupts(base, kMU_Rx0FullInterruptEnable);
		MU_DisableInterrupts(base, kMU_Rx1FullInterruptEnable);
		MU_DisableInterrupts(base, kMU_Rx2FullInterruptEnable);
		MU_DisableInterrupts(base, kMU_Rx3FullInterruptEnable);
	}
#elif CONFIG_IPM_IMX_MAX_DATA_SIZE_8
	if (enable) {
		MU_EnableInterrupts(base, kMU_Rx1FullInterruptEnable);
		MU_EnableInterrupts(base, kMU_Rx3FullInterruptEnable);
	} else {
		MU_DisableInterrupts(base, kMU_Rx1FullInterruptEnable);
		MU_DisableInterrupts(base, kMU_Rx3FullInterruptEnable);
	}
#elif CONFIG_IPM_IMX_MAX_DATA_SIZE_16
	if (enable) {
		MU_EnableInterrupts(base, kMU_Rx3FullInterruptEnable);
	} else {
		MU_DisableInterrupts(base, kMU_Rx3FullInterruptEnable);
	}
#else
#error "CONFIG_IPM_IMX_MAX_DATA_SIZE_n is not set"
#endif
#else
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
		    NULL,
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
