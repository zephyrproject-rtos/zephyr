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

/* SMARTDMA peripheral registers, taken from MCUX driver implementation*/
struct smartdma_periph {
	volatile uint32_t BOOT;
	volatile uint32_t CTRL;
	volatile uint32_t PC;
	volatile uint32_t SP;
	volatile uint32_t BREAK_ADDR;
	volatile uint32_t BREAK_VECT;
	volatile uint32_t EMER_VECT;
	volatile uint32_t EMER_SEL;
	volatile uint32_t ARM2SMARTDMA;
	volatile uint32_t SMARTDMA2ARM;
	volatile uint32_t PENDTRAP;
};

struct dma_mcux_smartdma_config {
	struct smartdma_periph *base;
	void (*irq_config_func)(const struct device *dev);

	void (**smartdma_progs)(void);
};

struct dma_mcux_smartdma_data {
	uint32_t smartdma_stack[32]; /* Stack for SMARTDMA */
	/* Installed DMA callback and user data */
	dma_callback_t callback;
	void *user_data;
};

/* Seems to be written to smartDMA control register when it is configured */
#define SMARTDMA_MAGIC 0xC0DE0000U
/* These bits are set when the SMARTDMA boots, cleared to reset it */
#define SMARTDMA_BOOT 0x11

static inline bool dma_mcux_smartdma_prog_is_mipi(uint32_t prog)
{
	return ((prog == kSMARTDMA_MIPI_RGB565_DMA) ||
		(prog == kSMARTDMA_MIPI_RGB888_DMA) ||
		(prog == kSMARTDMA_MIPI_RGB565_R180_DMA) ||
		(prog == kSMARTDMA_MIPI_RGB888_R180_DMA));
}

/* Configure a channel */
static int dma_mcux_smartdma_configure(const struct device *dev,
				uint32_t channel, struct dma_config *config)
{
	const struct dma_mcux_smartdma_config *dev_config = dev->config;
	struct dma_mcux_smartdma_data *data = dev->data;
	uint32_t prog_idx;
	bool swap_pixels = false;

	/* SMARTDMA does not have channels */
	ARG_UNUSED(channel);

	data->callback = config->dma_callback;
	data->user_data = config->user_data;

	/* Reset smartDMA */
	SMARTDMA_Reset();

	/*
	 * The dma_slot parameter is used to determine which SMARTDMA program
	 * to run. First, convert the Zephyr define to a HAL enum.
	 */
	switch (config->dma_slot) {
	case DMA_SMARTDMA_MIPI_RGB565_DMA:
		prog_idx = kSMARTDMA_MIPI_RGB565_DMA;
		break;
	case DMA_SMARTDMA_MIPI_RGB888_DMA:
		prog_idx = kSMARTDMA_MIPI_RGB888_DMA;
		break;
	case DMA_SMARTDMA_MIPI_RGB565_180:
		prog_idx = kSMARTDMA_MIPI_RGB565_R180_DMA;
		break;
	case DMA_SMARTDMA_MIPI_RGB888_180:
		prog_idx = kSMARTDMA_MIPI_RGB888_R180_DMA;
		break;
	case DMA_SMARTDMA_MIPI_RGB565_DMA_SWAP:
		swap_pixels = true;
		prog_idx = kSMARTDMA_MIPI_RGB565_DMA;
		break;
	case DMA_SMARTDMA_MIPI_RGB888_DMA_SWAP:
		swap_pixels = true;
		prog_idx = kSMARTDMA_MIPI_RGB888_DMA;
		break;
	case DMA_SMARTDMA_MIPI_RGB565_180_SWAP:
		swap_pixels = true;
		prog_idx = kSMARTDMA_MIPI_RGB565_R180_DMA;
		break;
	case DMA_SMARTDMA_MIPI_RGB888_180_SWAP:
		swap_pixels = true;
		prog_idx = kSMARTDMA_MIPI_RGB888_R180_DMA;
		break;
	default:
		prog_idx = config->dma_slot;
		break;
	}

	if (dma_mcux_smartdma_prog_is_mipi(prog_idx)) {
		smartdma_dsi_param_t param = {.disablePixelByteSwap = (swap_pixels == false)};

		if (config->block_count != 1) {
			return -ENOTSUP;
		}
		/* Setup SMARTDMA */
		param.p_buffer = (uint8_t *)config->head_block->source_address;
		param.buffersize = config->head_block->block_size;
		param.smartdma_stack = data->smartdma_stack;
		/* Save configuration to SMARTDMA */
		dev_config->base->ARM2SMARTDMA = (uint32_t)(&param);
	} else {
		/* For other cases, we simply pass the entire DMA config
		 * struct to the SMARTDMA. The user's application could either
		 * populate this structure with data, or choose to write
		 * different configuration data to the SMARTDMA in their
		 * application
		 */
		dev_config->base->ARM2SMARTDMA = ((uint32_t)config);
	}
	/* Save program */
	dev_config->base->BOOT = (uint32_t)dev_config->smartdma_progs[prog_idx];
	LOG_DBG("Boot address set to 0x%X", dev_config->base->BOOT);
	return 0;
}

static int dma_mcux_smartdma_start(const struct device *dev, uint32_t channel)
{
	const struct dma_mcux_smartdma_config *config = dev->config;

#ifdef CONFIG_PM
	/* Block PM transition until DMA completes */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
#endif
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
#ifdef CONFIG_PM
	/* Release PM lock */
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
#endif
	return 0;
}

static int dma_mcux_smartdma_init(const struct device *dev)
{
	const struct dma_mcux_smartdma_config *config = dev->config;
	/*
	 * Initialize the SMARTDMA with firmware. The default firmware
	 * from MCUX SDK is a display firmware, which has functions
	 * implemented above in the dma configuration function. The
	 * user can install another firmware using `dma_smartdma_install_fw`
	 */
	SMARTDMA_Init((uint32_t)config->smartdma_progs,
		      s_smartdmaDisplayFirmware,
		      SMARTDMA_DISPLAY_FIRMWARE_SIZE);
	config->irq_config_func(dev);

	return 0;
}

static void dma_mcux_smartdma_irq(const struct device *dev)
{
	const struct dma_mcux_smartdma_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->user_data, 0, 0);
	}
#ifdef CONFIG_PM
	/* Release PM lock */
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
#endif
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
		.base = (struct smartdma_periph *)DT_INST_REG_ADDR(n),		\
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
