/*
 * Copyright (c) 2021 BrainCo Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_hsem_mailbox

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include "stm32_hsem.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(ipm_stm32_hsem, CONFIG_IPM_LOG_LEVEL);

#define HSEM_CPU1                   1
#define HSEM_CPU2                   2

#if CONFIG_IPM_STM32_HSEM_CPU == HSEM_CPU1
#define ll_hsem_enableit_cier       LL_HSEM_EnableIT_C1IER
#define ll_hsem_disableit_cier      LL_HSEM_DisableIT_C1IER
#define ll_hsem_clearflag_cicr      LL_HSEM_ClearFlag_C1ICR
#define ll_hsem_isactiveflag_cmisr   LL_HSEM_IsActiveFlag_C1MISR
#else /* HSEM_CPU2 */
#define ll_hsem_enableit_cier       LL_HSEM_EnableIT_C2IER
#define ll_hsem_disableit_cier      LL_HSEM_DisableIT_C2IER
#define ll_hsem_clearflag_cicr      LL_HSEM_ClearFlag_C2ICR
#define ll_hsem_isactiveflag_cmisr   LL_HSEM_IsActiveFlag_C2MISR
#endif /* CONFIG_IPM_STM32_HSEM_CPU */

struct stm32_hsem_mailbox_config {
	void (*irq_config_func)(const struct device *dev);
	struct stm32_pclken pclken;
};

struct stm32_hsem_mailbox_data {
	uint32_t tx_semid;
	uint32_t rx_semid;
	ipm_callback_t callback;
	void *user_data;
};

static struct stm32_hsem_mailbox_data stm32_hsem_mailbox_0_data;

void stm32_hsem_mailbox_ipm_rx_isr(const struct device *dev)
{
	struct stm32_hsem_mailbox_data *data = dev->data;
	uint32_t mask_semid = (1U << data->rx_semid);

	/* Check semaphore rx_semid interrupt status */
	if (!ll_hsem_isactiveflag_cmisr(HSEM, mask_semid))
		return;

	/* Notify user with NULL data pointer */
	if (data->callback) {
		data->callback(dev, data->user_data, 0, NULL);
	}

	/* Clear semaphore rx_semid interrupt status and masked status */
	ll_hsem_clearflag_cicr(HSEM, mask_semid);
}

static void stm32_hsem_mailbox_irq_config_func(const struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    stm32_hsem_mailbox_ipm_rx_isr, DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));
}

int stm32_hsem_mailbox_ipm_send(const struct device *dev, int wait, uint32_t id,
			  const void *buff, int size)
{
	struct stm32_hsem_mailbox_data *data = dev->data;

	ARG_UNUSED(wait);
	ARG_UNUSED(buff);

	if (size) {
		LOG_WRN("stm32 HSEM not support data transfer");
		return -EMSGSIZE;
	}

	if (id) {
		LOG_WRN("stm32 HSEM only support a single instance of mailbox");
		return -EINVAL;
	}

	/* Lock the semaphore tx_semid */
	z_stm32_hsem_lock(data->tx_semid, HSEM_LOCK_DEFAULT_RETRY);

	/**
	 * Release the semaphore tx_semid.
	 * This will trigger a HSEMx interrupt on another CPU.
	 */
	z_stm32_hsem_unlock(data->tx_semid);

	return 0;
}

void stm32_hsem_mailbox_ipm_register_callback(const struct device *dev,
					ipm_callback_t cb,
					void *user_data)
{
	struct stm32_hsem_mailbox_data *data = dev->data;

	data->callback = cb;
	data->user_data = user_data;
}

int stm32_hsem_mailbox_ipm_max_data_size_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* stm32 HSEM not support data transfer */
	return 0;
}

uint32_t stm32_hsem_mailbox_ipm_max_id_val_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* stm32 HSEM only support a single instance of mailbox */
	return 0;
}

int stm32_hsem_mailbox_ipm_set_enabled(const struct device *dev, int enable)
{
	struct stm32_hsem_mailbox_data *data = dev->data;
	uint32_t mask_semid = (1U << data->rx_semid);

	if (enable) {
		/* Clear semaphore rx_semid interrupt status and masked status */
		ll_hsem_clearflag_cicr(HSEM, mask_semid);
		/* Enable semaphore rx_semid on HESMx interrupt */
		ll_hsem_enableit_cier(HSEM, mask_semid);
	} else {
		/* Disable semaphore rx_semid on HSEMx interrupt */
		ll_hsem_disableit_cier(HSEM, mask_semid);
	}

	return 0;
}

static int stm32_hsem_mailbox_init(const struct device *dev)
{
	struct stm32_hsem_mailbox_data *data = dev->data;
	const struct stm32_hsem_mailbox_config *cfg = dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	/* Config transfer semaphore */
	switch (CONFIG_IPM_STM32_HSEM_CPU) {
	case HSEM_CPU1:
		if (!device_is_ready(clk)) {
			LOG_ERR("clock control device not ready");
			return -ENODEV;
		}

		/* Enable clock */
		if (clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken) != 0) {
			LOG_WRN("Failed to enable clock");
			return -EIO;
		}

		data->tx_semid = CFG_HW_IPM_CPU2_SEMID;
		data->rx_semid = CFG_HW_IPM_CPU1_SEMID;
		break;
	case HSEM_CPU2:
		data->tx_semid = CFG_HW_IPM_CPU1_SEMID;
		data->rx_semid = CFG_HW_IPM_CPU2_SEMID;
		break;
	}

	cfg->irq_config_func(dev);

	return 0;
}

static const struct ipm_driver_api stm32_hsem_mailbox_ipm_dirver_api = {
	.send = stm32_hsem_mailbox_ipm_send,
	.register_callback = stm32_hsem_mailbox_ipm_register_callback,
	.max_data_size_get = stm32_hsem_mailbox_ipm_max_data_size_get,
	.max_id_val_get = stm32_hsem_mailbox_ipm_max_id_val_get,
	.set_enabled = stm32_hsem_mailbox_ipm_set_enabled,
};

static const struct stm32_hsem_mailbox_config stm32_hsem_mailbox_0_config = {
	.irq_config_func = stm32_hsem_mailbox_irq_config_func,
	.pclken = {
		.bus = DT_INST_CLOCKS_CELL(0, bus),
		.enr = DT_INST_CLOCKS_CELL(0, bits)
	},
};


/*
 * STM32 HSEM has its own LL_HSEM(low-level HSEM) API provided by the hal_stm32 module.
 * The ipm_stm32_hsem driver only picks up two semaphore IDs from stm32_hsem.h to simulate
 * a virtual mailbox device. So there will have only one instance.
 */
#define IPM_STM32_HSEM_INIT(inst)						\
	BUILD_ASSERT((inst) == 0,							\
		     "multiple instances not supported");		\
	DEVICE_DT_INST_DEFINE(0,							\
				&stm32_hsem_mailbox_init,				\
				NULL,			\
				&stm32_hsem_mailbox_0_data,				\
				&stm32_hsem_mailbox_0_config,			\
				POST_KERNEL,							\
				CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
				&stm32_hsem_mailbox_ipm_dirver_api);	\

DT_INST_FOREACH_STATUS_OKAY(IPM_STM32_HSEM_INIT)
