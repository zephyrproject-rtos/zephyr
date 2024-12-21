/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/policy.h>
#include <zephyr/drivers/dma/dma_mcux_smartdma.h>

#include <soc.h>
#include <fsl_smartdma.h>
#include <fsl_inputmux.h>
#include <fsl_power.h>

#define DT_DRV_COMPAT nxp_smartdma

LOG_MODULE_REGISTER(dma_mcux_smartdma, CONFIG_DMA_LOG_LEVEL);

struct dma_mcux_smartdma_config {
	SMARTDMA_Type *base;
	void (*irq_config_func)(const struct device *dev);

	void (**smartdma_progs)(void);
};

struct dma_mcux_smartdma_data {
	/* Installed DMA callback and user data */
	dma_callback_t callback;
	void *user_data;
};

/* Seems to be written to smartDMA control register when it is configured */
#define SMARTDMA_MAGIC 0xC0DE0000U
/* These bits are set when the SMARTDMA boots, cleared to reset it */
#define SMARTDMA_BOOT 0x11

/* Configure a channel */
static int dma_mcux_smartdma_configure(const struct device *dev,
				uint32_t channel, struct dma_config *config)
{
	const struct dma_mcux_smartdma_config *dev_config = dev->config;
	struct dma_mcux_smartdma_data *data = dev->data;
	uint32_t prog_idx = config->dma_slot;

	/* SMARTDMA does not have channels */
	ARG_UNUSED(channel);

	data->callback = config->dma_callback;
	data->user_data = config->user_data;

	/* Reset smartDMA */
	SMARTDMA_Reset();

	/* Write the head block pointer directly to SMARTDMA */
	dev_config->base->ARM2EZH = (uint32_t)config->head_block;
	/* Save program */
	dev_config->base->BOOTADR = (uint32_t)dev_config->smartdma_progs[prog_idx];
	LOG_DBG("Boot address set to 0x%X", dev_config->base->BOOTADR);
	return 0;
}

static int dma_mcux_smartdma_start(const struct device *dev, uint32_t channel)
{
	const struct dma_mcux_smartdma_config *config = dev->config;

	/* Block PM transition until DMA completes */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	/* Kick off SMARTDMA */
	config->base->CTRL = SMARTDMA_MAGIC | SMARTDMA_BOOT;

	return 0;
}


static int dma_mcux_smartdma_stop(const struct device *dev, uint32_t channel)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);

	/* Stop DMA  */
	SMARTDMA_Reset();

	/* Release PM lock */
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	return 0;
}

static int dma_mcux_smartdma_init(const struct device *dev)
{
	const struct dma_mcux_smartdma_config *config = dev->config;
	SMARTDMA_InitWithoutFirmware();
	config->irq_config_func(dev);

	return 0;
}

static void dma_mcux_smartdma_irq(const struct device *dev)
{
	const struct dma_mcux_smartdma_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->user_data, 0, 0);
	}

	/* Release PM lock */
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
}

/**
 * @brief install SMARTDMA firmware
 *
 * Install a custom firmware for the smartDMA. This function allows the user
 * to install a custom firmware into the smartDMA, which implements
 * different API functions than the standard MCUX SDK firmware.
 * @param dev: smartDMA device
 * @param firmware: address of buffer containing smartDMA firmware
 * @param len: length of firmware buffer
 */
void dma_smartdma_install_fw(const struct device *dev, uint8_t *firmware,
			     uint32_t len)
{
	const struct dma_mcux_smartdma_config *config = dev->config;

	SMARTDMA_InstallFirmware((uint32_t)config->smartdma_progs, firmware, len);
}

static const struct dma_driver_api dma_mcux_smartdma_api = {
	.config = dma_mcux_smartdma_configure,
	.start = dma_mcux_smartdma_start,
	.stop = dma_mcux_smartdma_stop,
};


#define SMARTDMA_INIT(n)							\
	static void dma_mcux_smartdma_config_func_##n(const struct device *dev) \
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
				dma_mcux_smartdma_irq,				\
				DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static const struct dma_mcux_smartdma_config smartdma_##n##_config = {	\
		.base = (SMARTDMA_Type *)DT_INST_REG_ADDR(n),			\
		.smartdma_progs = (void (**)(void))DT_INST_PROP(n, program_mem),\
		.irq_config_func = dma_mcux_smartdma_config_func_##n,		\
	};									\
	static struct dma_mcux_smartdma_data smartdma_##n##_data;		\
										\
	DEVICE_DT_INST_DEFINE(n,						\
				&dma_mcux_smartdma_init,			\
				NULL,						\
				&smartdma_##n##_data, &smartdma_##n##_config,	\
				POST_KERNEL, CONFIG_DMA_INIT_PRIORITY,		\
				&dma_mcux_smartdma_api);

DT_INST_FOREACH_STATUS_OKAY(SMARTDMA_INIT)
