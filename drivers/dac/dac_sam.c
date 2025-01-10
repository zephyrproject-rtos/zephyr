/*
 * Copyright (c) 2021 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief DAC driver for Atmel SAM MCU family.
 */

#define DT_DRV_COMPAT atmel_sam_dac

#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(dac_sam, CONFIG_DAC_LOG_LEVEL);

#define DAC_CHANNEL_NO  2

/* Device constant configuration parameters */
struct dac_sam_dev_cfg {
	Dacc *regs;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config)(void);
	uint8_t irq_id;
	uint8_t prescaler;
};

struct dac_channel {
	struct k_sem sem;
};

/* Device run time data */
struct dac_sam_dev_data {
#if defined(SOC_SERIES_SAMX7X)
	struct dac_channel dac_channels[DAC_CHANNEL_NO];
#else
	struct dac_channel dac_channel;
#endif
};

static void dac_sam_isr(const struct device *dev)
{
	const struct dac_sam_dev_cfg *const dev_cfg = dev->config;
	struct dac_sam_dev_data *const dev_data = dev->data;
	Dacc *const dac = dev_cfg->regs;
	uint32_t int_stat;

	/* Retrieve interrupt status */
	int_stat = dac->DACC_ISR & dac->DACC_IMR;

#if defined(SOC_SERIES_SAMX7X)
	if ((int_stat & DACC_ISR_TXRDY0) != 0) {
		/* Disable Transmit Ready Interrupt */
		dac->DACC_IDR = DACC_IDR_TXRDY0;
		k_sem_give(&dev_data->dac_channels[0].sem);
	}
	if ((int_stat & DACC_ISR_TXRDY1) != 0) {
		/* Disable Transmit Ready Interrupt */
		dac->DACC_IDR = DACC_IDR_TXRDY1;
		k_sem_give(&dev_data->dac_channels[1].sem);
	}
#else
	if ((int_stat & DACC_ISR_TXRDY) != 0) {
		/* Disable Transmit Ready Interrupt */
		dac->DACC_IDR = DACC_IDR_TXRDY;
		k_sem_give(&dev_data->dac_channel.sem);
	}
#endif
}

static int dac_sam_channel_setup(const struct device *dev,
				 const struct dac_channel_cfg *channel_cfg)
{
	const struct dac_sam_dev_cfg *const dev_cfg = dev->config;
	Dacc *const dac = dev_cfg->regs;

	if (channel_cfg->channel_id >= DAC_CHANNEL_NO) {
		return -EINVAL;
	}
	if (channel_cfg->resolution != 12) {
		return -ENOTSUP;
	}

	if (channel_cfg->internal) {
		return -ENOTSUP;
	}

	/* Enable Channel */
	dac->DACC_CHER = DACC_CHER_CH0 << channel_cfg->channel_id;

	return 0;
}

static int dac_sam_write_value(const struct device *dev, uint8_t channel,
			       uint32_t value)
{
	struct dac_sam_dev_data *const dev_data = dev->data;
	const struct dac_sam_dev_cfg *const dev_cfg = dev->config;
	Dacc *const dac = dev_cfg->regs;

	if (channel >= DAC_CHANNEL_NO) {
		return -EINVAL;
	}

#if defined(SOC_SERIES_SAMX7X)
	if (dac->DACC_IMR & (DACC_IMR_TXRDY0 << channel)) {
#else
	if (dac->DACC_IMR & DACC_IMR_TXRDY) {
#endif
		/* Attempting to send data on channel that's already in use */
		return -EINVAL;
	}

	if (value >= BIT(12)) {
		LOG_ERR("value %d out of range", value);
		return -EINVAL;
	}

#if defined(SOC_SERIES_SAMX7X)
	k_sem_take(&dev_data->dac_channels[channel].sem, K_FOREVER);

	/* Trigger conversion */
	dac->DACC_CDR[channel] = DACC_CDR_DATA0(value);

	/* Enable Transmit Ready Interrupt */
	dac->DACC_IER = DACC_IER_TXRDY0 << channel;
#else
	k_sem_take(&dev_data->dac_channel.sem, K_FOREVER);

	/* Select the channel */
	dac->DACC_MR = DACC_MR_USER_SEL(channel) | DACC_MR_ONE;

	/* Trigger conversion */
	dac->DACC_CDR = DACC_CDR_DATA(value);

	/* Enable Transmit Ready Interrupt */
	dac->DACC_IER = DACC_IER_TXRDY;
#endif

	return 0;
}

static int dac_sam_init(const struct device *dev)
{
	const struct dac_sam_dev_cfg *const dev_cfg = dev->config;
	struct dac_sam_dev_data *const dev_data = dev->data;
	int retval;
#if defined(SOC_SERIES_SAMX7X)
	Dacc *const dac = dev_cfg->regs;
#endif

	/* Configure interrupts */
	dev_cfg->irq_config();

	/* Initialize semaphores */
#if defined(SOC_SERIES_SAMX7X)
	for (int i = 0; i < ARRAY_SIZE(dev_data->dac_channels); i++) {
		k_sem_init(&dev_data->dac_channels[i].sem, 1, 1);
	}
#else
	k_sem_init(&dev_data->dac_channel.sem, 1, 1);
#endif

	/* Enable DAC clock in PMC */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&dev_cfg->clock_cfg);

	retval = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

#if defined(SOC_SERIES_SAMX7X)
	/* Set Mode Register */
	dac->DACC_MR = DACC_MR_PRESCALER(dev_cfg->prescaler);
#endif

	/* Enable module's IRQ */
	irq_enable(dev_cfg->irq_id);

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static DEVICE_API(dac, dac_sam_driver_api) = {
	.channel_setup = dac_sam_channel_setup,
	.write_value = dac_sam_write_value,
};

/* DACC */

static void dacc_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), dac_sam_isr,
		    DEVICE_DT_INST_GET(0), 0);
}

PINCTRL_DT_INST_DEFINE(0);

static const struct dac_sam_dev_cfg dacc_sam_config = {
	.regs = (Dacc *)DT_INST_REG_ADDR(0),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.irq_id = DT_INST_IRQN(0),
	.irq_config = dacc_irq_config,
	.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(0),
	.prescaler = DT_INST_PROP(0, prescaler),
};

static struct dac_sam_dev_data dacc_sam_data;

DEVICE_DT_INST_DEFINE(0, dac_sam_init, NULL, &dacc_sam_data, &dacc_sam_config,
		      POST_KERNEL, CONFIG_DAC_INIT_PRIORITY,
		      &dac_sam_driver_api);
