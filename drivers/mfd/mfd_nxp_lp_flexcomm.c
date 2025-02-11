/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lp_flexcomm

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/mfd/nxp_lp_flexcomm.h>

LOG_MODULE_REGISTER(mfd_nxp_lp_flexcomm, CONFIG_MFD_LOG_LEVEL);

struct nxp_lp_flexcomm_child {
	const struct device *dev;
	uint8_t periph;
	child_isr_t lp_flexcomm_child_isr;
};

struct nxp_lp_flexcomm_data {
	struct nxp_lp_flexcomm_child *children;
	size_t num_children;
};

struct nxp_lp_flexcomm_config {
	LP_FLEXCOMM_Type *base;
	void (*irq_config_func)(const struct device *dev);
};

void nxp_lp_flexcomm_isr(const struct device *dev)
{
	uint32_t interrupt_status;
	const struct nxp_lp_flexcomm_config *config = dev->config;
	struct nxp_lp_flexcomm_data *data = dev->data;
	uint32_t instance = LP_FLEXCOMM_GetInstance(config->base);
	struct nxp_lp_flexcomm_child *child;

	interrupt_status = LP_FLEXCOMM_GetInterruptStatus(instance);
	if ((interrupt_status &
	    ((uint32_t)kLPFLEXCOMM_I2cSlaveInterruptFlag |
	     (uint32_t)kLPFLEXCOMM_I2cMasterInterruptFlag)) != 0U) {
		child = &data->children[LP_FLEXCOMM_PERIPH_LPI2C];

		if (child->lp_flexcomm_child_isr != NULL) {
			child->lp_flexcomm_child_isr(child->dev);
		}
	}
	if ((interrupt_status &
	     ((uint32_t)kLPFLEXCOMM_UartRxInterruptFlag |
	      (uint32_t)kLPFLEXCOMM_UartTxInterruptFlag)) != 0U) {
		child = &data->children[LP_FLEXCOMM_PERIPH_LPUART];

		if (child->lp_flexcomm_child_isr != NULL) {
			child->lp_flexcomm_child_isr(child->dev);
		}
	}
	if (((interrupt_status &
	     (uint32_t)kLPFLEXCOMM_SpiInterruptFlag)) != 0U) {
		child = &data->children[LP_FLEXCOMM_PERIPH_LPSPI];

		if (child->lp_flexcomm_child_isr != NULL) {
			child->lp_flexcomm_child_isr(child->dev);
		}
	}
}

void nxp_lp_flexcomm_setirqhandler(const struct device *dev, const struct device *child_dev,
				   LP_FLEXCOMM_PERIPH_T periph, child_isr_t handler)
{
	struct nxp_lp_flexcomm_data *data = dev->data;
	struct nxp_lp_flexcomm_child *child;

	child = &data->children[periph];

	/* Store the interrupt handler and the child device node */
	child->lp_flexcomm_child_isr = handler;
	child->dev = child_dev;
}

static int nxp_lp_flexcomm_init(const struct device *dev)
{
	const struct nxp_lp_flexcomm_config *config = dev->config;
	struct nxp_lp_flexcomm_data *data = dev->data;
	uint32_t instance;
	struct nxp_lp_flexcomm_child *child = NULL;
	bool spi_found = false;
	bool uart_found = false;
	bool i2c_found = false;

	for (int i = 1; i < data->num_children; i++) {
		child = &data->children[i];
		if (child->periph == LP_FLEXCOMM_PERIPH_LPSPI) {
			spi_found = true;
		}
		if (child->periph == LP_FLEXCOMM_PERIPH_LPI2C) {
			i2c_found = true;
		}
		if (child->periph == LP_FLEXCOMM_PERIPH_LPUART) {
			uart_found = true;
		}
	}

	/* If SPI is enabled with another interface type return an error */
	if (spi_found && (i2c_found || uart_found)) {
		return -EINVAL;
	}

	instance = LP_FLEXCOMM_GetInstance(config->base);

	if (uart_found && i2c_found) {
		LP_FLEXCOMM_Init(instance, LP_FLEXCOMM_PERIPH_LPI2CAndLPUART);
	} else if (uart_found) {
		LP_FLEXCOMM_Init(instance, LP_FLEXCOMM_PERIPH_LPUART);
	} else if (i2c_found) {
		LP_FLEXCOMM_Init(instance, LP_FLEXCOMM_PERIPH_LPI2C);
	} else if (spi_found) {
		LP_FLEXCOMM_Init(instance, LP_FLEXCOMM_PERIPH_LPSPI);
	}

	config->irq_config_func(dev);

	return 0;
}

#define MCUX_FLEXCOMM_CHILD_INIT(child_node_id)					\
	[DT_NODE_CHILD_IDX(child_node_id) + 1] = {				\
		.periph = DT_NODE_CHILD_IDX(child_node_id) + 1,			\
	},

#define NXP_LP_FLEXCOMM_INIT(n)						\
										\
	static struct nxp_lp_flexcomm_child					\
		nxp_lp_flexcomm_children_##n[LP_FLEXCOMM_PERIPH_LPI2C + 1] = {	\
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, MCUX_FLEXCOMM_CHILD_INIT)	\
	};									\
										\
	static void nxp_lp_flexcomm_config_func_##n(const struct device *dev);	\
										\
	static const struct nxp_lp_flexcomm_config nxp_lp_flexcomm_config_##n = { \
		.base = (LP_FLEXCOMM_Type *)DT_INST_REG_ADDR(n),		\
		.irq_config_func = nxp_lp_flexcomm_config_func_##n,		\
	};									\
										\
	static struct nxp_lp_flexcomm_data nxp_lp_flexcomm_data_##n = {		\
		.children = nxp_lp_flexcomm_children_##n,			\
		.num_children = ARRAY_SIZE(nxp_lp_flexcomm_children_##n),	\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			    &nxp_lp_flexcomm_init,				\
			    NULL,						\
			    &nxp_lp_flexcomm_data_##n,				\
			    &nxp_lp_flexcomm_config_##n,			\
			    PRE_KERNEL_1,					\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,		\
			    NULL);						\
										\
	static void nxp_lp_flexcomm_config_func_##n(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			    nxp_lp_flexcomm_isr, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQN(n));					\
	}

DT_INST_FOREACH_STATUS_OKAY(NXP_LP_FLEXCOMM_INIT)
