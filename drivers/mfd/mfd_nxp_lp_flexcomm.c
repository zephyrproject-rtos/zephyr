/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lp_flexcomm

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/mfd/nxp_lp_flexcomm.h>

LOG_MODULE_REGISTER(mfd_nxp_lp_flexcomm, CONFIG_MFD_LOG_LEVEL);

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev)  ((const struct nxp_lp_flexcomm_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct nxp_lp_flexcomm_data *)(_dev)->data)

struct nxp_lp_flexcomm_child {
	const struct device *dev;
	uint8_t periph;
	child_isr_t lp_flexcomm_child_isr;
};

struct nxp_lp_flexcomm_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	struct nxp_lp_flexcomm_child *children;
	size_t num_children;
};

struct nxp_lp_flexcomm_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	struct reset_dt_spec reset;
	void (*irq_config_func)(const struct device *dev);
};

void nxp_lp_flexcomm_isr(const struct device *dev)
{
	LP_FLEXCOMM_Type *base = (LP_FLEXCOMM_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	uint32_t interrupt_status;
	struct nxp_lp_flexcomm_data *data = dev->data;
	uint32_t instance = LP_FLEXCOMM_GetInstance(base);
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

/* Select the peripheral communications function in the LP_FLEXCOMM wrapper
 * (writes PSELID[PERSEL]) based on the enabled child nodes.
 */
static int nxp_lp_flexcomm_select_periph(const struct device *dev)
{
	struct nxp_lp_flexcomm_data *data = dev->data;
	LP_FLEXCOMM_Type *base = (LP_FLEXCOMM_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	uint32_t instance = LP_FLEXCOMM_GetInstance(base);
	bool spi_found = false;
	bool uart_found = false;
	bool i2c_found = false;

	for (int i = 1; i < data->num_children; i++) {
		struct nxp_lp_flexcomm_child *child = &data->children[i];

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

	if (uart_found && i2c_found) {
		LP_FLEXCOMM_Init(instance, LP_FLEXCOMM_PERIPH_LPI2CAndLPUART);
	} else if (uart_found) {
		LP_FLEXCOMM_Init(instance, LP_FLEXCOMM_PERIPH_LPUART);
	} else if (i2c_found) {
		LP_FLEXCOMM_Init(instance, LP_FLEXCOMM_PERIPH_LPI2C);
	} else if (spi_found) {
		LP_FLEXCOMM_Init(instance, LP_FLEXCOMM_PERIPH_LPSPI);
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int nxp_lp_flexcomm_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct nxp_lp_flexcomm_config *config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/*
		 * A low-power state may collapse the wrapper's power domain (e.g.
		 * i.MX RT700 deep-sleep-retention), resetting it: PSELID clears so
		 * no function is selected and the child module's register clock is
		 * gated. Re-deassert reset and re-select the peripheral before any
		 * child (e.g. LPUART) resume touches its registers.
		 */
		if (config->reset.dev != NULL) {
			(void)reset_line_deassert_dt(&config->reset);
		}
		return nxp_lp_flexcomm_select_periph(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static int nxp_lp_flexcomm_init(const struct device *dev)
{
	const struct nxp_lp_flexcomm_config *config = dev->config;
	int ret;

	if (config->reset.dev != NULL) {
		if (!device_is_ready(config->reset.dev)) {
			return -ENODEV;
		}

		ret = reset_line_deassert_dt(&config->reset);
		if (ret != 0) {
			return ret;
		}
	}

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	ret = nxp_lp_flexcomm_select_periph(dev);
	if (ret != 0) {
		return ret;
	}

	config->irq_config_func(dev);

#ifdef CONFIG_PM_DEVICE
	return pm_device_driver_init(dev, nxp_lp_flexcomm_pm_action);
#else
	return 0;
#endif
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
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),		\
		.reset = RESET_DT_SPEC_INST_GET_OR(n, {0}),			\
		.irq_config_func = nxp_lp_flexcomm_config_func_##n,		\
	};									\
										\
	static struct nxp_lp_flexcomm_data nxp_lp_flexcomm_data_##n = {		\
		.children = nxp_lp_flexcomm_children_##n,			\
		.num_children = ARRAY_SIZE(nxp_lp_flexcomm_children_##n),	\
	};									\
										\
	PM_DEVICE_DT_INST_DEFINE(n, nxp_lp_flexcomm_pm_action);			\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			    &nxp_lp_flexcomm_init,				\
			    PM_DEVICE_DT_INST_GET(n),				\
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
